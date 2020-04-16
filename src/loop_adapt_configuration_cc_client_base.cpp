#include "loop_adapt_configuration_cc_client_base.hpp"

/******* Public Methods *******/

/// Constructor of the class
BaseClient::BaseClient(char* data, char* user, char* pass,
                       const std::string& prog_name,
                       const std::string& proj_name)

    : driver(get_driver_instance())
{

#ifdef USE_MPI
    if (world.rank() == 0)
    {
#endif // USE_MPI
        session = driver->connect(data, user, pass);
#ifdef USE_MPI
    }
    else
    {
        session = nullptr;
    }
#endif // USE_MPI
    if (session != nullptr)
    {
        session->setSchema("job_handler");
    }

    current_best_config_time = std::numeric_limits<double>::infinity();
    program_name = prog_name;
    project_name = proj_name;

    add_objective("min_time");
}

/// Destructor
BaseClient::~BaseClient()
{
    delete session;
    delete tr_session;
}

/// @param str Arguments to guide the tuning process
void BaseClient::add_argument(const std::string& str)
{
    if (arguments.empty())
    {
        arguments = str;
    }
    else
    {
        arguments.append("|").append(str);
    }
}

/// @param str Objectives of the tuning process
void BaseClient::add_objective(const std::string& str)
{
    if (objective.empty())
    {
        objective = str;
    }
    else
    {
        std::cout << "Too many objectives" << std::endl;
    }
}

/// @param str Parameters to be tuned
void BaseClient::add_parameter(const std::string& str)
{
    if (parameters.empty())
    {
        parameters = str;
    }
    else
    {
        parameters.append("|").append(str);
    }
}

/// @param pol Policy with the objective for tuning
void BaseClient::add_policy(std::unique_ptr<Policy> pol)
{
    policy = std::move(pol);
}

/// @return Returns true if the tuning run is finished or aborted.
bool BaseClient::check_completed()
{
    bool retval = false;
#ifdef USE_MPI
    if (world.rank() == 0)
    {
#endif // USE_MPI
        std::unique_ptr<sql::PreparedStatement> stmt(session->prepareStatement(
                    "SELECT id, manipulator_settings, tuning_run_id, "
                    "job_handler_state, database_name, program_name, "
                    "project_name, best_config, opentuner_args, objective, "
                    "error_message, error FROM job_handler WHERE "
                    "tuning_run_id = ? AND (job_handler_state "
                    "= 'COMPLETE' OR job_handler_state = 'ABORTED');"));

        stmt->setInt(1, tuning_run_id);
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
        if (!res->next())
        {
            retval = false;
        }
        else
        {
            retval = true;
        }
#ifdef USE_MPI
    }
    world.barrier();
    broadcast(world, retval, 0);
#endif // USE_MPI

    return retval;
}

/// Close tuning session
void BaseClient::close()
{
    close_tr_session();
    get_best_config();
    read_final_config();
    finish();
}

/// Return type of element at given position in the config
const std::string BaseClient::get_element_type(int pos)
{
  std::stringstream outer(parameters);
  std::string token;
  char delim1 = '|';
  char delim2 = ',';

  for (int i = 0; i < pos; i++)
  {
    if (std::getline(outer, token, delim1) && i == pos - 1)
    {
      std::stringstream inner(token);
      for (int i = 0; i < 2; i++)
      {
        if (std::getline(inner, token, delim2) && i == 1)
        {
          std::cout << token << std::endl;
          return token;
        }
      }
    }
  }
  return "";
}

void BaseClient::report_config(double rtime)
{
    if (check_completed())
    {
        return;
    }
    update_best_config(rtime);
    generate_result(get_id(config), rtime, get_id(desired_requests));
    resultset_next(desired_requests, desired_requests_next);
}

