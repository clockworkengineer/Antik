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
// Description: Log on to a given IMAP server and download all e-mails for a given
// mailbox and create an .eml file for them  in a specified destination folder. The
// .eml files are created within a sub-folder with the mailbox name and with filenames 
// consisting of the mail UID prefix and the subject line. If parameter --updates is set 
// then the date of the newest .eml in the destination folder is used as the basis of
// the IMAP search (ie. only download new e-mails).  If parameter --all is set then the
// server is interrogated for a complete mailbox list and these are downloaded.
//
// Note: MIME encoded words in the email subject line are decoded to the best ASCII fit
// available.
// 
// Dependencies: C11++, Classes (CFileMIME, CMailIMAP, CMailIMAPParse, CMailIMAPBodyStruct),
//               Linux, Boost C++ Libraries.
//

//
// C++ STL definitions
//

#include <iostream>
#include <fstream>
#include <sstream>

//
// Classes
//

#include "CMailIMAP.hpp"
#include "CMailIMAPParse.hpp"
#include "CMailIMAPBodyStruct.hpp"
#include "CMailSMTP.hpp"
#include "CFileMIME.hpp"

//
// Boost program options  & file system library definitions
//

#include "boost/program_options.hpp" 
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

//
// Command line parameter data
//

struct ParamArgData {
    std::string userNameStr; // Email account user name
    std::string userPasswordStr; // Email account user name password
    std::string serverURLStr; // SMTP server URL
    std::string mailBoxNameStr; // Mailbox name
    fs::path destinationFolder; // Destination folder for attachments
    std::string configFileNameStr; // Configuration file name
    bool bOnlyUpdates; // = true search date since last .eml archived
    bool bAllMailBoxes; // = true archive all mailboxes

};

//
// Maximum subject line to take in file name
//

const int kMaxSubjectLine = 100;

//
// .eml file extention
//

const std::string kEMLFileExt(".eml");

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
            ("destination,d", po::value<fs::path>(&argData.destinationFolder)->required(), "Destination for attachments")
            ("updates,u", "Search since last file archived.")
            ("all,a", "Search since last file archived.");

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

        if (vm.count("updates")) {
            argData.bOnlyUpdates = true;
        }

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

CMailIMAPParse::BASERESPONSE parseCommandResponse(std::string commandStr, std::string commandResponseStr) {

    CMailIMAPParse::BASERESPONSE parsedResponse;

    try {
        parsedResponse = CMailIMAPParse::parseResponse(commandResponseStr);
    } catch (CMailIMAPParse::Exception &e) {
        std::cerr << "RESPONSE IN ERRROR: [" << commandResponseStr << "]" << std::endl;
        throw (e);
    }

    if (parsedResponse->bBYESent) {
        throw CMailIMAP::Exception("Received BYE from server: " + parsedResponse->errorMessageStr);
    } else if (parsedResponse->status != CMailIMAPParse::RespCode::OK) {
        throw CMailIMAP::Exception(commandStr + ": " + parsedResponse->errorMessageStr);
    }

    return (parsedResponse);

}

//
// Send command to IMAP server. At present it checks for any errors and just exits.
//

std::string sendCommand(CMailIMAP& imap, const std::string& mailBoxNameStr, std::string& commandStr) {

    std::string commandResponseStr;
    
    try {
        commandResponseStr = imap.sendCommand(commandStr);
    } catch (CMailIMAP::Exception &e) {
        std::cerr << "IMAP ERROR: Need to reconnect to server" << std::endl;
        throw (e);
    }

    return (commandResponseStr);

}

//
// Fetch a given emails body and subject line and create an .eml file for it.
//

