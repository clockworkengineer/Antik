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

// ==========================
// PUBLIC TYPES AND CONSTANTS
// ==========================

//
// Line terminator
//

const std::string CMailIMAP::kEOLStr("\r\n");

//
// Command string constants
//

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

        CMailIMAP::BASERESPONSE noopResp;

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

        noopResp = this->sendCommand(CMailIMAP::kNOOPStr);

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
// Send single IMAP command and decode/return response.
//

CMailIMAP::BASERESPONSE CMailIMAP::sendCommand(const std::string& commandLine) {

    this->commandResponse.clear();
    this->commandHeader.clear();

    curl_easy_setopt(this->curl, CURLOPT_CUSTOMREQUEST, commandLine.c_str());

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

    return (CMailIMAP::decodeResponse(commandLine, this->commandHeader));

}

//
// Extract the contents between to delimeters
//

std::string CMailIMAP::contentsBetween(const std::string& line, const char first, const char last) {
    return (line.substr(line.find_first_of(first) + 1, line.find_first_of(last) - line.find_first_of(first) - 1));
}

//
// Extract number between two characters (ie. number and spaces)
//

std::string CMailIMAP::cutOut(const std::string& line, const char seperator) {
    return (line.substr(line.find_first_of(seperator) + 1, line.find_last_of(seperator) - line.find_first_of(seperator) - 1));
}

//
// Decode SELECT/EXAMINE Response
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeSELECT(const std::string& commandLine, std::istringstream& responseStream) {

    CMailIMAP::SELECTRESPONSE resp(new CMailIMAP::SelectResponse);
    std::string responseValue;

    resp->command = commandLine.substr(0, commandLine.find_first_of(' '));
    resp->mailBoxName = commandLine.substr(commandLine.find_last_of(' ') + 1);
    resp->status = CMailIMAP::RespCode::OK;

    if (resp->mailBoxName.back() == '\"') resp->mailBoxName.pop_back();
    if (resp->mailBoxName.front() == '\"') resp->mailBoxName = resp->mailBoxName.substr(1);

    for (std::string line; std::getline(responseStream, line, '\n');) {

        line.pop_back();

        if (line.find("* FLAGS") == 0) {
            responseValue = contentsBetween(line, '(', ')');
            resp->responseMap.insert({"FLAGS", responseValue});

        } else if (line.find("* OK [PERMANENTFLAGS") == 0) {
            responseValue = contentsBetween(line, '(', ')');
            resp->responseMap.insert({"PERMANENTFLAGS", responseValue});

        } else if (line.find("* OK [UIDVALIDITY") == 0) {
            responseValue = contentsBetween(line, '[', ']');
            responseValue = responseValue.substr(responseValue.find_first_of(' ') + 1);
            resp->responseMap.insert({"UIDVALIDITY", responseValue});

        } else if (line.find("* OK [UIDNEXT") == 0) {
            responseValue = contentsBetween(line, '[', ']');
            responseValue = responseValue.substr(responseValue.find_first_of(' ') + 1);
            resp->responseMap.insert({"UIDNEXT", responseValue});

        } else if (line.find("* OK [HIGHESTMODSEQ") == 0) {
            responseValue = contentsBetween(line, '[', ']');
            responseValue = responseValue.substr(responseValue.find_first_of(' ') + 1);
            resp->responseMap.insert({"HIGHESTMODSEQ", responseValue});

        } else if (line.find("* OK [UNSEEN") == 0) {
            responseValue = contentsBetween(line, '[', ']');
            responseValue = responseValue.substr(responseValue.find_first_of(' ') + 1);
            resp->responseMap.insert({"UNSEEN", responseValue});

        } else if (line.find("EXISTS") != std::string::npos) {
            responseValue = cutOut(line, ' ');
            resp->responseMap.insert({"EXISTS", responseValue});

        } else if (line.find("RECENT") != std::string::npos) {
            responseValue = cutOut(line, ' ');
            resp->responseMap.insert({"RECENT", responseValue});

        } else if (line.find("* CAPABILITY") == 0) {
            responseValue = line.substr((std::string("* CAPABILITY").length()) + 1);
            resp->responseMap.insert({"CAPABILITY", responseValue});

        } else if ((line.find("] " + resp->mailBoxName) != std::string::npos) ||
                (line.find("] " + resp->command + " completed.") != std::string::npos)) {
            resp->mailBoxAccess = contentsBetween(line, '[', ']');

        } else {
            std::cerr << "UKNOWN RESPONSE TYPE = [" << line << "]" << std::endl;
        }

    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));

}

//
// Decode SEARCH Response
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeSEARCH(const std::string& commandLine, std::istringstream& responseStream) {

    CMailIMAP::SEARCHRESPONSE resp(new CMailIMAP::SearchResponse);
    uint64_t index;

    resp->command = commandLine.substr(0, commandLine.find_first_of(' '));
    resp->status = CMailIMAP::RespCode::OK;

    for (std::string line; std::getline(responseStream, line, '\n');) {

        line.pop_back();

        if (line.find("* " + resp->command) == 0) {

            line = line.substr(line.find_first_of(' ') + 1);
            line = line.substr(line.find_first_of(' ') + 1);

            std::istringstream listStream(line);
            while (listStream.good()) {
                listStream >> index;
                resp->indexes.push_back(index);
            }

        } else {
            std::cerr << "UKNOWN RESPONSE TYPE = [" << line << "]" << std::endl;
        }


    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));

}

