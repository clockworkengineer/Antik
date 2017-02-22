#include "HOST.hpp"
/*
 * File:   ArchiveMailBox.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2016, 2:33 PM
 *
 * Copyright 2016.
 *
 */

//
// Program: ArchiveMailBox
//
// Description: Log on to a given IMAP server and download all emails for a specified
// mailbox and create an .eml file for them  in a specified destination folder. Note: 
// The .eml files are created within a sub-folder with the mailbox name and with 
// filenames consisting of the mail UID prefix and then subject line.
//
// Note: At present MIME encoded-words in the subject are not decoded and can result in
// a weird file name. At a later date it is intended to deal with this correctly.
// 
// Dependencies: C11++, Classes (CMailIMAP, CMailIMAPParse, CMailIMAPBodyStruct),
//               Linux, Boost C++ Libraries.
//

//
// C++ STL definitions
//

#include <iostream>
#include <fstream>

//
// Classes
//

#include "CMailIMAP.hpp"
#include "CMailIMAPParse.hpp"
#include "CMailIMAPBodyStruct.hpp"
#include "CMailSMTP.hpp"

//
// Boost program options  & file system library definitions
//

#include "boost/program_options.hpp" 
#include <boost/filesystem.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

//
// Command line parameter data
//

struct ParamArgData {
    std::string userNameStr;            // Email account user name
    std::string userPasswordStr;        // Email account user name password
    std::string serverURLStr;           // SMTP server URL
    std::string mailBoxNameStr;         // Mailbox name
    std::string destinationFolderStr;   // Destination folder for attachments
    std::string configFileNameStr;      // Configuration file name
};

//
// Maximum subject line to take in file name
//

const int kMaxSubjectLine = 80;

//
// Exit with error message/status
//

void exitWithError(std::string errMsgStr) {

    // Closedown email, display error and exit.

    CMailIMAP::closedown();
    std::cerr << errMsgStr << std::endl;
    exit(EXIT_FAILURE);

}

//
// Add options common to both command line and config file
//

void addCommonOptions(po::options_description& commonOptions, ParamArgData& argData) {

    commonOptions.add_options()
            ("server,s", po::value<std::string>(&argData.serverURLStr)->required(), "IMAP Server URL and port")
            ("user,u", po::value<std::string>(&argData.userNameStr)->required(), "Account username")
            ("password,p", po::value<std::string>(&argData.userPasswordStr)->required(), "User password")
            ("mailbox,m", po::value<std::string>(&argData.mailBoxNameStr)->required(), "Mailbox name")
            ("destination,d", po::value<std::string>(&argData.destinationFolderStr)->required(), "Destination for attachments");

}
//
// Read in and process command line arguments using boost.
//

