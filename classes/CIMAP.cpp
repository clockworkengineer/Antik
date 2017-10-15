#include "HOST.hpp"
/*
 * File:   CIMAP.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on January 24, 2017, 2:33 PM
 *
 * Copyright 2016.
 *
 */

//
// Class: CIMAP
// 
// Description: A class to connect to an IMAP server, send commands
// and receive string responses to them. It uses Boost::asio to provide 
// connection and command/response transport functionality. 
//
// Dependencies:   C11++     - Language standard features used.
//                 CSocket   - Used to talk to IMAP server.
//

// =================
// CLASS DEFINITIONS
// =================

#include "CIMAP.hpp"

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

//
// Antik IMAP pasrser
//

#include "CIMAPParse.hpp"

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace IMAP {

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
        // Send IMAP command to server. 
        //

        void CIMAP::sendIMAPCommand(const std::string& command) {

            size_t bytesSent { 0 };
            int bytesCopied { 0 };

            do {

                bytesSent = m_imapSocket.write(&command[bytesCopied], command.length() - bytesCopied);
                bytesCopied += bytesSent;

            } while ((bytesCopied < command.length()));

        }

        //
        // Wait for reply from sent IMAP command. Append received data onto the end of 
        // commandResponse and exit when command tag encountered.
        //

        void CIMAP::waitForIMAPCommandResponse(const std::string& commandTag, std::string& commandResponse) {

            std::string searchTag{ commandTag + " "};
            size_t recvLength{ 0};

            commandResponse.clear();

            do {


                recvLength = m_imapSocket.read(m_ioBuffer, sizeof (m_ioBuffer));

                m_ioBuffer[recvLength] = '\0';
                commandResponse.append(m_ioBuffer);

                recvLength = commandResponse.length();
                if ((commandResponse[recvLength - 2] == '\r') &&
                        (commandResponse[recvLength - 1] == '\n')) {
                    // Find the previous end of line and search for tag from there.
                    // This cuts down search time on large buffered responses ie.
                    // encoded attachments.
                    size_t prevNewLinePos = commandResponse.rfind(kEOL, recvLength - 3);
                    if (prevNewLinePos == std::string::npos) {
                        prevNewLinePos = 0;
                    }
                    if (commandResponse.find(searchTag, prevNewLinePos) != std::string::npos) {
                        break;
                    }
                }


            } while (true);

        }

        //
        // Generate next command tag. This is just "A"+number at the moment but the
        // tag counter that is used is incremented so that the tag will be different on
        // the next call. Note: The numeric component has leading zeros.
        //

        void CIMAP::generateTag() {
            std::ostringstream ss;
            ss << m_tagPrefix << std::setw(6) << std::setfill('0') << std::to_string(m_tagCount++);
            m_currentTag = ss.str();
        }

        //
        // Send IDLE command (requires a special handler). When IDLE is sent it then waits
        // for a '+' from the server. Here it knows to wait for an un-tagged response where
        // upon it sends "DONE" and waits for the final tagged IDLE response. Note: The
        // un-tagged response before "DONE" sent is saved and placed onto the front of
        // the final IDLE response. Any response that is empty signals a server disconnect
        // so stop processing and pass up.
        //

        void CIMAP::sendCommandIDLE(const std::string& commandLine) {

            std::string response;

            sendIMAPCommand(commandLine);
            waitForIMAPCommandResponse(kContinuation, m_commandResponse);

            if (!m_commandResponse.empty()) {

                waitForIMAPCommandResponse(kUntagged, response);

                if (!response.empty()) {
                    sendIMAPCommand(static_cast<std::string> (kDONE) + kEOL);
                    waitForIMAPCommandResponse(m_currentTag, m_commandResponse);
                    if (!m_commandResponse.empty()) {
                        response += m_commandResponse;
                        m_commandResponse = response;
                    }

                } else {
                    m_commandResponse.clear();
                }

            }

        }

        //
        // Send APPPEND command (requires a special handler). The command up to  including the octet string
        // size has a "\r\n" appended and is sent. It then waits for a "+' where upon it sends the rest of the
        // octet string and the waits for the final APPEND response.Any response that is empty signals a server 
        // disconnect so stop processing and pass up.
        //

        void CIMAP::sendCommandAPPEND(const std::string& commandLine) {

            sendIMAPCommand(commandLine.substr(0, commandLine.find_first_of('}') + 1) + kEOL);
            waitForIMAPCommandResponse(kContinuation, m_commandResponse);

            if (!m_commandResponse.empty()) {
                sendIMAPCommand(commandLine.substr(commandLine.find_first_of('}') + 1));
                waitForIMAPCommandResponse(m_currentTag, m_commandResponse);
            }

        }

        // ==============
        // PUBLIC METHODS
        // ==============

        //
        // Set IMAP server URL
        // 

        void CIMAP::setServer(const std::string& serverURL) {

            m_serverURL = serverURL;

            std::string server = serverURL.substr(serverURL.find("//") + 2);

            m_imapSocket.setHostAddress(server.substr(0, server.find(":")));
            m_imapSocket.setHostPort(serverURL.substr(serverURL.rfind(":") + 1));


        }

        //
        // Get IMAP server URL
        // 

        std::string CIMAP::getServer(void) const {

            return (m_serverURL);

        }

        //
        // Set email account details
        //

        void CIMAP::setUserAndPassword(const std::string& userName, const std::string& userPassword) {

            m_userName = userName;
            m_userPassword = userPassword;

        }

        //
        // Get email account user details
        //

        std::string CIMAP::getUser(void) const {

            return (m_userName);

        }

        //
        // Get current connection status with server
        //

        bool CIMAP::getConnectedStatus(void) const {

            return (m_connected);

        }

        //
        // Set IMAP command tag prefix.
        //

        void CIMAP::setTagPrefix(const std::string& tagPrefix) {

            m_tagPrefix = tagPrefix;

        }

        //
        // Setup connection to server
        //

        void CIMAP::connect(void) {

            if (m_connected) {
                Exception("Already connected to a server.");
            }

            // Connect and perform TLS handshake 
            
            m_imapSocket.connect();
            m_imapSocket.tlsHandshake();

            m_connected = true;
            
            // Login using set credentials

            CIMAPParse::COMMANDRESPONSE parsedResponse = CIMAPParse::parseResponse(sendCommand("LOGIN " + this->m_userName + " " + this->m_userPassword));
            if (parsedResponse->bBYESent) {
                throw CIMAP::Exception("Received BYE from server: " + parsedResponse->errorMessage);
            } else if (parsedResponse->status != CIMAPParse::RespCode::OK) {
                throw CIMAP::Exception(static_cast<std::string> ("LOGIN ") + ": " + parsedResponse->errorMessage);
            }

        }

        //
        // Disconnect from server
        //

        void CIMAP::disconnect(void) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            m_imapSocket.close();
            
            m_tagCount = 1;
            m_connected = false;

        }

        //
        // Send single IMAP command and return response including tagged command line.
        //

        std::string CIMAP::sendCommand(const std::string& commandLine) {

            if (!m_connected) {
                throw Exception("Not connected to server.");
            }

            generateTag();

            if (commandLine.compare(kIDLE) == 0) {
                sendCommandIDLE(m_currentTag + " " + commandLine + kEOL);
            } else if (commandLine.compare(kAPPEND) == 0) {
                sendCommandAPPEND(m_currentTag + " " + commandLine);
            } else {
                sendIMAPCommand(m_currentTag + " " + commandLine + kEOL);
                waitForIMAPCommandResponse(m_currentTag, m_commandResponse);
            }

            // If response is empty then server disconnect without BYE

            if (m_commandResponse.empty()) {
                disconnect();
                throw Exception("Server Disconnect without BYE.");
            }

            return (m_currentTag + " " + commandLine + kEOL + m_commandResponse);

        }

        //
        // Main CIMAP object constructor. 
        //

        CIMAP::CIMAP() {

        }

        //
        // CIMAP Destructor
        //

        CIMAP::~CIMAP() {

        }

        //
        // CIMAP initialization (NOT NEEDED NOW).
        //

        void CIMAP::init(bool bCurlVerbosity) {


        }

        //
        // CIMAP closedown
        //

        void CIMAP::closedown(void) {


        }

    } // namespace IMAP
} // namespace Antik
