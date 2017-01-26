/*
 * File:   CMailIMAP.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on January 24, 2017, 2:33 PM
 *
 * Copyright 2016.
 *
 */

//
// Class: CMailIMAP
// 
// Description: A class to connect to an IMAP server and send
// commands and recieve responses to them. The IDLE command is also
// supported (this provides a wait for activity on a mailbox such 
// as a new message has arrived). The class uses the libcurl to provide
// its functionality.
//
// Dependencies: C11++, libcurl, Linux.
//

// =================
// CLASS DEFINITIONS
// =================

#include "CMailIMAP.hpp"

// ====================
// CLASS IMPLEMENTATION
// ====================

// ===========================
// PRIVATE TYPES AND CONSTANTS
// ===========================

// Line terminator

const std::string CMailIMAP::kEOL("\r\n");

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
// Send IMAP command direct to server (used to implement IDLE).
//

void CMailIMAP::sendCommandDirect(const std::string& command) {

    size_t len = 0;

    std::cout << command;

    this->errMsgBuffer[0];
    this->res = curl_easy_send(this->curl, command.c_str(), command.length(), &len);

    if (this->res != CURLE_OK) {
        std::string errMsg;
        if (std::strlen(this->errMsgBuffer) != 0) {
            errMsg = this->errMsgBuffer;
        } else {
            errMsg = curl_easy_strerror(res);
        }
        throw std::runtime_error(std::string("curl_easy_send() failed: ") + errMsg);
    }

}

//
// Wait for reply from direct command (used to implement IDLE).
//

void CMailIMAP::waitForCommandResponse(const std::string& commandTag, std::string& commandResponse) {

    size_t len = 0;

    do {

        this->errMsgBuffer[0];
        this->res = curl_easy_recv(this->curl, this->rxBuffer, sizeof (this->rxBuffer) - 1, &len);

        if (this->res == CURLE_OK) {

            this->rxBuffer[len] = '\0';

            if (strstr(this->rxBuffer, commandTag.c_str()) != nullptr) {
                commandResponse.copy(this->rxBuffer, len);
                break;
            }
            
        } else if (this->res != CURLE_AGAIN) {
            std::string errMsg;
            if (std::strlen(this->errMsgBuffer) != 0) {
                errMsg = this->errMsgBuffer;
            } else {
                errMsg = curl_easy_strerror(this->res);
            }
            throw std::runtime_error(std::string("curl_easy_recv() failed: ") + errMsg);
        }
            
    } while (true);


}

//
// Append curl receive buffer onto string.
//

size_t CMailIMAP::writeFunction(void *ptr, size_t size, size_t nmemb, std::string* data) {
    data->append((char*) ptr, size * nmemb);
    return size * nmemb;
}

// ==============
// PUBLIC METHODS
// ==============

//
// Set IMAP server URL
// 

void CMailIMAP::setServer(const std::string& serverURL) {

    this->serverURL = serverURL;
}

//
// Set email account details
//

void CMailIMAP::setUserAndPassword(const std::string& userName, const std::string& userPassword) {

    this->userName = userName;
    this->userPassword = userPassword;

}

//
// Wait for IDLE on a mailbox to return.
//

void CMailIMAP::waitOnIdle(const std::string& imapMailBox) {

    if (this->curl) {

        curl_easy_setopt(this->curl, CURLOPT_USERNAME, this->userName.c_str());
        curl_easy_setopt(this->curl, CURLOPT_PASSWORD, this->userPassword.c_str());

        curl_easy_setopt(this->curl, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(this->curl, CURLOPT_URL, this->serverURL.c_str());

        curl_easy_setopt(this->curl, CURLOPT_CONNECT_ONLY, 1L);
        curl_easy_setopt(this->curl, CURLOPT_MAXCONNECTS, 1L);

        this->errMsgBuffer[0] = 0;
        this->res = curl_easy_perform(this->curl);

        if (this->res != CURLE_OK) {
            std::string errMsg;
            if (std::strlen(this->errMsgBuffer) != 0) {
                errMsg = this->errMsgBuffer;
            } else {
                errMsg = curl_easy_strerror(this->res);
            }
            throw std::runtime_error(std::string("curl_easy_perform() failed: ") + errMsg);
        }

        this->sendCommandDirect("A010 SELECT \"" + imapMailBox + "\"\r\n");
        this->waitForCommandResponse("A010", this->commandResponse);

        this->sendCommandDirect("A011 IDLE\r\n");
        this->waitForCommandResponse("*", this->commandResponse);

        this->sendCommandDirect("DONE\r\n");

        this->waitForCommandResponse("A011", this->commandResponse);

        curl_easy_setopt(this->curl, CURLOPT_CONNECT_ONLY, 0L);
        curl_easy_setopt(this->curl, CURLOPT_MAXCONNECTS, 5L);


    }


}

//
// Setup connection to server
//

void CMailIMAP::connect(void) {

    if (this->curl) {

        curl_easy_setopt(this->curl, CURLOPT_PROTOCOLS, CURLPROTO_IMAPS | CURLPROTO_IMAP);
        curl_easy_setopt(this->curl, CURLOPT_USERNAME, this->userName.c_str());
        curl_easy_setopt(this->curl, CURLOPT_PASSWORD, this->userPassword.c_str());
        curl_easy_setopt(this->curl, CURLOPT_URL, this->serverURL.c_str());

        curl_easy_setopt(this->curl, CURLOPT_USE_SSL, (long) CURLUSESSL_ALL);

        curl_easy_setopt(this->curl, CURLOPT_ERRORBUFFER, this->errMsgBuffer);

        curl_easy_setopt(this->curl, CURLOPT_VERBOSE, 0L);

        curl_easy_setopt(this->curl, CURLOPT_WRITEFUNCTION, CMailIMAP::writeFunction);
        curl_easy_setopt(this->curl, CURLOPT_WRITEDATA, &this->commandResponse);
        curl_easy_setopt(this->curl, CURLOPT_HEADERDATA, &this->commandHeader);

    }

}

//
// Disconnect from server
//

void CMailIMAP::disconnect() {
    if (this->curl) {
        curl_easy_cleanup(this->curl);
        this->curl = nullptr;
    }
}

//
// Send single IMAP command and return reply
//

std::string CMailIMAP::sendIMAPCommand(const std::string& command) {

    this->commandResponse.clear();
    this->commandHeader.clear();
    
    curl_easy_setopt(this->curl, CURLOPT_CUSTOMREQUEST, command.c_str());

    this->errMsgBuffer[0] = 0;
    this->res = curl_easy_perform(this->curl);

    if (this->res != CURLE_OK) {
        std::string errMsg;
        if (std::strlen(this->errMsgBuffer) != 0) {
            errMsg = this->errMsgBuffer;
        } else {
            errMsg = curl_easy_strerror(this->res);
        }
        throw std::runtime_error(std::string("curl_easy_perform() failed: ") + errMsg);
    }

    return (this->commandHeader);

}

//
// Main CMailIMAP object constructor. 
//

CMailIMAP::CMailIMAP() {

    this->curl = curl_easy_init();

}

//
// CMailIMAP Destructor
//

CMailIMAP::~CMailIMAP() {

    if (this->curl) {
        curl_easy_cleanup(this->curl);
    }
    
}

//
// CMailIMAP initialisation.
//

void CMailIMAP::init(void) {

    if (curl_global_init(CURL_GLOBAL_ALL)) {
        throw std::runtime_error(std::string("curl_global_init() : failure to initialize libcurl."));
    }

}

//
// CMailIMAP closedown
//

void CMailIMAP::closedown(void) {

    curl_global_cleanup();

}

