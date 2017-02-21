#include "HOST.hpp"
/*
 * File:   DownloadAllAttachments.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2016, 2:33 PM
 *
 * Copyright 2016.
 *
 */

//
// Program: DownloadAllAttachments
//
// Description: Log on to a given IMAP server and download attachments found
// in all mail in a specific mailbox to a given local folder.
// 
/// 
// Dependencies: C11++, Classes (CMailIMAP, CMailIMAPParse, CMailIMAPBodyStruct),
//               Linux, Boost C++ Libraries.
//
 
//
// C++ STL definitions
//

#include <iostream>

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
    std::string userName;            // Email account user name
    std::string userPassword;        // Email account user name password
    std::string serverURL;           // SMTP server URL
    std::string mailBoxName;         // Mailbox name
    std::string destinationFolder;   // Destination folder for attachments
    std::string configFileName;      // Configuration file name
};

//
// Exit with error message/status
//

void exitWithError(std::string errmsg) {

    // Closedown email, display error and exit.

    CMailIMAP::closedown();
    std::cerr << errmsg << std::endl;
    exit(EXIT_FAILURE);

}

//
// Add options common to both command line and config file
//

void addCommonOptions(po::options_description& commonOptions, ParamArgData& argData) {
    
    commonOptions.add_options()
            ("server,s", po::value<std::string>(&argData.serverURL)->required(), "IMAP Server URL and port")
            ("user,u", po::value<std::string>(&argData.userName)->required(), "Account username")
            ("password,p", po::value<std::string>(&argData.userPassword)->required(), "User password")
            ("mailbox,m", po::value<std::string>(&argData.mailBoxName)->required(), "Mailbox name")
            ("destination,d", po::value<std::string>(&argData.destinationFolder)->required(), "Destination for attachments");

}
//
// Read in and process command line arguments using boost.
//

