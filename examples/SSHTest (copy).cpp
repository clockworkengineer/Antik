/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SSHTest.cpp
 * Author: robt
 *
 * Created on 27 December 2017, 17:08
 */

#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>
#include <thread>

#include "SSHSessionUtil.hpp"
#include "SFTPUtil.hpp"
#include "FTPUtil.hpp"
#include "SSHChannelUtil.hpp"

using namespace std;
using namespace Antik::SSH;

//
// Boost program options  & file system library
//

#include <boost/program_options.hpp>  
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/range/iterator_range.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

struct ParamArgData {
    std::string userName; // FTP account user name
    std::string userPassword; // FTP account user name password
    std::string serverName; // FTP server
    std::string serverPort; // FTP server port
    std::string localDirectory; // Local directory
    std::string configFileName; // Configuration file name
    std::vector<std::string> fileList; // File list
    int stressTestCount {0 }; // Stress test repeat count
    int generalTestCount { 0 }; // General test repeat count
};

//
// Exit with error message/status.
//

static void exitWithError(const string &errMsg) {

    // Display error and exit.

    cerr << errMsg << endl;
    exit(EXIT_FAILURE);

}

//
// Add options common to both command line and config file
//

void addCommonOptions(po::options_description& commonOptions, ParamArgData& argData) {

    commonOptions.add_options()
            ("server,s", po::value<std::string>(&argData.serverName)->required(), "FTP Server name")
            ("port,o", po::value<std::string>(&argData.serverPort)->required(), "FTP Server port")
            ("user,u", po::value<std::string>(&argData.userName)->required(), "Account username")
            ("password,p", po::value<std::string>(&argData.userPassword)->required(), "User password")
     //       ("local,l", po::value<std::string>(&argData.localDirectory)->required(), "Local directory")
     //       ("files,f", po::value<std::vector < std::string >> (&argData.fileList)->multitoken()->required(), "Files")
            ("stress,t", po::value<int>(&argData.stressTestCount), "Stress test repeat count")
            ("general,g", po::value<int>(&argData.generalTestCount), "General test repeat count");

}

//
// Read in and process command line arguments using boost.
//