//
// Decode LIST/LSUB Response
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeLIST(const std::string& commandLine, std::istringstream& responseStream) {

    CMailIMAP::LISTRESPONSE resp(new CMailIMAP::ListResponse);
    CMailIMAP::ListRespData mailBoxEntry;

    resp->command = commandLine.substr(0, commandLine.find_first_of(' '));
    resp->status = CMailIMAP::RespCode::OK;

    for (std::string line; std::getline(responseStream, line, '\n');) {

        line.pop_back();

        if (line.find("* " + resp->command) != std::string::npos) {
            mailBoxEntry.attributes = contentsBetween(line, '(', ')');
            mailBoxEntry.hierDel = contentsBetween(line, '\"', '\"').front();
            if (line.back() != '\"') {
                mailBoxEntry.name = line.substr(line.find_last_of(' '));
            } else {
                line.pop_back();
                mailBoxEntry.name = line.substr(line.find_last_of('\"'));
                mailBoxEntry.name += '\"';
            }

            resp->mailBoxList.push_back(mailBoxEntry);

        } else {
            std::cerr << "UKNOWN RESPONSE TYPE = [" << line << "]" << std::endl;
        }
    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));


}

//
// Decode STATUS Response
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeSTATUS(const std::string& commandLine, std::istringstream& responseStream) {

    CMailIMAP::STATUSRESPONSE resp(new CMailIMAP::StatusResponse);
    std::string line;

    resp->command = commandLine.substr(0, commandLine.find_first_of(' '));
    resp->status = CMailIMAP::RespCode::OK;

    for (std::string line; std::getline(responseStream, line, '\n');) {

        line.pop_back();

        if (line.find("* " + resp->command) == 0) {

            line = line.substr(line.find_first_of(' ') + 1);
            line = line.substr(line.find_first_of(' ') + 1);
            resp->mailBoxName = line.substr(0, line.find_first_of(' '));

            line = contentsBetween(line, '(', ')');

            std::istringstream listStream(line);
            std::string item, value;

            while (listStream.good()) {
                listStream >> item >> value;
                resp->responseMap.insert({item, value});
            }

        } else {
            std::cerr << "UKNOWN RESPONSE TYPE = [" << line << "]" << std::endl;
        }

    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));

}

//
// Decode EXPUNGE Response
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeEXPUNGE(const std::string& commandLine, std::istringstream& responseStream) {

    CMailIMAP::EXPUNGERESPONSE resp(new CMailIMAP::ExpungeResponse);
    std::string line;

    resp->command = commandLine.substr(0, commandLine.find_first_of(' '));
    resp->status = CMailIMAP::RespCode::OK;

    for (std::string line; std::getline(responseStream, line, '\n');) {

        line.pop_back();

        if (line.find("EXISTS") != std::string::npos) {
            line = cutOut(line, ' ');
            resp->exists.push_back(std::strtoull(line.c_str(), nullptr, 10));
        } else if (line.find("EXPUNGE") != std::string::npos) {
            line = cutOut(line, ' ');
            resp->expunged.push_back(std::strtoull(line.c_str(), nullptr, 10));
        } else {
            std::cerr << "UKNOWN RESPONSE TYPE = [" << line << "]" << std::endl;
        }

    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));


}

//
// Decode STORE Response
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeSTORE(const std::string& commandLine, std::istringstream& responseStream) {

    CMailIMAP::STORERESPONSE resp(new CMailIMAP::StoreResponse);
    StoreRespData storeData;
    std::string line;

    resp->command = commandLine.substr(0, commandLine.find_first_of(' '));
    resp->status = CMailIMAP::RespCode::OK;

    for (std::string line; std::getline(responseStream, line, '\n');) {

        line.pop_back();

        if (line.find("FETCH") != std::string::npos) {
            storeData.flags = contentsBetween(line.substr(line.find_first_of('(') + 1), '(', ')');
            storeData.index = std::strtoull(cutOut(line, ' ').c_str(), nullptr, 10);
            resp->storeList.push_back(storeData);
        } else {
            std::cerr << "UKNOWN RESPONSE TYPE = [" << line << "]" << std::endl;
        }

    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));


}

//
// Decode Command Response
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeResponse(const std::string& commandLine, const std::string & commandResponse) {

    std::istringstream responseStream(commandResponse);
    std::istringstream commandStream(commandLine);
    std::string command = commandLine.substr(0, commandLine.find_first_of(' '));

    if ((command.compare(CMailIMAP::kLISTStr) == 0) || (command.compare(CMailIMAP::kLSUBStr) == 0)) {
        return (CMailIMAP::decodeLIST(commandLine, responseStream));
    }

    if (command.compare(CMailIMAP::kSEARCHStr) == 0) {
        return (CMailIMAP::decodeSEARCH(commandLine, responseStream));
    }

    if ((command.compare(CMailIMAP::kSELECTStr) == 0) || (commandLine.compare(CMailIMAP::kEXAMINEStr) == 0)) {
        return (CMailIMAP::decodeSELECT(commandLine, responseStream));
    }

    if (command.compare(CMailIMAP::kSTATUSStr) == 0) {
        return (CMailIMAP::decodeSTATUS(commandLine, responseStream));
    }

    if (command.compare(CMailIMAP::kEXPUNGEStr) == 0) {
        return (CMailIMAP::decodeEXPUNGE(commandLine, responseStream));
    }

    if (command.compare(CMailIMAP::kSTOREStr) == 0) {
        return (CMailIMAP::decodeSTORE(commandLine, responseStream));
    }


    return (static_cast<CMailIMAP::BASERESPONSE> (new CMailIMAP::BResponse{command, RespCode::OK}));

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

