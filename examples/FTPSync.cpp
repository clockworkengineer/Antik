#include "HOST.hpp"
/*
 * File:   FTPSync.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2017, 2:33 PM
 *
 * Copyright 2017.
 *
 */

//
// Program: FTPSync
//
// Description: Simple FTP program that takes a local directory and keeps it 
// sycnhronized with a local server directory.
//
// Dependencies: C11++, Classes (CFTP, CSocket), Boost C++ Libraries.
//
// FTPSync
// Program Options:
//   --help                 Print help messages
//   -c [ --config ] arg    Config File Name
//   -s [ --server ] arg    FTP Server
//   -p [ --port ] arg      FTP Server port
//   -u [ --user ] arg      Account username
//   -p [ --password ] arg  User password
//   -r [ --remote ] arg    Remote server directory to restore
//   -l [ --local ] arg     Local directory to use as base for restore
//

// =============
// INCLUDE FILES
// =============

//
// C++ STL
//

#include <iostream>
#include <unordered_map>

//
// Antik Classes
//

#include "CFTP.hpp"
#include "FTPUtil.hpp"

using namespace Antik::FTP;

//
// Boost program options  & file system library
//

#include <boost/program_options.hpp>  
#include <boost/filesystem.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

// ======================
// LOCAL TYES/DEFINITIONS
// ======================

// Command line parameter data

struct ParamArgData {
    std::string userName;        // FTP account user name
    std::string userPassword;    // FTP account user name password
    std::string serverName;      // FTP server
    std::string serverPort;      // FTP server port
    std::string remoteDirectory; // FTP remote directory for sync
    std::string localDirectory;  // Local directory for sync with server
    std::string configFileName;  // Configuration file name
};

// ===============
// LOCAL FUNCTIONS
// ===============

//
// Exit with error message/status
//

void exitWithError(std::string errMsg) {

    // Display error and exit.

    std::cout.flush();
    std::cerr << errMsg << std::endl;
    exit(EXIT_FAILURE);

}

//
// Add options common to both command line and config file
//

void addCommonOptions(po::options_description& commonOptions, ParamArgData& argData) {

    commonOptions.add_options()
            ("server,s", po::value<std::string>(&argData.serverName)->required(), "FTP Server name")
            ("port,o", po::value<std::string>(&argData.serverPort)->required(), "FTP Server port")
            ("user,u", po::value<std::string>(&argData.userName)->required(), "Account username")
            ("password,p", po::value<std::string>(&argData.userPassword)->required(), "User password")
            ("remote,r", po::value<std::string>(&argData.remoteDirectory)->required(), "Remote directory to restore")
            ("local,l", po::value<std::string>(&argData.localDirectory)->required(), "Local directory as base for restore");

}

//
// Read in and process command line arguments using boost.
//

void procCmdLine(int argc, char** argv, ParamArgData &argData) {

    // Define and parse the program options

    po::options_description commandLine("Program Options");
    commandLine.add_options()
            ("help", "Print help messages")
            ("config,c", po::value<std::string>(&argData.configFileName), "Config File Name");

    addCommonOptions(commandLine, argData);

    po::options_description configFile("Config Files Options");

    addCommonOptions(configFile, argData);

    po::variables_map vm;

    try {

        // Process arguments

        po::store(po::parse_command_line(argc, argv, commandLine), vm);

        // Display options and exit with success

        if (vm.count("help")) {
            std::cout << "FTPSync" << std::endl << commandLine << std::endl;
            exit(EXIT_SUCCESS);
        }

        if (vm.count("config")) {
            if (fs::exists(vm["config"].as<std::string>().c_str())) {
                std::ifstream ifs{vm["config"].as<std::string>().c_str()};
                if (ifs) {
                    po::store(po::parse_config_file(ifs, configFile), vm);
                }
            } else {
                throw po::error("Specified config file does not exist.");
            }
        }

        po::notify(vm);

    } catch (po::error& e) {
        std::cerr << "FTPSync Error: " << e.what() << std::endl << std::endl;
        std::cerr << commandLine << std::endl;
        exit(EXIT_FAILURE);
    }

}

//
// Convert local file path to remote server path
//

static inline std::string localFileToRemote(const std::string &localDirectory, const std::string &localFilePath) {
    return(localFilePath.substr(localDirectory.rfind('/')));
}

//
// Convert remote server file path to local path
//