void fetchEmailAndArchive(CMailIMAP& imap, std::string mailBoxNameStr, fs::path& destinationFolder, std::uint64_t index) {

    std::string commandStr, commandResponseStr, subject, emailBody;
    CMailIMAPParse::BASERESPONSE parsedResponse;

    commandStr = "UID FETCH " + std::to_string(index) + " (BODY[] BODY[HEADER.FIELDS (SUBJECT)])";
    commandResponseStr = sendCommand(imap, mailBoxNameStr, commandStr);
    parsedResponse = parseCommandResponse(commandStr, commandResponseStr);

    if (parsedResponse) {

        auto *ptr = static_cast<CMailIMAPParse::FetchResponse *> (parsedResponse.get());
        for (auto fetchEntry : ptr->fetchList) {
            std::cout << "EMAIL MESSAGE NO. [" << fetchEntry.index << "]" << std::endl;
            for (auto resp : fetchEntry.responseMap) {
                if (resp.first.find("BODY[]") == 0) {
                    emailBody = resp.second;
                } else if (resp.first.find("BODY[HEADER.FIELDS (SUBJECT)]") == 0) {
                    if (resp.second.length() > 8) { // Contains "Subject:"
                        subject = resp.second.substr(8);
                        subject = CFileMIME::convertMIMEStringToASCII(subject);
                        if (subject.length() > kMaxSubjectLine) { // Truncate for file name
                            subject = subject.substr(0, kMaxSubjectLine);
                        }
                        for (auto &ch : subject) { // Remove any possible folder hierarchy delimeter
                            if ((ch == '\\') || (ch == '/')) ch = ' ';
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
                if (emlFileStream) {
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
// Find the date on the last modified .eml file and use for SEARCH
//

std::string getSearchDate(fs::path destinationFolder) {

    std::string dateBuffer(32, ' ');

    if (fs::exists(destinationFolder) && fs::is_directory(destinationFolder)) {

        std::time_t lastModified;
        std::vector<std::time_t> fileTimes;

        for (auto& entry : boost::make_iterator_range(fs::directory_iterator(destinationFolder),{})) {
            if (fs::is_regular_file(entry.status()) && (entry.path().extension().compare(kEMLFileExt) == 0)) {
                fileTimes.push_back(fs::last_write_time(entry.path()));
            }
        }

        if (!fileTimes.empty()) {
            lastModified = fileTimes.back();
            fileTimes.pop_back();
            while (!fileTimes.empty()) {
                if (std::difftime(lastModified, fileTimes.back()) < 0) {
                    lastModified = fileTimes.back();
                }
                fileTimes.pop_back();
            }

            std::tm * ptm = std::localtime(&lastModified);
            dateBuffer.resize(std::strftime(&dateBuffer[0], dateBuffer.length(), "%d-%b-%Y", ptm));

        } else {
            dateBuffer = "";
        }

    }

    return (dateBuffer);

}

//
// Convert list of comma separated mailbox names / list all mailboxes and place into vector or mailbox name strings.
//

void createMailBoxList(CMailIMAP& imap, ParamArgData& argData, std::vector<std::string>& mailBoxList) {

    if (argData.bAllMailBoxes) {
        std::string commandStr, commandResponseStr;
        CMailIMAPParse::BASERESPONSE parsedResponse;

        commandStr = "LIST \"\" *";
        commandResponseStr = sendCommand(imap, "", commandStr);
        parsedResponse = parseCommandResponse(commandStr, commandResponseStr);

        if (parsedResponse) {

            auto ptr = static_cast<CMailIMAPParse::ListResponse *> (parsedResponse.get());

            for (auto mailBoxEntry : ptr->mailBoxList) {
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
        CMailIMAP imap;
        std::vector<std::string> mailBoxList;

        // Read in command line parameters and process

        procCmdLine(argc, argv, argData);

        // Initialise CMailIMAP internals

        CMailIMAP::init();

        // Set mail account user name and password

        imap.setServer(argData.serverURLStr);
        imap.setUserAndPassword(argData.userNameStr, argData.userPasswordStr);

        // Connect

        std::cout << "Connecting to server [" << argData.serverURLStr << "]" << std::endl;

        imap.connect();

        // Create mailbox list

        createMailBoxList(imap, argData, mailBoxList);

        for (std::string mailBoxStr : mailBoxList) {

            CMailIMAPParse::BASERESPONSE parsedResponse;
            fs::path mailBoxPath;
            std::string commandStr, commandResponseStr, searchDate;

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
                searchDate = getSearchDate(mailBoxPath);
            }

            // SEARCH for all present email and then create an archive for them.

            if (!searchDate.empty()) {
                std::cout << "Searching from [" << searchDate << "]" << std::endl;
                commandStr = "UID SEARCH SENTSINCE " + searchDate;
            } else {
                commandStr = "UID SEARCH 1:*";
            }

            commandResponseStr = sendCommand(imap, mailBoxStr, commandStr);
            parsedResponse = parseCommandResponse(commandStr, commandResponseStr);
            if (parsedResponse) {
                auto *ptr = static_cast<CMailIMAPParse::SearchResponse *> (parsedResponse.get());
                std::cout << "Messages found = " << ptr->indexes.size() << std::endl;
                for (auto index : ptr->indexes) {
                    fetchEmailAndArchive(imap, mailBoxStr, mailBoxPath, index);
                }
            }

        }

        std::cout << "Disconnecting from server [" << argData.serverURLStr << "]" << std::endl;

        imap.disconnect();

        //
        // Catch any errors
        //    

    } catch (CMailIMAP::Exception &e) {
        exitWithError(e.what());
    } catch (CMailIMAPParse::Exception &e) {
        exitWithError(e.what());
    } catch (const fs::filesystem_error & e) {
        exitWithError(std::string("BOOST file system exception occured: [")+e.what()+"]");
    } catch (std::exception & e) {
        exitWithError(std::string("Standard exception occured: [") + e.what() + "]");
    }

    // IMAP closedown

    CMailIMAP::closedown();

    exit(EXIT_SUCCESS);


}

