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
#include <thread>
#include <memory>
#include <mutex>

//
// Boost ASIO
//

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

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
            
            //
            // Date Time (could be more compact)
            //
            
            struct DateTime {
                std::uint8_t day;
                std::uint8_t month;
                std::uint16_t year;
                std::uint8_t second;
                std::uint8_t minute;
                std::uint8_t hour;
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

            std::uint16_t connect(void);
            std::uint16_t disconnect(void);
            bool getConnectedStatus(void) const;
            
            // Set FTP passive transfer mode 
            // == true passive mode otherwise active
            
            void setPassiveTransferMode(bool passiveEnabled);
            
            // FTP get and put file
            
            std::uint16_t getFile(const std::string &remoteFilePath, const std::string &localFilePath);
            std::uint16_t putFile(const std::string &remoteFilePath, const std::string &localFilePath);

            // FTP list file/directory
            
            std::uint16_t list(const std::string &directoryPath, std::string &listOutput);
            std::uint16_t listFiles(const std::string &directoryPath, std::vector<std::string> &fileList);
            std::uint16_t listDirectory(const std::string &directoryPath, std::string &listOutput);
            std::uint16_t listFile(const std::string &filePath, std::string &listOutput);
              
            // FTP set/get current working directory
            
            std::uint16_t changeWorkingDirectory(const std::string &workingDirectoryPath);
            std::uint16_t getCurrentWoringDirectory(std::string &currentWoringDirectory);
            
       
            // FTP make/remove server directory

            std::uint16_t makeDirectory(const std::string &directoryName);            
            std::uint16_t removeDirectory(const std::string &directoryName);
 
            // FTP delete remote file, get size in bytes
            
            std::uint16_t deleteFile(const std::string &fileName);
            std::uint16_t fileSize(const std::string &fileName, size_t &fileSize);
            
            // FTP get file last modified time
            
            std::uint16_t getModifiedDateTime(const std::string &filePath, DateTime &modifiedDateTime);
            
            // FTP Is file a directory
            
            bool isDirectory(const std::string &fileName);
            
            // Enable/Disable SSL
            
            void setSslEnabled(bool sslEnabled);
            bool isSslEnabled() const;
            
            // Get last FTP command , returned status code, raw response string.
            
            std::string getLastCommand() const;
            std::uint16_t getCommandStatusCode() const;
            std::string getCommandResponse() const;
        
            // ================
            // PUBLIC VARIABLES
            // ================

        private:

            // ===========================
            // PRIVATE TYPES AND CONSTANTS
            // ===========================

            typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> SSLSocket;
                        
            // ===========================================
            // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
            // ===========================================

            CFTP(const CFTP & orig) = delete;
            CFTP(const CFTP && orig) = delete;
            CFTP& operator=(CFTP other) = delete;

            // ===============
            // PRIVATE METHODS
            // ===============

            // Socket I/O functions
            
            void connectSocket(SSLSocket &socket, const std::string &hostAddress, const std::string &hostPort);
            size_t readFromSocket(SSLSocket &socket);       
            size_t writeToSocket(SSLSocket &socket, const char *writeBuffer, size_t writeLength);
            void closeSocket(SSLSocket &socket);    
            void  switchOnSSL(SSLSocket &socket);   
            bool socketClosedByServer();
            
            // Get local IP address
            
            std::string determineLocalIPAddress();
            
            // Set data channel transfer mode
            
            bool sendTransferMode();
            
            // FTP command channel I/O server
            
            void ftpCommand(const std::string& commandLine);
            std::uint16_t ftpResponse();
            
            // Data channel I/O
            
            void readCommandResponse(std::string &commandResponse);
            void transferFile(const std::string &file, bool downloading);            
            void downloadFile(const std::string &file);
            void uploadFile(const std::string &file);
            void generateListenPort();
            void transferConnectionListener();
            void postTransferCleanup();

            // PORT/PASV related methods
            
            void extractPassiveAddressPort(std::string &pasvResponse);          
            std::string createPortCommand(); 
           
            // =================
            // PRIVATE VARIABLES
            // =================

            bool m_connected { false }; // == true then connected to server
            
            std::string m_userName;     // FTP account user name
            std::string m_userPassword; // FTP account user name password
            std::string m_serverName;   // FTP server
            std::string m_serverPort;   // FTP server port

            std::string m_commandResponse;        // FTP last command response
            std::uint16_t m_commandStatusCode=0;  // FTP last returned command status code
            std::string m_lastCommand;            // FTP last command sent
            
            std::string m_dataChannelPassiveAddresss; // Data channel server ip address
            std::string m_dataChannelPassivePort;     // Data channel server port address      
            std::string m_dataChannelActiveAddresss;  // Data channel client ip address
            std::string m_dataChannelActivePort;      // Data channel client port address
            
            bool m_passiveMode { false }; // == true passive mode enabled, == false active mode

            boost::asio::io_service m_ioService;                                  // io Service
            std::array<char, 32*1024> m_ioBuffer;                                 // io Buffer
            boost::system::error_code m_ioSocketError;                            // io last socket error
            boost::asio::ip::tcp::resolver m_ioQueryResolver { m_ioService };     // io name resolver
            
            std::atomic<bool> m_isListenThreadRunning { false };    // Listen thread running flag
            std::shared_ptr<std::thread> m_dataChannelListenThread; // Active mode connection listen thread
   
            boost::asio::ssl::context sslContext { m_ioService, boost::asio::ssl::context::tlsv12 };

            SSLSocket m_controlChannelSocket {m_ioService, sslContext};
            SSLSocket m_dataChannelSocket {m_ioService, sslContext};
            bool m_sslConnectionActive { false };
            bool m_sslEnabled { false };

        };

    } // namespace FTP
} // namespace Antik

#endif /* CFTP_HPP */

