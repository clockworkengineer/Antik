#include "HOST.hpp"
/*
 * File:   SMTPSendMail.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on March 9th, 2017, 2:33 PM
 *
 * Copyright 2017.
 *
 */

//
// Program: SMTPSendMail
//
// Description:  A command line program to log on to an SMTP server and
// send an email to given recipients. The mails details such as contents,
// subject and any attachments are configured via command line arguments.
// All parameters and their meaning are obtained by running the program 
// with the  parameter --help.
//
// SMTPSendMail Example Application
// Program Options:
//   --help                   Print help messages
//   -c [ --config ] arg      Config File Name
//   -s [ --server ] arg      SMTP Server URL and port
//   -u [ --user ] arg        Account username
//   -p [ --password ] arg    User password
//   -r [ --recipients ] arg  Recipients list
//   -s [ --subject ] arg     Email subject
//   -c [ --contents ] arg    File containing email contents
//   -a [ --attachments ] arg File Attachments List
//
// Dependencies: C11++, Classes (CMailSMTP, CFileMIME),
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

#include "CMailSMTP.hpp"
#include "CFileMIME.hpp"

using namespace Antik::Mail;

//
// Boost program options  & file system library definitions
//

#include "boost/program_options.hpp" 
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
    std::string userNameStr; // Email account user name
    std::string userPasswordStr; // Email account user name password
    std::string serverURLStr; // SMTP server URL
    std::string configFileNameStr;  // Configuration file name
    std::string recipientsStr;  // List of recipeints
    std::string subjectStr; // Email subject
    std::string mailContentsFileStr;    // File containing email contents
    std::string attachmentListStr;  // List of attachments
};

// ===============
// LOCAL FUNCTIONS
// ===============

//
// Exit with error message/status
//

void exitWithError(std::string errMsgStr) {

    // Closedown SMTP transport and display error and exit.

    CMailSMTP::closedown();
    std::cerr << errMsgStr << std::endl;
    exit(EXIT_FAILURE);

}

//
// Add options common to both command line and config file
//

void addCommonOptions(po::options_description& commonOptions, ParamArgData& argData) {

    commonOptions.add_options()
            ("server,s", po::value<std::string>(&argData.serverURLStr)->required(), "SMTP Server URL and port")
            ("user,u", po::value<std::string>(&argData.userNameStr)->required(), "Account username")
            ("password,p", po::value<std::string>(&argData.userPasswordStr)->required(), "User password")
            ("recipients,r", po::value<std::string>(&argData.recipientsStr)->required(), "Recipients list")
            ("subject,s", po::value<std::string>(&argData.subjectStr)->required(), "Email subject")
            ("contents,c", po::value<std::string>(&argData.mailContentsFileStr)->required(), "File containing email contents")
            ("attachments,a", po::value<std::string>(&argData.attachmentListStr)->required(), "File Attachments List");

}

//
// Read in and process command line arguments using boost.
//

void procCmdLine(int argc, char** argv, ParamArgData &argData) {

    // Default values


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
            std::cout << "SMTPSendMail Example Application" << std::endl << commandLine << std::endl;
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
        std::cerr << "SMTPSendMail Error: " << e.what() << std::endl << std::endl;
        std::cerr << commandLine << std::endl;
        exit(1);
    }

}

// ============================
// ===== MAIN ENTRY POINT =====
// ============================

int main(int argc, char** argv) {

    try {

        CMailSMTP mail;
        std::vector<std::string> mailMessage;

        // Read in command lien parameters and process

        ParamArgData argData;
        procCmdLine(argc, argv, argData);

        // Initialise SMTP transport
        
        CMailSMTP::init(true);

        // Set server and account details
        
        mail.setServer(argData.serverURLStr);
        mail.setUserAndPassword(argData.userNameStr, argData.userPasswordStr);

        // Set Sender and recipients
        
        mail.setFromAddress("<" + argData.userNameStr + ">");
        mail.setToAddress(argData.recipientsStr);

        // Set mail subject
        
        mail.setMailSubject(argData.subjectStr);

        // Set mail contents
        
        if (!argData.configFileNameStr.empty()) {
            if (fs::exists(argData.mailContentsFileStr)) {
                std::ifstream mailContentsStream(argData.mailContentsFileStr);
                if (mailContentsStream.is_open()) {
                    for (std::string lineStr; std::getline(mailContentsStream, lineStr, '\n');) {
                        mailMessage.push_back(lineStr);
                    }
                    mail.setMailMessage(mailMessage);
                }
            }
        }

        // Add any attachments. Note all base64 encoded.
        
        if (!argData.attachmentListStr.empty()) {
            std::istringstream attachListStream(argData.attachmentListStr);
            for (std::string attachmentStr; std::getline(attachListStream, attachmentStr, ',');) {
                attachmentStr = attachmentStr.substr(attachmentStr.find_first_not_of(' '));
                attachmentStr = attachmentStr.substr(0, attachmentStr.find_last_not_of(' ') + 1);
                if (fs::exists(attachmentStr)){
                    std::cout << "Attaching file [" << attachmentStr << "]" << std::endl;
                    mail.addFileAttachment(attachmentStr, Antik::File::CFileMIME::getFileMIMEType(attachmentStr), "base64");
                } else {
                    std::cout << "File does not exist [" << attachmentStr << "]" << std::endl;
                }

            }
        }
      
        // Send mail
        
        mail.postMail();
        
        //
        // Catch any errors
        //    

    } catch (CMailSMTP::Exception &e) {
        exitWithError(e.what());
    } catch (const fs::filesystem_error & e) {
        exitWithError(std::string("BOOST file system exception occured: [") + e.what() + "]");
    } catch (std::exception & e) {
        exitWithError(std::string("Standard exception occured: [") + e.what() + "]");
    }

    // Closedown SMTP transport
    
    CMailSMTP::closedown();

    exit(EXIT_SUCCESS);

}


