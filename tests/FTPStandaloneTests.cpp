#include "HOST.hpp"
/*
 * File:   FTPStandaloneTests.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2016, 2:33 PM
 *
 * Copyright 2016.
 *
 */

//
// Program: FTPStandaloneTests
//
// Description: Run a series of standalone tests on an FTP server using class CFTP.
// This not only tests CFTP but the FTP servers (CogWheel, vsftp etc) response. The
// of tests will grow over time and at the moment consist of both stress tests and
// general tests such as file transfer.
// 
// Dependencies: C11++, Classes (CFTP).
//               Linux, Boost C++ Libraries.
//
// FTPStandaloneTests
// Program Options:
//   --help                 Print help messages
//   -c [ --config ] arg    Config File Name
//   -s [ --server ] arg    FTP Server
//   -p [ --port ] arg      FTP Server port
//   -u [ --user ] arg      Account username
//   -p [ --password ] arg  User password
//   -r [ --remote ] arg    Remote server directory
//   -l [ --local ] arg     Local directory
//   -f [ --files ] ard     List of files to use in test
//   -t [ --stress ]  arg   Stress test repeat count
//   -g [ --general ]  arg  General text repeat count

// =============
// INCLUDE FILES
// =============

//
// C++ STL
//

#include <iostream>

//
// Antikythera Classes
//

#include "CFTP.hpp"
#include "FTPUtil.hpp"

using namespace Antik::FTP;

//
// Boost program options  & file system library
//

#include <boost/program_options.hpp>  
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/range/iterator_range.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

// ======================
// LOCAL TYES/DEFINITIONS
// ======================

// Command line parameter data

// Command line parameter data

struct ParamArgData {
    std::string userName; // FTP account user name
    std::string userPassword; // FTP account user name password
    std::string serverName; // FTP server
    std::string serverPort; // FTP server port
    std::string remoteDirectory; // FTP remote directory
    std::string localDirectory; // Local directory
    std::string configFileName; // Configuration file name
    std::vector<std::string> fileList; // File list
    int stressTestCount {0 }; // Stress test repeat count
    int generalTestCount { 0 }; // General test repeat count
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
            ("remote,r", po::value<std::string>(&argData.remoteDirectory)->required(), "Remote server directory")
            ("local,l", po::value<std::string>(&argData.localDirectory)->required(), "Local directory")
            ("files,f", po::value<std::vector < std::string >> (&argData.fileList)->multitoken()->required(), "Files")
            ("stress,t", po::value<int>(&argData.stressTestCount), "Stress test repeat count")
            ("general,g", po::value<int>(&argData.generalTestCount), "General test repeat count");

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
            std::cout << "FTPStandaloneTests" << std::endl << commandLine << std::endl;
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
        std::cerr << "FTPStandaloneTests Error: " << e.what() << std::endl << std::endl;
        std::cerr << commandLine << std::endl;
        exit(EXIT_FAILURE);
    }

}

//
// Check FTP command return status against expected  values and display any errors.
//

static inline void checkFTPCommandResponse(CFTP &ftpServer, std::set<std::uint16_t> expectedResults, int count = -1) {

    if (expectedResults.find(ftpServer.getCommandStatusCode()) == expectedResults.end()) {
        if (count != -1) std::cout << "Count [" << count << "]";
        std::cout << "Status code returned : [" << ftpServer.getCommandStatusCode() << "], when expecting [";
        for (auto result : expectedResults) std::cout << result;
        std::cout << "] [Failure]" << std::endl;
        std::cout << "Full response = " << ftpServer.getCommandResponse();
    } else {
        if (count == -1) {
            std::cout << ftpServer.getLastCommand() << " [Success]" << std::endl;
        }
    }

}

//
// Perform a stress test
//