void procCmdLine(int argc, char** argv, ParamArgData &argData) {

    // Define and parse the program options

    po::options_description commandLine("Program Options");
    commandLine.add_options()
            ("help", "Print help messages")
            ("config,c", po::value<std::string>(&argData.configFileNameStr)->required(), "Config File Name");

    addCommonOptions(commandLine, argData);

    po::options_description configFile("Config Files Options");

    addCommonOptions(configFile, argData);

    po::variables_map vm;

    try {

        // Process arguments

        po::store(po::parse_command_line(argc, argv, commandLine), vm);

        // Display options and exit with success

        if (vm.count("help")) {
            std::cout << "ArchiveMailBox Example Application" << std::endl << commandLine << std::endl;
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
        std::cerr << "ArchiveMailBox Error: " << e.what() << std::endl << std::endl;
        std::cerr << commandLine << std::endl;
        exit(EXIT_FAILURE);
    }

}

//
// Fetch a given emails body and subject line and create an .eml file for it.
//

void fetchEmailAndArchive(CMailIMAP& imap, const std::string& destinationFolderStr, std::uint64_t index) {

    std::string parsedResponseStr, subject, emailBody;
    CMailIMAPParse::BASERESPONSE parsedResponse;

    parsedResponseStr = imap.sendCommand("UID FETCH " + std::to_string(index) + " (BODY[] BODY[HEADER.FIELDS (SUBJECT)])");
    parsedResponse = CMailIMAPParse::parseResponse(parsedResponseStr);
    if (parsedResponse->status != CMailIMAPParse::RespCode::OK) {
        throw CMailIMAP::Exception("IMAP FETCH " + parsedResponse->errorMessageStr);
    }

    auto *ptr = static_cast<CMailIMAPParse::FetchResponse *> (parsedResponse.get());
    for (auto fetchEntry : ptr->fetchList) {
        std::cout << "EMAIL INDEX [" << fetchEntry.index << "]" << std::endl;
        for (auto resp : fetchEntry.responseMap) {
            if (resp.first.find("BODY[]") == 0) {
                emailBody = resp.second;
            } else if (resp.first.find("BODY[HEADER.FIELDS (SUBJECT)]") == 0) {
                // Modify subject line removing prefix, any non printable and trailing spaces.
                // Also limit size as this will cause issues with filename lengths if excessively
                // long.
                subject = resp.second.substr(9); 
                for (auto &ch : subject) {
                    if (!std::isprint(ch)) {
                        ch = ' ';
                    }
                }
                while(!subject.empty() && (subject.back()==' ')) {
                    subject.pop_back();
                }
                if (subject.length()>kMaxSubjectLine) {
                    subject = subject.substr(0, kMaxSubjectLine);
                }
             } 
        }
    }

    // Have email body so create .eml file for it.
    
    if (!emailBody.empty()) {
        std::istringstream emailBodyStream(emailBody);
        std::ofstream ofs(destinationFolderStr + "(" + std::to_string(index) + ") " + subject + ".eml", std::ios::binary);
        std::cout << "Creating [" << destinationFolderStr + "(" + std::to_string(index) + ") " + subject + ".eml" << "]" << std::endl;
        for (std::string lineStr; std::getline(emailBodyStream, lineStr, '\n');) {
            lineStr.push_back('\n');
            ofs.write(&lineStr[0], lineStr.length());
        }
    }

}

// ============================
// ===== MAIN ENTRY POINT =====
// ============================

int main(int argc, char** argv) {

    try {

        ParamArgData argData;
        CMailIMAP imap;
        std::string parsedResponseStr;
        CMailIMAPParse::BASERESPONSE parsedResponse;

        // Read in command line parameters and process

        procCmdLine(argc, argv, argData);

        // Initialise CMailIMAP internals

        CMailIMAP::init();

        // Set mail account user name and password

        imap.setServer(argData.serverURLStr);
        imap.setUserAndPassword(argData.userNameStr, argData.userPasswordStr);
        
        // Create destination folder

        if (argData.destinationFolderStr.back() != '/') argData.destinationFolderStr.push_back('/');
        argData.destinationFolderStr += argData.mailBoxNameStr + "/";
        if (!argData.destinationFolderStr.empty() && !fs::exists(argData.destinationFolderStr)) {
            fs::create_directory(argData.destinationFolderStr);
        }

        // Connect

        imap.connect();

        // SELECT mailbox

        parsedResponseStr = imap.sendCommand("SELECT " + argData.mailBoxNameStr);
        parsedResponse = CMailIMAPParse::parseResponse(parsedResponseStr);
        if (parsedResponse->status != CMailIMAPParse::RespCode::OK) {
            throw CMailIMAP::Exception("IMAP SELECT " + parsedResponse->errorMessageStr);
        }

        // SEARCH for all present email and then create an archive for them.

        parsedResponseStr = imap.sendCommand("UID SEARCH 1:*");
        parsedResponse = CMailIMAPParse::parseResponse(parsedResponseStr);
        if (parsedResponse->status != CMailIMAPParse::RespCode::OK) {
            throw CMailIMAP::Exception("IMAP SEARCH " + parsedResponse->errorMessageStr);
            
        } else {
            auto *ptr = static_cast<CMailIMAPParse::SearchResponse *> (parsedResponse.get());
            std::cout << "Messages found = " << ptr->indexes.size() << std::endl;
            for (auto index : ptr->indexes) {
                fetchEmailAndArchive(imap, argData.destinationFolderStr, index);
            }
            std::cout << std::endl;
        }

        imap.disconnect();
        
    //
    // Catch any errors
    //    

    } catch (CMailIMAP::Exception &e) {
        exitWithError(e.what());
    } catch (std::exception & e) {
        exitWithError(std::string("Standard exception occured: [") + e.what() + "]");
    }

    // IMAP closedown

    CMailIMAP::closedown();

    exit(EXIT_SUCCESS);

}
