#include "HOST.hpp"
/*
 * File:   ZIPArchiveInfo.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on April 4, 2017, 2:33 PM
 *
 * Copyright 2016.
 *
 */

//
// Program: ZIPArchiveInfo
//
// Description: This is a command line program to scan a ZIP archive and output 
// informatio about it. All parameters and their meaning are obtained by running 
// the program with the parameter --help.
//
// ZIPArchiveInfo Example Application
// Command Line Options:
//   --help                      Display help message
//   -c [ --config ] arg         Config File Name
//   -z [ --zip ] arg            ZIP Archive Name
// 
// Dependencies: C11++, Classes (CFileZIPIO), Linux, Boost C++ Libraries.
//

// =============
// INCLUDE FILES
// =============

//
// C++ STL definitions
//

#include <iostream>
#include <iomanip>

//
// BOOST file system, program options processing definitions
//

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

//
// Antikythera Classes
//

#include "CFileZIPIO.hpp"

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
            ("zip,z", po::value<std::string>(&argData.zipFileNameStr)->required(), "ZIP Archive Name");

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
            std::cout << "ZIPArchiveInfo Example Application" << std::endl << commandLine << std::endl;
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
        std::cerr << "ZIPArchiveInfo Error: " << e.what() << std::endl << std::endl;
        std::cerr << commandLine << std::endl;
        exit(1);
    }

}

//
// Output byte array (in hex).
//

void dumpBytes(std::vector<std::uint8_t>& bytes) {

    std::uint32_t byteCount=1;
     std::cout << std::hex;
    for (int byte : bytes) {
        std::cout << "0x" << byte << " ";
        if (!((byteCount++)&0xF) )std::cout << std::endl;
    }
    std::cout << std::dec << std::endl;
}

//
// Output End Of Central Directory record information.
//

void dumpEOCentralDirectoryRecord( CFileZIPIO::EOCentralDirectoryRecord& endOfCentralDirectory) {

    std::cout << "End Of Central Directory Record" << std::endl;
    std::cout << "-------------------------------\n" << std::endl;
    std::cout << "Start Disk Number                         : " << endOfCentralDirectory.startDiskNumber << std::endl;
    std::cout << "Total Disk Number                         : " << endOfCentralDirectory.diskNumber << std::endl;
    std::cout << "Number Of Central Directory Entries       : " << endOfCentralDirectory.numberOfCentralDirRecords << std::endl;
    std::cout << "Total Number Of Central Directory Entries : " << endOfCentralDirectory.totalCentralDirRecords << std::endl;
    std::cout << "Central Directory Offset                  : " << endOfCentralDirectory.offsetCentralDirRecords << std::endl;
    std::cout << "Comment length                            : " << endOfCentralDirectory.commentLength << std::endl;
    
    if (endOfCentralDirectory.commentLength) {
        std::cout << "Comment                                   : " << endOfCentralDirectory.commentStr << std::endl;
    }
    
    std::cout << std::endl;
    
}

//
// Output Central Directory File Header record information.
//

void dumpCentralDirectoryFileHeader( CFileZIPIO::CentralDirectoryFileHeader& fileHeader, std::uint32_t number) {

    std::cout << "Central Directory File Header No: " << number << std::endl;
    std::cout << "--------------------------------\n" << std::endl;
    
    std::cout << "File Name Length        : " << fileHeader.fileNameLength << std::endl;
    std::cout << "File Name               : " << fileHeader.fileNameStr << std::endl;
    std::cout << "General Bit Flag        : " << fileHeader.bitFlag << std::endl;
    std::cout << "Compressed Size         : " << fileHeader.compressedSize << std::endl;
    std::cout << "Compression Method      : " << fileHeader.compression << std::endl;
    std::cout << "CRC 32                  : " << fileHeader.crc32 << std::endl;
    std::cout << "Creator Version         : " << fileHeader.creatorVersion << std::endl;
    std::cout << "Start Disk Number       : " << fileHeader.diskNoStart << std::endl;
    std::cout << "External File Attribute : " << fileHeader.externalFileAttrib << std::endl;
    std::cout << "Extractor Version       : " << fileHeader.extractorVersion << std::endl;
    std::cout << "File HeaderOffset       : " << fileHeader.fileHeaderOffset << std::endl;
    std::cout << "Internal File Attribute : " << fileHeader.internalFileAttrib << std::endl;
    std::cout << "Modification Date       : " << fileHeader.modificationDate << std::endl;
    std::cout << "Modification Time       : " << fileHeader.modificationTime << std::endl;
    std::cout << "Uncompressed Size       : " << fileHeader.uncompressedSize << std::endl;
    std::cout << "File Comment Length     : " << fileHeader.fileCommentLength << std::endl;
    std::cout << "Extra Field Length      : " << fileHeader.extraFieldLength << std::endl;
       
    if (fileHeader.fileCommentLength) {
        std::cout << "Comment                 : " << fileHeader.fileCommentStr << std::endl;
    }
    
    if (fileHeader.extraFieldLength) {
        std::cout << "Extra Field             :\n";
        dumpBytes(fileHeader.extraField);
    }
    
    std::cout << std::endl;
        
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

            CFileZIPIO zipFile;
            CFileZIPIO::EOCentralDirectoryRecord endOfCentralDirectory;

            //  Open zip file for read
            
            zipFile.openZIPFile(argData.zipFileNameStr, std::ios::in);

            // Read End Of Central Directory and display info
            
            zipFile.getEOCentralDirectoryRecord(endOfCentralDirectory);
            dumpEOCentralDirectoryRecord(endOfCentralDirectory);
            
            // Move to start of Central Directory and loop displaying entries.
            
            zipFile.positionInZIPFile(endOfCentralDirectory.offsetCentralDirRecords);
            
            for (auto entryNumber = 0; entryNumber < endOfCentralDirectory.numberOfCentralDirRecords; entryNumber++) {
                CFileZIPIO::CentralDirectoryFileHeader fileHeader;
                zipFile.getCentralDirectoryFileHeader(fileHeader);
                dumpCentralDirectoryFileHeader(fileHeader, entryNumber );
            }
            
            // Close archive.
            
            zipFile.closeZIPFile();

        }


        //
        // Catch any errors
        //

   } catch (const CFileZIPIO::Exception & e) {
        exitWithError(e.what());
    } catch (const std::exception & e) {
        exitWithError(std::string("Standard exception occured: [") + e.what() + "]");
    }

    //
    // Normal closedown
    //

    exit(EXIT_SUCCESS);

}