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
// Description: This is a command line program to log on to an IMAP server and download e-mails
// from a configured mailbox, command separated mailbox list or all mailboxes for an account.
// A file (.eml) is created for each e-mail in a folder with the same name as the mailbox; with
// the files name being a combination of the mails UID/index prefix and the subject name. All 
// parameters and their meaning are obtained by running the program with the parameter --help.
//
// ArchiveMailBox Example Application
// Program Options:
//  --help                   Print help messages
//   -c [ --config ] arg      Config File Name
//   -s [ --server ] arg      IMAP Server URL and port
//   -u [ --user ] arg        Account username
//   -p [ --password ] arg    User password
//   -m [ --mailbox ] arg     Mailbox name
//   -d [ --destination ] arg Destination for attachments
//   -u [ --updates ]         Search since last file archived.
//   -a [ --all ]             Download files for all mailboxes.
//
// Note: MIME encoded words in the email subject line are decoded to the best ASCII fit
// available.
// 
// Dependencies: C11++, Classes (CFileMIME, CMailIMAP, CMailIMAPParse, CMailIMAPBodyStruct),
//               Linux, Boost C++ Libraries.
//

// =============
// INCLUDE FILES
// =============

//
// C++ STL definitions
//

#include <iostream>
#include <fstream>
#include <sstream>

//
// Antikythera Classes
//

#include "CIMAP.hpp"
#include "CIMAPParse.hpp"
#include "CMIME.hpp"

using namespace Antik::IMAP;

//
// Boost program options  & file system library definitions
//

#include <boost/program_options.hpp> 
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

// ======================
// LOCAL TYES/DEFINITIONS
// ======================

//
// Command line parameter data
//

struct ParamArgData {
    std::string userNameStr;        // Email account user name
    std::string userPasswordStr;    // Email account user name password
    std::string serverURLStr;       // SMTP server URL
    std::string mailBoxNameStr;     // Mailbox name
    fs::path destinationFolder;     // Destination folder for e-mail archive
    std::string configFileNameStr;  // Configuration file name
    bool bOnlyUpdates;              // = true search date since last .eml archived
    bool bAllMailBoxes;             // = true archive all mailboxes

};

//
// Maximum subject line to take in file name
//

const int kMaxSubjectLine = 80;

//
// .eml file extention
//

const std::string kEMLFileExt(".eml");

// ===============
// LOCAL FUNCTIONS
// ===============

//
// Exit with error message/status
//

void exitWithError(const std::string errMsgStr) {

    // Closedown email, display error and exit.

    CIMAP::closedown();
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
            ("destination,d", po::value<fs::path>(&argData.destinationFolder)->required(), "Destination for e-mail archive")
            ("updates,u", "Search since last file archived.")
            ("all,a", "Download files for all mailboxes.");

}

//
// Read in and process command line arguments using boost.
//

void procCmdLine(int argc, char** argv, ParamArgData &argData) {

    // Default values

    argData.bOnlyUpdates = false;
    argData.bAllMailBoxes = false;

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

        // Search for new e-mails only
        
        if (vm.count("updates")) {
            argData.bOnlyUpdates = true;
        }

        // Download all mailboxes
        
        if (vm.count("all")) {
            argData.bAllMailBoxes = true;
        }

        po::notify(vm);

    } catch (po::error& e) {
        std::cerr << "ArchiveMailBox Error: " << e.what() << std::endl << std::endl;
        std::cerr << commandLine << std::endl;
        exit(EXIT_FAILURE);
    }

}

//
// Parse command response and return pointer to parsed data.
//

CIMAPParse::COMMANDRESPONSE parseCommandResponse(const std::string& commandStr, 
                             const std::string& commandResponseStr) {

    CIMAPParse::COMMANDRESPONSE parsedResponse;

    try {
        parsedResponse = CIMAPParse::parseResponse(commandResponseStr);
    } catch (CIMAPParse::Exception &e) {
        std::cerr << "RESPONSE IN ERRROR: [" << commandResponseStr << "]" << std::endl;
        throw (e);
    }

    if (parsedResponse->bBYESent) {
        throw CIMAP::Exception("Received BYE from server: " + parsedResponse->errorMessageStr);
    } else if (parsedResponse->status != CIMAPParse::RespCode::OK) {
        throw CIMAP::Exception(commandStr + ": " + parsedResponse->errorMessageStr);
    }

    return (parsedResponse);

}

//
// Send command to IMAP server. At present it checks for any errors and just exits.
//