static inline void performStressTest(CFTP &ftpServer, int stressTestCount, std::set<std::uint16_t> expectedResults, 
        std::function<void(CFTP &ftpServer) > stressTestFn) {

    if (ftpServer.connect() != 230) {
        throw CFTP::Exception("Unable to connect status returned = " + ftpServer.getCommandResponse());
    }

    for (auto cnt01 = 0; cnt01 < stressTestCount; cnt01++) {
        stressTestFn(ftpServer);
        checkFTPCommandResponse(ftpServer, expectedResults, cnt01);
    }

    ftpServer.disconnect();
//
}

// ============================
// ===== MAIN ENTRY POint =====
// ============================

int main(int argc, char** argv) {

    try {

        ParamArgData argData;
        CFTP ftpServer;

        // Read in command line parameters and process

        procCmdLine(argc, argv, argData);

        std::cout << "SERVER [" << argData.serverName << "]" << std::endl;
        std::cout << "SERVER PORT [" << argData.serverPort << "]" << std::endl;
        std::cout << "USER [" << argData.userName << "]" << std::endl;
        std::cout << "REMOTE DIRECTORY [" << argData.remoteDirectory << "]" << std::endl;
        std::cout << "LOCAL DIRECTORY [" << argData.localDirectory << "]" << std::endl;
        std::cout << "FILES ";
        for (auto file : argData.fileList) {
            std::cout << "[" << file << "]";
        }
        std::cout << "\n" << std::endl;

        // Set server and port

        ftpServer.setServerAndPort(argData.serverName, argData.serverPort);

        // Set FTP account user name and password

        ftpServer.setUserAndPassword(argData.userName, argData.userPassword);

        // Enable SSL

        ftpServer.setSslEnabled(true);

        // Passive mode list root (stress test)

        std::cout << "Passive mode list root " + std::to_string(argData.stressTestCount) + " times (stress test)" << std::endl;

        ftpServer.setPassiveTransferMode(true);
        performStressTest(ftpServer, argData.stressTestCount,{226}, [] (CFTP &ftpServer) {
            std::string listOutput; ftpServer.list("", listOutput); });

        // Active mode list root (stress test)

        std::cout << "Active mode list root " + std::to_string(argData.stressTestCount) + " times (stress test)" << std::endl;

        ftpServer.setPassiveTransferMode(false);
        performStressTest(ftpServer, argData.stressTestCount,{226}, [] (CFTP &ftpServer) {
            std::string listOutput; ftpServer.list("", listOutput); });

        // Passive mode list non-existent path (stress test)

        std::cout << "Passive mode list non-existent path " + std::to_string(argData.stressTestCount) + " times (stress test)" << std::endl;

        ftpServer.setPassiveTransferMode(true);
        performStressTest(ftpServer, argData.stressTestCount,{226, 550}, [] (CFTP &ftpServer) {
            std::string listOutput; ftpServer.list("xxxx", listOutput); });

        // Active mode list non-existent path (stress test)

        std::cout << "Active mode list non-existent path " + std::to_string(argData.stressTestCount) + " times (stress test)" << std::endl;

        ftpServer.setPassiveTransferMode(false);
        performStressTest(ftpServer, argData.stressTestCount,{226, 550}, [] (CFTP &ftpServer) {
            std::string listOutput; ftpServer.list("xxxx", listOutput); });

        // General tests

        for (auto cnt01 = 0; cnt01 < argData.generalTestCount; cnt01++) {

            // Connect

            if (ftpServer.connect() != 230) {
                throw CFTP::Exception("Unable to connect status returned = " + ftpServer.getCommandResponse());
            }

            // Set binary transfer mode, set passive flag

            ftpServer.setBinaryTransfer(true);
            checkFTPCommandResponse(ftpServer,{200});

            ftpServer.setPassiveTransferMode(true);

            // Get current working directory 

            std::string workingDirectory;
            ftpServer.getCurrentWoringDirectory(workingDirectory);
            std::cout << "Current Working Directory = [" << workingDirectory << "]" << std::endl;
            checkFTPCommandResponse(ftpServer,{257});

            // List directory 

            std::string listOutput;
            ftpServer.list("", listOutput);
            checkFTPCommandResponse(ftpServer,{226});

            // Make directory "Test"

            ftpServer.makeDirectory("Test");
            checkFTPCommandResponse(ftpServer,{257});

            // Remove directory "Test"

            ftpServer.removeDirectory("Test");
            checkFTPCommandResponse(ftpServer,{250});

            // Remove directory not there

            ftpServer.removeDirectory("Test");
            checkFTPCommandResponse(ftpServer,{550});

            // Make directory "Test" again

            ftpServer.makeDirectory("Test");
            checkFTPCommandResponse(ftpServer,{257});

            // Change directory to "Test"

            ftpServer.changeWorkingDirectory("Test");
            checkFTPCommandResponse(ftpServer,{250});

            std::cout << "Passive mode file transfers." << std::endl;

            ftpServer.setPassiveTransferMode(true);

            for (auto file : argData.fileList) {
                ftpServer.putFile(file, argData.localDirectory + file);
                checkFTPCommandResponse(ftpServer,{226});
                ftpServer.getFile(file, argData.localDirectory + file);
                checkFTPCommandResponse(ftpServer,{226});
            }

            std::cout << "Active mode file transfers." << std::endl;

            ftpServer.setPassiveTransferMode(false);

            for (auto file : argData.fileList) {
                ftpServer.getFile(file, argData.localDirectory + file);
                checkFTPCommandResponse(ftpServer,{226});
                ftpServer.putFile(file, argData.localDirectory + file);
                checkFTPCommandResponse(ftpServer,{226});
            }

            // Delete File

            ftpServer.deleteFile(argData.fileList[1]);
            checkFTPCommandResponse(ftpServer,{250});

            // Delete non-existent file

            ftpServer.deleteFile(argData.fileList[1]);
            checkFTPCommandResponse(ftpServer,{550});

            // Upload deleted file to server

            ftpServer.putFile(argData.fileList[1], argData.localDirectory + argData.fileList[1]);
            checkFTPCommandResponse(ftpServer,{226});
            
            // Rename file 

            ftpServer.renameFile(argData.fileList[1], argData.fileList[1]+"~");
            checkFTPCommandResponse(ftpServer,{250});
            
            // Rename file back again

            ftpServer.renameFile(argData.fileList[1]+"~", argData.fileList[1]);
            checkFTPCommandResponse(ftpServer,{250});
            
            // Rename file that does not exist

            ftpServer.renameFile(argData.fileList[1]+"~", argData.fileList[1]);
            checkFTPCommandResponse(ftpServer,{550});

            // Get files size

            size_t fileSize;
            ftpServer.fileSize(argData.fileList[2], fileSize);
            checkFTPCommandResponse(ftpServer,{213});
            std::cout << "File Size = " << fileSize << std::endl;

            // Get size of non-existent file

            ftpServer.fileSize(argData.fileList[2] + "xx", fileSize);
            checkFTPCommandResponse(ftpServer,{550});

            // Get files last modified time

            CFTP::DateTime modifiedDateTime;
            ftpServer.getModifiedDateTime(argData.fileList[2], modifiedDateTime);
            checkFTPCommandResponse(ftpServer,{213});

            // Get files last modified time of non-existent file

            ftpServer.getModifiedDateTime(argData.fileList[2] + "xx", modifiedDateTime);
            checkFTPCommandResponse(ftpServer,{550});
            
            // Remove files

            for (auto file : argData.fileList) {
                ftpServer.deleteFile(file);
                checkFTPCommandResponse(ftpServer,{250});
            }
            
            // Remove directory "Test"

            ftpServer.changeWorkingDirectory("../");
            checkFTPCommandResponse(ftpServer,{250});                      
            ftpServer.removeDirectory("Test");
            checkFTPCommandResponse(ftpServer,{250});        

            // Disconnect
            
            ftpServer.disconnect();

        }

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