/// @return true if there is a usable database for the tuning run request.
bool BaseClient::server_alive() const
{
    bool retval = false;

#ifdef USE_MPI
    if (world.rank() == 0)
    {
#endif // USE_MPI

        if (!session->isValid())
        {
            std::cout << "Connection with the server failed!" << std::endl;
            retval = false;
        }

        std::unique_ptr<sql::Statement> stmt(session->createStatement());
        std::unique_ptr<sql::ResultSet> res(
            stmt->executeQuery("SHOW DATABASES LIKE 'job_handler';"));

        if (!res->next())
        {
            std::cout << "No usable job_handler database! ";
            std::cout << "Please initialize Server!" << std::endl;
            retval = false;
        }

        res = std::unique_ptr<sql::ResultSet>(
                  stmt->executeQuery("SHOW TABLES LIKE 'server_status'; "));

        if (!res->next())
        {
            std::cout << "No usable job_handler database! ";
            std::cout << "Please initialize Server!" << std::endl;
            retval = false;
        }

        res = std::unique_ptr<sql::ResultSet>(
                  stmt->executeQuery("SELECT id, server_state FROM server_status "
                                     "WHERE server_state = 'OFFLINE';"));
        if (res->next())
        {
            std::cout << res->getString("") << " ";
            std::cout << "Server is not running!" << std::endl;
            retval = false;
        }

#ifdef USE_MPI
        retval = true;
    }
    world.barrier();
    broadcast(world, retval, 0);
#endif // USE_MPI
    return retval;
}

const std::string BaseClient::set_config()
{
    if (check_completed())
    {
        config_data = current_best_config;
        return config_data;
    }
    if (desired_requests_next)
    {
        config.reset(get_next_config(desired_requests));
        resultset_next(config, config_next);
        if (config_next)
        {
            config_data = get_data_text(config);
        }
        else
        {
            resultset_next(desired_requests, desired_requests_next);
            set_config();
        }
    }
    else
    {
        get_desired_requests();
        set_config();
    }
    return config_data;
}

/// Setup tuning session
void BaseClient::setup()
{
    generate_tuning_run();
    get_database();
    connect_to_database();
    write_hostname();
}

void BaseClient::start_measurement()
{
    policy->start();
}

void BaseClient::stop_measurement()
{
    policy->stop();
}

void BaseClient::report_measurement()
{
    double result = policy->get_rating();
    report_config(result);

#ifdef USE_MPI
    if (world.rank() == 0)
#endif
    {
        std::cout << "Testing " << config_data;
        std::cout << ", Result = " << result / 1e9 << " seconds" << std::endl;
    }
}


/******* Private Methods *******/


void BaseClient::abort(sql::SQLException& e)
{
#ifdef USE_MPI
    if (world.rank() == 0)
    {
#endif // USE_MPI

        if (tr_session_open)
        {
            tr_session->close();
        }

        abort_job_handler();
        session->close();

        std::cout << "Client finished due to abortion" << std::endl;
        throw e;

#ifdef USE_MPI
    }
#endif // USE_MPI
}

/// Abort connection with the job_handler database
void BaseClient::abort_job_handler()
{
    std::unique_ptr<sql::PreparedStatement> prep_stmt(session->prepareStatement(
                "UPDATE job_handler "
                "SET job_handler_state = 'ABORTED' , error = 'CLIENT' "
                "WHERE id = ?;"));
    prep_stmt->setInt(1, job_id);
    prep_stmt->executeUpdate();
    session->commit();
}

void BaseClient::close_tr_session()
{
#ifdef USE_MPI
    if (world.rank() == 0)
    {
#endif // USE_MPI
        tr_session->close();
#ifdef USE_MPI
    }
#endif // USE_MPI
    tr_session_open = false;
}

void BaseClient::commit_jh_session()
{
#ifdef USE_MPI
    if (world.rank() == 0)
    {
#endif // USE_MPI
        session->commit();
#ifdef USE_MPI
    }
#endif // USE_MPI
}

void BaseClient::commit_tr_session()
{
#ifdef USE_MPI
    if (world.rank() == 0)
    {
#endif // USE_MPI
        tr_session->commit();
#ifdef USE_MPI
    }
#endif // USE_MPI
}

