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
#include <iomanip>

//
// Antik classes
//

#include "CommonAntik.hpp"
#include "CSocket.hpp"

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
 
                std::uint8_t day { 0 };
                std::uint8_t month { 0 };
                std::uint16_t year { 0 };
                std::uint8_t second { 0 };
                std::uint8_t minute { 0 };
                std::uint8_t hour { 0 };

                DateTime() { };

                DateTime(const std::string &dateTime) {
                    year = std::stoi(dateTime.substr(0, 4));
                    month = std::stoi(dateTime.substr(4, 2));
                    day = std::stoi(dateTime.substr(6, 2));
                    hour = std::stoi(dateTime.substr(8, 2));
                    minute = std::stoi(dateTime.substr(10, 2));
                    second = std::stoi(dateTime.substr(12, 2));
                }

                DateTime(std::tm *dateTime) {
                    year = dateTime->tm_year + 1900;
                    month = dateTime->tm_mon + 1;
                    day = dateTime->tm_mday;
                    hour = dateTime->tm_hour;
                    minute = dateTime->tm_min;
                    second = dateTime->tm_sec;
                }

                bool operator < (const DateTime &other) {
                    if (year != other.year)return (year < other.year);
                    if (month != other.month)return (month < other.month);
                    if (day != other.day)return (day < other.day);
                    if (hour != other.hour)return (hour < other.hour);
                    if (minute != other.minute)return (minute < other.minute);
                    if (second != other.second)return (second < other.second);
                    return (false);
                }

                operator std::string() {
                    std::ostringstream streamDateTime;
                    streamDateTime << std::setw(4) << std::setfill('0') << year;
                    streamDateTime << std::setw(2) << std::setfill('0') << static_cast<int>(month);
                    streamDateTime << std::setw(2) << std::setfill('0') << static_cast<int>(day);
                    streamDateTime << std::setw(2) << std::setfill('0') << static_cast<int>(hour);
                    streamDateTime << std::setw(2) << std::setfill('0') << static_cast<int>(minute);
                    streamDateTime << std::setw(2) << std::setfill('0') << static_cast<int>(second);
                    return streamDateTime.str();
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

            std::uint16_t connect(void);
            std::uint16_t disconnect(void);
            bool isConnected(void) const;
            
            // Set FTP passive transfer mode 
            // == true passive mode otherwise active
            
            void setPassiveTransferMode(bool passiveEnabled);
            
            // FTP get and put file
            
            std::uint16_t getFile(const std::string &remoteFilePath, const std::string &localFilePath);
            std::uint16_t putFile(const std::string &remoteFilePath, const std::string &localFilePath);

            // FTP list file/directory
            
            std::uint16_t list(const std::string &directoryPath, std::string &listOutput);
            std::uint16_t listFiles(const std::string &directoryPath, FileList &fileList);
            std::uint16_t listDirectory(const std::string &directoryPath, std::string &listOutput);
            std::uint16_t listFile(const std::string &filePath, std::string &listOutput);
              
            // FTP set/get current working directory
            
            std::uint16_t changeWorkingDirectory(const std::string &workingDirectoryPath);
            std::uint16_t getCurrentWoringDirectory(std::string &currentWoringDirectory);
            
       
            // FTP make/remove server directory

            std::uint16_t makeDirectory(const std::string &directoryName);            
            std::uint16_t removeDirectory(const std::string &directoryName);
            std::uint16_t cdUp();
 
            // FTP delete/rename remote file, get size in bytes
            
            std::uint16_t deleteFile(const std::string &fileName);
            std::uint16_t renameFile(const std::string &srcFileName, const std::string &dstFileName);
            std::uint16_t fileSize(const std::string &fileName, size_t &fileSize);
            
            // FTP get file last modified time
            
            std::uint16_t getModifiedDateTime(const std::string &filePath, DateTime &modifiedDateTime);
            
            // FTP Is file a directory, does file exist
            
            bool isDirectory(const std::string &fileName);
            bool fileExists(const std::string &fileName);
            
            // FTP server features
            
            std::vector<std::string> getServerFeatures();
            
            // Enable/Disable SSL
            
            void setSslEnabled(bool sslEnabled);
            bool isSslEnabled() const;
            
            // Get last FTP command , returned status code, raw response string.
            
            std::string getLastCommand() const;
            std::uint16_t getCommandStatusCode() const;
            std::string getCommandResponse() const;
            
            // Set transfer type ==true binary == false ASCII
            
            void setBinaryTransfer(bool binaryTransfer);
            bool isBinaryTransfer() const;
        
            // ================
            // PUBLIC VARIABLES
            // ================

        private:

            // ===========================
            // PRIVATE TYPES AND CONSTANTS
            // ===========================
          
            // Data channel transfer types
            
            enum DataTransferType {
                upload,
                download,
                commandResponse
            };
                        
            // ===========================================
            // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
            // ===========================================

            CFTP(const CFTP & orig) = delete;
            CFTP(const CFTP && orig) = delete;
            CFTP& operator=(CFTP other) = delete;

            // ===============
            // PRIVATE METHODS
            // ===============
                   
            // Set data channel transfer mode
            
            bool sendTransferMode();
            
            // FTP command channel I/O to server
            
            void ftpCommand(const std::string& commandLine);
            void ftpResponse();
            
            // Get FTP server features list
            
            void ftpServerFeatures(void);
                   
            // Data channel I/O
 
            void transferOnDataChannel(const std::string &file, DataTransferType transferType);
            void transferOnDataChannel(std::string &commandRespnse);      
            void transferOnDataChannel(const std::string &file, std::string &commandRespnse, DataTransferType transferType);            
 
            void downloadCommandResponse(std::string &commandResponse);
            void downloadFile(const std::string &file);
            void uploadFile(const std::string &file);
 
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
            
            bool m_binaryTransfer { false }; // == true binary transfer otherwise ASCII

            std::string m_commandResponse;        // FTP last command response
            std::uint16_t m_commandStatusCode=0;  // FTP last returned command status code
            std::string m_lastCommand;            // FTP last command sent
            
            bool m_passiveMode { false }; // == true passive mode enabled, == false active mode

            std::unique_ptr<char> m_ioBuffer { nullptr };  // io Buffer
            std::uint32_t m_ioBufferSize     { 64*1024 };

            Antik::Network::CSocket m_controlChannelSocket;
            Antik::Network::CSocket m_dataChannelSocket;

            bool m_sslEnabled { false };
            
            std::vector<std::string> m_serverFeatures;

        };

    } // namespace FTP
} // namespace Antik

#endif /* CFTP_HPP */

