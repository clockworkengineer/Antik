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

// ===========================
// PRIVATE TYPES AND CONSTANTS
// ===========================

// ==========================
// PUBLIC TYPES AND CONSTANTS
// ==========================

//
// End Of Line terminator
//

const std::string CMailIMAP::kEOLStr("\r\n");

//
// IMAP Command strings
//

const std::string CMailIMAP::kSTARTTLSStr("STARTTLS");
const std::string CMailIMAP::kAUTHENTICATEStr{"AUTHENTICATE"};
const std::string CMailIMAP::kSEARCHStr("SEARCH");
const std::string CMailIMAP::kSELECTStr("SELECT");
const std::string CMailIMAP::kEXAMINEStr("EXAMINE");
const std::string CMailIMAP::kCREATEStr("CREATE");
const std::string CMailIMAP::kDELETEStr("DELETE");
const std::string CMailIMAP::kRENAMEStr("RENAME");
const std::string CMailIMAP::kLOGINStr("LOGIN");
const std::string CMailIMAP::kSUBSCRIBEStr("SUBSCRIBE");
const std::string CMailIMAP::kUNSUBSCRIBEStr("UNSUBSCRIBE");
const std::string CMailIMAP::kLISTStr("LIST");
const std::string CMailIMAP::kLSUBStr("LSUB");
const std::string CMailIMAP::kSTATUSStr("STATUS");
const std::string CMailIMAP::kAPPENDStr("APPEND");
const std::string CMailIMAP::kCHECKStr("CHECK");
const std::string CMailIMAP::kCLOSEStr("CLOSE");
const std::string CMailIMAP::kEXPUNGEStr("EXPUNGE");
const std::string CMailIMAP::kFETCHStr("FETCH");
const std::string CMailIMAP::kSTOREStr("STORE");
const std::string CMailIMAP::kCOPYStr("COPY");
const std::string CMailIMAP::kNOOPStr("NOOP");
const std::string CMailIMAP::kLOGOUTStr("LOGOUT");
const std::string CMailIMAP::kIDLEStr("IDLE");
const std::string CMailIMAP::kCAPABILITYStr("CAPABILITY");
const std::string CMailIMAP::kUIDStr("UID");

//
// IMAP Response strings
//

const std::string CMailIMAP::kUntaggedStr("*");
const std::string CMailIMAP::kOKStr("OK");
const std::string CMailIMAP::kBADStr("BAD");
const std::string CMailIMAP::kNOStr("NO");
const std::string CMailIMAP::kFLAGSStr("FLAGS");
const std::string CMailIMAP::kPERMANENTFLAGSStr("PERMANENTFLAGS");
const std::string CMailIMAP::kUIDVALIDITYStr("UIDVALIDITY");
const std::string CMailIMAP::kUIDNEXTStr("UIDNEXT");
const std::string CMailIMAP::kHIGHESTMODSEQStr("HIGHESTMODSEQ");
const std::string CMailIMAP::kUNSEENStr("UNSEEN");
const std::string CMailIMAP::kEXISTSStr("EXISTS");
const std::string CMailIMAP::kRECENTStr("RECENT");
const std::string CMailIMAP::kDONEStr("DONE");
const std::string CMailIMAP::kContinuationStr("+");
const std::string CMailIMAP::kENVELOPEStr("ENVELOPE");
const std::string CMailIMAP::kBODYSTRUCTUREStr("BODYSTRUCTURE");
const std::string CMailIMAP::kBODYStr("BODY");
const std::string CMailIMAP::kRFC822Str("RFC822");
const std::string CMailIMAP::kINTERNALDATEStr("INTERNALDATE");
const std::string CMailIMAP::kRFC822HEADERStr("RFC822.HEADER");
const std::string CMailIMAP::kRFC822SIZEStr("RFC822.SIZE");
const std::string CMailIMAP::kRFC822TEXTStr("RFC822.TEXT");
const std::string CMailIMAP::kBYEStr("BYE");


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
// Send IMAP command to server. The maximum buffer size is CURL_MAX_WRITE_SIZE
// so split up message into chunks before sending.
//

void CMailIMAP::sendIMAPCommand(const std::string& commandStr) {

    size_t len = 0;
    int bytesCopied = 0;

    do {
        this->errMsgBuffer[0] = 0;
        this->res = curl_easy_send(this->curl, &commandStr[bytesCopied], std::min((static_cast<int> (commandStr.length()) - bytesCopied), CURL_MAX_WRITE_SIZE), &len);
        if (this->res == CURLE_AGAIN) {
            continue;
        } else if (this->res != CURLE_OK) {
            std::string errMsg;
            if (std::strlen(this->errMsgBuffer) != 0) {
                errMsg = this->errMsgBuffer;
            } else {
                errMsg = curl_easy_strerror(res);
            }
            throw CMailIMAP::Exception("curl_easy_send() failed: " + errMsg);
        }
        bytesCopied += len;
    } while ((bytesCopied < commandStr.length()));

}