void BaseClient::connect_to_database()
{
#ifdef USE_MPI
    if (world.rank() == 0)
    {
#endif // USE_MPI

        // Convert the python style tuning_run_database address to C++ style (tcp)
        std::string userpass = tuning_run_database.substr(0, tuning_run_database.find('@'));
        userpass = userpass.substr(userpass.find("//"), userpass.length());
        std::string user = userpass.substr(2, userpass.find(':') - 2);
        std::string pass = userpass.substr(userpass.find(':') + 1, userpass.find('@'));
        std::string address = tuning_run_database.substr(tuning_run_database.find('@') + 1,
                              tuning_run_database.length());
        address = "tcp://" + address;

        // Establish the connection with the opentuner database.
        tr_session = driver->connect(address, user, pass);
        tr_session_open = true;
        tr_session->setSchema("opentuner");

#ifdef USE_MPI
    }
#endif // USE_MPI
}

/// @return the CPU core count if possible.
int BaseClient::cpucount()
{
    return sysconf(_SC_NPROCESSORS_ONLN);
}

/// @return the CPU type if possible.
std::string BaseClient::cputype()
{
    // for OS X
#ifdef __APPLE__
    char buffer[128];
    size_t bufferlen = 128;
    sysctlbyname("machdep.cpu.brand_string", &buffer, &bufferlen, NULL, 0);
    return std::string(buffer);

    // for Linux
#elif __linux__
    std::string line;
    std::ifstream finfo("/proc/cpuinfo");
    while (std::getline(finfo, line))
    {
        std::stringstream str(line);
        std::string itype;
        std::string info;

        if (std::getline(str, itype, ':')
                && std::getline(str, info)
                && itype.substr(0, 10) == "model name")
        {
            return info;
        }
    }
#endif
}

void BaseClient::finish()
{
#ifdef USE_MPI
    world.barrier();
    if (world.rank() == 0)
    {
#endif // USE_MPI
        std::cout << "Client finished" << std::endl;
        session->close();
#ifdef USE_MPI
    }
#endif // USE_MPI
}

