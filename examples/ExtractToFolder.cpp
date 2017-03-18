#include "HOST.hpp"
/*
 * File:   ExtractToFolder.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on March 4, 2017, 2:33 PM
 *
 * Copyright 2016.
 *
 */

//
// Program: ExtractToFolder
//
// Description: This is a command line program that extracts the contents of a ZIP
// archive to a specified destination folder. Note: Any destination folders are created
// by the program before a file is extracted as the class does not to this. All parameters 
// and their meaning are obtained by running the program with the parameter --help.
//
// ExtractToFolder Example Application
// Command Line Options:
//   --help                      Display help message
//   -c [ --config ] arg         Config File Name
//   -d [ --destination ] arg    Destination folder for extract
//   -z [ --zip ] arg            ZIP File Name
// 
// Dependencies: C11++, Classes (CFileZIP), Linux, Boost C++ Libraries.
//

// =============
// INCLUDE FILES
// =============

//
// C++ STL definitions
//

#include <cstdlib>
#include <iostream>
#include <thread>
#include <iomanip>

//
// BOOST file system, program options processing and iterator definitions
//

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

//
// Antikythera Classes
//

#include "CFileZIP.hpp"

using namespace Antik;

// ======================
// LOCAL TYES/DEFINITIONS
// ======================

//
// Command line parameter data
//

struct ParamArgData {
    std::string configFileNameStr; // Configuration file name
    std::string zipFileNameStr; // ZIP Archive File Name
    std::string destinationFolderNameStr; // Destination folder
};

// ===============
// LOCAL FUNCTIONS
// ===============

//
// Exit with error message/status
//

void exitWithError(std::string errMsgStr) {

    std::cerr << errMsgStr << std::endl;
    exit(EXIT_FAILURE);

}

//
// Add options common to both command line and config file
//

void addCommonOptions(po::options_description& commonOptions, ParamArgData &argData) {

    commonOptions.add_options()
            ("destination,d", po::value<std::string>(&argData.destinationFolderNameStr)->required(), "Destination folder for extract")
            ("zip,z", po::value<std::string>(&argData.zipFileNameStr)->required(), "ZIP File Name");

}

//
// Read in and process command line arguments using boost. 
//

void procCmdLine(int argc, char** argv, ParamArgData &argData) {

    // Define and parse the program options

    po::options_description commandLine("Command Line Options");
    commandLine.add_options()
            ("help", "Display help message")
            ("config,c", po::value<std::string>(&argData.configFileNameStr), "Config File Name");

    addCommonOptions(commandLine, argData);

    po::options_description configFile("Config Files Options");

    addCommonOptions(configFile, argData);

    po::variables_map vm;

    try {

        // Process arguments

        po::store(po::parse_command_line(argc, argv, commandLine), vm);

        // Display options and exit with success

        if (vm.count("help")) {
            std::cout << "ExtractToFolder Example Application" << std::endl << commandLine << std::endl;
            exit(0);
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

        if (!fs::exists(vm["zip"].as<std::string>().c_str())) {
            throw po::error("Specified ZIP archive file does not exist.");
        }

        po::notify(vm);

    } catch (po::error& e) {
        std::cerr << "ExtractToFolder Error: " << e.what() << std::endl << std::endl;
        std::cerr << commandLine << std::endl;
        exit(1);
    }

}

// ============================
// ===== MAIN ENTRY POINT =====
// ============================

int main(int argc, char** argv) {

    ParamArgData argData;

    try {

        // Read in command line parameters and process

        procCmdLine(argc, argv, argData);

        if (!argData.zipFileNameStr.empty()) {

            CFileZIP zipFile(argData.zipFileNameStr);

            if (!fs::exists(argData.destinationFolderNameStr)) {
                fs::create_directories(argData.destinationFolderNameStr);
            }

            zipFile.open();
            
            std::vector<CFileZIP::FileDetail> zipContents(zipFile.contents());

            for (auto & file : zipContents) {
                fs::path destinationPath(argData.destinationFolderNameStr + file.fileNameStr);
                destinationPath.remove_filename();
                if (!fs::exists(destinationPath)) {
                    fs::create_directories(destinationPath);
                }
                destinationPath = argData.destinationFolderNameStr + file.fileNameStr;
                if (zipFile.extract(file.fileNameStr, argData.destinationFolderNameStr + file.fileNameStr)) {
                    std::cout << "Extracted [" << destinationPath.native() << "]" << std::endl;
                }
            }

            zipFile.close();

        }


        //
        // Catch any errors
        //

    } catch (const CFileZIP::Exception & e) {
        exitWithError(e.what());
    } catch (const fs::filesystem_error & e) {
        exitWithError(std::string("BOOST file system exception occured: [") + e.what() + "]");
    } catch (const std::exception & e) {
        exitWithError(std::string("Standard exception occured: [") + e.what() + "]");
    }

    //
    // Normal closedown
    //

    exit(EXIT_SUCCESS);

}