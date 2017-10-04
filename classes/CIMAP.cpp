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
// and receive string responses to them. It uses libcurl to provide 
// connection and command/response transport functionality. 
//
// Dependencies:   C11++     - Language standard features used.
//                 libcurl   - Used to talk to IMAP server.
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

    //
    // curl verbosity setting
    //
    
    bool CIMAP::m_curlVerbosity { false };
    
    // =======================
    // PUBLIC STATIC VARIABLES
    // =======================

    // ===============
    // PRIVATE METHODS
    // ===============

    //
    // Generate curl error message and throw exception
    //

    void CIMAP::throwCurlError(std::string baseMessage) {

        std::string errMsg;

        // Check for connected as an error can happen during connect
        
        if (this->m_connected) {
            disconnect();
        }

        if (std::strlen(this->m_curlErrMsgBuffer) != 0) {
            errMsg = this->m_curlErrMsgBuffer;
        } else {
            errMsg = curl_easy_strerror(this->m_curlResult);
        }
        throw Exception(baseMessage + errMsg);
    }

    //
    // Wait on send/recv curl socket event or timeout/error.
    //

    int CIMAP::waitOnSocket(bool bRecv, long timeoutMS) {

        struct timeval timeoutValue { 0 };
        fd_set recvfd { 0 };
        fd_set sendfd { 0 };
        fd_set errorfd { 0 };
        int res { 0 };

        timeoutValue.tv_sec = timeoutMS / 1000;
        timeoutValue.tv_usec = (timeoutMS % 1000) * 1000;

        FD_ZERO(&recvfd);
        FD_ZERO(&sendfd);
        FD_ZERO(&errorfd);

        FD_SET(this->m_curlSocketFD, &errorfd);

        if (bRecv) {
            FD_SET(this->m_curlSocketFD, &recvfd);
        } else {
            FD_SET(this->m_curlSocketFD, &sendfd);
        }

        res = select(this->m_curlSocketFD + 1, &recvfd, &sendfd, &errorfd, &timeoutValue);

        return res;

    }

    //
    // Send IMAP command to server. The maximum buffer size is CURL_MAX_WRITE_SIZE
    // so split up message into chunks before sending. This currently uses libcurls
    // curl_easy_send to transmit the data and this may return CURLE_AGAIN if the
    // underlying transport module is not ready to send the data; if so wait on socket.

    void CIMAP::sendIMAPCommand(const std::string& command) {

        size_t bytesSent { 0 };
        int bytesCopied { 0 };

        do {

            this->m_curlErrMsgBuffer[0] = 0;
            this->m_curlResult = curl_easy_send(this->m_curlHandle, &command[bytesCopied],
                    std::min((static_cast<int> (command.length()) - bytesCopied),
                    CURL_MAX_WRITE_SIZE), &bytesSent);

            if (this->m_curlResult == CURLE_AGAIN) {
                waitOnSocket(false, kWaitOnSocketTimeOut);
                continue;

            } else if (this->m_curlResult != CURLE_OK) {
                throwCurlError("curl_easy_send(): ");
            }

            bytesCopied += bytesSent;

        } while ((bytesCopied < command.length()));

    }

    //
    // Wait for reply from sent IMAP command. Append received data onto the end of 
    // commandResponse and exit when command tag encountered. If the server 
    // disconnects the socket then curl_easy_recv will return CURLE_OK and recvLength 
    // == 0 so return (clearing any response being received).
    //

    void CIMAP::waitForIMAPCommandResponse(const std::string& commandTag, std::string& commandResponse) {

        std::string searchTag { commandTag + " " };
        size_t recvLength { 0 };

        commandResponse.clear();

        do {

            this->m_curlErrMsgBuffer[0] = 0;
            this->m_curlResult = curl_easy_recv(this->m_curlHandle,
                    this->m_curlRxBuffer,
                    sizeof (this->m_curlRxBuffer), &recvLength);

            if (this->m_curlResult == CURLE_OK) {

                if (recvLength == 0) {
                    commandResponse.clear();
                    break;
                }

                this->m_curlRxBuffer[recvLength] = '\0';
                commandResponse.append(this->m_curlRxBuffer);

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

            } else if (this->m_curlResult == CURLE_AGAIN) {
                waitOnSocket(true, kWaitOnSocketTimeOut);

            } else {
                throwCurlError("curl_easy_recv(): ");
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
        ss << this->m_tagPrefix << std::setw(6) << std::setfill('0') << std::to_string(this->m_tagCount++);
        this->m_currentTag = ss.str();
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

        this->sendIMAPCommand(commandLine);
        this->waitForIMAPCommandResponse(kContinuation, this->m_commandResponse);

        if (!this->m_commandResponse.empty()) {

            this->waitForIMAPCommandResponse(kUntagged, response);

            if (!response.empty()) {
                this->sendIMAPCommand(static_cast<std::string> (kDONE) + kEOL);
                this->waitForIMAPCommandResponse(this->m_currentTag, this->m_commandResponse);
                if (!this->m_commandResponse.empty()) {
                    response += this->m_commandResponse;
                    this->m_commandResponse = response;
                }

            } else {
                this->m_commandResponse.clear();
            }

        }

    }

    //
    // Send APPPEND command (requires a special handler). The command up to  including the octet string
    // size has a "\r\n" appended and is sent. It then waits for a "+' where upon it sends the rest of the
    // octet string and the waits for the final APPEND response.Any response that is empty signals a server 
    // disconnect sso stop processing and pass up.
    //

    void CIMAP::sendCommandAPPEND(const std::string& commandLine) {

        this->sendIMAPCommand(commandLine.substr(0, commandLine.find_first_of('}') + 1) + kEOL);
        this->waitForIMAPCommandResponse(kContinuation, this->m_commandResponse);

        if (!this->m_commandResponse.empty()) {
            this->sendIMAPCommand(commandLine.substr(commandLine.find_first_of('}') + 1));
            this->waitForIMAPCommandResponse(this->m_currentTag, this->m_commandResponse);
        }

    }

    // ==============
    // PUBLIC METHODS
    // ==============

    //
    // Set IMAP server URL
    // 

    void CIMAP::setServer(const std::string& serverURL) {

        this->m_serverURL = serverURL;

    }

    //
    // Get IMAP server URL
    // 

    std::string CIMAP::getServer(void) const {

        return (this->m_serverURL);

    }

    //
    // Set email account details
    //

    void CIMAP::setUserAndPassword(const std::string& userName, const std::string& userPassword) {

        this->m_userName = userName;
        this->m_userPassword = userPassword;

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

        return (this->m_connected);

    }

    //
    // Set IMAP command tag prefix.
    //

    void CIMAP::setTagPrefix(const std::string& tagPrefix) {

        this->m_tagPrefix = tagPrefix;

    }
    
    //
    // Setup connection to server
    //

    void CIMAP::connect(void) {

        if (this->m_connected) {
            Exception("Already connected to a server.");
        }

        this->m_curlHandle = curl_easy_init();
        
        if (this->m_curlHandle) {

            curl_easy_setopt(this->m_curlHandle, CURLOPT_USERNAME, this->m_userName.c_str());
            curl_easy_setopt(this->m_curlHandle, CURLOPT_PASSWORD, this->m_userPassword.c_str());

            curl_easy_setopt(this->m_curlHandle, CURLOPT_VERBOSE, this->m_curlVerbosity);
            curl_easy_setopt(this->m_curlHandle, CURLOPT_URL, this->m_serverURL.c_str());

            curl_easy_setopt(this->m_curlHandle, CURLOPT_USE_SSL, (long) CURLUSESSL_ALL);
            curl_easy_setopt(this->m_curlHandle, CURLOPT_ERRORBUFFER, this->m_curlErrMsgBuffer);

            curl_easy_setopt(this->m_curlHandle, CURLOPT_CONNECT_ONLY, 1L);
            curl_easy_setopt(this->m_curlHandle, CURLOPT_MAXCONNECTS, 1L);

            this->m_curlErrMsgBuffer[0] = 0;
            this->m_curlResult = curl_easy_perform(this->m_curlHandle);
            if (this->m_curlResult != CURLE_OK) {
                throwCurlError("curl_easy_perform(): ");
            }

            // Get curl socket using CURLINFO_ACTIVESOCKET first then depreciated CURLINFO_LASTSOCKET

            this->m_curlErrMsgBuffer[0] = 0;
            this->m_curlResult = curl_easy_getinfo(this->m_curlHandle, CURLINFO_ACTIVESOCKET, &this->m_curlSocketFD);
            if (this->m_curlResult == CURLE_BAD_FUNCTION_ARGUMENT) {
                this->m_curlErrMsgBuffer[0] = 0;
                this->m_curlResult = curl_easy_getinfo(this->m_curlHandle, CURLINFO_LASTSOCKET, &this->m_curlSocketFD);
            }
            if (this->m_curlResult != CURLE_OK) {
                throwCurlError("Could not get curl socket.");
            }

            this->m_connected = true;

        }


    }

    //
    // Disconnect from server
    //

    void CIMAP::disconnect(void) {

        if (!this->m_connected) {
            throw Exception("Not connected to server.");
        }

        if (this->m_curlHandle) {
            curl_easy_cleanup(this->m_curlHandle);
            this->m_curlHandle = nullptr;
            this->m_tagCount = 1;
            this->m_connected = false;
        }

    }

    //
    // Send single IMAP command and return response including tagged command line.
    //

    std::string CIMAP::sendCommand(const std::string& commandLine) {

        if (!this->m_connected) {
            throw Exception("Not connected to server.");
        }

        this->generateTag();

        if (commandLine.compare(kIDLE) == 0) {
            sendCommandIDLE(this->m_currentTag + " " + commandLine + kEOL);
        } else if (commandLine.compare(kAPPEND) == 0) {
            sendCommandAPPEND(this->m_currentTag + " " + commandLine);
        } else {
            this->sendIMAPCommand(this->m_currentTag + " " + commandLine + kEOL);
            this->waitForIMAPCommandResponse(this->m_currentTag, this->m_commandResponse);
        }

        // If response is empty then server disconnect without BYE

        if (this->m_commandResponse.empty()) {
            disconnect();
            throw Exception("Server Disconnect without BYE.");
        }

        return (this->m_currentTag + " " + commandLine + kEOL + this->m_commandResponse);

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

        if (this->m_curlHandle) {
            curl_easy_cleanup(this->m_curlHandle);
        }

    }

    //
    // CIMAP initialization.
    //

    void CIMAP::init(bool bCurlVerbosity) {

        //
        //  CIMAP initialization. Globally init curl.
        //

        if (curl_global_init(CURL_GLOBAL_ALL)) {
            throw Exception("curl_global_init() : could not initialize libcurl.");
        }

        CIMAP::m_curlVerbosity = bCurlVerbosity;

    }

    //
    // CIMAP closedown
    //

    void CIMAP::closedown(void) {

        curl_global_cleanup();

    }
    
   } // namespace IMAP
} // namespace Antik
