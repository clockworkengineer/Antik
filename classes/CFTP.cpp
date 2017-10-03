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
        // this is the best code i have seen for doing it. Note: Fall back of localhost
        // on failure,
        // 

        std::string CFTP::determineLocalIPAddress() {

            std::string localIPAddress{ "127.0.0.1"};

            try {
                ip::udp::resolver resolver(this->ioService);
                ip::udp::resolver::query query(ip::udp::v4(), "google.com", "");
                ip::udp::resolver::iterator endpoints = resolver.resolve(query);
                ip::udp::socket socket(this->ioService);
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

            std::uint16_t statusCode = std::stoi(commandResponse.substr(0, commandResponse.find(' ')));

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

            this->dataChannelPassiveAddresss = passiveParams;
            this->dataChannelPassivePort = std::to_string(port);


        }
        
        //
        // Create PORT command to send over control channel.
        //
        
        std::string CFTP::createPortCommand() {

            std::string portCommand{ "PORT "};

            ip::tcp::acceptor acceptor(this->ioService, ip::tcp::endpoint(ip::tcp::v4(), 0));
            unsigned short port = acceptor.local_endpoint().port();
            auto address = acceptor.local_endpoint().address();

            this->dataChannelActivePort = std::to_string(port);
            portCommand += this->dataChannelActiveAddresss;
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

                size_t len = this->dataChannelSocket.read_some(asio::buffer(this->ioBuffer), this->socketError);

                if (this->socketError == asio::error::eof) {
                    break; // Connection closed cleanly by peer.
                } else if (this->socketError) {
                    throw Exception(this->socketError.message());
                }

                localFile.write(&this->ioBuffer[0], len);

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

                localFile.read(&this->ioBuffer[0], this->ioBuffer.size());

                if (localFileLength < this->ioBuffer.size()) {
                    asio::write(this->dataChannelSocket, asio::buffer(this->ioBuffer, localFileLength), this->socketError);
                } else {
                    asio::write(this->dataChannelSocket, asio::buffer(this->ioBuffer, this->ioBuffer.size()), this->socketError);
                }

                localFileLength -= this->ioBuffer.size();

                if (this->socketError == asio::error::eof) {
                    break; // Connection closed cleanly by peer.
                } else if (this->socketError) {
                    throw Exception(this->socketError.message());
                }

            }

            localFile.close();

        }

        //
        // Data channel socket listener thread function for incoming data 
        // channel connections.
        //
        
        void CFTP::dataChannelTransferListener() {

            this->isListenThreadRunning = true;

            if (this->dataChannelSocket.is_open()) {
                this->dataChannelSocket.close();
            }

            this->sendFTPCommand(this->createPortCommand() + "\r\n");

            ip::tcp::acceptor acceptor(this->ioService, ip::tcp::endpoint(ip::tcp::v4(), std::stoi(this->dataChannelActivePort)));
            acceptor.accept(this->dataChannelSocket);

            this->isListenThreadRunning = false;

        }

        //
        // Cleanup after data channel transfer. This includes stopping any unused listener
        // thread and closing the socket if still open.
        //
        
        void CFTP::dataChannelTransferCleanup() {

            if (this->isListenThreadRunning) {
                ip::tcp::socket socket{ this->ioService};
                ip::tcp::resolver::query query(this->dataChannelActiveAddresss, this->dataChannelActivePort);
                asio::connect(socket, this->queryResolver.resolve(query));
                this->dataChannelListenThread->join();
            }

            if (this->dataChannelSocket.is_open()) {
                this->dataChannelSocket.close();
            }

        }

        //
        // Read any response to command on data channel (ie.LIST).
        //
        
        void CFTP::readDataChannelCommandResponse(std::string &commandResponse) {

            try {

                if (this->passiveMode) {
                    ip::tcp::resolver::query query(this->dataChannelPassiveAddresss, this->dataChannelPassivePort);
                    asio::connect(this->dataChannelSocket, this->queryResolver.resolve(query));
                }

                this->waitForFTPCommandResponse(this->commandResponse);

                std::uint16_t statusCode = this->extractStatusCode(this->commandResponse);

                if ((statusCode == 125) || (statusCode == 150)) {

                    if (!this->passiveMode) {
                        this->dataChannelListenThread->join();
                    }

                    for (;;) {

                        size_t len = this->dataChannelSocket.read_some(asio::buffer(this->ioBuffer), this->socketError);

                        if (this->socketError == asio::error::eof) {
                            break; // Connection closed cleanly by peer.
                        } else if (this->socketError) {
                            throw Exception(this->socketError.message());
                        }

                        for (auto byte : this->ioBuffer) {
                            if (len == 0) break;
                            commandResponse.append(1, byte);
                            len--;
                        }

                    }

                    if (this->dataChannelSocket.is_open()) {
                        this->dataChannelSocket.close();
                    }

                    this->waitForFTPCommandResponse(this->commandResponse);

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

                if (this->passiveMode) {
                    ip::tcp::resolver::query query(this->dataChannelPassiveAddresss, this->dataChannelPassivePort);
                    asio::connect(this->dataChannelSocket, this->queryResolver.resolve(query));
                }

                this->waitForFTPCommandResponse(this->commandResponse);
                std::uint16_t statusCode = this->extractStatusCode(this->commandResponse);

                if ((statusCode == 125) || (statusCode == 150)) {

                    if (!this->passiveMode) {
                        this->dataChannelListenThread->join();
                    }

                    if (downloading) {
                        this->downloadFile(file);
                    } else {
                        this->uploadFile(file);
                    }

                    if (this->dataChannelSocket.is_open()) {
                        this->dataChannelSocket.close();
                    }

                    this->waitForFTPCommandResponse(this->commandResponse);

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

            asio::write(this->controlChannelSocket, asio::buffer(&command[0], command.size()), this->socketError);

            if (this->socketError) {
                throw Exception(this->socketError.message());
            }

        }
        
        //
        // Read FTP command response fromconrol channel.
        //

        void CFTP::waitForFTPCommandResponse(std::string& commandResponse) {

            size_t recvLength { 0 };

            commandResponse.clear();

            for (;;) {

                recvLength = this->controlChannelSocket.read_some(asio::buffer(this->ioBuffer, this->ioBuffer.size()), this->socketError);

                if (recvLength == 0) {
                    commandResponse.clear();
                    break;
                }

                this->ioBuffer[recvLength] = '\0';
                commandResponse.append(&this->ioBuffer[0]);

                recvLength = commandResponse.length();
                if ((commandResponse[recvLength - 2] == '\r') && (commandResponse[recvLength - 1] == '\n')) {
                    break;
                }

                if (this->socketError == asio::error::eof) {
                    break; // Connection closed cleanly by peer.
                } else if (this->socketError) {
                    throw Exception(this->socketError.message());
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

            this->userName = userName;
            this->userPassword = userPassword;

        }

        //
        // Set FTP server name and port
        //

        void CFTP::setServerAndPort(const std::string& serverName, const std::string& serverPort) {

            this->serverName = serverName;
            this->serverPort = serverPort;

        }

        //
        // Get current connection status with server
        //

        bool CFTP::getConnectedStatus(void) const {

            return (this->bConnected);

        }

        //
        // Setup connection to server
        //

        void CFTP::connect(void) {

            if (this->bConnected) {
                Exception("Already connected to a server.");
            }

            ip::tcp::resolver::query query(this->serverName, this->serverPort);

            asio::connect(this->controlChannelSocket, this->queryResolver.resolve(query), this->socketError);

            if (this->socketError) {
                throw Exception(this->socketError.message());
            }

            this->waitForFTPCommandResponse(this->commandResponse);

            this->bConnected = true;

            this->sendFTPCommand("USER " + this->userName + "\r\n");
            this->waitForFTPCommandResponse(this->commandResponse);
            this->sendFTPCommand("PASS " + this->userPassword + "\r\n");
            this->waitForFTPCommandResponse(this->commandResponse);

            this->dataChannelActiveAddresss = this->determineLocalIPAddress();


        }

        //
        // Disconnect from server
        //

        void CFTP::disconnect(void) {

            if (!this->bConnected) {
                throw Exception("Not connected to server.");
            }

            this->bConnected = false;

        }
        
        //
        // Set passive transfer mode.
        // == true passive enabled, == false active mode
        //

        void CFTP::setPassiveTransferMode(bool passiveEnabled) {

            if (!this->bConnected) {
                throw Exception("Not connected to server.");
            }

            this->passiveMode = passiveEnabled;

        }

        //
        // Send transfer mode to used over data channel.
        //
        
        bool CFTP::sendTransferMode() {
            if (this->passiveMode) {
                this->sendFTPCommand("PASV\r\n");
                this->waitForFTPCommandResponse(this->commandResponse);
                extractPassiveAddressPort(this->commandResponse);
                return (this->extractStatusCode(this->commandResponse) == 227);
            } else {
                this->dataChannelListenThread.reset(new std::thread(&CFTP::dataChannelTransferListener, this));
                this->waitForFTPCommandResponse(this->commandResponse);
                return (this->extractStatusCode(this->commandResponse) == 200);
            }
        }

        //
        // Transfer a file from the server to a local file.
        //
        
        std::uint16_t CFTP::getFile(const std::string &remoteFilePath, const std::string &localFilePath) {

            if (!this->bConnected) {
                throw Exception("Not connected to server.");
            }

            if (sendTransferMode()) {
                this->sendFTPCommand("RETR " + remoteFilePath + "\r\n");
                this->transferFile(localFilePath, true);
            }

            return (this->extractStatusCode(this->commandResponse));


        }

        //
        // Transfer a file to the server from a local file.
        //
        
        std::uint16_t CFTP::putFile(const std::string &remoteFilePath, const std::string &localFilePath) {

            if (!this->bConnected) {
                throw Exception("Not connected to server.");
            }

            if (sendTransferMode()) {
                this->sendFTPCommand("STOR " + remoteFilePath + "\r\n");
                this->transferFile(localFilePath, false);
            }

            return (this->extractStatusCode(this->commandResponse));

        }

        //
        // Produce a directory listing for the file/directory passed in or for the current
        // working directory if none is.
        //
        
        std::uint16_t CFTP::list(const std::string &directoryPath, std::string &listOutput) {

            if (!this->bConnected) {
                throw Exception("Not connected to server.");
            }

            listOutput.clear();

            if (sendTransferMode()) {
                this->sendFTPCommand("LIST " + directoryPath + "\r\n");
                this->readDataChannelCommandResponse(listOutput);
            }

            return (this->extractStatusCode(this->commandResponse));

        }

        //
        // Produce a file list for the file/directory passed in or for the current
        // working directory if none is.
        //
        
        std::uint16_t CFTP::listFiles(const std::string &directoryPath, std::string &listOutput) {

            if (!this->bConnected) {
                throw Exception("Not connected to server.");
            }

            listOutput.clear();

            if (sendTransferMode()) {
                this->sendFTPCommand("NLST " + directoryPath + "\r\n");
                this->readDataChannelCommandResponse(listOutput);
            }

            return (this->extractStatusCode(this->commandResponse));


        }

        //
        // Change current working directory on server.
        //
        
        std::uint16_t CFTP::changeWorkingDirectory(const std::string &workingDirectoryPath) {

            if (!this->bConnected) {
                throw Exception("Not connected to server.");
            }

            this->sendFTPCommand("CWD " + workingDirectoryPath + "\r\n");
            this->waitForFTPCommandResponse(this->commandResponse);

            return (this->extractStatusCode(this->commandResponse));

        }

        //
        // Fetch current working directory on server and return path as string.
        //
        
        std::uint16_t CFTP::getCurrentWoringDirectory(std::string &currentWoringDirectoryPath) {

            if (!this->bConnected) {
                throw Exception("Not connected to server.");
            }

            currentWoringDirectoryPath.clear();

            this->sendFTPCommand("PWD\r\n");
            this->waitForFTPCommandResponse(this->commandResponse);

            currentWoringDirectoryPath = this->commandResponse.substr(this->commandResponse.find_first_of('\"') + 1);
            currentWoringDirectoryPath = currentWoringDirectoryPath.substr(0, currentWoringDirectoryPath.find_first_of('\"'));

            return (this->extractStatusCode(this->commandResponse));

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