static inline std::string remoteFileToLocal(const std::string &localDirectory, const std::string &remoteFilePath) {
    return(localDirectory.substr(0, localDirectory.rfind('/'))+remoteFilePath);
}

// ============================
// ===== MAIN ENTRY POint =====
// ============================

int main(int argc, char** argv) {

    try {

        ParamArgData argData;
        CFTP ftpServer;
        std::vector<std::string> localFiles;
        std::vector<std::string> remoteFiles;

        // Read in command line parameters and process

        procCmdLine(argc, argv, argData);

        std::cout << "SERVER [" << argData.serverName << "]" << std::endl;
        std::cout << "SERVER PORT [" << argData.serverPort << "]" << std::endl;
        std::cout << "USER [" << argData.userName << "]" << std::endl;
        std::cout << "REMOTE DIRECTORY [" << argData.remoteDirectory << "]" << std::endl;
        std::cout << "LOCAL DIRECTORY [" << argData.localDirectory << "]\n" << std::endl;

        // Set server and port

        ftpServer.setServerAndPort(argData.serverName, argData.serverPort);

        // Set mail account user name and password

        ftpServer.setUserAndPassword(argData.userName, argData.userPassword);

        // Enable SSL

        ftpServer.setSslEnabled(true);

        // Connect

        if (ftpServer.connect() != 230) {
            throw CFTP::Exception("Unable to connect status returned = " + ftpServer.getCommandResponse());
        }

        // Get local directory file list and copy new files to server

        std::cout << "Transferring any new files to server." << std::endl; 

        listLocalRecursive(argData.localDirectory, localFiles);
        
        std::vector<std::string> newFiles;
        std::vector<std::string> newFilesTransfered;
        
        for (auto file : localFiles) {
            if (!ftpServer.fileExists(localFileToRemote(argData.localDirectory, file))) {
                newFiles.push_back(file);
            }
        }
                 
        if (!newFiles.empty()) {
            newFilesTransfered = putFiles(ftpServer, argData.localDirectory, newFiles);
            std::cout << "Number of new files transfered [" << newFilesTransfered.size() << "]" << std::endl;
        } else {
            std::cout << "No new files to transfer." << std::endl;           
        }
 
        // Remove any deleted local files from server

        std::cout << "Removing any deleted local files from server." << std::endl; 
               
        listRemoteRecursive(ftpServer, argData.remoteDirectory, remoteFiles);

        for (auto file : remoteFiles) {
            if (!fs::exists(remoteFileToLocal(argData.localDirectory, file))) {
                if (ftpServer.deleteFile(file) == 250) {
                    std::cout << "File [" << file << " ] removed from server." << std::endl;
                } else if (ftpServer.removeDirectory(file) == 250) {
                    std::cout << "Directory [" << file << " ] removed from server." << std::endl;
                } else {
                    std::cerr << "File [" << file << " ] could not be removed from server." << std::endl;
                }
            }
        }

        std::cout << "Copying updated local files to server." << std::endl; 
               
        std::unordered_map<std::string, CFTP::DateTime> remoteFileModifiedTimes;
        
        CFTP::DateTime modifiedDateTime;
        
        for (auto file : remoteFiles) {
            if (ftpServer.getModifiedDateTime(file, modifiedDateTime)==213) {
                remoteFileModifiedTimes[file] = modifiedDateTime;
            }
        }

        for (auto file : localFiles) {
            if (fs::is_regular_file(file)) {
                std::time_t localModifiedTime;
                localModifiedTime = fs::last_write_time(file);
                CFTP::DateTime modifiedDateTime(std::localtime(&localModifiedTime));
                if (remoteFileModifiedTimes[localFileToRemote(argData.localDirectory, file)] < modifiedDateTime) {
                    std::cout << "Server file " << localFileToRemote(argData.localDirectory, file) << " out of date." << std::endl;
                    if (ftpServer.putFile(localFileToRemote(argData.localDirectory, file), file) == 226) {
                        std::cout << "File [" << file << " ] copied to server." << std::endl;
                    } else {
                        std::cerr << "File [" << file << " ] not copied to server." << std::endl;
                    }
                }
            }
        }
        
        // Disconnect 

        ftpServer.disconnect();

        //
        // Catch any errors
        //    

    } catch (CFTP::Exception &e) {
        exitWithError(e.what());
    } catch (std::exception &e) {
        exitWithError(std::string("Standard exception occured: [") + e.what() + "]");
    }

    exit(EXIT_SUCCESS);

}