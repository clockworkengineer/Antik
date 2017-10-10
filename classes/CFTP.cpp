#include "HOST.hpp"
/*
 * File:   CFTP.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on September 23, 2017, 2:33 PM
 *
 * Copyright 2017.
 *
 */

//
// Class: CFTP
// 
// Description: A class to connect to an FTP server using provided credentials
// and enable the uploading/downloading of files along with assorted other commands.
// It uses boost ASIO for all communication with the server but besides that it is just
// standard C11++.
//
// Note: TLS/SSL connections are supported.
//
// Dependencies:   C11++        - Language standard features used.
//                 BOOST ASIO   - Used to talk to FTP server.
//

// =================
// CLASS DEFINITIONS
// =================

#include "CFTP.hpp"

// ====================
// CLASS IMPLEMENTATION
// ====================

//
// C++ STL
//

#include <iostream>
#include <fstream>

// =======
// IMPORTS
// =======

namespace asio = boost::asio;
namespace ip = asio::ip;

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace FTP {

        // ===========================
        // PRIVATE TYPES AND CONSTANTS
        // ===========================

        // ==========================
        // PUBLIC TYPES AND CONSTANTS
        // ==========================

        // ========================
        // PRIVATE STATIC VARIABLES
        // ========================

        // =======================
        // PUBLIC STATIC VARIABLES
        // =======================

        // ===============
        // PRIVATE METHODS
        // ===============

        //
        // Connect to a given host and port.
        //

        inline void CFTP::connectSocket(SSLSocket &socket, const std::string &hostAddress, const std::string &hostPort) {

            ip::tcp::resolver::query query(hostAddress, hostPort);
            socket.next_layer().connect(*m_ioQueryResolver.resolve(query), m_ioSocketError);
            if (m_ioSocketError) {
                throw Exception(m_ioSocketError.message());
            }

        }

        //
        // Read data from socket into io buffer
        //

        inline size_t CFTP::readFromSocket(SSLSocket &socket) {

            if (m_sslConnectionActive) {
                return (socket.read_some(asio::buffer(m_ioBuffer, m_ioBuffer.size() - 1), m_ioSocketError));
            } else {
                return (socket.next_layer().read_some(asio::buffer(m_ioBuffer, m_ioBuffer.size() - 1), m_ioSocketError));
            }

        }

        //
        // Write data to socket
        //

        inline size_t CFTP::writeToSocket(SSLSocket &socket, const char *writeBuffer, size_t writeLength) {

            if (m_sslConnectionActive) {
                return (socket.write_some(asio::buffer(writeBuffer, writeLength), m_ioSocketError));
            } else {
                return (socket.next_layer().write_some(asio::buffer(writeBuffer, writeLength), m_ioSocketError));
            }

        }

        //
        // Perform TLS handshake to enable SSL
        //

        inline void CFTP::switchOnSSL(SSLSocket &socket) {
            socket.handshake(SSLSocket::client, m_ioSocketError);
            if (m_ioSocketError) {
                throw Exception(m_ioSocketError.message());
            }
        }

        //
        // Closedown any running SSL and close socket.
        //

        inline void CFTP::closeSocket(SSLSocket &socket) {
            if (socket.next_layer().is_open()) {
                if (m_sslConnectionActive) {
                   socket.shutdown(m_ioSocketError);
                }
                socket.next_layer().close();
            }
        }

        //
        // Return true if socket closed by server otherwise false.
        // Also throw exception for any socket error detected,
        //

        inline bool CFTP::socketClosedByServer() {

            if (m_ioSocketError == asio::error::eof) {
                return (true); // Connection closed cleanly by peer.
            } else if (m_ioSocketError) {
                throw Exception(m_ioSocketError.message());
            }

            return (false);

        }

        //
        // Work out ip address for local machine. This is quite difficult to achieve but
        // this is the best code i have seen for doing it. It just tries to connect to
        // google.com with a udp connect to get the local socket endpoint.
        // Note: Fall back of localhost on failure.
        // 

        std::string CFTP::determineLocalIPAddress() {

            std::string localIPAddress{ "127.0.0.1"};

            try {
                ip::udp::resolver resolver(m_ioService);
                ip::udp::resolver::query query(ip::udp::v4(), "google.com", "");
                ip::udp::socket socket(m_ioService);
                socket.connect(*resolver.resolve(query));
                localIPAddress = socket.local_endpoint().address().to_string();
                socket.close();
            } catch (std::exception &e) {
                return (localIPAddress);
            }

            return (localIPAddress);

        }

        //
        // Extract host ip address and port information from passive command reply.
        //

        void CFTP::extractPassiveAddressPort(std::string &pasvResponse) {

            try {

                std::string passiveParams = pasvResponse.substr(pasvResponse.find('(') + 1);
                passiveParams = passiveParams.substr(0, passiveParams.find(')'));

                std::uint32_t port = std::stoi(passiveParams.substr(passiveParams.find_last_of(',') + 1));
                passiveParams = passiveParams.substr(0, passiveParams.find_last_of(','));
                port |= (std::stoi(passiveParams.substr(passiveParams.find_last_of(',') + 1)) << 8);

                passiveParams = passiveParams.substr(0, passiveParams.find_last_of(','));
                for (auto &byte : passiveParams) {
                    if (byte == ',') byte = '.';
                }

                m_dataChannelPassiveAddresss = passiveParams;
                m_dataChannelPassivePort = std::to_string(port);

            } catch (std::exception &e) {
                throw CFTP::Exception(e.what());
            }

        }

        //
        // Create PORT command to send over control channel.
        //

        std::string CFTP::createPortCommand() {

            std::string portCommand{ "PORT "};

            try {

                std::uint16_t port = std::stoi(m_dataChannelActivePort);
                portCommand += m_dataChannelActiveAddresss;
                for (auto &byte : portCommand) {
                    if (byte == '.') byte = ',';
                }
                portCommand += "," + std::to_string((port & 0xFF00) >> 8) + "," + std::to_string(port & 0xFF);


            } catch (std::exception &e) {
                throw CFTP::Exception(e.what());
            }

            return (portCommand);

        }

        //
        // Download a file from FTP server to local system.
        //

        void CFTP::downloadFile(const std::string &file) {

            std::ofstream localFile{ file, std::ofstream::trunc | std::ofstream::binary};

            do {

                size_t bytesRead = readFromSocket(m_dataChannelSocket);

                if (bytesRead) {
                    localFile.write(&m_ioBuffer[0], bytesRead);
                }

            } while (!socketClosedByServer());

            localFile.close();

        }

        //
        // Upload file from local system to FTP server,
        //

        void CFTP::uploadFile(const std::string &file) {

            std::ifstream localFile{ file, std::ifstream::binary};

            if (localFile) {

                do {

                    localFile.read(&m_ioBuffer[0], m_ioBuffer.size());

                    size_t bytesToWrite = localFile.gcount();

                    if (bytesToWrite) {

                        for (;;) {
                            bytesToWrite -= writeToSocket(m_dataChannelSocket, &m_ioBuffer[localFile.gcount() - bytesToWrite], bytesToWrite);
                            if ((bytesToWrite == 0) || socketClosedByServer()) {
                                break;
                            }
                        }

                    }

                } while (localFile && !socketClosedByServer());


                localFile.close();

            }

        }

        //
        // Generate random port to listen om in active connections
        //
        
        void CFTP::generateListenPort() {
            
            ip::tcp::acceptor acceptor(m_ioService, ip::tcp::endpoint(ip::tcp::v4(), 0));
            unsigned short port = acceptor.local_endpoint().port();
            auto address = acceptor.local_endpoint().address();

            m_dataChannelActivePort = std::to_string(port);
            
        }
        
        //
        // Data channel socket listener thread function for incoming data 
        // channel connections.
        //

        void CFTP::transferConnectionListener() {
            
            generateListenPort();

            ip::tcp::acceptor acceptor(m_ioService, ip::tcp::endpoint(ip::tcp::v4(), std::stoi(m_dataChannelActivePort)));

            m_isListenThreadRunning = true;
            acceptor.accept(m_dataChannelSocket.next_layer(), m_ioSocketError);
            if (m_ioSocketError) {
                throw Exception(m_ioSocketError.message());
            }
            m_isListenThreadRunning = false;
 

        }

        //
        // Cleanup after data channel transfer. This includes stopping any unused listener
        // thread and closing the socket if still open.
        //

        void CFTP::postTransferCleanup() {

            if (m_isListenThreadRunning) {
                m_isListenThreadRunning=false;
                try {
                    ip::tcp::socket socket{ m_ioService};
                    ip::tcp::resolver::query query(m_dataChannelActiveAddresss, m_dataChannelActivePort);
                    asio::connect(socket, m_ioQueryResolver.resolve(query));
                    m_dataChannelListenThread->join();
                } catch (std::exception &e) {
                    std::cerr << "Listener thread running when it should not be." << std::endl;
                    m_dataChannelListenThread->join();
                }

            }

            closeSocket(m_dataChannelSocket);

        }

        //
        // Read any response to command on data channel (ie.LIST).
        //

        void CFTP::readCommandResponse(std::string &commandResponse) {

            try {

                m_commandStatusCode = ftpResponse();

                if ((m_commandStatusCode == 125) || (m_commandStatusCode == 150)) {

                    if (!m_passiveMode) {
                        m_dataChannelListenThread->join();
                    } 

                    if (m_sslConnectionActive) {
                        switchOnSSL(m_dataChannelSocket);
                    }

                    do {

                        size_t bytesRead = readFromSocket(m_dataChannelSocket);

                        if (bytesRead) {
                            m_ioBuffer[bytesRead] = '\0';
                            commandResponse.append(&m_ioBuffer[0]);
                        }


                    } while (!socketClosedByServer());

                    closeSocket(m_dataChannelSocket);

                    m_commandStatusCode = ftpResponse();

                }

            } catch (CFTP::Exception& e) {
                postTransferCleanup();
                throw;
            } catch (std::exception &e) {
                postTransferCleanup();
                throw Exception(e.what());
            }

            postTransferCleanup();

        }

        //
        // Transfer (upload/download) file over data channel.
        //

        void CFTP::transferFile(const std::string &file, bool downloading) {

            try {

                m_commandStatusCode = ftpResponse();

                if ((m_commandStatusCode == 125) || (m_commandStatusCode == 150)) {

                    if (!m_passiveMode) {
                        m_dataChannelListenThread->join();
                    } 

                    if (m_sslConnectionActive) {
                        switchOnSSL(m_dataChannelSocket);
                    }

                    if (downloading) {
                        downloadFile(file);
                    } else {
                        uploadFile(file);
                    }

                    closeSocket(m_dataChannelSocket);

                    m_commandStatusCode = ftpResponse();

                }

            } catch (std::exception &e) {
                postTransferCleanup();
                throw Exception(e.what());
            }

            postTransferCleanup();


        }

        //
        // Send FTP over control channel
        //

        void CFTP::ftpCommand(const std::string& command) {

            size_t commandLength = command.size();

            do {

                commandLength -= writeToSocket(m_controlChannelSocket, &command[command.size() - commandLength], commandLength);
                if (m_ioSocketError) {
                    throw Exception(m_ioSocketError.message());
                }

            } while (commandLength != 0);

            this->m_lastCommand = command;
            this->m_lastCommand.pop_back();
            this->m_lastCommand.pop_back();

        }

        //
        // Read FTP command response from control channel (return its status code).
        //

        std::uint16_t CFTP::ftpResponse() {

            bool extendedResponse=false;
            std::uint16_t statusCode;
            
            m_commandResponse.clear();

            do {

                do {

                    size_t bytesRead = readFromSocket(m_controlChannelSocket);

                    if (bytesRead) {
                        m_ioBuffer[bytesRead] = '\0';
                        m_commandResponse.append(&m_ioBuffer[0]);
                    }

                } while (!socketClosedByServer() && (m_commandResponse.back() != '\n'));

                if (m_ioSocketError) {
                    throw Exception(m_ioSocketError.message());
                }

                if (!extendedResponse && (m_commandResponse[3] == '-')) {
                    extendedResponse = true;
                    statusCode = std::stoi(m_commandResponse);
                } 
                
                if (extendedResponse) {
                    size_t multiLine;
                    multiLine = m_commandResponse.rfind("\r\n"+std::to_string(statusCode)+" ");
                    if (multiLine != std::string::npos) {
                        extendedResponse=false;
                    }
                }

            } while (extendedResponse);
            
            
            return (std::stoi(m_commandResponse));


        }


        // ==============
        // PUBLIC METHODS
        // ==============

        //
        // Set FTP account details
        //

        void CFTP::setUserAndPassword(const std::string& userName, const std::string& userPassword) {

            m_userName = userName;
            m_userPassword = userPassword;

        }

        //
        // Set FTP server name and port
        //

        void CFTP::setServerAndPort(const std::string& serverName, const std::string& serverPort) {

            m_serverName = serverName;
            m_serverPort = serverPort;

        }

        //
        // Get current connection status with server
        //

        bool CFTP::getConnectedStatus(void) const {

            return (m_connected);

        }

        //
        // Enabled/disable SSL
        //

        void CFTP::setSslEnabled(bool sslEnabled) {
            if (!m_connected) {
                m_sslEnabled = sslEnabled;
            } else {
                throw Exception("Cannot set SSL mode while connected.");
            }
        }

        bool CFTP::isSslEnabled() const {
            return m_sslEnabled;
        }

        //
        // Get last raw FTP command
        //

        std::string CFTP::getLastCommand() const {
            return m_lastCommand;
        }

        //
        // Get last FTP response status code
        //

        std::uint16_t CFTP::getCommandStatusCode() const {
            return m_commandStatusCode;
        }

        //
        // Get last FTP full response string
        //
        std::string CFTP::getCommandResponse() const {
            return m_commandResponse;
        }

        //
        // Setup connection to server
        //

        std::uint16_t CFTP::connect(void) {

            if (m_connected) {
                Exception("Already connected to a server.");
            }

            m_dataChannelActiveAddresss = determineLocalIPAddress();

            connectSocket(m_controlChannelSocket, m_serverName, m_serverPort);

            m_commandStatusCode = ftpResponse();

            if (m_commandStatusCode == 220) {

                if (m_sslEnabled) {
                    ftpCommand("AUTH TLS\r\n");
                    m_commandStatusCode = ftpResponse();
                    if (m_commandStatusCode == 234) {
                        switchOnSSL(m_controlChannelSocket);
                        m_sslConnectionActive = true;
                        ftpCommand("PBSZ 0\r\n");
                        m_commandStatusCode = ftpResponse();
                        if (m_commandStatusCode == 200) {
                            ftpCommand("PROT P\r\n");
                            m_commandStatusCode = ftpResponse();
                        }
                    }

                }

                m_connected = true;

                ftpCommand("USER " + m_userName + "\r\n");
                m_commandStatusCode = ftpResponse();

                if (m_commandStatusCode == 331) {
                    ftpCommand("PASS " + m_userPassword + "\r\n");
                    m_commandStatusCode = ftpResponse();
                }

            }

            return (m_commandStatusCode);

        }

        //
        // Disconnect from server
        //

        std::uint16_t CFTP::disconnect(void) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            ftpCommand("QUIT\r\n");
            m_commandStatusCode = ftpResponse();

            m_sslConnectionActive = false;
            m_connected = false;

            closeSocket(m_controlChannelSocket);

            return (m_commandStatusCode);

        }

        //
        // Set passive transfer mode.
        // == true passive enabled, == false active mode
        //

        void CFTP::setPassiveTransferMode(bool passiveEnabled) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            m_passiveMode = passiveEnabled;

        }

        //
        // Send transfer mode to used over data channel.
        //

        bool CFTP::sendTransferMode() {

            if (m_passiveMode) {
                ftpCommand("PASV\r\n");
                m_commandStatusCode = ftpResponse();
                if (m_commandStatusCode == 227) {
                    extractPassiveAddressPort(m_commandResponse);
                    connectSocket(m_dataChannelSocket, m_dataChannelPassiveAddresss, m_dataChannelPassivePort);
                }
                return (m_commandStatusCode == 227);
            } else {
                m_dataChannelListenThread.reset(new std::thread(&CFTP::transferConnectionListener, this));
                while(!m_isListenThreadRunning) {   // Wait for until listening before sending PORT command
                    continue;                       // Could use conditional but use existing flag for now
                }
                ftpCommand(createPortCommand() + "\r\n");
                m_commandStatusCode = ftpResponse();
                return (m_commandStatusCode == 200);
            }
            
        }

        //
        // Transfer a file from the server to a local file.
        //

        std::uint16_t CFTP::getFile(const std::string &remoteFilePath, const std::string &localFilePath) {

            std::ofstream localFile { localFilePath, std::ofstream::binary};
                       
            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            if (localFile) {
                localFile.close();
                if (sendTransferMode()) {
                    ftpCommand("RETR " + remoteFilePath + "\r\n");
                    transferFile(localFilePath, true);
                }
            } else {
                m_commandStatusCode = 550;
                throw Exception("Local file " + localFilePath + " could not be created.");
            }

            return (m_commandStatusCode);


        }

        //
        // Transfer a file to the server from a local file.
        //

        std::uint16_t CFTP::putFile(const std::string &remoteFilePath, const std::string &localFilePath) {


            std::ifstream localFile{ localFilePath, std::ifstream::binary};

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            if (localFile) {
                localFile.close();
                if (sendTransferMode()) {
                    ftpCommand("STOR " + remoteFilePath + "\r\n");
                    transferFile(localFilePath, false);
                }
            } else {
                m_commandStatusCode = 550;
                throw Exception("Local file " + localFilePath + " does not exist.");
            }

            return (m_commandStatusCode);

        }

        //
        // Produce a directory listing for the file/directory passed in or for the current
        // working directory if none is.
        //

        std::uint16_t CFTP::list(const std::string &directoryPath, std::string &listOutput) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            listOutput.clear();

            if (sendTransferMode()) {
                ftpCommand("LIST " + directoryPath + "\r\n");
                readCommandResponse(listOutput);
            }

            return (m_commandStatusCode);

        }

        //
        // Produce a file list for the file/directory passed in or for the current
        // working directory if none is.
        //

        std::uint16_t CFTP::listFiles(const std::string &directoryPath, std::vector<std::string> &fileList) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }
            
            fileList.clear();

            if (sendTransferMode()) {
                std::string listOutput;
                ftpCommand("NLST " + directoryPath + "\r\n");
                readCommandResponse(listOutput);
                if(m_commandStatusCode==226) {
                    std::string file;
                    std::istringstream listOutputStream{ listOutput};
                    while (std::getline(listOutputStream, file, '\n')) {
                        file.pop_back();
                        fileList.push_back(file);
                    }
                 
                }
            }

            return (m_commandStatusCode);


        }

        //
        // Produce a file information list for the directory passed in or for the current
        // working directory if none is.
        //

        std::uint16_t CFTP::listDirectory(const std::string &directoryPath, std::string &listOutput) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            listOutput.clear();

            if (sendTransferMode()) {
                ftpCommand("MLSD " + directoryPath + "\r\n");
                readCommandResponse(listOutput);
            }

            return (m_commandStatusCode);


        }

        //
        // Produce file information for the file passed in or for the current
        // working directory if none is. Note: Reply is sent on control and not
        // the data channel.
        //

        std::uint16_t CFTP::listFile(const std::string &filePath, std::string &listOutput) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            listOutput.clear();

            ftpCommand("MLST " + filePath + "\r\n");

            m_commandStatusCode = ftpResponse();

            if (m_commandStatusCode == 250) {
                listOutput = m_commandResponse.substr(m_commandResponse.find('\n') + 1);
                listOutput = listOutput.substr(0, listOutput.find('\r'));
            }

            return (m_commandStatusCode);


        }

        //
        // Make remote FTP server directory
        //

        std::uint16_t CFTP::makeDirectory(const std::string &directoryName) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            ftpCommand("MKD " + directoryName + "\r\n");

            return (m_commandStatusCode = ftpResponse());

        }

        //
        // Remove remote FTP server directory
        //

        std::uint16_t CFTP::removeDirectory(const std::string &directoryName) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            ftpCommand("RMD " + directoryName + "\r\n");

            return (m_commandStatusCode = ftpResponse());

        }

        //
        // Remove remote FTP server directory
        //

        std::uint16_t CFTP::fileSize(const std::string &fileName, size_t &fileSize) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            ftpCommand("SIZE " + fileName + "\r\n");

            m_commandStatusCode = ftpResponse();

            if (m_commandStatusCode == 213) {
                fileSize = std::stoi(m_commandResponse.substr(m_commandResponse.find(' ') + 1));
            }

            return (m_commandStatusCode);

        }

        //
        // Delete remote FTP server file
        //

        std::uint16_t CFTP::deleteFile(const std::string &fileName) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            ftpCommand("DELE " + fileName + "\r\n");

            return (m_commandStatusCode = ftpResponse());

        }

        //
        // Change current working directory on server.
        //

        std::uint16_t CFTP::changeWorkingDirectory(const std::string &workingDirectoryPath) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            ftpCommand("CWD " + workingDirectoryPath + "\r\n");

            return (m_commandStatusCode = ftpResponse());

        }

        //
        // Fetch current working directory on server and return path as string.
        //

        std::uint16_t CFTP::getCurrentWoringDirectory(std::string &currentWoringDirectoryPath) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            currentWoringDirectoryPath.clear();

            ftpCommand("PWD\r\n");

            m_commandStatusCode = ftpResponse();

            if (m_commandStatusCode == 250) {
                currentWoringDirectoryPath = m_commandResponse.substr(m_commandResponse.find_first_of('\"') + 1);
                currentWoringDirectoryPath = currentWoringDirectoryPath.substr(0, currentWoringDirectoryPath.find_first_of('\"'));
            }

            return (m_commandStatusCode);

        }

        //
        // Fetch files last modified date/time.
        //

        std::uint16_t CFTP::getModifiedDateTime(const std::string &filePath, DateTime &modifiedDateTime) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            ftpCommand("MDTM " + filePath + "\r\n");
            m_commandStatusCode = ftpResponse();

            if (m_commandStatusCode == 213) {
                std::string dateTime = m_commandResponse.substr(m_commandResponse.find(' ') + 1);
                modifiedDateTime.year = std::stoi(dateTime.substr(0, 4));
                modifiedDateTime.month = std::stoi(dateTime.substr(4, 2));
                modifiedDateTime.day = std::stoi(dateTime.substr(6, 2));
                modifiedDateTime.hour = std::stoi(dateTime.substr(8, 2));
                modifiedDateTime.minute = std::stoi(dateTime.substr(10, 2));
                modifiedDateTime.second = std::stoi(dateTime.substr(12, 2));
            }

            return (m_commandStatusCode);

        }
        
        //
        // Return true if passed in file is a directory false for a file.
        //

        bool CFTP::isDirectory(const std::string &fileName) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            ftpCommand("STAT " + fileName + "\r\n");

            m_commandStatusCode = ftpResponse();
            
            if (m_commandStatusCode==213) {
                size_t dirPosition = m_commandResponse.find("\r\n")+2;
                if ((dirPosition != std::string::npos) && 
                     (m_commandResponse[dirPosition]=='d')) {
                    return(true);
                }
            }

            return (false);

        }


        //
        // Main CFTP object constructor. 
        //

        CFTP::CFTP() {

        }

        //
        // CFTP Destructor
        //

        CFTP::~CFTP() {

        }

    } // namespace FTP
} // namespace Antik
