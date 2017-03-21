#include "HOST.hpp"
/*
 * File:   ArchiveFolder.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on March 4, 2017, 2:33 PM
 *
 * Copyright 2016.
 *
 */

//
// Program: ArchiveFolder
//
// Description: This is a command line program that writes the contents of a source
// folder to a ZIP archive; traversing it recursively and adding any sub-folder contents.
// All parameters and their meaning are obtained by running the program with the 
// parameter --help.
//
// ArchiveFolder Example Application
// Command Line Options:
//   --help                Display help message
//   -c [ --config ] arg   Config File Name
//   -s [ --Source ] arg   Source Folder To ZIP
//   -z [ --zip ] arg      ZIP File Name
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
    std::string sourceFolderNameStr;    // Source folder
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
            ("Source,s", po::value<std::string>(&argData.sourceFolderNameStr)->required(), "Source Folder To ZIP")
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
            std::cout << "ArchiveFolder Example Application" << std::endl << commandLine << std::endl;
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

        po::notify(vm);

    } catch (po::error& e) {
        std::cerr << "ArchiveFolder Error: " << e.what() << std::endl << std::endl;
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

            // Create Archive
            
            zipFile.create();
            
            // Iterate recursively through folder hierarchy creating file list
            
            fs::path zipFolder(argData.sourceFolderNameStr);
            fs::recursive_directory_iterator begin(zipFolder), end;
            std::vector<fs::directory_entry> fileNameList(begin, end);
            
            zipFile.open();
            
            // Add files to archive
            
            std::cout << "There are " << fileNameList.size() << " files: " << std::endl;
            for (auto& fileName : fileNameList) {
                std::cout << "Add " << fileName.path().string() << '\n';
                if (fs::is_regular_file(fileName.path()))  zipFile.add(fileName.path().string(),fileName.path().string().substr(1));
            }
            
            // Save archive
            
            std::cout << "Creating Archive " << argData.zipFileNameStr << "." << std::endl;           
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