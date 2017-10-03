/*
 * File:   CFTP.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on September 23, 2017, 2:33 PM
 *
 * Copyright 2017.
 *
 */

#ifndef CFTP_HPP
#define CFTP_HPP

//
// C++ STL
//

#include <vector>
#include <string>
#include <stdexcept>
#include <fstream>
#include <thread>
#include <memory>
#include <mutex>

//
// Boost ASIO
//

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/filesystem.hpp>

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace FTP {

        // ==========================
        // PUBLIC TYPES AND CONSTANTS
        // ==========================

        // ================
        // CLASS DEFINITION
        // ================

        class CFTP {
        public:

            // ==========================
            // PUBLIC TYPES AND CONSTANTS
            // ==========================

            //
            // Class exception
            //

            struct Exception : public std::runtime_error {

                Exception(std::string const& message)
                : std::runtime_error("CFTP Failure: " + message) {
                }

            };

            // ============
            // CONSTRUCTORS
            // ============

            //
            // Main constructor
            //

            CFTP();

            // ==========
            // DESTRUCTOR
            // ==========

            virtual ~CFTP();

            // ==============
            // PUBLIC METHODS
            // ==============

            //
            // Set/Get FTP server account details
            //

            void setServer(const std::string& serverURL);
            void setServerAndPort(const std::string& serverName,const std::string& serverPort);
            void setUserAndPassword(const std::string& userName, const std::string& userPassword);
            std::string getServer(void) const;
            std::string getUser(void) const;

            //
            // FTP connect, disconnect and connection status
            //

            void connect(void);
            void disconnect(void);
            bool getConnectedStatus(void) const;
            
            // Set FTP passive transfer mode 
            // == true passive mode otherwise active
            
            void setPassiveTransferMode(bool passiveEnabled);
            
            // FTP get and put file
            
            std::uint16_t getFile(const std::string &remoteFilePath, const std::string &localFilePath);
            std::uint16_t putFile(const std::string &remoteFilePath, const std::string &localFilePath);

            // FTP list file/directory
            
            std::uint16_t list(const std::string &directoryPath, std::string &listOutput);
            std::uint16_t listFiles(const std::string &directoryPath, std::string &listOutput);
            
            // FTP set/get current working directory
            
            std::uint16_t changeWorkingDirectory(const std::string &workingDirectoryPath);
            std::uint16_t getCurrentWoringDirectory(std::string &currentWoringDirectory);
        
            // ================
            // PUBLIC VARIABLES
            // ================

        private:

            // ===========================
            // PRIVATE TYPES AND CONSTANTS
            // ===========================

            // ===========================================
            // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
            // ===========================================

            CFTP(const CFTP & orig) = delete;
            CFTP(const CFTP && orig) = delete;
            CFTP& operator=(CFTP other) = delete;

            // ===============
            // PRIVATE METHODS
            // ===============

            std::string determineLocalIPAddress();
            
            bool sendTransferMode();
            
            void sendFTPCommand(const std::string& commandLine);
            void waitForFTPCommandResponse(std::string& commandResponse);
            
            void readDataChannelCommandResponse(std::string &commandResponse);

            void transferFile(const std::string &file, bool downloading);
            
            void downloadFile(const std::string &file);
            void uploadFile(const std::string &file);
                  
            void extractPassiveAddressPort(std::string &pasvResponse);
            
            std::string createPortCommand(); 
            std::uint16_t extractStatusCode(const std::string &commandResponse);
            
            void dataChannelTransferListener();
            void dataChannelTransferCleanup();
            
            // =================
            // PRIVATE VARIABLES
            // =================

            bool bConnected { false }; // == true then connected to server
            
            std::string userName;     // FTP account user name
            std::string userPassword; // FTP account user name password
            std::string serverName;   // FTP server
            std::string serverPort;   // FTP server port

            std::string commandResponse; // FTP command response
            
            std::string dataChannelPassiveAddresss; // Data channel server ip address
            std::string dataChannelPassivePort;     // Data channel server port address
         
            std::string dataChannelActiveAddresss;  // Data channel client ip address
            std::string dataChannelActivePort;      // Data channel client port address
            
            bool passiveMode { false }; // == true passive mode enabled, == false active mode

            boost::asio::io_service ioService;                                // IO Service
            boost::array<char, 32*1024> ioBuffer;                             // IO Buffer
            boost::system::error_code socketError;                            // Last socket error
            boost::asio::ip::tcp::socket controlChannelSocket { ioService };  // Control channel socket
            boost::asio::ip::tcp::socket dataChannelSocket { ioService };     // Data channel socket
            boost::asio::ip::tcp::resolver queryResolver { ioService };       // Name resolver
            
            std::atomic<bool> isListenThreadRunning { false };    // Listen thread running flag
            std::shared_ptr<std::thread> dataChannelListenThread; // Active mode connection listen thread
                

        };

    } // namespace FTP
} // namespace Antik

#endif /* CFTP_HPP */