std::string sendCommand(CIMAP& imap, const std::string& mailBoxNameStr, 
                           const std::string& commandStr) {

    std::string commandResponseStr;

    try {
        commandResponseStr = imap.sendCommand(commandStr);
    } catch (CIMAP::Exception &e) {
        std::cerr << "IMAP ERROR: Need to reconnect to server" << std::endl;
        throw (e);
    }

    return (commandResponseStr);

}

//
// Fetch a given e-mails body and subject line and create an .eml file for it.
//

void fetchEmailAndArchive(CIMAP& imap, const std::string& mailBoxNameStr, 
                     const fs::path& destinationFolder, std::uint64_t index) {

    std::string commandStr, commandResponseStr, subject, emailBody;
    CIMAPParse::COMMANDRESPONSE parsedResponse;

    commandStr = "UID FETCH " + std::to_string(index) + " (BODY[] BODY[HEADER.FIELDS (SUBJECT)])";
    commandResponseStr = sendCommand(imap, mailBoxNameStr, commandStr);
    parsedResponse = parseCommandResponse(commandStr, commandResponseStr);

    if (parsedResponse) {

        for (auto fetchEntry : parsedResponse->fetchList) {
            std::cout << "EMAIL MESSAGE NO. [" << fetchEntry.index << "]" << std::endl;
            for (auto resp : fetchEntry.responseMap) {
                if (resp.first.find("BODY[]") == 0) {
                    emailBody = resp.second;
                } else if (resp.first.find("BODY[HEADER.FIELDS (SUBJECT)]") == 0) {
                    if (resp.second.find("Subject:") != std::string::npos) { // Contains "Subject:"
                        subject = resp.second.substr(8);
                        subject = Antik::File::CMIME::convertMIMEStringToASCII(subject);
                        if (subject.length() > kMaxSubjectLine) { // Truncate for file name
                            subject = subject.substr(0, kMaxSubjectLine);
                        }
                        for (auto &ch : subject) { // Remove all but alpha numeric from subject
                            if (!std::isalnum(ch)) ch = ' ';
                        }
                    }
                }
            }
        }

        // Have email body so create .eml file for it.

        if (!emailBody.empty()) {
            fs::path fullFilePath = destinationFolder;
            fullFilePath /= "(" + std::to_string(index) + ") " + subject + kEMLFileExt;
            if (!fs::exists(fullFilePath)) {
                std::istringstream emailBodyStream(emailBody);
                std::ofstream emlFileStream(fullFilePath.string(), std::ios::binary);
                if (emlFileStream.is_open()) {
                    std::cout << "Creating [" << fullFilePath.native() << "]" << std::endl;
                    for (std::string lineStr; std::getline(emailBodyStream, lineStr, '\n');) {
                        lineStr.push_back('\n');
                        emlFileStream.write(&lineStr[0], lineStr.length());
                    }
                } else {
                    std::cerr << "Failed to create file [" << fullFilePath << "]" << std::endl;
                }
            }
        }

    }

}

//
// Find the UID on the last message saved and search from that. Each saved .eml file has a "(UID)"
// prefix; get the UID from this.
//

std::uint64_t getLowerSearchLimit(const fs::path& destinationFolder) {

    if (fs::exists(destinationFolder) && fs::is_directory(destinationFolder)) {

        std::uint64_t highestUID=1, currentUID=0;

        for (auto& entry : boost::make_iterator_range(fs::directory_iterator(destinationFolder),{})) {
            if (fs::is_regular_file(entry.status()) && (entry.path().extension().compare(kEMLFileExt) == 0)) {
                currentUID=std::strtoull(CIMAPParse::stringBetween(entry.path().filename().string(),'(', ')').c_str(), nullptr, 10);
                if (currentUID > highestUID) {
                    highestUID = currentUID;
                } 
            }
        }
        
        return (highestUID);
        
    }
    
    return(0);

}

//
// Convert list of comma separated mailbox names / list all mailboxes and place into vector or mailbox name strings.
//