//
// Wait for reply from sent IMAP command. Keep filling buffer until the commandTag is found and
// we have a full line. Also if we run out of buffer space then append current buffer to response
// and start at front of rxBuffer. Note: Any old response left over is cleared. Also if connection
// closed by server then CURLE_UNSUPPORTED_PROTOCOL returned so append current response and return.
//

void CMailIMAP::waitForIMAPCommandResponse(const std::string& commandTag, std::string& commandResponseStr) {

    std::string searchTag{ commandTag + " "};
    size_t len = 0;
    size_t currPos = 0;
    char *tagptr;

    commandResponseStr.clear();

    do {

        this->errMsgBuffer[0] = 0;
        this->res = curl_easy_recv(this->curl, &this->rxBuffer[currPos], sizeof (this->rxBuffer) - currPos, &len);

        if (this->res == CURLE_OK) {

            this->rxBuffer[currPos + len] = '\0';
            if ((tagptr = strstr(this->rxBuffer, searchTag.c_str())) != nullptr) {
                if ((this->rxBuffer[currPos + len - 2] == '\r') &&
                        (this->rxBuffer[currPos + len - 1] == '\n')) {
                    commandResponseStr.append(this->rxBuffer);
                    break;
                }
            // !!! This should be the proper exit for connection closed by server !!!.
            } else if (len == 0) {
                commandResponseStr.append(this->rxBuffer);
                break;

            }

            currPos += len;
        
        // !!! Connection closed by server !!!.          
        } else if (this->res == CURLE_UNSUPPORTED_PROTOCOL) {
            commandResponseStr.append(this->rxBuffer);
            break;

        } else if (this->res != CURLE_AGAIN) {
            std::string errMsg;

            if (std::strlen(this->errMsgBuffer) != 0) {
                errMsg = this->errMsgBuffer;
            } else {
                errMsg = curl_easy_strerror(this->res);
            }
            throw CMailIMAP::Exception("curl_easy_recv() failed: " + errMsg);
        }

        if (((sizeof (this->rxBuffer) - currPos)) == 0) {
            commandResponseStr.append(this->rxBuffer);
            currPos = 0;
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
// un-tagged response before "DONE" sent is saved and tagged onto the front of
// the final IDLE response.
//

void CMailIMAP::sendCommandIDLE(const std::string& commandLineStr) {

    std::string responseStr;

    this->sendIMAPCommand(commandLineStr);
    this->waitForIMAPCommandResponse(kContinuationStr, this->commandResponseStr);

    this->waitForIMAPCommandResponse(kUntaggedStr, responseStr);

    this->sendIMAPCommand(kDONEStr + kEOLStr);

    this->waitForIMAPCommandResponse(this->currentTagStr, this->commandResponseStr);

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
// Set email account details
//

void CMailIMAP::setUserAndPassword(const std::string& userNameStr, const std::string& userPasswordStr) {

    this->userNameStr = userNameStr;
    this->userPasswordStr = userPasswordStr;

}

//
// Setup connection to server
//

void CMailIMAP::connect(void) {

    if (this->bConnected) {
        CMailIMAP::Exception("Already connected to a server.");
    }

    if (this->curl) {

        curl_easy_setopt(this->curl, CURLOPT_USERNAME, this->userNameStr.c_str());
        curl_easy_setopt(this->curl, CURLOPT_PASSWORD, this->userPasswordStr.c_str());

        curl_easy_setopt(this->curl, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(this->curl, CURLOPT_URL, this->serverURLStr.c_str());

        curl_easy_setopt(this->curl, CURLOPT_USE_SSL, (long) CURLUSESSL_ALL);
        curl_easy_setopt(this->curl, CURLOPT_ERRORBUFFER, this->errMsgBuffer);

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
            throw CMailIMAP::Exception("curl_easy_perform() failed: " + errMsg);
        }

        this->bConnected = true;

    }

}

//
// Disconnect from server
//

void CMailIMAP::disconnect() {

    if (!this->bConnected) {
        throw CMailIMAP::Exception("Not connected to server.");
    }

    if (this->curl) {
        curl_easy_cleanup(this->curl);
        this->curl = nullptr;
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

    if (commandLineStr.compare(kIDLEStr)==0) {
        sendCommandIDLE(this->currentTagStr + " " + commandLineStr + kEOLStr);
    } else if (commandLineStr.compare(kAPPENDStr)==0) {
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
// CMailIMAP initialization.
//

void CMailIMAP::init(void) {

    //
    // Initialize curl
    //

    if (curl_global_init(CURL_GLOBAL_ALL)) {
        throw CMailIMAP::Exception("curl_global_init() : failure to initialize libcurl.");
    }

}

//
// CMailIMAP closedown
//

void CMailIMAP::closedown(void) {

    curl_global_cleanup();

}

