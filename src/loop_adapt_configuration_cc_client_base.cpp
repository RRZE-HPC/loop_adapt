#include "loop_adapt_configuration_cc_client_base.hpp"

Client::Client(char* address, char* user, char* pass,
               const std::vector<std::string>& arguments,
               const std::vector<std::string>& parameters,
               const std::string& programName,
               const std::string& projectName,
               std::unique_ptr<Policy> policy)
  : m_policy(std::move(policy))
{
  m_bestConfigRating = std::numeric_limits<double>::max();

#ifdef USE_MPI
  if (world.rank() == 0)
  {
#endif // USE_MPI

    sql::Driver* driver = get_driver_instance();
    m_JHSession = driver->connect(address, user, pass);
    m_JHSession->setSchema("job_handler");

    std::string args = buildString(arguments);
    std::string params = buildString(parameters);

    generateTuningRun(args, params, "min_time", programName, projectName);
    connectToDatabase();
    writeHostname();

#ifdef USE_MPI
  }
#endif // USE_MPI
}

Client::~Client()
{
#ifdef USE_MPI
  if (world.rank() == 0)
  {
#endif // USE_MPI

    m_OTSession->close();
    m_JHSession->close();

#ifdef USE_MPI
  }
#endif // USE_MPI

  delete m_OTSession;
  delete m_JHSession;
}

std::string Client::buildString(const std::vector<std::string>& list) const
{
  std::string str;
  for (const std::string& entry : list)
  {
    if (str.empty())
    {
      str = entry;
    }
    else
    {
      str.append("|").append(entry);
    }
  }
  return str;
}

void Client::generateTuningRun(const std::string& arguments,
                               const std::string& parameters,
                               const std::string& objective,
                               const std::string& programName,
                               const std::string& projectName)
{
  std::unique_ptr<sql::Statement> stmt(m_JHSession->createStatement());

  if (objective.empty())
  {
    std::unique_ptr<sql::PreparedStatement> prepStmt(m_JHSession->prepareStatement(
          "INSERT INTO job_handler (manipulator_settings, program_name, "
          "project_name, opentuner_args, job_handler_state, error_message, "
          "error) VALUES (?, ?, ?, ?, 'REQUESTED', 'no_error', 'NONE');"));
    prepStmt->setString(1, parameters);
    prepStmt->setString(2, programName);
    prepStmt->setString(3, projectName);
    prepStmt->setString(4, arguments);
    prepStmt->execute();

    std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT LAST_INSERT_ID() AS id"));
    res->next();
    m_jobId = res->getInt("id");
  }
  else
  {
    std::unique_ptr<sql::PreparedStatement> prepStmt(m_JHSession->prepareStatement(
          "INSERT INTO job_handler (manipulator_settings, program_name, "
          "project_name, opentuner_args, job_handler_state, error_message, "
          "error, objective) VALUES (?, ?, "
          "?, ?, 'REQUESTED', 'no_error', 'NONE',?);"));
    prepStmt->setString(1, parameters);
    prepStmt->setString(2, programName);
    prepStmt->setString(3, projectName);
    prepStmt->setString(4, arguments);
    prepStmt->setString(5, objective);
    prepStmt->execute();

    std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT LAST_INSERT_ID() AS id"));
    res->next();
    m_jobId = res->getInt("id");
  }
}

std::string Client::getDatabase()
{
  std::string tuningRunDatabase;
  std::unique_ptr<sql::ResultSet> res;

  while (true)
  {
    std::unique_ptr<sql::PreparedStatement> prepStmt(m_JHSession->prepareStatement(
          "SELECT tuning_run_id, database_name "
          "FROM job_handler "
          "WHERE job_handler_state = 'RUNNING' AND id = ?;"));
    prepStmt->setInt(1, m_jobId);
    res.reset(prepStmt->executeQuery());

    if (res->next())
    {
      m_tuningRunId = res->getInt("tuning_run_id");
      tuningRunDatabase = res->getString("database_name");
      break;
    }
  }
  return tuningRunDatabase;
}

