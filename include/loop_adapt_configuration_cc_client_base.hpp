#ifndef MPI1_BASECLIENT_HPP
#define MPI1_BASECLIENT_HPP

#include <fstream>
#include <sstream>
#include <random>
#include <string>
#include <map>
#include <ctime>
#include <chrono>
#include <functional>
#include <vector>
#include <memory>
#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <sys/sysctl.h>
#ifdef USE_MPI
#include <boost/mpi.hpp>
#endif
#include "loop_adapt_configuration_cc_client_policy.hpp"

class BaseClient
{

  /*!
    The Client communicates with opentuner in a dialog via a database to do a requested tuning by the client.
        Steps by the client:
            1. The Client writes a tuning run request with the needed parameters for the tuning run in a database.
            2. The accepted requests generates an opentuner tuning run.
            3. Opentuner generates desired results with configurations which get tested by the client.
            4. The tested configurations generate results which are written into the database.
            5. When the tuning is over the client can continue with the best result.

        The client need the job handler database as argument.
        Interface for the Client:
            start() and generate_result() has to be implemented
            result_order has to be set if run_time_limit() is used
  */

public:

  /// Constructor of the class
  /// @param data tcp address of the tuning database
  /// @param user username to log in to the database
  /// @param pass password for the given username
  /// @param prog_name name of the program
  /// @param proj_name name of the project
  BaseClient(char* data, char* user, char* pass, const std::string& prog_name,
             const std::string& proj_name);

  /// Destructor
  ~BaseClient();

  /// Set arguments for the tuning run
  /// @param str Arguments to guide the tuning process
  void add_argument(const std::string& str);

  /// Set parameters for the tuning run
  /// @param str Parameters to be tuned
  void add_parameter(const std::string& str);

  /// Set policy for the tuning run
  /// @param pol Policy with the objective for tuning
  void add_policy(std::unique_ptr<Policy> pol);

  /// Checks if the tuning run is completed
  /// @returns true if the tuning run is finished or aborted.
  bool check_completed();

  /// End tuning session
  void close();

  /// @param pos Position of the element in the configuration
  /// @returns type of the config element at given position
  const std::string get_element_type(int pos);

  /// Returns the configuration element with given name. Parses a config with the
  /// form x,1|y,true|z,3.14|... where each element is separated by "|".
  /// @param type Type of the element (e.g. "int", "string", "bool", "double",...)
  /// @param name Name of the element in the config
  /// @return the configuration element as the previously specified type
  template <typename T>
  T get_config_at(const std::string& type, const std::string& name)
  {
    std::stringstream outer(config_data);
    std::string token;
    char delim1 = '|';
    char delim2 = ',';

    while (std::getline(outer, token, delim1))
    {
      std::stringstream inner(token);
      if (std::getline(inner, token, delim2))
      {
        if (token == name)
        {
          std::getline(inner, token, delim2);
          break;
        }
      }
    }

    if (type == "int")
    {
      return stoi(token);
    }
    if (type == "double")
    {
      return stod(token);
    }
    if (type == "float")
    {
      return stof(token);
    }
    if (type == "bool")
    {
      return (token == "true") ? true : false;
    }
    return T();
  }

  /// Reports the performance of the current config
  /// @param rtime performance measurement of the current config
  void report_config(double rtime);

  /// @return True if there is a usable database for the tuning run request.
  bool server_alive() const;

  /// Sets a new configuration to be tested.
  /// If tuning completed, continue with the best config.
  /// @return configuration as string
  const std::string set_config();

  /// Setup tuning session
  /// Generate tuning run
  /// Get database address
  /// Connects to database
  /// Writes machine information to database
  void setup();

  /// Start measurement for a single run of the to-be-tuned program
  void start_measurement();

  /// Stop measurement for a single run of the to-be-tuned program
  void stop_measurement();

  /// Evaluate measurement for a single run of the to-be-tuned program
  void report_measurement();

private:

  /// Abort the tuning session
  /// @param e exception to be thrown
  void abort(sql::SQLException& e);