void procCmdLine(int argc, char** argv, ParamArgData &argData) {

    // Define and parse the program options

    po::options_description commandLine("Program Options");
    commandLine.add_options()
            ("help", "Print help messages")
            ("config,c", po::value<std::string>(&argData.configFileName)->required(), "Config File Name");

    addCommonOptions(commandLine, argData);
    
    po::options_description configFile("Config Files Options");
  
    addCommonOptions(configFile, argData);
       
    po::variables_map vm;

    try {

        // Process arguments

        po::store(po::parse_command_line(argc, argv, commandLine), vm);

        // Display options and exit with success

        if (vm.count("help")) {
            std::cout << "DownloadAllAttachments Example Application" << std::endl << commandLine << std::endl;
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
        std::cerr << "DownloadAllAttachments Error: " << e.what() << std::endl << std::endl;
        std::cerr << commandLine << std::endl;
        exit(EXIT_FAILURE);
    }

}

//
// Download an attachment, decode it and write to local folder.
//

void downloadAttachment(CMailIMAP& imap, const std::string& folderPath, CMailIMAPBodyStruct::Attachment &attachment) {

    std::string commandLine("FETCH " + attachment.index + " BODY[" + attachment.partNo + "]");
    std::string parsedResponseStr(imap.sendCommand(commandLine));
    CMailIMAPParse::BASERESPONSE parsedResponse(CMailIMAPParse::parseResponse(parsedResponseStr));

    if ((parsedResponse->status == CMailIMAPParse::RespCode::BAD) ||
            (parsedResponse->status == CMailIMAPParse::RespCode::NO)) {
        throw CMailIMAP::Exception("IMAP FETCH "+parsedResponse->errorMessageStr);
    }

    CMailIMAPParse::FetchResponse *ptr = static_cast<CMailIMAPParse::FetchResponse *> (parsedResponse.get());

    for (auto fetchEntry : ptr->fetchList) {
        for (auto resp : fetchEntry.responseMap) {
            if (resp.first.find("BODY[" + attachment.partNo + "]") == 0) {
                std::string decodedString;
                std::istringstream responseStream(resp.second);
                std::ofstream ofs(folderPath+attachment.fileName, std::ios::binary);
                std::cout << "Creating [" << folderPath+attachment.fileName << "]" << std::endl;
                for (std::string lineStr; std::getline(responseStream, lineStr, '\n');) {
                    lineStr.pop_back();
                    CMailSMTP::decodeFromBase64(lineStr, decodedString, lineStr.length());
                    ofs.write(&decodedString[0], decodedString.length());
                }
            }
        }
    }
}

//
// For a passed in BODTSTRUCTURE parse and download any base64 encoded attachments.
//

void getBodyStructAttachments(CMailIMAP& imap, std::uint64_t index, const std::string& folderPath, const std::string& bodyStructure) {

    std::unique_ptr<CMailIMAPBodyStruct::BodyNode> treeBase{ new CMailIMAPBodyStruct::BodyNode()};
    std::shared_ptr<void> attachmentData{ new CMailIMAPBodyStruct::AttachmentData()};

    CMailIMAPBodyStruct::consructBodyStructTree(treeBase, bodyStructure);
    CMailIMAPBodyStruct::walkBodyStructTree(treeBase, CMailIMAPBodyStruct::attachmentFn, attachmentData);

    auto attachments = static_cast<CMailIMAPBodyStruct::AttachmentData *> (attachmentData.get());

    if (!attachments->attachmentsList.empty()) {
        for (auto attachment : attachments->attachmentsList) {
            if (CMailIMAPParse::stringEqual(attachment.encoding, CMailSMTP::kEncodingBase64)) {
                attachment.index = std::to_string(index);
                downloadAttachment(imap, folderPath, attachment);
            } else {
                std::cout << "Attachment not base64 encoded but " << attachment.encoding << "]" << std::endl;
            }
        }
    } else {
        std::cout << "No attachments present." << std::endl;
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
        CMailIMAPParse::BASERESPONSE  parsedResponse;

        // Read in command line parameters and process

        procCmdLine(argc, argv, argData);

        // Initialise CMailIMAP internals

        CMailIMAP::init();

        // Set mail account user name and password
        
        imap.setServer(argData.serverURL);
        imap.setUserAndPassword(argData.userName, argData.userPassword);

        // Connect
        
        imap.connect();

        // SELECT mailbox
        
        parsedResponseStr=imap.sendCommand("SELECT "+argData.mailBoxName);
        parsedResponse = CMailIMAPParse::parseResponse(parsedResponseStr);
        if (parsedResponse->status != CMailIMAPParse::RespCode::OK) {
            throw CMailIMAP::Exception("IMAP SELECT "+parsedResponse->errorMessageStr);
        }

        // FETCH BODYSTRUCTURE for all mail
        
        parsedResponseStr=imap.sendCommand("FETCH 1:* BODYSTRUCTURE");
        parsedResponse = CMailIMAPParse::parseResponse(parsedResponseStr);
        if (parsedResponse->status != CMailIMAPParse::RespCode::OK) {
            throw CMailIMAP::Exception("IMAP FETCH "+parsedResponse->errorMessageStr);
        }

        CMailIMAPParse::FetchResponse *ptr = static_cast<CMailIMAPParse::FetchResponse *> (parsedResponse.get());
        std::cout << "COMMAND = " << CMailIMAPParse::commandCodeString(ptr->command) << std::endl;

        //  Take decoded response and get any attachments specified in BODYSTRUCTURE.
        
        for (auto fetchEntry : ptr->fetchList) {
            std::cout << "EMAIL INDEX [" << fetchEntry.index << "]" << std::endl;
            for (auto resp : fetchEntry.responseMap) {
                if (resp.first.compare(CMailIMAP::kBODYSTRUCTUREStr) == 0) {
                    getBodyStructAttachments(imap, fetchEntry.index, argData.destinationFolder,resp.second);
                } else {
                    std::cout << resp.first << " = " << resp.second << std::endl;
                }
            }
        }

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
