#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <vector>
#include <limits>
#include <ctime>
#include <map>
#include <mysql_connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/driver.h>
#include <boost/serialization/vector.hpp>
#include <boost/lexical_cast.hpp>
#ifdef USE_MPI
#include <boost/mpi.hpp>
#endif
#include <sys/sysctl.h>
#include "loop_adapt_configuration_cc_client_policy.hpp"

class Client
{
  sql::Connection* m_JHSession; // Job handler session
  sql::Connection* m_OTSession; // Opentuner session

  std::unique_ptr<Policy> m_policy; // Policy to measure performance

  double m_bestConfigRating; // Rating of the currently best config

  int m_jobId; // Id of the job handler session
  int m_tuningRunId; // Id of the opentuner session
  int m_machineId; // Id of the system
  int m_currentConfigId; // Id of the current configuration

  // Contains the resultset that points to (multiple) configs
  std::shared_ptr<sql::ResultSet> m_desiredResults; 

  typedef std::map<std::string, std::string> config_t;

  // Current configuration being tested
  config_t m_currentConfig;
  // Best configuration so far
  config_t m_currentBestConfig;

  std::map<int, config_t> m_pendingConfigs;

  bool m_useBestConfig = false; // If true, using best config so far (waiting for new configs)
  bool m_tuningFinished = false; // Stores current status of the tuning run

#ifdef USE_MPI
  boost::mpi::communicator world; // MPI communicator
#endif // USE_MPI

public:
  /** 
   * Constructor of the class
   * @param address TCP address of the tuning database
   * @param user Username to log in to the database
   * @param password Password for the given username
   * @param arguments Vector containing the arguments for opentuner
   * @param parameters Vector containing the parameters for opentuner
   * @param program_name Name of the program
   * @param project_name Name of the project
   * @param policy Measurement policy
   */
  Client(char* address, char* user, char* password,
         const std::vector<std::string>& arguments,
         const std::vector<std::string>& parameters,
         const std::string& programName,
         const std::string& projectName,
         std::unique_ptr<Policy> policy);

  ~Client();
private:
  /**
   * Combines a vector of strings in a single string using "|"" als separation.
   * Used to build a parameter/arguments string for opentuner
   * @param list Vector of strings 
   * @returns String containing all the strings from the vector
   */
  std::string buildString(const std::vector<std::string>& list) const;

  /**
   * Generate and write the tuning run request into the database.
   * @param arguments String containing the arguments for opentuner
   * @param parameters String containing the parameters for opentuner
   * @param objective String containing the objective for opentuner
   * @param program_name Name of the program
   * @param project_name Name of the project
   */
  void generateTuningRun(const std::string& arguments,
                         const std::string& parameters,
                         const std::string& objective,
                         const std::string& programName,
                         const std::string& projectName);

  /**
   * Wait for the job handler to accept the request and for its initialization.
   * Also get the tuning run id and the opentuner database information for the dialog.
   * @returns Address, user and password of the opentuner database in a String
   */
  std::string getDatabase();

  /**
   * Establish the connection with the opentuner database.
   */
  void connectToDatabase();

public:
  /**
   * Checks if the server is available
   * @return True if there is a usable database for the tuning run request.
   */
  bool serverAlive() const;

private:
  /**
   * Write the machine information into the database.
   */
  void writeHostname();

  /**
   * Checks for the number of processors/cores in the system
   * @returns The CPU count if possible.
   */
  int cpuCount() const;

  /**
   * Checks for the type of CPU in the system
   * @returns The CPU type if possible.
   */
  std::string cpuType() const;

  /**
   * Checks for the amount of memory in the system
   * @returns The memory size if possible.
   */
  int memorySize() const;

public:
  /**
   * Sets a new configuration to be tested.
   * If tuning completed or waiting for new configs, 
   * continue with the best config up to date.
   */
  void nextConfig();

  /**
   * Queries the database to check if the tuning run is completed
   * Sets m_tuningFinished to true if tuning is completed
   * @returns True if the tuning is finished
   */
  bool checkCompleted();

private:
  /**
   * Queries the database for new available configurations.
   * Then reads configs from a desired result and stores them 
   * in m_pendingConfigs
   */
  void queryDesiredResults();

  /**
   * Reads config from resultset and stores it in the client
   * @param configRS ResultSet containing the config
   * @returns Config stored in a std::map
   */
  std::map<std::string, std::string> getConfigAsMap(const std::string& configString);

public:
  /**
   * Returns config element with the given name
   * @param name Name of the element inside the config
   * @returns Element as string so it is compatible with all other languages
   */
  std::string getConfigAt(const std::string& name) const;

  /**
   * Returns the configuration element with given name.
   * @param name Name of the element in the config
   * @return the configuration element as the previously specified type
   */
  template <typename T> T getConfigAt(const std::string& name) const
  {
    return boost::lexical_cast<T>(getConfigAt(name));
  }

  /**
   * Starts measurement of the policy
   */
  void startMeasurement() const;

  /**
   * Stops measurement of the policy
   */
  void stopMeasurement() const;

  /**
   * Evaluates measurement of the policy and reports it to the database
   */
  void reportMeasurement();

  /**
   * Converts the config map to a string and returns it
   * @returns the current configuration as a String
   */
  std::string configToString() const;
};

#endif // CLIENT_HPP 