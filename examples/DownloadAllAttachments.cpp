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
// Description: A command line program to log on to an IMAP server and download
// any attachments found in any e-mails in a configured mailbox. The destination 
// for any attachment being a folder with the same name as the mailbox created 
// off of the destination folder.All parameters and their meaning are obtained 
// by running the program with the parameter --help
//
// DownloadAllAttachments Example Application
// Program Options:
//   --help                   Print help messages
//   -c [ --config ] arg      Config File Name
//   -s [ --server ] arg      IMAP Server URL and port
//   -u [ --user ] arg        Account username
//   -p [ --password ] arg    User password
//   -m [ --mailbox ] arg     Mailbox name
//   -d [ --destination ] arg Destination for attachments
// 
// Dependencies: C11++, Classes (CMailIMAP, CMailIMAPParse, CMailIMAPBodyStruct),
//               Linux, Boost C++ Libraries.
//
 
// =============
// INCLUDE FILES
// =============

//
// C++ STL definitions
//

#include <iostream>

//
// Antikythera Classes
//

#include "CMailIMAP.hpp"
#include "CMailIMAPParse.hpp"
#include "CMailIMAPBodyStruct.hpp"
#include "CMailSMTP.hpp"

using namespace Antik::Mail;

//
// Boost program options  & file system library definitions
//

#include <boost/program_options.hpp> 
#include <boost/filesystem.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

// ======================
// LOCAL TYES/DEFINITIONS
// ======================

//
// Command line parameter data
//

struct ParamArgData {
    std::string userNameStr;            // Email account user name
    std::string userPasswordStr;        // Email account user name password
    std::string serverURLStr;           // SMTP server URL
    std::string mailBoxNameStr;         // Mailbox name
    fs::path destinationFolder;         // Destination folder for attachments
    std::string configFileNameStr;      // Configuration file name
};

// ===============
// LOCAL FUNCTIONS
// ===============

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
            ("destination,d", po::value<fs::path>(&argData.destinationFolder)->required(), "Destination for attachments");

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

void downloadAttachment(CMailIMAP& imap, fs::path& destinationFolder, CMailIMAPBodyStruct::Attachment &attachment) {

    std::string commandLineStr("FETCH " + attachment.indexStr + " BODY[" + attachment.partNoStr + "]");
    std::string parsedResponseStr(imap.sendCommand(commandLineStr));
    CMailIMAPParse::COMMANDRESPONSE parsedResponse(CMailIMAPParse::parseResponse(parsedResponseStr));

    if ((parsedResponse->status == CMailIMAPParse::RespCode::BAD) ||
            (parsedResponse->status == CMailIMAPParse::RespCode::NO)) {
        throw CMailIMAP::Exception("IMAP FETCH "+parsedResponse->errorMessageStr);
    }

    for (auto fetchEntry : parsedResponse->fetchList) {

        for (auto resp : fetchEntry.responseMap) {

            if (resp.first.find("BODY[" + attachment.partNoStr + "]") == 0) {
                fs::path fullFilePath = destinationFolder / attachment.fileNameStr;

                if (!fs::exists(fullFilePath)) {
                    std::string decodedStringStr;
                    std::istringstream responseStream(resp.second);
                    std::ofstream attachmentFileStream(fullFilePath.string(), std::ios::binary);
                    if (attachmentFileStream.is_open()) {
                        std::cout << "Creating [" << fullFilePath.native() << "]" << std::endl;
                        // Encoded lines have terminating '\r\n' the getline removes '\n'
                        for (std::string lineStr; std::getline(responseStream, lineStr, '\n');) {
                            lineStr.pop_back(); // Remove '\r'
                            CMailSMTP::decodeFromBase64(lineStr, decodedStringStr, lineStr.length());
                            attachmentFileStream.write(&decodedStringStr[0], decodedStringStr.length());
                        }
                    } else {
                        std::cout << "Failed to create file [" << fullFilePath.native() << "]" << std::endl;
                    }
                }

            }
        }
    }
}

//
// For a passed in BODTSTRUCTURE parse and download any base64 encoded attachments.
//

