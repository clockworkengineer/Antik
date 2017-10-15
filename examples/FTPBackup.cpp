#include "HOST.hpp"
/*
 * File:   FTPBackup.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2017, 2:33 PM
 *
 * Copyright 2017.
 *
 */

//
// Program: FTPBackup
//
// Description: Simple FTP backup program that takes a local directory and backs it up
// to a specified FTP server using account details provided.
//
// Dependencies: C11++, Classes (CFTP, CSocket), Boost C++ Libraries.
//
// FTPBackup
// Program Options:
//   --help                 Print help messages
//   -c [ --config ] arg    Config File Name
//   -s [ --server ] arg    FTP Server
//   -p [ --port ] arg      FTP Server port
//   -u [ --user ] arg      Account username
//   -p [ --password ] arg  User password
//   -d [ --directory ] arg Local Directory to backup

// =============
// INCLUDE FILES
// =============

//
// C++ STL
//

#include <iostream>

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
    std::string userName; // FTP account user name
    std::string userPassword; // FTP account user name password
    std::string serverName; // FTP server
    std::string serverPort; // FTP server port
    std::string localDirectory; // Directory to backup
    std::string configFileName; // Configuration file name
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
            ("directory,d", po::value<std::string>(&argData.localDirectory)->required(), "Directory to backup");

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
            std::cout << "FTPBackup" << std::endl << commandLine << std::endl;
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
        std::cerr << "FTPBackup Error: " << e.what() << std::endl << std::endl;
        std::cerr << commandLine << std::endl;
        exit(EXIT_FAILURE);
    }

}

// ============================
// ===== MAIN ENTRY POint =====
// ============================

int main(int argc, char** argv) {

    try {

        ParamArgData argData;
        CFTP ftpServer;
        std::uint16_t statusCode;
        std::vector<std::string> backedUp;

        // Read in command line parameters and process

        procCmdLine(argc, argv, argData);

        std::cout << "SERVER [" << argData.serverName << "]" << std::endl;
        std::cout << "SERVER PORT [" << argData.serverPort << "]" << std::endl;
        std::cout << "USER [" << argData.userName << "]" << std::endl;
        std::cout << "DIRECTORY [" << argData.localDirectory << "]\n" << std::endl;
        
        ftpServer.setServerAndPort(argData.serverName, argData.serverPort);

        // Set mail account user name and password

        ftpServer.setUserAndPassword(argData.userName, argData.userPassword);

        // Enable SSL

        ftpServer.setSslEnabled(true);

        // Connect

        statusCode = ftpServer.connect();
        if (statusCode != 230) {
            throw CFTP::Exception("Unable to connect status returned = " + ftpServer.getCommandResponse());
        }

        // Copy local directory to FTP Server
        
        backedUp = putFiles(ftpServer, argData.localDirectory);

        // Signal success or failure
        
        if (!backedUp.empty()) {
            for (auto file : backedUp) {
                std::cout << "Sucessfully backed up [" << file << "]" << std::endl;
            }
        } else {
            std::cout << "Backup failed."<< std::endl;
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