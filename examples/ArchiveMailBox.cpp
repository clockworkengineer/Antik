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
// the IMAP search (ie. only download new e-mails). 
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
    bool bOnlyUpdates; // =true search date since last .eml archived
};

//
// Maximum subject line to take in file name
//

const int kMaxSubjectLine = 100;

//
// .eml file exstention
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
            ("updates,u", "Search since last file archived.");

}
//
// Read in and process command line arguments using boost.
//

void procCmdLine(int argc, char** argv, ParamArgData &argData) {

    // Default values

    argData.bOnlyUpdates = false;

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

void fetchEmailAndArchive(CMailIMAP& imap, fs::path& destinationFolder, std::uint64_t index) {

    std::string parsedResponseStr, subject, emailBody;
    CMailIMAPParse::BASERESPONSE parsedResponse;

    parsedResponseStr = imap.sendCommand("UID FETCH " + std::to_string(index) + " (BODY[] BODY[HEADER.FIELDS (SUBJECT)])");
    parsedResponse = CMailIMAPParse::parseResponse(parsedResponseStr);
    if (parsedResponse->status != CMailIMAPParse::RespCode::OK) {
        throw CMailIMAP::Exception("IMAP FETCH " + parsedResponse->errorMessageStr);
    } else if (parsedResponse->bBYESent) {
        throw CMailIMAP::Exception("Received BYE from server: " + parsedResponse->errorMessageStr);
    }

    auto *ptr = static_cast<CMailIMAPParse::FetchResponse *> (parsedResponse.get());
    for (auto fetchEntry : ptr->fetchList) {
        std::cout << "EMAIL INDEX [" << fetchEntry.index << "]" << std::endl;
        for (auto resp : fetchEntry.responseMap) {
            if (resp.first.find("BODY[]") == 0) {
                emailBody = resp.second;
            } else if (resp.first.find("BODY[HEADER.FIELDS (SUBJECT)]") == 0) {
                subject = resp.second.substr(8);
                subject = CFileMIME::convertMIMEStringToASCII(subject);
                if (subject.length() > kMaxSubjectLine) {   // Truncate for file name
                    subject = subject.substr(0, kMaxSubjectLine);
                }
                for (auto &ch : subject) { // Remove any possible folder hierarchy delimeter
                    if ((ch == '\\')|| (ch=='/')) ch = ' ';
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
            std::ofstream ofs(fullFilePath.string(), std::ios::binary);
            std::cout << "Creating [" << fullFilePath << "]" << std::endl;
            for (std::string lineStr; std::getline(emailBodyStream, lineStr, '\n');) {
                lineStr.push_back('\n');
                ofs.write(&lineStr[0], lineStr.length());
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
            dateBuffer="";
        }


    }

    return (dateBuffer);

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
        std::string searchDate;

        // Read in command line parameters and process

        procCmdLine(argc, argv, argData);

        // Initialise CMailIMAP internals

        CMailIMAP::init(true);

        // Set mail account user name and password

        imap.setServer(argData.serverURLStr);
        imap.setUserAndPassword(argData.userNameStr, argData.userPasswordStr);

        // Create destination folder

        argData.destinationFolder /= argData.mailBoxNameStr;
        if (!argData.destinationFolder.string().empty() && !fs::exists(argData.destinationFolder)) {
            fs::create_directory(argData.destinationFolder);
        }


        // Get newest file creation date for search

        if (argData.bOnlyUpdates) {
            searchDate = getSearchDate(argData.destinationFolder);
        }

        // Connect

        imap.connect();

        // SELECT mailbox

        parsedResponseStr = imap.sendCommand("SELECT " + argData.mailBoxNameStr);
        parsedResponse = CMailIMAPParse::parseResponse(parsedResponseStr);
        if (parsedResponse->status != CMailIMAPParse::RespCode::OK) {
            throw CMailIMAP::Exception("IMAP SELECT " + parsedResponse->errorMessageStr);
        } else if (parsedResponse->bBYESent) {
            throw CMailIMAP::Exception("Received BYE from server: " + parsedResponse->errorMessageStr);
        }

        // SEARCH for all present email and then create an archive for them.

        if (!searchDate.empty()) {
            std::cout << "Searching from [" << searchDate << "]" << std::endl;
            parsedResponseStr = imap.sendCommand("UID SEARCH SENTSINCE " + searchDate);
        } else {
            parsedResponseStr = imap.sendCommand("UID SEARCH 1:*");
        }

        parsedResponse = CMailIMAPParse::parseResponse(parsedResponseStr);
        if (parsedResponse->status != CMailIMAPParse::RespCode::OK) {
            throw CMailIMAP::Exception("IMAP SEARCH " + parsedResponse->errorMessageStr);
        } else if (parsedResponse->bBYESent) {
            throw CMailIMAP::Exception("Received BYE from server: " + parsedResponse->errorMessageStr);
        } else {
            auto *ptr = static_cast<CMailIMAPParse::SearchResponse *> (parsedResponse.get());
            std::cout << "Messages found = " << ptr->indexes.size() << std::endl;
            for (auto index : ptr->indexes) {
                fetchEmailAndArchive(imap, argData.destinationFolder, index);
            }
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