void getBodyStructAttachments(CMailIMAP& imap, std::uint64_t index, fs::path & destinationFolder, const std::string& bodyStructureStr) {

    std::unique_ptr<CMailIMAPBodyStruct::BodyNode> treeBase{ new CMailIMAPBodyStruct::BodyNode()};
    std::shared_ptr<void> attachmentData{ new CMailIMAPBodyStruct::AttachmentData()};

    CMailIMAPBodyStruct::consructBodyStructTree(treeBase, bodyStructureStr);
    CMailIMAPBodyStruct::walkBodyStructTree(treeBase, CMailIMAPBodyStruct::attachmentFn, attachmentData);

    auto attachments = static_cast<CMailIMAPBodyStruct::AttachmentData *> (attachmentData.get());

    if (!attachments->attachmentsList.empty()) {
        for (auto attachment : attachments->attachmentsList) {
            if (CMailIMAPParse::stringEqual(attachment.encodingStr, CMailSMTP::kEncodingBase64Str)) {
                attachment.indexStr = std::to_string(index);
                downloadAttachment(imap, destinationFolder, attachment);
            } else {
                std::cout << "Attachment not base64 encoded but " << attachment.encodingStr << "]" << std::endl;
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
        CMailIMAPParse::COMMANDRESPONSE  parsedResponse;

        // Read in command line parameters and process

        procCmdLine(argc, argv, argData);

        // Initialise CMailIMAP internals

        CMailIMAP::init();

        // Set mail account user name and password
        
        imap.setServer(argData.serverURLStr);
        imap.setUserAndPassword(argData.userNameStr, argData.userPasswordStr);

       // Create destination folder

        argData.destinationFolder += argData.mailBoxNameStr;
        if (!argData.destinationFolder.string().empty() && !fs::exists(argData.destinationFolder)) {
            std::cout << "Creating destination folder = [" << argData.destinationFolder.native() << "]" << std::endl;
            fs::create_directories(argData.destinationFolder);
        }
        
        // Connect
 
        std::cout << "Connecting to server [" << argData.serverURLStr << "]" << std::endl;
               
        imap.connect();

        // SELECT mailbox
        
        parsedResponseStr=imap.sendCommand("SELECT "+argData.mailBoxNameStr);
        parsedResponse = CMailIMAPParse::parseResponse(parsedResponseStr);
        if (parsedResponse->status != CMailIMAPParse::RespCode::OK) {
            throw CMailIMAP::Exception("IMAP SELECT "+parsedResponse->errorMessageStr);
        } else if (parsedResponse->bBYESent) {
            throw CMailIMAP::Exception("Received BYE from server: " + parsedResponse->errorMessageStr);
        }

        // FETCH BODYSTRUCTURE for all mail
        
        parsedResponseStr=imap.sendCommand("FETCH 1:* BODYSTRUCTURE");
        parsedResponse = CMailIMAPParse::parseResponse(parsedResponseStr);
        if (parsedResponse->status != CMailIMAPParse::RespCode::OK) {
            throw CMailIMAP::Exception("IMAP FETCH "+parsedResponse->errorMessageStr);
        }  else if (parsedResponse->bBYESent) {
            throw CMailIMAP::Exception("Received BYE from server: " + parsedResponse->errorMessageStr);
        }

        std::cout << "COMMAND = " << CMailIMAPParse::commandCodeString(parsedResponse->command) << std::endl;

        //  Take decoded response and get any attachments specified in BODYSTRUCTURE.
        
        for (auto fetchEntry : parsedResponse->fetchList) {
            std::cout << "EMAIL INDEX [" << fetchEntry.index << "]" << std::endl;
            for (auto resp : fetchEntry.responseMap) {
                if (resp.first.compare(CMailIMAP::kBODYSTRUCTUREStr) == 0) {
                    getBodyStructAttachments(imap, fetchEntry.index, argData.destinationFolder,resp.second);
                } else {
                    std::cout << resp.first << " = " << resp.second << std::endl;
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
        exitWithError(std::string("BOOST file system exception occured: [") + e.what() + "]");
    } catch (std::exception & e) {
        exitWithError(std::string("Standard exception occured: [") + e.what() + "]");
    }

    // IMAP closedown
    
    CMailIMAP::closedown();

    exit(EXIT_SUCCESS);

}
