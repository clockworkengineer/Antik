#include "HOST.hpp"
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

#include "CMailIMAP.hpp"

// ====================
// CLASS IMPLEMENTATION
// ====================

//
// C++ STL definitions
//

#include <cstring>
#include <sstream>
#include <iomanip>
#include <iostream>

// ===========================
// PRIVATE TYPES AND CONSTANTS
// ===========================

// ==========================
// PUBLIC TYPES AND CONSTANTS
// ==========================

//
// End Of Line terminator
//

const char *CMailIMAP::kEOLStr = "\r\n";

//
// IMAP Command strings
//

const char *CMailIMAP::kSTARTTLSStr = "STARTTLS";
const char *CMailIMAP::kAUTHENTICATEStr{"AUTHENTICATE"};
const char *CMailIMAP::kSEARCHStr = "SEARCH";
const char *CMailIMAP::kSELECTStr = "SELECT";
const char *CMailIMAP::kEXAMINEStr = "EXAMINE";
const char *CMailIMAP::kCREATEStr = "CREATE";
const char *CMailIMAP::kDELETEStr = "DELETE";
const char *CMailIMAP::kRENAMEStr = "RENAME";
const char *CMailIMAP::kLOGINStr = "LOGIN";
const char *CMailIMAP::kSUBSCRIBEStr = "SUBSCRIBE";
const char *CMailIMAP::kUNSUBSCRIBEStr = "UNSUBSCRIBE";
const char *CMailIMAP::kLISTStr = "LIST";
const char *CMailIMAP::kLSUBStr = "LSUB";
const char *CMailIMAP::kSTATUSStr = "STATUS";
const char *CMailIMAP::kAPPENDStr = "APPEND";
const char *CMailIMAP::kCHECKStr = "CHECK";
const char *CMailIMAP::kCLOSEStr = "CLOSE";
const char *CMailIMAP::kEXPUNGEStr = "EXPUNGE";
const char *CMailIMAP::kFETCHStr = "FETCH";
const char *CMailIMAP::kSTOREStr = "STORE";
const char *CMailIMAP::kCOPYStr = "COPY";
const char *CMailIMAP::kNOOPStr = "NOOP";
const char *CMailIMAP::kLOGOUTStr = "LOGOUT";
const char *CMailIMAP::kIDLEStr = "IDLE";
const char *CMailIMAP::kCAPABILITYStr = "CAPABILITY";
const char *CMailIMAP::kUIDStr = "UID";

//
// IMAP Response strings
//

const char *CMailIMAP::kUntaggedStr = "*";
const char *CMailIMAP::kOKStr = "OK";
const char *CMailIMAP::kBADStr = "BAD";
const char *CMailIMAP::kNOStr = "NO";
const char *CMailIMAP::kFLAGSStr = "FLAGS";
const char *CMailIMAP::kPERMANENTFLAGSStr = "PERMANENTFLAGS";
const char *CMailIMAP::kUIDVALIDITYStr = "UIDVALIDITY";
const char *CMailIMAP::kUIDNEXTStr = "UIDNEXT";
const char *CMailIMAP::kHIGHESTMODSEQStr = "HIGHESTMODSEQ";
const char *CMailIMAP::kUNSEENStr = "UNSEEN";
const char *CMailIMAP::kEXISTSStr = "EXISTS";
const char *CMailIMAP::kRECENTStr = "RECENT";
const char *CMailIMAP::kDONEStr = "DONE";
const char *CMailIMAP::kContinuationStr = "+";
const char *CMailIMAP::kENVELOPEStr = "ENVELOPE";
const char *CMailIMAP::kBODYSTRUCTUREStr = "BODYSTRUCTURE";
const char *CMailIMAP::kBODYStr = "BODY";
const char *CMailIMAP::kRFC822Str = "RFC822";
const char *CMailIMAP::kINTERNALDATEStr = "INTERNALDATE";
const char *CMailIMAP::kRFC822HEADERStr = "RFC822.HEADER";
const char *CMailIMAP::kRFC822SIZEStr = "RFC822.SIZE";
const char *CMailIMAP::kRFC822TEXTStr = "RFC822.TEXT";
const char *CMailIMAP::kBYEStr = "BYE";