void Client::connectToDatabase()
{
  sql::Driver* driver = get_driver_instance();
  std::string tuningRunDatabase = getDatabase();

  // Convert the python style tuning_run_database address to C++ style (tcp)
  std::string userpass = tuningRunDatabase.substr(0, tuningRunDatabase.find('@'));
  userpass = userpass.substr(userpass.find("//"), userpass.length());
  std::string user = userpass.substr(2, userpass.find(':') - 2);
  std::string pass = userpass.substr(userpass.find(':') + 1, userpass.find('@'));
  std::string address = tuningRunDatabase.substr(tuningRunDatabase.find('@') + 1,
                        tuningRunDatabase.length());
  address = "tcp://" + address;

  // Establish the connection with the opentuner database.
  m_OTSession = driver->connect(address, user, pass);
  m_OTSession->setSchema("opentuner");
}

bool Client::serverAlive() const
{
  bool retval = false;

#ifdef USE_MPI
  if (world.rank() == 0)
  {
#endif // USE_MPI

    if (!m_JHSession->isValid())
    {
      std::cout << "Connection with the server failed!" << std::endl;
      retval = false;
    }

    std::unique_ptr<sql::Statement> stmt(m_JHSession->createStatement());
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
  broadcast(world, retval, 0);
#endif // USE_MPI
  return retval;
}

void Client::writeHostname()
{
  std::string machineClassName, machineName;

  char hostname[1024];
  gethostname(hostname, 1023);
  machineClassName = std::string(hostname);
  machineName = std::string(hostname);

  std::unique_ptr<sql::ResultSet> machine;
  std::unique_ptr<sql::Statement> stmt(m_OTSession->createStatement());

  std::unique_ptr<sql::PreparedStatement> machineClassSelectStmt(m_OTSession->prepareStatement(
        "SELECT id "
        "FROM machine_class "
        "WHERE name = ?;"));
  machineClassSelectStmt->setString(1, machineClassName);
  std::unique_ptr<sql::ResultSet> machineClass(machineClassSelectStmt->executeQuery());

  std::unique_ptr<sql::PreparedStatement> machineClassInsertStmt(m_OTSession->prepareStatement(
        "INSERT INTO machine_class (name)"
        "VALUES (?);"));
  machineClassInsertStmt->setString(1, machineClassName);

  int machineClassId;

  if (!machineClass->next())
  {
    machineClassInsertStmt->execute();
    std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT LAST_INSERT_ID() AS id"));
    res->next();
    machineClassId = res->getInt("id");
  }
  else
  {
    machineClassId = machineClass->getInt("id");
  }

  std::unique_ptr<sql::PreparedStatement> machineSelectStmt(m_OTSession->prepareStatement(
        "SELECT id "
        "FROM machine WHERE name = ?;"));
  machineSelectStmt->setString(1, machineName);
  machine.reset(machineSelectStmt->executeQuery());

  if (machine->next())
  {
    m_machineId = machine->getInt("id");
  }

  std::unique_ptr<sql::PreparedStatement> machineInsertStmt(m_OTSession->prepareStatement(
        "INSERT INTO machine (name, cpu, cores, memory_gb, machine_class_id) "
        "VALUES (?, ?, ?, ?, ?);"));
  machineInsertStmt->setString(1, machineName);
  machineInsertStmt->setString(2, cpuType());
  machineInsertStmt->setInt(3, cpuCount());
  machineInsertStmt->setDouble(4, memorySize() / (1024.0 * 1024.0 * 1024.0));
  machineInsertStmt->setInt(5, machineClassId);
  machineInsertStmt->execute();

  std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT LAST_INSERT_ID() AS id"));
  res->next();
  m_machineId = res->getInt("id");
}

int Client::cpuCount() const
{
  return sysconf(_SC_NPROCESSORS_ONLN);
}

std::string Client::cpuType() const
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

int Client::memorySize() const
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

void Client::nextConfig()
{
  if (checkCompleted())
  {
    // If tuning finished, exit. Best config is already set as current
    return;
  }
  else
  {
    // If there are new configs available select one
    if (!m_pendingConfigs.empty())
    {
      // Pop first pending config
      m_currentConfigId = m_pendingConfigs.begin()->first;
      m_currentConfig = m_pendingConfigs.begin()->second;
      m_pendingConfigs.erase(m_pendingConfigs.begin());
      m_useBestConfig = false;
    }
    else
    {
      // No more pending configs, ask for more
      queryDesiredResults();
      // Check if there are now pending configs
      if (!m_pendingConfigs.empty())
      {
        // Pop first pending config
        m_currentConfigId = m_pendingConfigs.begin()->first;
        m_currentConfig = m_pendingConfigs.begin()->second;
        m_pendingConfigs.erase(m_pendingConfigs.begin());
        m_useBestConfig = false;
      }
      else
      {
        // If still no available config use best config
        if (!m_currentBestConfig.empty())
        {
          m_currentConfig = m_currentBestConfig;
          m_useBestConfig = true;
        }
        else
        {
          // If no config yet tested, loop until there are available configs
          while(m_pendingConfigs.empty())
          {
            queryDesiredResults();
          }
          // Pop first pending config
          m_currentConfigId = m_pendingConfigs.begin()->first;
          m_currentConfig = m_pendingConfigs.begin()->second;
          m_pendingConfigs.erase(m_pendingConfigs.begin());
          m_useBestConfig = false;
        }
      }
    }
  }
}

bool Client::checkCompleted()
{
  if (m_tuningFinished)
  {
    return true;
  }

#ifdef USE_MPI
  if (world.rank() == 0)
  {
#endif // USE_MPI
    std::unique_ptr<sql::PreparedStatement> stmt(m_JHSession->prepareStatement(
          "SELECT id "
          "FROM job_handler WHERE "
          "tuning_run_id = ? AND (job_handler_state "
          "= 'COMPLETE' OR job_handler_state = 'ABORTED');"));
    stmt->setInt(1, m_tuningRunId);

    std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());

    if (!res->next())
    {
      m_tuningFinished = false;
    }
    else
    {
      m_currentConfig = m_currentBestConfig;
      m_tuningFinished = true;
    }
#ifdef USE_MPI
  }
  broadcast(world, m_tuningFinished, 0);
#endif // USE_MPI
  return m_tuningFinished;
}