void createMailBoxList(CIMAP& imap, const ParamArgData& argData, 
                       std::vector<std::string>& mailBoxList) {

    if (argData.bAllMailBoxes) {
        std::string commandStr, commandResponseStr;
        CIMAPParse::COMMANDRESPONSE parsedResponse;

        commandStr = "LIST \"\" *";
        commandResponseStr = sendCommand(imap, "", commandStr);
        parsedResponse = parseCommandResponse(commandStr, commandResponseStr);

        if (parsedResponse) {

            for (auto mailBoxEntry : parsedResponse->mailBoxList) {
                if (mailBoxEntry.mailBoxNameStr.front() == ' ') mailBoxEntry.mailBoxNameStr = mailBoxEntry.mailBoxNameStr.substr(1);
                if (mailBoxEntry.attributesStr.find("\\Noselect") == std::string::npos) {
                    mailBoxList.push_back(mailBoxEntry.mailBoxNameStr);
                }
            }

        }
    } else {
        std::istringstream mailBoxStream(argData.mailBoxNameStr);
        for (std::string mailBoxStr; std::getline(mailBoxStream, mailBoxStr, ',');) {
            mailBoxStr = mailBoxStr.substr(mailBoxStr.find_first_not_of(' '));
            mailBoxStr = mailBoxStr.substr(0, mailBoxStr.find_last_not_of(' ') + 1);
            mailBoxList.push_back(mailBoxStr);
        }
    }
}

// ============================
// ===== MAIN ENTRY POINT =====
// ============================

int main(int argc, char** argv) {

    try {

        ParamArgData argData;
        CIMAP imap;
        std::vector<std::string> mailBoxList;

        // Read in command line parameters and process

        procCmdLine(argc, argv, argData);

        // Initialise CMailIMAP internals

        CIMAP::init();

        // Set mail account user name and password

        imap.setServer(argData.serverURLStr);
        imap.setUserAndPassword(argData.userNameStr, argData.userPasswordStr);

        // Connect

        std::cout << "Connecting to server [" << argData.serverURLStr << "]" << std::endl;

        imap.connect();

        // Create mailbox list

        createMailBoxList(imap, argData, mailBoxList);

        for (std::string mailBoxStr : mailBoxList) {

            CIMAPParse::COMMANDRESPONSE parsedResponse;
            fs::path mailBoxPath;
            std::string commandStr, commandResponseStr;
            std::uint64_t searchUID=0;
            
            std::cout << "MAIL BOX [" << mailBoxStr << "]" << std::endl;

            // SELECT mailbox

            commandStr = "SELECT " + mailBoxStr;
            commandResponseStr = sendCommand(imap, mailBoxStr, commandStr);
            parsedResponse = parseCommandResponse(commandStr, commandResponseStr);

            // Clear any quotes from mailbox name for folder name

            if (mailBoxStr.front() == '\"') mailBoxStr = mailBoxStr.substr(1);
            if (mailBoxStr.back() == '\"') mailBoxStr.pop_back();

            // Create destination folder

            mailBoxPath = argData.destinationFolder / mailBoxStr;
            if (!argData.destinationFolder.string().empty() && !fs::exists(mailBoxPath)) {
                std::cout << "Creating destination folder = [" << mailBoxPath.native() << "]" << std::endl;
                fs::create_directories(mailBoxPath);
            }

            // Get newest file creation date for search

            if (argData.bOnlyUpdates) {
                searchUID = getLowerSearchLimit(mailBoxPath);
            }

            // SEARCH for all present email and then create an archive for them.

            if (searchUID!=0) {
                std::cout << "Searching from [" << std::to_string(searchUID) << "]" << std::endl;
                commandStr = "UID SEARCH "+std::to_string(searchUID)+":*";
            } else {
                commandStr = "UID SEARCH 1:*";
            }

            commandResponseStr = sendCommand(imap, mailBoxStr, commandStr);
            parsedResponse = parseCommandResponse(commandStr, commandResponseStr);
            if (parsedResponse) {
                if ((parsedResponse->indexes.size() == 1) && (parsedResponse->indexes[0] == searchUID)) {
                    std::cout << "Messages found = " << 0 << std::endl;
                } else {
                    std::cout << "Messages found = " << parsedResponse->indexes.size() << std::endl;
                    for (auto index : parsedResponse->indexes) {
                        fetchEmailAndArchive(imap, mailBoxStr, mailBoxPath, index);
                    }
                }
            }

        }

        std::cout << "Disconnecting from server [" << argData.serverURLStr << "]" << std::endl;

        imap.disconnect();

    //
    // Catch any errors
    //    

    } catch (CIMAP::Exception &e) {
        exitWithError(e.what());
    } catch (CIMAPParse::Exception &e) {
        exitWithError(e.what());
    } catch (const fs::filesystem_error & e) {
        exitWithError(std::string("BOOST file system exception occured: [") + e.what() + "]");
    } catch (std::exception & e) {
        exitWithError(std::string("Standard exception occured: [") + e.what() + "]");
    }

    // IMAP closedown

    CIMAP::closedown();

    exit(EXIT_SUCCESS);


}