//
// Response MAP generated entries.
//

const char *CMailIMAP::kMAILBOXNAMEStr = "MAILBOX-NAME";
const char *CMailIMAP::kMAILBOXACCESSStr = "MAILBOX-ACCESS";


// ========================
// PRIVATE STATIC VARIABLES
// ========================

// curl verbosity setting

bool CMailIMAP::bCurlVerbosity = false;

// =======================
// PUBLIC STATIC VARIABLES
// =======================

// ===============
// PRIVATE METHODS
// ===============

//
// Generate curl error message and throw exception
//

void CMailIMAP::throwCurlError(std::string baseMessageStr) {

    std::string errMsgStr;
    if (std::strlen(this->curlErrMsgBuffer) != 0) {
        errMsgStr = this->curlErrMsgBuffer;
    } else {
        errMsgStr = curl_easy_strerror(this->curlResult);
    }
    throw CMailIMAP::Exception(baseMessageStr + errMsgStr);
}

//
// Wait on send/recv curl socket event or timeout/error.
//

int CMailIMAP::waitOnSocket(bool bRecv, long timeoutMS) {

    struct timeval timeoutValue;
    fd_set recvfd, sendfd, errorfd;
    int res;

    timeoutValue.tv_sec = timeoutMS / 1000;
    timeoutValue.tv_usec = (timeoutMS % 1000) * 1000;

    FD_ZERO(&recvfd);
    FD_ZERO(&sendfd);
    FD_ZERO(&errorfd);

    FD_SET(this->curlSocketFD, &errorfd);

    if (bRecv) {
        FD_SET(this->curlSocketFD, &recvfd);
    } else {
        FD_SET(this->curlSocketFD, &sendfd);
    }

    res = select(this->curlSocketFD + 1, &recvfd, &sendfd, &errorfd, &timeoutValue);

    return res;

}

//
// Send IMAP command to server. The maximum buffer size is CURL_MAX_WRITE_SIZE
// so split up message into chunks before sending. This currently uses libcurls
// curl_easy_send to transmit the data and this may return CURLE_AGAIN if the
// underlying transport module is not ready to send the data; if so wait on socket.

void CMailIMAP::sendIMAPCommand(const std::string& commandStr) {

    size_t bytesSent = 0;
    int bytesCopied = 0;

    do {

        this->curlErrMsgBuffer[0] = 0;
        this->curlResult = curl_easy_send(this->curlHandle, &commandStr[bytesCopied],
                std::min((static_cast<int> (commandStr.length()) - bytesCopied),
                CURL_MAX_WRITE_SIZE), &bytesSent);

        if (this->curlResult == CURLE_AGAIN) {
            waitOnSocket(false, kWaitOnSocketTimeOut);
            continue;

        } else if (this->curlResult != CURLE_OK) {
            throwCurlError("curl_easy_send(): ");
        }

        bytesCopied += bytesSent;

    } while ((bytesCopied < commandStr.length()));

}

//
// Wait for reply from sent IMAP command. Append received data onto the end of 
// commandResponseStr and exit when command tag encountered. If the server 
// disconnects the socket then curl_easy_recv will return CURLE_OK and recvLength 
// == 0 so return.
//

