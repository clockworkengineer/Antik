#include "HOST.hpp"
/*
 * File:   CFTP.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on January 24, 2017, 2:33 PM
 *
 * Copyright 2016.
 *
 */

//
// Class: CFTP
// 
// Description: A class to connect to an FTP server.
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

        std::string CFTP::determineLocalIPAddress() {

            std::string localIPAddress{ "127.0.0.1"};

            try {
                boost::asio::io_service netService;
                boost::asio::ip::udp::resolver resolver(netService);
                boost::asio::ip::udp::resolver::query query(boost::asio::ip::udp::v4(), "google.com", "");
                boost::asio::ip::udp::resolver::iterator endpoints = resolver.resolve(query);
                boost::asio::ip::udp::endpoint ep = *endpoints;
                boost::asio::ip::udp::socket socket(netService);
                socket.connect(ep);
                localIPAddress = socket.local_endpoint().address().to_string();
            } catch (std::exception& e) {
                return (localIPAddress);
            }

            return (localIPAddress);

        }

        std::uint16_t CFTP::extractStatusCode(const std::string &commandResponse) {

            std::uint16_t statusCode = std::stoi(commandResponse.substr(0, commandResponse.find(' ')));

            //   std::cout << "STATUS CODE : " << statusCode << std::endl;
            //   std::cout << "MESSAGE : " << commandResponse.substr(commandResponse.find(' ') + 1);

            return (statusCode);

        }

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

        std::string CFTP::createPortCommand() {

            std::string portCommand{ "PORT "};

            tcp::acceptor acceptor(this->ioService, tcp::endpoint(tcp::v4(), 0));
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

        void CFTP::downloadFileWrite(tcp::socket &socket, const std::string &file) {

            fs::path localFilePath{ file};

            if (fs::exists(localFilePath)) {
                fs::remove(localFilePath);
            }

            std::ofstream localFile{ localFilePath.string(), std::ofstream::binary};

            for (;;) {

                size_t len = socket.read_some(boost::asio::buffer(this->ioBuffer), this->socketError);

                if (this->socketError == boost::asio::error::eof) {
                    break; // Connection closed cleanly by peer.
                } else if (this->socketError) {
                    throw Exception(this->socketError.message());
                }

                localFile.write(&this->ioBuffer[0], len);

            }

            localFile.close();

        }

        void CFTP::uploadFileRead(tcp::socket &socket, const std::string &file) {

            fs::path localFilePath{ file};

            std::ifstream localFile{ localFilePath.string(), std::ofstream::binary};

            std::size_t localFileLength = localFile.tellg();

            for (;;) {

                if (localFile) break;

                localFile.read(&this->ioBuffer[0], this->ioBuffer.size());

                if (localFileLength < this->ioBuffer.size()) {
                    boost::asio::write(socket, boost::asio::buffer(this->ioBuffer, localFileLength), this->socketError);
                } else {
                    boost::asio::write(socket, boost::asio::buffer(this->ioBuffer, this->ioBuffer.size()), this->socketError);
                }

                localFileLength -= this->ioBuffer.size();

                if (this->socketError == boost::asio::error::eof) {
                    break; // Connection closed cleanly by peer.
                } else if (this->socketError) {
                    throw Exception(this->socketError.message());
                }

            }

        }

        void CFTP::dataChannelLisenter() {

            this->sendFTPCommand(this->createPortCommand() + "\r\n");
            tcp::acceptor acceptor(this->ioService, tcp::endpoint(tcp::v4(), std::stoi(this->dataChannelActivePort)));
            acceptor.accept(this->dataChannelSocket);

        }

        void CFTP::readCommandResponse(std::string &buffer) {


            try {

                if (this->passiveMode) {
                    tcp::resolver::query query(this->dataChannelPassiveAddresss, this->dataChannelPassivePort);
                    boost::asio::connect(this->dataChannelSocket, this->queryResolver.resolve(query));
                } else {
                    this->dataChannelListenThread->join();
                }

                for (;;) {

                    size_t len = this->dataChannelSocket.read_some(boost::asio::buffer(this->ioBuffer), this->socketError);

                    if (this->socketError == boost::asio::error::eof) {
                        break; // Connection closed cleanly by peer.
                    } else if (this->socketError) {
                        throw Exception(this->socketError.message());
                    }

                    for (auto byte : this->ioBuffer) {
                        if (len == 0) break;
                        buffer.append(1, byte);
                        len--;
                    }

                }

            } catch (CFTP::Exception& e) {
                if (this->dataChannelSocket.is_open()) {
                    this->dataChannelSocket.close();
                }
                throw;
            } catch (std::exception& e) {
                if (this->dataChannelSocket.is_open()) {
                    this->dataChannelSocket.close();
                }
                throw Exception(e.what());
            }

            if (this->dataChannelSocket.is_open()) {
                this->dataChannelSocket.close();
            }

        }

        void CFTP::downloadingFile(const std::string &file) {

            try {

                if (this->passiveMode) {
                    tcp::resolver::query query(this->dataChannelPassiveAddresss, this->dataChannelPassivePort);
                    boost::asio::connect(this->dataChannelSocket, this->queryResolver.resolve(query));
                } else {
                    this->dataChannelListenThread->join();
                }

                this->downloadFileWrite(this->dataChannelSocket, file);

            } catch (std::exception& e) {
                if (this->dataChannelSocket.is_open()) {
                    this->dataChannelSocket.close();
                }
                throw Exception(e.what());
            }
            if (this->dataChannelSocket.is_open()) {
                this->dataChannelSocket.close();
            }
        }

        void CFTP::uploadingFile(const std::string &file) {

            try {

                if (this->passiveMode) {
                    tcp::resolver::query query(this->dataChannelPassiveAddresss, this->dataChannelPassivePort);
                    boost::asio::connect(this->dataChannelSocket, this->queryResolver.resolve(query));
                } else {
                    this->dataChannelListenThread->join();
                }

                this->uploadFileRead(this->dataChannelSocket, file);

            } catch (std::exception& e) {
                if (this->dataChannelSocket.is_open()) {
                    this->dataChannelSocket.close();
                }
                throw Exception(e.what());
            }

            if (this->dataChannelSocket.is_open()) {
                this->dataChannelSocket.close();
            }

        }

        void CFTP::sendFTPCommand(const std::string& command) {

            boost::asio::write(this->controlChannelSocket, boost::asio::buffer(command.data(), command.size()), this->socketError);

            if (this->socketError) {
                throw Exception(this->socketError.message());
            }

        }

        void CFTP::waitForFTPCommandResponse(std::string& commandResponse) {

            size_t recvLength{ 0};

            commandResponse.clear();

            for (;;) {

                recvLength = this->controlChannelSocket.read_some(boost::asio::buffer(this->ioBuffer, this->ioBuffer.size()), this->socketError);

                if (recvLength == 0) {
                    commandResponse.clear();
                    break;
                }

                this->ioBuffer[recvLength] = '\0';
                commandResponse.append(&this->ioBuffer[0]);

                recvLength = commandResponse.length();
                if ((commandResponse[recvLength - 2] == '\r') &&(commandResponse[recvLength - 1] == '\n')) {
                    break;
                }

                if (this->socketError == boost::asio::error::eof) {
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

            tcp::resolver::query query(this->serverName, this->serverPort);

            boost::asio::connect(this->controlChannelSocket, this->queryResolver.resolve(query), this->socketError);

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

        std::uint16_t CFTP::getFile(const std::string &remoteFilePath, const std::string &localFilePath) {

            if (!this->bConnected) {
                throw Exception("Not connected to server.");
            }

            this->sendFTPCommand("RETR " + remoteFilePath + "\r\n");
            this->waitForFTPCommandResponse(this->commandResponse);

            std::uint16_t returnedStatus = this->extractStatusCode(this->commandResponse);

            if (returnedStatus == 125 || returnedStatus == 150) {
                this->downloadingFile(localFilePath);
                this->waitForFTPCommandResponse(this->commandResponse);
            }

            return (this->extractStatusCode(this->commandResponse));


        }

        std::uint16_t CFTP::putFile(const std::string &remoteFilePath, const std::string &localFilePath) {

            if (!this->bConnected) {
                throw Exception("Not connected to server.");
            }

            this->sendFTPCommand("STOR " + remoteFilePath + "\r\n");
            this->waitForFTPCommandResponse(this->commandResponse);

            std::uint16_t returnedStatus = this->extractStatusCode(this->commandResponse);

            if (returnedStatus == 150) {
                this->uploadingFile(localFilePath);
                this->waitForFTPCommandResponse(this->commandResponse);
            }

            return (this->extractStatusCode(this->commandResponse));

        }

        std::uint16_t CFTP::setPassiveTrasnferMode(bool passiveEnabled) {

            if (!this->bConnected) {
                throw Exception("Not connected to server.");
            }

            if (passiveEnabled) {
                this->passiveMode = true;
                this->sendFTPCommand("PASV\r\n");
                this->waitForFTPCommandResponse(this->commandResponse);
                extractPassiveAddressPort(this->commandResponse);
                return (this->extractStatusCode(this->commandResponse));
            } else {
                this->passiveMode = false;
                this->dataChannelListenThread.reset(new std::thread(&CFTP::dataChannelLisenter, this));
                this->waitForFTPCommandResponse(this->commandResponse);
                return (this->extractStatusCode(this->commandResponse));
            }

            return (-1);

        }

        std::string CFTP::listDirectory(std::string directoryPath) {

            std::string listing;

            if (!this->bConnected) {
                throw Exception("Not connected to server.");
            }

            this->sendFTPCommand("LIST " + directoryPath + "\r\n");
            this->waitForFTPCommandResponse(this->commandResponse);

            std::uint16_t returnedStatus = this->extractStatusCode(this->commandResponse);

            if (returnedStatus == 150) {
                this->readCommandResponse(listing);
                this->waitForFTPCommandResponse(this->commandResponse);
                this->extractStatusCode(this->commandResponse);
            }

            return (listing);

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