void Client::queryDesiredResults()
{
  std::vector<int> configIDs;
  std::vector<std::string> configTexts;

#ifdef USE_MPI
  if (world.rank() == 0)
  {
#endif // USE_MPI

    std::unique_ptr<sql::PreparedStatement> queryStmt(m_OTSession->prepareStatement(
          "SELECT id "
          "FROM desired_result "
          "WHERE tuning_run_id = ? and state = 'REQUESTED' and generation = "
          "(SELECT max(generation) FROM desired_result WHERE tuning_run_id = ?);"));

    std::unique_ptr<sql::PreparedStatement> updateStmt(m_OTSession->prepareStatement(
          "UPDATE desired_result "
          "SET state = 'RUNNING', start_date = CURRENT_TIMESTAMP "
          "WHERE id = ?;"));

    std::unique_ptr<sql::PreparedStatement> configStmt(m_OTSession->prepareStatement(
          "SELECT id, data_text "
          "FROM configuration "
          "WHERE id = "
          "(SELECT configuration_id FROM desired_result "
          "WHERE id = ?);"));

    queryStmt->setInt(1, m_tuningRunId);
    queryStmt->setInt(2, m_tuningRunId);

    m_desiredResults.reset(queryStmt->executeQuery()); // Reset pointer with new ResultSet

    while(m_desiredResults->next())
    {
      // Mark desired results as accepted
      updateStmt->setInt(1, m_desiredResults->getInt("id"));
      updateStmt->executeUpdate();

      // Get configs from desired results
      configStmt->setInt(1, m_desiredResults->getInt("id"));
      std::shared_ptr<sql::ResultSet> configRS(configStmt->executeQuery());

      while(configRS->next())
      {
        configIDs.push_back(configRS->getInt("id"));
        configTexts.push_back(configRS->getString("data_text"));
      }
    }
    m_desiredResults->beforeFirst();
#ifdef USE_MPI
  }
  broadcast(world, configIDs, 0);
  broadcast(world, configTexts, 0);
#endif // USE_MPI

  for (int i = 0; i < configIDs.size(); i++)
  {
    m_pendingConfigs[configIDs[i]] = getConfigAsMap(configTexts[i]);
  }
}