void CMailIMAP::waitForIMAPCommandResponse(const std::string& commandTagStr, std::string& commandResponseStr) {

    std::string searchTagStr{ commandTagStr + " "};
    size_t recvLength = 0;

    commandResponseStr.clear();

    do {

        this->curlErrMsgBuffer[0] = 0;
        this->curlResult = curl_easy_recv(this->curlHandle,
                this->curlRxBuffer,
                sizeof (this->curlRxBuffer), &recvLength);

        if (this->curlResult == CURLE_OK) {

            if (recvLength == 0) {
                break;
            }

            this->curlRxBuffer[recvLength] = '\0';
            commandResponseStr.append(this->curlRxBuffer);

            recvLength = commandResponseStr.length();
            if ((commandResponseStr[recvLength - 2] == '\r') &&
                    (commandResponseStr[recvLength - 1] == '\n')) {
                size_t prevNewLinePos = commandResponseStr.rfind(CMailIMAP::kEOLStr, recvLength - 3);
                if (prevNewLinePos == std::string::npos) {
                    prevNewLinePos = 0;
                }
                if (commandResponseStr.find(searchTagStr, prevNewLinePos) != std::string::npos) {
                    break;
                }
            }

        } else if (this->curlResult == CURLE_AGAIN) {
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

inline void CMailIMAP::generateTag() {
    std::ostringstream ss;
    ss << "A" << std::setw(6) << std::setfill('0') << std::to_string(this->tagCount++);
    this->currentTagStr = ss.str();
}

//
// Send IDLE command (requires a special handler). When IDLE is sent it then waits
// for a '+' from the server. Here it knows to wait for an un-tagged response where
// upon it sends "DONE" and waits for the final tagged IDLE response. Note: The
// un-tagged response before "DONE" sent is saved and placed onto the front of
// the final IDLE response.
//

void CMailIMAP::sendCommandIDLE(const std::string& commandLineStr) {

    std::string responseStr;

    this->sendIMAPCommand(commandLineStr);
    this->waitForIMAPCommandResponse(kContinuationStr, this->commandResponseStr);
    this->waitForIMAPCommandResponse(kUntaggedStr, responseStr);

    if (!responseStr.empty()) {
        this->sendIMAPCommand(static_cast<std::string>(kDONEStr) + kEOLStr);
        this->waitForIMAPCommandResponse(this->currentTagStr, this->commandResponseStr);
    } else {
        throw CMailIMAP::Exception("Server Disconnect without BYE.");
    }

    responseStr += this->commandResponseStr;
    this->commandResponseStr = responseStr;


}

//
// Send APPPEND command (requires a special handler). The command up to  including the octet string
// size has a "\r\n" appended and is sent. It then waits for a "+' where upon it sends the rest of the
// octet string and the waits for the final APPEND response.
//

void CMailIMAP::sendCommandAPPEND(const std::string& commandLineStr) {

    this->sendIMAPCommand(commandLineStr.substr(0, commandLineStr.find_first_of('}') + 1) + kEOLStr);
    this->waitForIMAPCommandResponse(kContinuationStr, this->commandResponseStr);

    this->sendIMAPCommand(commandLineStr.substr(commandLineStr.find_first_of('}') + 1));
    this->waitForIMAPCommandResponse(this->currentTagStr, this->commandResponseStr);

}

// ==============
// PUBLIC METHODS
// ==============

//
// Set IMAP server URL
// 

void CMailIMAP::setServer(const std::string& serverURLStr) {

    this->serverURLStr = serverURLStr;

}

//
// Get IMAP server URL
// 

std::string CMailIMAP::getServer(void) const {

    return (this->serverURLStr);

}

//
// Set email account details
//

void CMailIMAP::setUserAndPassword(const std::string& userNameStr, const std::string& userPasswordStr) {

    this->userNameStr = userNameStr;
    this->userPasswordStr = userPasswordStr;

}

//
// Get email account user details
//

std::string CMailIMAP::getUser(void) const {

    return(userNameStr);

}

//
// Get current connection status with server
//

bool CMailIMAP::getConnectedStatus(void) const {

    return (this->bConnected);

}

//
// Setup connection to server
//

void CMailIMAP::connect(void) {

    if (this->bConnected) {
        CMailIMAP::Exception("Already connected to a server.");
    }

    if (this->curlHandle) {

        curl_easy_setopt(this->curlHandle, CURLOPT_USERNAME, this->userNameStr.c_str());
        curl_easy_setopt(this->curlHandle, CURLOPT_PASSWORD, this->userPasswordStr.c_str());

        curl_easy_setopt(this->curlHandle, CURLOPT_VERBOSE, CMailIMAP::bCurlVerbosity);
        curl_easy_setopt(this->curlHandle, CURLOPT_URL, this->serverURLStr.c_str());

        curl_easy_setopt(this->curlHandle, CURLOPT_USE_SSL, (long) CURLUSESSL_ALL);
        curl_easy_setopt(this->curlHandle, CURLOPT_ERRORBUFFER, this->curlErrMsgBuffer);

        curl_easy_setopt(this->curlHandle, CURLOPT_CONNECT_ONLY, 1L);
        curl_easy_setopt(this->curlHandle, CURLOPT_MAXCONNECTS, 1L);

        this->curlErrMsgBuffer[0] = 0;
        this->curlResult = curl_easy_perform(this->curlHandle);
        if (this->curlResult != CURLE_OK) {
            throwCurlError("curl_easy_perform(): ");
        }

        // Get curl socket using CURLINFO_ACTIVESOCKET first then depreciated CURLINFO_LASTSOCKET

        this->curlErrMsgBuffer[0] = 0;
        this->curlResult = curl_easy_getinfo(this->curlHandle, CURLINFO_ACTIVESOCKET, &this->curlSocketFD);
        if (this->curlResult == CURLE_BAD_FUNCTION_ARGUMENT) {
            this->curlErrMsgBuffer[0] = 0;
            this->curlResult = curl_easy_getinfo(this->curlHandle, CURLINFO_LASTSOCKET, &this->curlSocketFD);
        }
        if (this->curlResult != CURLE_OK) {
            throwCurlError("Could not get curl socket.");
        }

        this->bConnected = true;

    }


}

//
// Disconnect from server
//

void CMailIMAP::disconnect(void) {

    if (!this->bConnected) {
        throw CMailIMAP::Exception("Not connected to server.");
    }

    if (this->curlHandle) {
        curl_easy_cleanup(this->curlHandle);
        this->curlHandle = nullptr;
        this->tagCount = 1;
        this->bConnected = false;
    }

}

//
// Send single IMAP command and return response including tagged command line.
//

std::string CMailIMAP::sendCommand(const std::string& commandLineStr) {

    if (!this->bConnected) {
        throw CMailIMAP::Exception("Not connected to server.");
    }

    this->generateTag();

    if (commandLineStr.compare(kIDLEStr) == 0) {
        sendCommandIDLE(this->currentTagStr + " " + commandLineStr + kEOLStr);
    } else if (commandLineStr.compare(kAPPENDStr) == 0) {
        sendCommandAPPEND(this->currentTagStr + " " + commandLineStr);
    } else {
        this->sendIMAPCommand(this->currentTagStr + " " + commandLineStr + kEOLStr);
        this->waitForIMAPCommandResponse(this->currentTagStr, this->commandResponseStr);
    }

    return (this->currentTagStr + " " + commandLineStr + kEOLStr + this->commandResponseStr);

}

//
// Main CMailIMAP object constructor. 
//

CMailIMAP::CMailIMAP() {

    this->curlHandle = curl_easy_init();

}

//
// CMailIMAP Destructor
//

CMailIMAP::~CMailIMAP() {

    if (this->curlHandle) {
        curl_easy_cleanup(this->curlHandle);
    }

}

//
// CMailIMAP initialization.
//

void CMailIMAP::init(bool bCurlVerbosity) {

    //
    //  CMailIMAP initialization.. Globally init curl.
    //

    if (curl_global_init(CURL_GLOBAL_ALL)) {
        throw CMailIMAP::Exception("curl_global_init() : could not initialize libcurl.");
    }

    CMailIMAP::bCurlVerbosity = bCurlVerbosity;

}

//
// CMailIMAP closedown
//

void CMailIMAP::closedown(void) {

    curl_global_cleanup();

}
