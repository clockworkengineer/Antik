/*
 * File:   CFTP.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on January 24, 2017, 2:33 PM
 *
 * Copyright 2016.
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

//
// Boost ASIO
//

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/filesystem.hpp>

using boost::asio::ip::tcp;
namespace fs = boost::filesystem;

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
            // Set/Get email server account details
            //

            void setServer(const std::string& serverURL);
            void setServerAndPort(const std::string& serverName,const std::string& serverPort);
            void setUserAndPassword(const std::string& userName, const std::string& userPassword);
            std::string getServer(void) const;
            std::string getUser(void) const;

            //
            // FTP connect, send command, disconnect and connection status
            //

            void connect(void);
            std::uint16_t sendCommand(const std::string& commandLine);
            void disconnect(void);
            bool getConnectedStatus(void) const;
            
            // FTP passive/active mode on/off
            
            std::uint16_t setPassiveMode(bool enable);
            std::uint16_t setActiveMode(bool enable);
            
            // FTP get and put file
            
            std::uint16_t getFile(const std::string &remoteFilePath, const std::string &localFilePath);
            std::uint16_t putFile(const std::string &remoteFilePath, const std::string &localFilePath);

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
                   
            void sendFTPCommand(const std::string& commandLine);
            void waitForFTPCommandResponse(std::string& commandResponse);
     
            void readReponse(tcp::socket &socket, std::string &buffer);
            
            void listenOnDataChannelAndReadResponse(std::string &buffer);
            void connectOnDataChannelAndReadResponse(std::string &buffer);

            void listenOnDataChannelAndDownloadFile(const std::string &file);
            void connectOnDataChannelAndDownloadFile(const std::string &file);
            void listenOnDataChannelAndUploadFile(const std::string &file);
            void connectOnDataChannelAndUploadFile(const std::string &file);
            
            void downloadFile(tcp::socket &socket, const std::string &file);
            void uploadFile(tcp::socket &socket, const std::string &file);
                  
            void setPassiveAddressPort(std::string &pasvResponse);
            std::string createPortCommand(); 
            std::uint16_t extractStatusCode(const std::string &commandResponse);
            
            // =================
            // PRIVATE VARIABLES
            // =================

            bool bConnected{ false}; // == true then connected to server
            
            std::string userName; // Email account user name
            std::string userPassword; // Email account user name password
            std::string serverName; // FTP server
            std::string serverPort; // FTP server port

            std::string commandResponse; // FTP command response

            boost::asio::io_service ioService;
            boost::array<char, 1024> ioBuffer;
            boost::system::error_code socketError;
            tcp::socket controlChannelSocket { ioService };
                
            std::string dataChannelPassiveAddresss;
            std::string dataChannelPassivePort;
         
            std::string dataChannelActiveAddresss;
            std::string dataChannelActivePort;
            
            bool passiveMode=false;
            bool activeMode=false;

        };

    } // namespace FTP
} // namespace Antik

#endif /* CFTP_HPP */

