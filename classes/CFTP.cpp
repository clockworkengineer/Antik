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
            socket.next_layer().connect(*m_queryResolver.resolve(query), m_socketError);
            if (m_socketError) {
                throw Exception(m_socketError.message());
            }    
            
        }
        
        //
        // Read data from socket into io buffer
        //
        
        inline size_t CFTP::readFromSocket(SSLSocket &socket) {
            
            if (m_sslConnectionActive) {
                return(socket.read_some(asio::buffer(m_ioBuffer, m_ioBuffer.size() - 1), m_socketError));
            } else {
                return(socket.next_layer().read_some(asio::buffer(m_ioBuffer, m_ioBuffer.size() - 1), m_socketError));
            }
            
        }
        
        //
        // Write data to socket
        //
        
        inline size_t CFTP::writeToSocket(SSLSocket &socket, const char *writeBuffer, size_t writeLength) {
            
            if (m_sslConnectionActive) {
                return(socket.write_some(asio::buffer(writeBuffer, writeLength), m_socketError));
            } else {
                return(socket.next_layer().write_some(asio::buffer(writeBuffer, writeLength), m_socketError));
            }
            
        }
        
        //
        // Perform TLS handshake to enable SSL
        //
        
        inline void  CFTP::switchOnSSL(SSLSocket &socket) {
            socket.handshake(SSLSocket::client, m_socketError);
            if (m_socketError) {
                throw Exception(m_socketError.message());
            }         
        }
        
        //
        // Return true if socket closed by server otherwise false.
        // Also throw exception for any socket error detected,
        //

        inline bool CFTP::socketClosedByServer() {

            if (m_socketError == asio::error::eof) {
                return (true); // Connection closed cleanly by peer.
            } else if (m_socketError) {
                throw Exception(m_socketError.message());
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

                ip::tcp::acceptor acceptor(m_ioService, ip::tcp::endpoint(ip::tcp::v4(), 0));
                unsigned short port = acceptor.local_endpoint().port();
                auto address = acceptor.local_endpoint().address();

                m_dataChannelActivePort = std::to_string(port);
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

                do  {

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
        // Data channel socket listener thread function for incoming data 
        // channel connections.
        //

        void CFTP::dataChannelTransferListener() {

            m_isListenThreadRunning = true;

            if (m_dataChannelSocket.next_layer().is_open()) {
                m_dataChannelSocket.next_layer().close();
            }

            sendFTPCommand(createPortCommand() + "\r\n");

            ip::tcp::acceptor acceptor(m_ioService, ip::tcp::endpoint(ip::tcp::v4(), std::stoi(m_dataChannelActivePort)));
            acceptor.accept(m_dataChannelSocket.next_layer(), m_socketError);
            if (m_socketError) {
                throw Exception(m_socketError.message());
            }

            if (m_sslConnectionActive) {
                m_dataChannelSocket.handshake(SSLSocket::client, m_socketError);
                if (m_socketError) {
                    throw Exception(m_socketError.message());
                }
            }

            m_isListenThreadRunning = false;

        }

        //
        // Cleanup after data channel transfer. This includes stopping any unused listener
        // thread and closing the socket if still open.
        //

        void CFTP::dataChannelTransferCleanup() {

            if (m_isListenThreadRunning) {

                try {
                    ip::tcp::socket socket { m_ioService };
                    ip::tcp::resolver::query query(m_dataChannelActiveAddresss, m_dataChannelActivePort);
                    asio::connect(socket, m_queryResolver.resolve(query));
                    m_dataChannelListenThread->join();
                } catch (std::exception &e) {
                    std::cerr << "Listener thread running when it should not be." << std::endl;
                }

            }

            if (m_dataChannelSocket.next_layer().is_open()) {
                m_dataChannelSocket.next_layer().close();
            }

        }

        //
        // Read any response to command on data channel (ie.LIST).
        //

        void CFTP::readDataChannelCommandResponse(std::string &commandResponse) {

            try {

                if (m_passiveMode) {
                    connectSocket(m_dataChannelSocket,m_dataChannelPassiveAddresss, m_dataChannelPassivePort);
                    if (m_sslConnectionActive) {
                        switchOnSSL(m_dataChannelSocket);
                    }
                }

                m_commandStatusCode = waitForFTPCommandResponse();

                if ((m_commandStatusCode == 125) || (m_commandStatusCode == 150)) {

                    if (!m_passiveMode) {
                        m_dataChannelListenThread->join();
                    }

                    do {

                        size_t bytesRead = readFromSocket(m_dataChannelSocket);

                        if (bytesRead) {
                            m_ioBuffer[bytesRead] = '\0';
                            commandResponse.append(&m_ioBuffer[0]);
                        }


                    } while (!socketClosedByServer());

                    if (m_dataChannelSocket.next_layer().is_open()) {
                        m_dataChannelSocket.next_layer().close();
                    }

                    m_commandStatusCode = waitForFTPCommandResponse();

                }

            } catch (CFTP::Exception& e) {
                dataChannelTransferCleanup();
                throw;
            } catch (std::exception &e) {
                dataChannelTransferCleanup();
                throw Exception(e.what());
            }

            dataChannelTransferCleanup();

        }

        //
        // Transfer (upload/download) file over data channel.
        //

        void CFTP::transferFile(const std::string &file, bool downloading) {

            try {

                if (m_passiveMode) {
                    connectSocket(m_dataChannelSocket, m_dataChannelPassiveAddresss, m_dataChannelPassivePort);
                    if (m_sslConnectionActive) {
                        switchOnSSL(m_dataChannelSocket);
                    }
                }

                m_commandStatusCode = waitForFTPCommandResponse();

                if ((m_commandStatusCode == 125) || (m_commandStatusCode == 150)) {

                    if (!m_passiveMode) {
                        m_dataChannelListenThread->join();
                    }

                    if (downloading) {
                        downloadFile(file);
                    } else {
                        uploadFile(file);
                    }

                    if (m_dataChannelSocket.next_layer().is_open()) {
                        m_dataChannelSocket.next_layer().close();
                    }

                    m_commandStatusCode = waitForFTPCommandResponse();

                }

            } catch (std::exception &e) {
                dataChannelTransferCleanup();
                throw Exception(e.what());
            }

            dataChannelTransferCleanup();


        }

        //
        // Send FTP over control channel
        //

        void CFTP::sendFTPCommand(const std::string& command) {

            size_t commandLength = command.size();

           do {

                commandLength -= writeToSocket(m_controlChannelSocket, &command[command.size() - commandLength], commandLength);           
                if (m_socketError) {
                    throw Exception(m_socketError.message());
                }

            } while (commandLength != 0);

        }

        //
        // Read FTP command response from control channel (return its status code).
        //

        uint16_t CFTP::waitForFTPCommandResponse() {


            m_commandResponse.clear();

            do  {
                
                size_t bytesRead = readFromSocket(m_controlChannelSocket);

                if (bytesRead) {
                    m_ioBuffer[bytesRead] = '\0';
                    m_commandResponse.append(&m_ioBuffer[0]);
                }

            } while (!socketClosedByServer() && (m_commandResponse.back() != '\n'));
       

            if (m_socketError) {
                throw Exception(m_socketError.message());
            }

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
        // Setup connection to server
        //

        uint16_t CFTP::connect(void) {

            if (m_connected) {
                Exception("Already connected to a server.");
            }

            m_dataChannelActiveAddresss = determineLocalIPAddress();

            connectSocket(m_controlChannelSocket, m_serverName, m_serverPort);

            m_commandStatusCode = waitForFTPCommandResponse();

            if (m_commandStatusCode == 220) {

                if (m_sslEnabled) {
                    sendFTPCommand("AUTH TLS\r\n");
                    m_commandStatusCode = waitForFTPCommandResponse();
                    if (m_commandStatusCode == 234) {
                        switchOnSSL(m_controlChannelSocket);
                        m_sslConnectionActive = true;
                        sendFTPCommand("PBSZ 0\r\n");
                        m_commandStatusCode = waitForFTPCommandResponse();
                        if (m_commandStatusCode == 200) {
                            sendFTPCommand("PROT P\r\n");
                            m_commandStatusCode = waitForFTPCommandResponse();
                        }
                    }

                }

                m_connected = true;

                sendFTPCommand("USER " + m_userName + "\r\n");
                m_commandStatusCode = waitForFTPCommandResponse();

                if (m_commandStatusCode == 331) {
                    sendFTPCommand("PASS " + m_userPassword + "\r\n");
                    m_commandStatusCode = waitForFTPCommandResponse();
                }

            }

            return (m_commandStatusCode);

        }

        //
        // Disconnect from server
        //

        void CFTP::disconnect(void) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            m_connected = false;

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
                sendFTPCommand("PASV\r\n");
                m_commandStatusCode = waitForFTPCommandResponse();
                if (m_commandStatusCode == 227) {
                    extractPassiveAddressPort(m_commandResponse);
                }
                return (m_commandStatusCode == 227);
            } else {
                m_dataChannelListenThread.reset(new std::thread(&CFTP::dataChannelTransferListener, this));
                m_commandStatusCode = waitForFTPCommandResponse();
                return (m_commandStatusCode == 200);
            }
        }

        //
        // Transfer a file from the server to a local file.
        //

        uint16_t CFTP::getFile(const std::string &remoteFilePath, const std::string &localFilePath) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            if (sendTransferMode()) {
                sendFTPCommand("RETR " + remoteFilePath + "\r\n");
                transferFile(localFilePath, true);
            }

            return (m_commandStatusCode);


        }

        //
        // Transfer a file to the server from a local file.
        //

        uint16_t CFTP::putFile(const std::string &remoteFilePath, const std::string &localFilePath) {


            std::ifstream localFile{ localFilePath, std::ifstream::binary};

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            if (localFile) {
                localFile.close();
                if (sendTransferMode()) {
                    sendFTPCommand("STOR " + remoteFilePath + "\r\n");
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

        uint16_t CFTP::list(const std::string &directoryPath, std::string &listOutput) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            listOutput.clear();

            if (sendTransferMode()) {
                sendFTPCommand("LIST " + directoryPath + "\r\n");
                readDataChannelCommandResponse(listOutput);
            }

            return (m_commandStatusCode);

        }

        //
        // Produce a file list for the file/directory passed in or for the current
        // working directory if none is.
        //

        uint16_t CFTP::listFiles(const std::string &directoryPath, std::string &listOutput) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            listOutput.clear();

            if (sendTransferMode()) {
                sendFTPCommand("NLST " + directoryPath + "\r\n");
                readDataChannelCommandResponse(listOutput);
            }

            return (m_commandStatusCode);


        }

        //
        // Produce a file information list for the directory passed in or for the current
        // working directory if none is.
        //

        uint16_t CFTP::listDirectory(const std::string &directoryPath, std::string &listOutput) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            listOutput.clear();

            if (sendTransferMode()) {
                sendFTPCommand("MLSD " + directoryPath + "\r\n");
                readDataChannelCommandResponse(listOutput);
            }

            return (m_commandStatusCode);


        }

        //
        // Produce file information for the file passed in or for the current
        // working directory if none is. Note: Reply is sent on control and not
        // the data channel.
        //

        uint16_t CFTP::listFile(const std::string &filePath, std::string &listOutput) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            listOutput.clear();

            sendFTPCommand("MLST " + filePath + "\r\n");

            m_commandStatusCode = waitForFTPCommandResponse();

            if (m_commandStatusCode == 250) {
                listOutput = m_commandResponse.substr(m_commandResponse.find('\n') + 1);
                listOutput = listOutput.substr(0, listOutput.find('\r'));
            }

            return (m_commandStatusCode);


        }

        //
        // Make remote FTP server directory
        //

        uint16_t CFTP::makeDirectory(const std::string &directoryName) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            sendFTPCommand("MKD " + directoryName + "\r\n");

            return (m_commandStatusCode = waitForFTPCommandResponse());

        }

        //
        // Remove remote FTP server directory
        //

        uint16_t CFTP::removeDirectory(const std::string &directoryName) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            sendFTPCommand("RMD " + directoryName + "\r\n");

            return (m_commandStatusCode = waitForFTPCommandResponse());

        }

        //
        // Remove remote FTP server directory
        //

        uint16_t CFTP::fileSize(const std::string &fileName, size_t &fileSize) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            sendFTPCommand("SIZE " + fileName + "\r\n");

            m_commandStatusCode = waitForFTPCommandResponse();

            if (m_commandStatusCode == 213) {
                fileSize = std::stoi(m_commandResponse.substr(m_commandResponse.find(' ') + 1));
            }

            return (m_commandStatusCode);

        }

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
        // Delete remote FTP server file
        //

        uint16_t CFTP::deleteFile(const std::string &fileName) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            sendFTPCommand("DELE " + fileName + "\r\n");

            return (m_commandStatusCode = waitForFTPCommandResponse());

        }

        //
        // Change current working directory on server.
        //

        uint16_t CFTP::changeWorkingDirectory(const std::string &workingDirectoryPath) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            sendFTPCommand("CWD " + workingDirectoryPath + "\r\n");

            return (m_commandStatusCode = waitForFTPCommandResponse());

        }

        //
        // Fetch current working directory on server and return path as string.
        //

        uint16_t CFTP::getCurrentWoringDirectory(std::string &currentWoringDirectoryPath) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            currentWoringDirectoryPath.clear();

            sendFTPCommand("PWD\r\n");

            m_commandStatusCode = waitForFTPCommandResponse();

            if (m_commandStatusCode == 250) {
                currentWoringDirectoryPath = m_commandResponse.substr(m_commandResponse.find_first_of('\"') + 1);
                currentWoringDirectoryPath = currentWoringDirectoryPath.substr(0, currentWoringDirectoryPath.find_first_of('\"'));
            }

            return (m_commandStatusCode);

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