void procCmdLine(int argc, char** argv, ParamArgData &argData) {

    // Define and parse the program options

    po::options_description commandLine("Program Options");
    commandLine.add_options()
            ("help", "Print help messages")
            ("config,c", po::value<std::string>(&argData.configFileName), "Config File Name");

    addCommonOptions(commandLine, argData);

    po::options_description configFile("Config Files Options");

    addCommonOptions(configFile, argData);

    po::variables_map vm;

    try {

        // Process arguments

        po::store(po::parse_command_line(argc, argv, commandLine), vm);

        // Display options and exit with success

        if (vm.count("help")) {
            std::cout << "SSHStandaloneTests" << std::endl << commandLine << std::endl;
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
        std::cerr << "SSHStandaloneTests Error: " << e.what() << std::endl << std::endl;
        std::cerr << commandLine << std::endl;
        exit(EXIT_FAILURE);
    }

}


//void writeOutput(void *ioBuffer, uint32_t bytesToWrite) {
//    std::cerr.write(static_cast<char *> (ioBuffer), bytesToWrite);
//    std::cerr.flush();
//}
//
//int channelProcessing(CSSHSession &session) {
//
//    CSSHChannel channel{ session};
//
//    try {
//
//        channel.open();
//        interactiveShell(channel, 80, 24);
//        channel.sendEndOfFile();
//        channel.close();
//
//    } catch (CSSHChannel::Exception &e) {
//        channel.close();
//        throw;
//    } catch (CSSHSession::Exception &e) {
//        channel.close();
//        throw;
//    }
//
//}
//
//int web_server(CSSHSession &session) {
//
//    std::unique_ptr<CSSHChannel> channel;
//    int port = 0;
//std::string helloworld{""
//        "HTTP/1.1 200 OK\n"
//        "Content-Type: text/html\n"
//        "Content-Length: 113\n"
//        "\n"
//        "<html>\n"
//        "  <head>\n"
//        "    <title>Hello,x World!</title>\n"
//        "  </head>\n"
//        "  <body>\n"
//        "    <h1>Hello, World!</h1>\n"
//        "  </body>\n"
//        "</html>\n"};
//
//    CSSHChannel::listenForward(session, "", 8080, &port);
//
//    channel = CSSHChannel::acceptForward(session, 60000, &port);
//
//    if (channel) {
//        while (1) {
//            channel->read(channel->getIoBuffer().get(), channel->getIoBufferSize(), 0);
//            if (strncmp(channel->getIoBuffer().get(), "GET /", 5)) continue;
//            channel->write(&helloworld[0], helloworld.size());
//            printf("Sent answer\n");
//        }
//        channel->sendEndOfFile();
//        channel->close();
//    } else {
//            printf("Time out\n");       
//    }
//
//}

void sessionAndChannelStressTest(ParamArgData &argData, int count) {
    
    for (int cnt01 = 0; cnt01 < count; cnt01++) {
        
        CSSHSession sshSession;
  
        std::cout << "Session connect ..." << cnt01 << std::endl;

        sshSession.setServer(argData.serverName);
        sshSession.setUser(argData.userName);
        sshSession.setUserPassword(argData.userPassword);
        sshSession.setPort(std::stoi(argData.serverPort));

        sshSession.connect();

        // Verify the server's identity

        if (!verifyKnownServer(sshSession)) {
            throw runtime_error("Unable to verify server.");
        } else {
            std::cout << "Server verified..." << std::endl;
        }

        // Authenticate ourselves

        if (!userAuthorize(sshSession)) {
            throw runtime_error("Server unable to authorize client");
        } else {
            std::cout << "Client authorized..." << std::endl;
        }
          
        
        for (int cnt02 = 0; cnt02 < count; cnt02++) {
            CSSHChannel channel {sshSession};
            std::cout << "Channel ..." << cnt02 << std::endl;
            channel.open();
            channel.close();
        }

        std::cout << "Session dicconnect." << std::endl;

        sshSession.disconnect();

    }
    
}

int main(int argc, char** argv) {
    
       ParamArgData argData;
   
        // Read in command line parameters and process

        procCmdLine(argc, argv, argData);

        std::cout << "SERVER [" << argData.serverName << "]" << std::endl;
        std::cout << "SERVER PORT [" << argData.serverPort << "]" << std::endl;
        std::cout << "USER [" << argData.userName << "]" << std::endl;
        
    CSSHSession sshSession;

    try {

        cout << hex << "LIBSSH Version: " << LIBSSH_VERSION_INT << dec << endl;

        sessionAndChannelStressTest(argData, 100);
        
        exit(-1);
          //sshSession.setLogging(SSH_LOG_FUNCTIONS);
      //  sshSession.setLogging(SSH_LOG_FUNCTIONS);
        // Connect to server

         
        sshSession.connect();

        // Verify the server's identity

        if (!verifyKnownServer(sshSession)) {
            throw runtime_error("Unable to verify server.");
        } else {
            std::cout << "Server verified..." << std::endl;
        }

        // Authenticate ourselves

        if (!userAuthorize(sshSession)) {
            throw runtime_error("Server unable to authorize client");
        } else {
            std::cout << "Client authorized..." << std::endl;
        }

        std::cout << "Server SSH Version : " << sshSession.getSSHVersion() << std::endl;
        std::cout << std::hex << "Server Open SSH Version : " << sshSession.getOpenSSHVersion() << std::dec << std::endl;
        std::cout << "Server Session Cipher In : " << sshSession.getCipherIn() << std::endl;
        std::cout << "Server Session Cipher Out : " << sshSession.getCipherOut() << std::endl;
        std::cout << "Client Banner : " << sshSession.getClientBanner() << std::endl;
        std::cout << "Server Banner : " << sshSession.getServerBanner() << std::endl;

        std::cout << "Server Banner \n" << std::string(80, '*') + "\n" << sshSession.getBanner() << std::string(80, '*') << std::endl;

       // web_server(sshSession);
//
//        CSSHChannel forwardingChannel{sshSession};
//
//        std::thread forwardingThread = directForwarding(forwardingChannel, "www.google.com", 80, "localhost", 5555, writeOutput);
//
//        std::string http_get{ "GET / HTTP/1.1\nHost: www.google.com\n\n"};
//        forwardingChannel.write(&http_get[0], http_get.size());
//
//        std::this_thread::sleep_for(std::chrono::seconds(1));
//
//        forwardingChannel.sendEndOfFile();
//        forwardingChannel.close();
//
//        forwardingThread.join();

        //      forwardingChannel.close();


        
 //      channelProcessing(sshSession);

//        CSFTP sftp{ sshSession};
//
//        sftp.open();
//
//        std::cout << "Server SFTP Version : " << sftp.getServerVersion() << std::endl;
//
//        std::cout << "Server Extension: " << sftp.getExtensionCount() << std::endl;
//
//        for (int index = 0; index < sftp.getExtensionCount(); index++) {
//            std::cout << "Name: [" << sftp.getExtensionName(index) << "]  Data: [" << sftp.getExtensionData(index) << "]" << std::endl;
//        }

        //        FileList fileList;
        //        
        //        listRemoteRecursive(sftp, "/home/pi/ftproot/", fileList);
        //        
        //        for (auto remoteFile : fileList) {
        //            std::cout << remoteFile << std::endl;
        //        }
//
        //sftp.close();

        sshSession.disconnect();

        cout << "[" << sshSession.getDisconnectMessage() << "]" << endl;

    } catch (const CSSHChannel::Exception &e) {
        exitWithError(e.getMessage());
    } catch (const CSSHSession::Exception &e) {
        exitWithError(e.getMessage());
    } catch (const CSFTP::Exception &e) {
        exitWithError(e.getMessage());
    } catch (const runtime_error &e) {
        exitWithError(e.what());
    } catch (const exception &e) {
        exitWithError(string("Standard exception occured: [") + e.what() + "]");
    }

    exit(EXIT_SUCCESS);

}
