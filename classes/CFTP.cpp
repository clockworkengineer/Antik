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
// Note: At present TLS/SSL connections are not supported so all commands go in the clear.
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

#include <cstring>
#include <sstream>
#include <iomanip>
#include <iostream>

// =======
// IMPORTS
// =======

namespace asio = boost::asio;
namespace ip = asio::ip;
namespace fs = boost::filesystem;

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
                ip::udp::resolver::iterator endpoints = resolver.resolve(query);
                ip::udp::socket socket(m_ioService);
                socket.connect(*endpoints);
                localIPAddress = socket.local_endpoint().address().to_string();
                socket.close();
            } catch (std::exception& e) {
                return (localIPAddress);
            }

            return (localIPAddress);

        }

        //
        // Extract status code from FTP command reply.
        //
        
        std::uint16_t CFTP::extractStatusCode(const std::string &commandResponse) {

            std::uint16_t statusCode = std::stoi(commandResponse);

            // std::cout << "STATUS CODE : " << statusCode << std::endl;
            // std::cout << "MESSAGE : " << commandResponse.substr(commandResponse.find(' ') + 1);

            return (statusCode);

        }
        
        //
        // Extract host ip address and port information from passive command reply.
        //

        void CFTP::extractPassiveAddressPort(std::string &pasvResponse) {

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


        }
        
        //
        // Create PORT command to send over control channel.
        //
        
        std::string CFTP::createPortCommand() {

            std::string portCommand{ "PORT "};

            ip::tcp::acceptor acceptor(m_ioService, ip::tcp::endpoint(ip::tcp::v4(), 0));
            unsigned short port = acceptor.local_endpoint().port();
            auto address = acceptor.local_endpoint().address();

            m_dataChannelActivePort = std::to_string(port);
            portCommand += m_dataChannelActiveAddresss;
            for (auto &byte : portCommand) {
                if (byte == '.') byte = ',';
            }
            portCommand += "," + std::to_string((port & 0xFF00) >> 8) + "," + std::to_string(port & 0xFF);

            return (portCommand);

        }
        
        //
        // Download a file from FTP server to local system.
        //

        void CFTP::downloadFile(const std::string &file) {

            fs::path localFilePath{ file};

            if (fs::exists(localFilePath)) {
                fs::remove(localFilePath);
            }

            std::ofstream localFile{ localFilePath.string(), std::ofstream::binary};

            for (;;) {

                size_t len = m_dataChannelSocket.read_some(asio::buffer(m_ioBuffer), m_socketError);

                if (m_socketError == asio::error::eof) {
                    break; // Connection closed cleanly by peer.
                } else if (m_socketError) {
                    throw Exception(m_socketError.message());
                }

                localFile.write(&m_ioBuffer[0], len);

            }

            localFile.close();

        }
        
        //
        // Upload file from local system to FTP server,
        //

        void CFTP::uploadFile(const std::string &file) {

            fs::path localFilePath{ file};

            std::ifstream localFile{ localFilePath.string(), std::ifstream::binary};

            std::size_t localFileLength = fs::file_size(localFilePath);

            for (;;) {

                if (!localFile) break;

                localFile.read(&m_ioBuffer[0], m_ioBuffer.size());

                if (localFileLength < m_ioBuffer.size()) {
                    asio::write(m_dataChannelSocket, asio::buffer(m_ioBuffer, localFileLength), m_socketError);
                } else {
                    asio::write(m_dataChannelSocket, asio::buffer(m_ioBuffer, m_ioBuffer.size()), m_socketError);
                }

                localFileLength -= m_ioBuffer.size();

                if (m_socketError == asio::error::eof) {
                    break; // Connection closed cleanly by peer.
                } else if (m_socketError) {
                    throw Exception(m_socketError.message());
                }

            }

            localFile.close();

        }

        //
        // Data channel socket listener thread function for incoming data 
        // channel connections.
        //
        
        void CFTP::dataChannelTransferListener() {

            m_isListenThreadRunning = true;

            if (m_dataChannelSocket.is_open()) {
                m_dataChannelSocket.close();
            }

            sendFTPCommand(createPortCommand() + "\r\n");

            ip::tcp::acceptor acceptor(m_ioService, ip::tcp::endpoint(ip::tcp::v4(), std::stoi(m_dataChannelActivePort)));
            acceptor.accept(m_dataChannelSocket);

            m_isListenThreadRunning = false;

        }

        //
        // Cleanup after data channel transfer. This includes stopping any unused listener
        // thread and closing the socket if still open.
        //
        
        void CFTP::dataChannelTransferCleanup() {

            if (m_isListenThreadRunning) {
                ip::tcp::socket socket{ m_ioService};
                ip::tcp::resolver::query query(m_dataChannelActiveAddresss, m_dataChannelActivePort);
                asio::connect(socket, m_queryResolver.resolve(query));
                m_dataChannelListenThread->join();
            }

            if (m_dataChannelSocket.is_open()) {
                m_dataChannelSocket.close();
            }

        }

        //
        // Read any response to command on data channel (ie.LIST).
        //
        
        void CFTP::readDataChannelCommandResponse(std::string &commandResponse) {

            try {

                if (m_passiveMode) {
                    ip::tcp::resolver::query query(m_dataChannelPassiveAddresss, m_dataChannelPassivePort);
                    asio::connect(m_dataChannelSocket, m_queryResolver.resolve(query));
                }

                waitForFTPCommandResponse(m_commandResponse);

                std::uint16_t statusCode = extractStatusCode(m_commandResponse);

                if ((statusCode == 125) || (statusCode == 150)) {

                    if (!m_passiveMode) {
                        m_dataChannelListenThread->join();
                    }

                    for (;;) {

                        size_t len = m_dataChannelSocket.read_some(asio::buffer(m_ioBuffer), m_socketError);

                        if (m_socketError == asio::error::eof) {
                            break; // Connection closed cleanly by peer.
                        } else if (m_socketError) {
                            throw Exception(m_socketError.message());
                        }

                        for (auto byte : m_ioBuffer) {
                            if (len == 0) break;
                            commandResponse.append(1, byte);
                            len--;
                        }

                    }

                    if (m_dataChannelSocket.is_open()) {
                        m_dataChannelSocket.close();
                    }

                    waitForFTPCommandResponse(m_commandResponse);

                }

            } catch (CFTP::Exception& e) {
                dataChannelTransferCleanup();
                throw;
            } catch (std::exception& e) {
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
                    ip::tcp::resolver::query query(m_dataChannelPassiveAddresss, m_dataChannelPassivePort);
                    asio::connect(m_dataChannelSocket, m_queryResolver.resolve(query));
                }

                waitForFTPCommandResponse(m_commandResponse);
                std::uint16_t statusCode = extractStatusCode(m_commandResponse);

                if ((statusCode == 125) || (statusCode == 150)) {

                    if (!m_passiveMode) {
                        m_dataChannelListenThread->join();
                    }

                    if (downloading) {
                        downloadFile(file);
                    } else {
                        uploadFile(file);
                    }

                    if (m_dataChannelSocket.is_open()) {
                        m_dataChannelSocket.close();
                    }

                    waitForFTPCommandResponse(m_commandResponse);

                }

            } catch (std::exception& e) {
                dataChannelTransferCleanup();
                throw Exception(e.what());
            }

            dataChannelTransferCleanup();


        }

        //
        // Send FTP over control channel
        //
        
        void CFTP::sendFTPCommand(const std::string& command) {

            asio::write(m_controlChannelSocket, asio::buffer(&command[0], command.size()), m_socketError);

            if (m_socketError) {
                throw Exception(m_socketError.message());
            }

        }
        
        //
        // Read FTP command response fromconrol channel.
        //

        void CFTP::waitForFTPCommandResponse(std::string& commandResponse) {

            size_t recvLength { 0 };

            commandResponse.clear();

            for (;;) {

                recvLength = m_controlChannelSocket.read_some(asio::buffer(m_ioBuffer, m_ioBuffer.size()), m_socketError);

                if (recvLength == 0) {
                    commandResponse.clear();
                    break;
                }

                m_ioBuffer[recvLength] = '\0';
                commandResponse.append(&m_ioBuffer[0]);

                recvLength = commandResponse.length();
                if ((commandResponse[recvLength - 2] == '\r') && (commandResponse[recvLength - 1] == '\n')) {
                    break;
                }

                if (m_socketError == asio::error::eof) {
                    break; // Connection closed cleanly by peer.
                } else if (m_socketError) {
                    throw Exception(m_socketError.message());
                }

            }

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

        void CFTP::connect(void) {

            if (m_connected) {
                Exception("Already connected to a server.");
            }

            ip::tcp::resolver::query query(m_serverName, m_serverPort);

            asio::connect(m_controlChannelSocket, m_queryResolver.resolve(query), m_socketError);

            if (m_socketError) {
                throw Exception(m_socketError.message());
            }

            waitForFTPCommandResponse(m_commandResponse);

            m_connected = true;

            sendFTPCommand("USER " + m_userName + "\r\n");
            waitForFTPCommandResponse(m_commandResponse);
            sendFTPCommand("PASS " + m_userPassword + "\r\n");
            waitForFTPCommandResponse(m_commandResponse);

            m_dataChannelActiveAddresss = determineLocalIPAddress();


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
                waitForFTPCommandResponse(m_commandResponse);
                extractPassiveAddressPort(m_commandResponse);
                return (extractStatusCode(m_commandResponse) == 227);
            } else {
                m_dataChannelListenThread.reset(new std::thread(&CFTP::dataChannelTransferListener, this));
                waitForFTPCommandResponse(m_commandResponse);
                return (extractStatusCode(m_commandResponse) == 200);
            }
        }

        //
        // Transfer a file from the server to a local file.
        //
        
        std::uint16_t CFTP::getFile(const std::string &remoteFilePath, const std::string &localFilePath) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            if (sendTransferMode()) {
                sendFTPCommand("RETR " + remoteFilePath + "\r\n");
                transferFile(localFilePath, true);
            }

            return (extractStatusCode(m_commandResponse));


        }

        //
        // Transfer a file to the server from a local file.
        //
        
        std::uint16_t CFTP::putFile(const std::string &remoteFilePath, const std::string &localFilePath) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            if (sendTransferMode()) {
                sendFTPCommand("STOR " + remoteFilePath + "\r\n");
                transferFile(localFilePath, false);
            }

            return (extractStatusCode(m_commandResponse));

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
                sendFTPCommand("LIST " + directoryPath + "\r\n");
                readDataChannelCommandResponse(listOutput);
            }

            return (extractStatusCode(m_commandResponse));

        }

        //
        // Produce a file list for the file/directory passed in or for the current
        // working directory if none is.
        //
        
        std::uint16_t CFTP::listFiles(const std::string &directoryPath, std::string &listOutput) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            listOutput.clear();

            if (sendTransferMode()) {
                sendFTPCommand("NLST " + directoryPath + "\r\n");
                readDataChannelCommandResponse(listOutput);
            }

            return (extractStatusCode(m_commandResponse));


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
                sendFTPCommand("MLSD " + directoryPath + "\r\n");
                readDataChannelCommandResponse(listOutput);
            }

            return (extractStatusCode(m_commandResponse));


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

            sendFTPCommand("MLST " + filePath + "\r\n");
            waitForFTPCommandResponse(m_commandResponse);
            
            if (extractStatusCode(m_commandResponse)==250) {
                listOutput = m_commandResponse.substr(m_commandResponse.find('\n')+1);
                listOutput = listOutput.substr(0, listOutput.find('\r'));
            }

            return (extractStatusCode(m_commandResponse));


        }
        
        //
        // Change current working directory on server.
        //
        
        std::uint16_t CFTP::changeWorkingDirectory(const std::string &workingDirectoryPath) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            sendFTPCommand("CWD " + workingDirectoryPath + "\r\n");
            waitForFTPCommandResponse(m_commandResponse);

            return (extractStatusCode(m_commandResponse));

        }

        //
        // Fetch current working directory on server and return path as string.
        //
        
        std::uint16_t CFTP::getCurrentWoringDirectory(std::string &currentWoringDirectoryPath) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            currentWoringDirectoryPath.clear();

            sendFTPCommand("PWD\r\n");
            waitForFTPCommandResponse(m_commandResponse);

            currentWoringDirectoryPath = m_commandResponse.substr(m_commandResponse.find_first_of('\"') + 1);
            currentWoringDirectoryPath = currentWoringDirectoryPath.substr(0, currentWoringDirectoryPath.find_first_of('\"'));

            return (extractStatusCode(m_commandResponse));

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