  /// Abort connection with the job_handler database
  void abort_job_handler();

  /// Set objectives for the tuning run
  /// @param str Objectives of the tuning process
  void add_objective(const std::string& str);

  /// Close connection with tuning_run database
  void close_tr_session();

  /// Commit session to the job_handler database
  void commit_jh_session();

  /// Commit session to the tuning_run database
  void commit_tr_session();

  /// Establish the connection with the opentuner database.
  void connect_to_database();

  /// @return the CPU count if possible.
  int cpucount();

  /// @return the CPU type if possible.
  std::string cputype();

  /// Finish the whole tuning session
  void finish();

  /// Writes the result of the given configuration into the opentuner database
  /// @param config_id id of the tested configuration
  /// @param rtime result measured with the given configuration
  /// @param dr_id id of the desired request
  void generate_result(const int config_id, const double rtime, const int dr_id);

  /// Generate and write the tuning run request into the database.
  void generate_tuning_run();

  /// Set the best configuration to continue the calculation
  void get_best_config();

  /// Get configuration as string from a ResultSet
  /// @param rs ResultSet containing the string
  /// @return value of the string object
  std::string get_data_text(std::shared_ptr<sql::ResultSet> rs);

  /// Wait for the job handler to accept the request and for its initialization.
  /// Also get the tuning run id and the opentuner database information for the dialog.
  void get_database();

  /// Get configurations to be tested from the database, update accepted results if config list not empty
  void get_desired_requests();

  /// Wait for opentuner to generate new desired results.
  /// If there are any desired results the ones with the newest generation are selected.
  /// @return ResultSet containing new desired results
  sql::ResultSet* get_desired_results();

  /// Get id from a ResultSet
  /// @param rs ResultSet containing the int
  /// @return value of the int object
  int get_id(std::shared_ptr<sql::ResultSet> rs);

  /// get (or create) the machine and the machine_class
  /// @param machine_class_name type of the client machine
  /// @param machine_name name of the client machine
  /// @return ResultSet containing client machine information
  std::shared_ptr<sql::ResultSet> get_machine(const std::string& machine_class_name,
      const std::string& machine_name);

  /// Get next configuration from the desired requests
  /// @param desired_requests pointer to a ResultSet containing configurations to be tested
  /// @return ResultSet containing next configuration to be tested
  sql::ResultSet* get_next_config(std::shared_ptr<sql::ResultSet> desired_requests);

  /// @return time elapsed since the last call to lap_timer.
  clock_t lap_timer();

  /// @return the memory size if possible.
  int memorysize();

  /// Reads the final configuration and displays it in the terminal
  void read_final_config();

  /// Checks if the given ResultSet has a next entry
  /// and stores it in the given bool variable for all processes
  void resultset_next(std::shared_ptr<sql::ResultSet> rs, bool& rs_next);

  /// Update the Client with the best config
  /// @param rtime measured performance with last configuration
  void update_best_config(double rtime);

  /// Write into the database that the desired results are accepted and getting tested
  void write_accepted_results(std::shared_ptr<sql::ResultSet> desired_requests);

  // Write the machine information into the database.
  void write_hostname();


  sql::Driver* driver;
  sql::Connection* session;
  sql::Connection* tr_session;

  std::string arguments;
  std::string objective;
  std::string parameters;
  std::string program_name;
  std::string project_name;
  std::string tuning_run_database;

  std::unique_ptr<Policy> policy;

  clock_t laptime;

  int machine_id;
  int job_id;
  int tuning_run_id;

  std::shared_ptr<sql::ResultSet> job;
  std::shared_ptr<sql::ResultSet> machine;

  bool tr_session_open;

  std::string current_best_config;
  double current_best_config_time;

  std::shared_ptr<sql::ResultSet> desired_requests;
  bool desired_requests_next;

  std::shared_ptr<sql::ResultSet> config;
  std::string config_data;
  bool config_next;

#ifdef USE_MPI
  boost::mpi::communicator world;
#endif // USE_MPI
};

#endif //MPI1_BASECLIENT_H