/// Writes the result of the given configuration into the opentuner database
void BaseClient::generate_result(const int config_id, const double rtime, const int dr_id)
{
#ifdef USE_MPI
    if (world.rank() == 0)
    {
#endif // USE_MPI

        // Write result into database
        std::unique_ptr<sql::PreparedStatement> insert_stmt(tr_session->prepareStatement(
                    "INSERT INTO result (configuration_id, machine_id, tuning_run_id, collection_date, "
                    "collection_cost, state, time) "
                    "VALUES (?, ?, ?, CURRENT_TIMESTAMP, ?, 'OK', ? );"));

        insert_stmt->setInt(1, config_id);
        insert_stmt->setInt(2, machine_id);
        insert_stmt->setInt(3, tuning_run_id);
        insert_stmt->setString(4, std::to_string((int) lap_timer() / CLOCKS_PER_SEC));
        insert_stmt->setDouble(5, rtime);
        insert_stmt->execute();

        // Get id of last inserted result
        std::unique_ptr<sql::Statement> stmt(tr_session->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT LAST_INSERT_ID() AS id"));
        res->next();
        int result_id = res->getInt("id");

        // Update desired result to the last inserted result
        std::unique_ptr<sql::PreparedStatement> update_stmt(tr_session->prepareStatement(
                    "UPDATE desired_result "
                    "SET state = 'COMPLETE', result_id = ? "
                    "WHERE id = ? ;"));
        update_stmt->setInt(1, result_id);
        update_stmt->setInt(2, dr_id);
        update_stmt->executeUpdate();
        tr_session->commit();

#ifdef USE_MPI
    }
#endif // USE_MPI
}

// Generate and write the tuning run request into the database.
void BaseClient::generate_tuning_run()
{

#ifdef USE_MPI
    if (world.rank() == 0)
    {
#endif // USE_MPI

        std::unique_ptr<sql::Statement> stmt(session->createStatement());

        if (objective.empty())
        {
            std::unique_ptr<sql::PreparedStatement> prep_stmt(session->prepareStatement(
                        "INSERT INTO job_handler (manipulator_settings, program_name, "
                        "project_name, opentuner_args, job_handler_state, error_message, "
                        "error) VALUES (?, ?, ?, ?, 'REQUESTED', 'no_error', 'NONE');"));
            prep_stmt->setString(1, parameters);
            prep_stmt->setString(2, program_name);
            prep_stmt->setString(3, project_name);
            prep_stmt->setString(4, arguments);
            prep_stmt->execute();

            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT LAST_INSERT_ID() AS id"));
            res->next();
            job_id = res->getInt("id");
        }
        else
        {
            std::unique_ptr<sql::PreparedStatement> prep_stmt(session->prepareStatement(
                        "INSERT INTO job_handler (manipulator_settings, program_name, "
                        "project_name, opentuner_args, job_handler_state, error_message, "
                        "error, objective) VALUES (?, ?, "
                        "?, ?, 'REQUESTED', 'no_error', 'NONE',?);"));
            prep_stmt->setString(1, parameters);
            prep_stmt->setString(2, program_name);
            prep_stmt->setString(3, project_name);
            prep_stmt->setString(4, arguments);
            prep_stmt->setString(5, objective);
            prep_stmt->execute();

            std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT LAST_INSERT_ID() AS id"));
            res->next();
            job_id = res->getInt("id");
        }
        session->commit();

#ifdef USE_MPI
    }
#endif // USE_MPI
}

/// Set the best configuration to continue the calculation
void BaseClient::get_best_config()
{
#ifdef USE_MPI
    if (world.rank() == 0)
    {
#endif // USE_MPI
        std::unique_ptr<sql::PreparedStatement> prep_stmt(session->prepareStatement(
                    "SELECT id, manipulator_settings, tuning_run_id, job_handler_state, database_name, "
                    "program_name, project_name, best_config, opentuner_args, objective, error_message, error "
                    "FROM job_handler WHERE id= ?;"
                ));
        prep_stmt->setInt(1, job_id);
        job.reset(prep_stmt->executeQuery());
#ifdef USE_MPI
    }
#endif // USE_MPI
}

std::string BaseClient::get_data_text(std::shared_ptr<sql::ResultSet> rs)
{
    std::string data;
#ifdef USE_MPI
    if (world.rank() == 0)
    {
#endif // USE_MPI
        data = rs->getString("data_text");
#ifdef USE_MPI
    }
    world.barrier();
    broadcast(world, data, 0);
#endif // USE_MPI
    return data;
}

// Wait for the job handler to accept the request and for its initialization.
// Also get the tuning run id and the opentuner database information for the dialog.
void BaseClient::get_database()
{
#ifdef USE_MPI
    if (world.rank() == 0)
    {
#endif // USE_MPI

        while (true)
        {
            std::unique_ptr<sql::PreparedStatement> prep_stmt(session->prepareStatement(
                        "SELECT id, manipulator_settings, tuning_run_id, "
                        "job_handler_state, database_name, "
                        "program_name, project_name, best_config, "
                        "opentuner_args, objective, error_message, error "
                        "FROM job_handler "
                        "WHERE job_handler_state = 'RUNNING' AND id = ?;"));
            prep_stmt->setInt(1, job_id);
            job.reset(prep_stmt->executeQuery());

            if (job->next())
            {
                tuning_run_id = job->getInt("tuning_run_id");
                tuning_run_database = job->getString("database_name");
                break;
            }
        }
#ifdef USE_MPI
    }
#endif // USE_MPI
}

/// Get configurations to be tested from the database, update accepted results if config list not empty
void BaseClient::get_desired_requests()
{
    desired_requests.reset(get_desired_results());
    resultset_next(desired_requests, desired_requests_next);

    if (desired_requests_next)
    {
        write_accepted_results(desired_requests);
        resultset_next(desired_requests, desired_requests_next);
    }
}

/// Wait for opentuner to generate new desired results.
/// If there are any desired results the ones with the newest generation are selected.
sql::ResultSet* BaseClient::get_desired_results()
{
#ifdef USE_MPI
    if (world.rank() == 0)
    {
#endif // USE_MPI
        std::unique_ptr<sql::PreparedStatement> tr_stmt(tr_session->prepareStatement(
                    "SELECT id, configuration_id, desired_result.limit , priority, tuning_run_id, "
                    "generation, requestor, request_date, state, result_id, start_date "
                    "FROM desired_result "
                    "WHERE tuning_run_id = ? and state = 'REQUESTED' and "
                    "generation = (SELECT max(generation) FROM desired_result WHERE tuning_run_id = ?);"));
        tr_stmt->setInt(1, tuning_run_id);
        tr_stmt->setInt(2, tuning_run_id);
        return tr_stmt->executeQuery();
#ifdef USE_MPI
    }
    return nullptr;
#endif // USE_MPI
}

int BaseClient::get_id(std::shared_ptr<sql::ResultSet> rs)
{
#ifdef USE_MPI
    if (world.rank() == 0)
    {
#endif // USE_MPI
        return rs->getInt("id");
#ifdef USE_MPI
    }
    return 0;
#endif // USE_MPI
}

/// get (or create) the machine and the machine_class
std::shared_ptr<sql::ResultSet>
BaseClient::get_machine(const std::string& machine_class_name, const std::string& machine_name)
{
    std::unique_ptr<sql::Statement> stmt(tr_session->createStatement());

    std::unique_ptr<sql::PreparedStatement> machine_class_select_stmt(tr_session->prepareStatement(
                "SELECT id, name "
                "FROM machine_class "
                "WHERE name = ?;"));
    machine_class_select_stmt->setString(1, machine_class_name);
    std::unique_ptr<sql::ResultSet> machine_class(machine_class_select_stmt->executeQuery());

    std::unique_ptr<sql::PreparedStatement> machine_class_insert_stmt(tr_session->prepareStatement(
                "INSERT INTO machine_class (name)"
                "VALUES (?);"));
    machine_class_insert_stmt->setString(1, machine_class_name);

    int machine_class_id;

    if (!machine_class->next())
    {
        machine_class_insert_stmt->execute();
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT LAST_INSERT_ID() AS id"));
        res->next();
        machine_class_id = res->getInt("id");
        tr_session->commit();
    }
    else
    {
        machine_class_id = machine_class->getInt("id");
    }

    std::unique_ptr<sql::PreparedStatement> machine_select_stmt(tr_session->prepareStatement(
                "SELECT id, name, cpu, cores, memory_gb, machine_class_id "
                "FROM machine WHERE name = ?;"));
    machine_select_stmt->setString(1, machine_name);
    std::shared_ptr<sql::ResultSet> machine(machine_select_stmt->executeQuery());

    if (machine->next())
    {
        machine_id = machine->getInt("id");
        return machine;
    }

    std::unique_ptr<sql::PreparedStatement> machine_insert_stmt(tr_session->prepareStatement(
                "INSERT INTO machine (name, cpu, cores, memory_gb, machine_class_id) "
                "VALUES (?, ?, ?, ?, ?);"));
    machine_insert_stmt->setString(1, machine_name);
    machine_insert_stmt->setString(2, cputype());
    machine_insert_stmt->setInt(3, cpucount());
    machine_insert_stmt->setDouble(4, memorysize() / (1024.0 * 1024.0 * 1024.0));
    machine_insert_stmt->setInt(5, machine_class_id);
    machine_insert_stmt->execute();

    std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT LAST_INSERT_ID() AS id"));
    res->next();
    machine_id = res->getInt("id");
    tr_session->commit();

    std::shared_ptr<sql::ResultSet> to_return(machine_select_stmt->executeQuery());
    return to_return;
}

/// Get next configuration
/// Process 0 will get a ResultSet pointer, all other processes will get nullptr
sql::ResultSet* BaseClient::get_next_config(std::shared_ptr<sql::ResultSet> desired_requests)
{
#ifdef USE_MPI
    if (world.rank() == 0)
    {
#endif // USE_MPI
        std::unique_ptr<sql::PreparedStatement> prep_stmt(tr_session->prepareStatement(
                    "SELECT id, data_text "
                    "FROM configuration "
                    "WHERE id = "
                    "(SELECT configuration_id FROM desired_result "
                    "WHERE id = ? and tuning_run_id = ?);"));
        prep_stmt->setInt(1, desired_requests->getInt("id"));
        prep_stmt->setInt(2, desired_requests->getInt("tuning_run_id"));

        return prep_stmt->executeQuery();
#ifdef USE_MPI
    }
    return nullptr;
#endif // USE_MPI
}

/// @return time elapsed since the last call to lap_timer.
clock_t BaseClient::lap_timer()
{
    clock_t t = clock();
    clock_t r = t - laptime;
    laptime = t;
    return r;
}

/// @return the memory size if possible.
int BaseClient::memorysize()
{
    // for OS X
#ifdef __APPLE__
    char buffer[128];
    size_t bufferlen = 128;
    sysctlbyname("hw.memsize", &buffer, &bufferlen, NULL, 0);
    return atoi(buffer);
    // for linux
#elif __linux__
    size_t mem = (size_t) sysconf(_SC_PHYS_PAGES) * (size_t) sysconf(_SC_PAGESIZE);
    return static_cast<int>(mem);
#endif
}

void BaseClient::read_final_config()
{
#ifdef USE_MPI
    if (world.rank() == 0)
    {
#endif // USE_MPI
        std::string config;
        if (job->next())
        {
            try
            {
                config = job->getString("best_config");
                if (config.empty())
                {
                    throw sql::SQLException("Error: Could not read final configuration");
                }
            }
            catch (sql::SQLException& e)
            {
                throw e;
            }
        }
        // Print final configuration.
        std::cout << "Final configuration: " << std::endl << config << std::endl;
#ifdef USE_MPI
    }
    world.barrier();
#endif // USE_MPI
}

void BaseClient::resultset_next(std::shared_ptr<sql::ResultSet> rs, bool& rs_next)
{
#ifdef USE_MPI
    if (world.rank() == 0)
    {
#endif // USE_MPI
        rs_next = rs->next();
#ifdef USE_MPI
    }
    world.barrier();
    broadcast(world, rs_next, 0);
#endif // USE_MPI
}

/// Update the Client with the best config
void BaseClient::update_best_config(double rtime)
{
    if (current_best_config_time == std::numeric_limits<double>::infinity())
    {
        current_best_config_time = rtime;
        current_best_config = config_data;
    }
    if (rtime < current_best_config_time)
    {
        current_best_config_time = rtime;
        current_best_config = config_data;
    }
}

/// Write into the database that the desired results are accepted and getting tested
void BaseClient::write_accepted_results(std::shared_ptr<sql::ResultSet> desired_requests)
{

#ifdef USE_MPI
    if (world.rank() == 0)
    {
#endif // USE_MPI

        desired_requests->beforeFirst();

        std::unique_ptr<sql::PreparedStatement> prep_tr_stmt(tr_session->prepareStatement(
                    "UPDATE desired_result "
                    "SET state = 'RUNNING', start_date = CURRENT_TIMESTAMP "
                    "WHERE tuning_run_id = ? and id = ?;"));

        while (desired_requests->next())
        {
            prep_tr_stmt->setString(1, desired_requests->getString("tuning_run_id"));
            prep_tr_stmt->setInt(2, desired_requests->getInt("id"));
            prep_tr_stmt->executeUpdate();
        }
        tr_session->commit();

        desired_requests->beforeFirst();

#ifdef USE_MPI
    }
#endif // USE_MPI
}

// Write the machine information into the database.
void BaseClient::write_hostname()
{
#ifdef USE_MPI
    if (world.rank() == 0)
    {
#endif // USE_MPI
        char hostname[1024];
        gethostname(hostname, 1023);
        machine = get_machine(hostname, hostname);
#ifdef USE_MPI
    }
#endif // USE_MPI
}