std::map<std::string, std::string> Client::getConfigAsMap(const std::string& configString)
{
  std::map<std::string, std::string> config;

  std::stringstream outer(configString);
  std::string key;
  char delim1 = '|';
  char delim2 = ',';

  while (std::getline(outer, key, delim1))
  {
    std::stringstream inner(key);
    if (std::getline(inner, key, delim2))
    {
      std::string value;
      std::getline(inner, value, delim2);
      config[key] = value;
    }
  }
  return config;
}

std::string Client::getConfigAt(const std::string& name) const
{
  return m_currentConfig.at(name);
}

void Client::startMeasurement() const
{
#ifdef USE_MPI
  if (world.rank() == 0)
  {
#endif // USE_MPI
    m_policy->start();
#ifdef USE_MPI
  }
#endif // USE_MPI
}

void Client::stopMeasurement() const
{
#ifdef USE_MPI
  if (world.rank() == 0)
  {
#endif // USE_MPI
    m_policy->stop();
#ifdef USE_MPI
  }
#endif // USE_MPI
}

void Client::reportMeasurement()
{
  // Use best config
  if (m_tuningFinished || m_useBestConfig)
  {
    return;
  }

  double rating;
  std::string rawMeasurement;
#ifdef USE_MPI
  if (world.rank() == 0)
  {
#endif // USE_MPI
    rating = m_policy->getRating();
    rawMeasurement = m_policy->getMeasurementString();
#ifdef USE_MPI
  }
  broadcast(world, rating, 0);
#endif // USE_MPI

  // Update best config
  if (rating < m_bestConfigRating)
  {
    m_bestConfigRating = rating;
    m_currentBestConfig = m_currentConfig;
  }

#ifdef USE_MPI
  if (world.rank() == 0)
  {
#endif // USE_MPI
    m_desiredResults->next();
    int drId = m_desiredResults->getInt("id");
    time_t now = std::time(0);

    // Write result into database
    std::unique_ptr<sql::PreparedStatement> insertStmt(m_OTSession->prepareStatement(
          "INSERT INTO result (configuration_id, machine_id, tuning_run_id, collection_date, "
          "collection_cost, state, time, raw_measurement) "
          "VALUES (?, ?, ?, CURRENT_TIMESTAMP, ?, 'OK', ? , ?);"));

    insertStmt->setInt(1, m_currentConfigId);
    insertStmt->setInt(2, m_machineId);
    insertStmt->setInt(3, m_tuningRunId);
    insertStmt->setString(4, std::string(std::ctime(&now)));
    insertStmt->setDouble(5, rating);
    insertStmt->setString(6, rawMeasurement);
    insertStmt->execute();

    // Get id of last inserted result
    std::unique_ptr<sql::Statement> stmt(m_OTSession->createStatement());
    std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT LAST_INSERT_ID() AS id"));
    res->next();
    int resultId = res->getInt("id");

    // Update desired result to the last inserted result
    std::unique_ptr<sql::PreparedStatement> updateStmt(m_OTSession->prepareStatement(
          "UPDATE desired_result "
          "SET state = 'COMPLETE', result_id = ? "
          "WHERE id = ? ;"));
    updateStmt->setInt(1, resultId);
    updateStmt->setInt(2, drId);
    updateStmt->executeUpdate();

#ifdef USE_MPI
  }
#endif // USE_MPI
}

std::string Client::configToString() const
{
  std::string configString = "";
  int i = 0;
  for (const std::pair<std::string, std::string>& c : m_currentConfig)
  {
    configString.append(c.first).append(",").append(c.second);
    if (i == m_currentConfig.size() - 1)
    {
      break;
    }
    i++;
    configString.append("|");
  }
  return configString;
}