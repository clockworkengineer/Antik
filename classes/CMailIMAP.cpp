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

//
// Line terminator
//

const std::string CMailIMAP::kEOLStr("\r\n");

//
// Response strings
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
const std::string CMailIMAP::kCAPABILITYStr("CAPABILITY");
const std::string CMailIMAP::kDONEStr("DONE");
const std::string CMailIMAP::kContinuationStr("+");

// ==========================
// PUBLIC TYPES AND CONSTANTS
// ==========================

//
// Command strings
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

//
// IMAP command to decode response mapping table
//

std::unordered_map<std::string, CMailIMAP::DecodeFunction> CMailIMAP::decodeCommmandMap;

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
// Wait for reply from direct command. Keep filling buffer until the commandTag is found and
// we have a full line.
//

void CMailIMAP::waitForCommandResponse(const std::string& commandTag, std::string& commandResponse) {

    size_t len = 0;
    size_t currPos=0;
    char *tagptr;

    do {
 
        this->errMsgBuffer[0];
        this->res = curl_easy_recv(this->curl, &this->rxBuffer[currPos], sizeof (this->rxBuffer) - currPos, &len);

        if (this->res == CURLE_OK) {

            this->rxBuffer[currPos+len] = '\0';
    
            if ((tagptr = strstr(this->rxBuffer, commandTag.c_str())) != nullptr) {
                if ((this->rxBuffer[currPos+len-2]=='\r') &&
                    (this->rxBuffer[currPos+len-1]=='\n'))  {
                    std::string toCopy(this->rxBuffer);
                    commandResponse = toCopy;
                    break;
                }
            }
            
            currPos += len;

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

//
// Generate next command tag
//

void CMailIMAP::generateTag() {
    this->currentTag = "A" + std::to_string(this->tagCount++);
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
    std::string tag = commandLine.substr(0, commandLine.find_first_of(' '));

    resp->command = commandLine.substr(commandLine.find_first_of(' ') + 1);
    resp->command = resp->command.substr(0, resp->command.find_first_of(' '));

    resp->mailBoxName = commandLine.substr(commandLine.find_last_of(' ') + 1);

    if (resp->mailBoxName.back() == '\"') resp->mailBoxName.pop_back();
    if (resp->mailBoxName.front() == '\"') resp->mailBoxName = resp->mailBoxName.substr(1);

    for (std::string line; std::getline(responseStream, line, '\n');) {

        line.pop_back();

        if (line.find(kUntaggedStr + " " + kFLAGSStr) == 0) {
            responseValue = contentsBetween(line, '(', ')');
            resp->responseMap.insert({kFLAGSStr, responseValue});

        } else if (line.find(kUntaggedStr + " " + kOKStr + " [" + kPERMANENTFLAGSStr) == 0) {
            responseValue = contentsBetween(line, '(', ')');
            resp->responseMap.insert({kPERMANENTFLAGSStr, responseValue});

        } else if (line.find(kUntaggedStr + " " + kOKStr + " [" + kUIDVALIDITYStr) == 0) {
            responseValue = contentsBetween(line, '[', ']');
            responseValue = responseValue.substr(responseValue.find_first_of(' ') + 1);
            resp->responseMap.insert({kUIDVALIDITYStr, responseValue});

        } else if (line.find(kUntaggedStr + " " + kOKStr + " [" + kUIDNEXTStr) == 0) {
            responseValue = contentsBetween(line, '[', ']');
            responseValue = responseValue.substr(responseValue.find_first_of(' ') + 1);
            resp->responseMap.insert({kUIDNEXTStr, responseValue});

        } else if (line.find(kUntaggedStr + " " + kOKStr + " [" + kHIGHESTMODSEQStr) == 0) {
            responseValue = contentsBetween(line, '[', ']');
            responseValue = responseValue.substr(responseValue.find_first_of(' ') + 1);
            resp->responseMap.insert({kHIGHESTMODSEQStr, responseValue});

        } else if (line.find(kUntaggedStr + " " + kOKStr + " [" + kUNSEENStr) == 0) {
            responseValue = contentsBetween(line, '[', ']');
            responseValue = responseValue.substr(responseValue.find_first_of(' ') + 1);
            resp->responseMap.insert({kUNSEENStr, responseValue});

        } else if (line.find(kEXISTSStr) != std::string::npos) {
            responseValue = cutOut(line, ' ');
            resp->responseMap.insert({kEXISTSStr, responseValue});

        } else if (line.find(kRECENTStr) != std::string::npos) {
            responseValue = cutOut(line, ' ');
            resp->responseMap.insert({kRECENTStr, responseValue});

        } else if (line.find(kUntaggedStr + " " + kCAPABILITYStr) == 0) {
            responseValue = line.substr((std::string(kUntaggedStr + " " + kCAPABILITYStr).length()) + 1);
            resp->responseMap.insert({kCAPABILITYStr, responseValue});

        } else if ((line.find("] " + resp->mailBoxName) != std::string::npos) ||
                (line.find("] " + resp->command + " completed.") != std::string::npos)) {
            resp->mailBoxAccess = contentsBetween(line, '[', ']');

        } else if (line.find(tag + " " + kOKStr) == 0) {
            resp->status = RespCode::OK;

        } else if (line.find(tag + " " + kNOStr) == 0) {
            resp->status = RespCode::NO;
            resp->errorMessage = line;
            
        } else if (line.find(tag + " " + kBADStr) == 0) {
            resp->status = RespCode::BAD;
            resp->errorMessage = line;
            
        } else if ((line.find(kUntaggedStr + " " + kNOStr) == 0)|| (line.find(kUntaggedStr + " " + kBADStr) == 0)) {
            std::cerr << line << std::endl;

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
    std::string tag = commandLine.substr(0, commandLine.find_first_of(' '));

    resp->command = commandLine.substr(commandLine.find_first_of(' ') + 1);
    resp->command = resp->command.substr(0, resp->command.find_first_of(' '));

    for (std::string line; std::getline(responseStream, line, '\n');) {

        line.pop_back();

        if (line.find(kUntaggedStr + " " + resp->command) == 0) {

            line = line.substr(line.find_first_of(' ') + 1);
            line = line.substr(line.find_first_of(' ') + 1);

            std::istringstream listStream(line);
            while (listStream.good()) {
                listStream >> index;
                resp->indexes.push_back(index);
            }

        } else if (line.find(tag + " " + kOKStr) == 0) {
            resp->status = RespCode::OK;

        } else if (line.find(tag + " " + kNOStr) == 0) {
            resp->status = RespCode::NO;
            resp->errorMessage = line;
            
        } else if (line.find(tag + " " + kBADStr) == 0) {
            resp->status = RespCode::BAD;
            resp->errorMessage = line;
            
        } else if ((line.find(kUntaggedStr + " " + kNOStr) == 0)|| (line.find(kUntaggedStr + " " + kBADStr) == 0)) {
            std::cerr << line << std::endl;

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
    std::string tag = commandLine.substr(0, commandLine.find_first_of(' '));

    resp->command = commandLine.substr(commandLine.find_first_of(' ') + 1);
    resp->command = resp->command.substr(0, resp->command.find_first_of(' '));

    for (std::string line; std::getline(responseStream, line, '\n');) {

        line.pop_back();

        if (line.find(kUntaggedStr + " " + resp->command) != std::string::npos) {
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

        } else if (line.find(tag + " " + kOKStr) == 0) {
            resp->status = RespCode::OK;

        } else if (line.find(tag + " " + kNOStr) == 0) {
            resp->status = RespCode::NO;
            resp->errorMessage = line;
            
        } else if (line.find(tag + " " + kBADStr) == 0) {
            resp->status = RespCode::BAD;
            resp->errorMessage = line;
            
        } else if ((line.find(kUntaggedStr + " " + kNOStr) == 0)|| (line.find(kUntaggedStr + " " + kBADStr) == 0)) {
            std::cerr << line << std::endl;
 
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
    std::string tag = commandLine.substr(0, commandLine.find_first_of(' '));

    resp->command = commandLine.substr(commandLine.find_first_of(' ') + 1);
    resp->command = resp->command.substr(0, resp->command.find_first_of(' '));

    for (std::string line; std::getline(responseStream, line, '\n');) {

        line.pop_back();

        if (line.find(kUntaggedStr + " " + resp->command) == 0) {

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

        } else if (line.find(tag + " " + kOKStr) == 0) {
            resp->status = RespCode::OK;

        } else if (line.find(tag + " " + kNOStr) == 0) {
            resp->status = RespCode::NO;
            resp->errorMessage = line;
            
        } else if (line.find(tag + " " + kBADStr) == 0) {
            resp->status = RespCode::BAD;
            resp->errorMessage = line;            

        } else if ((line.find(kUntaggedStr + " " + kNOStr) == 0)|| (line.find(kUntaggedStr + " " + kBADStr) == 0)) {
            std::cerr << line << std::endl; 

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
    std::string tag = commandLine.substr(0, commandLine.find_first_of(' '));

    resp->command = commandLine.substr(commandLine.find_first_of(' ') + 1);
    resp->command = resp->command.substr(0, resp->command.find_first_of(' '));

    for (std::string line; std::getline(responseStream, line, '\n');) {

        line.pop_back();

        if (line.find(kEXISTSStr) != std::string::npos) {
            line = cutOut(line, ' ');
            resp->exists.push_back(std::strtoull(line.c_str(), nullptr, 10));

        } else if (line.find(kEXPUNGEStr) != std::string::npos) {
            line = cutOut(line, ' ');
            resp->expunged.push_back(std::strtoull(line.c_str(), nullptr, 10));

        } else if (line.find(tag + " " + kOKStr) == 0) {
            resp->status = RespCode::OK;
            
        } else if (line.find(tag + " " + kNOStr) == 0) {
            resp->status = RespCode::NO;
            resp->errorMessage = line;
            
        } else if (line.find(tag + " " + kBADStr) == 0) {
            resp->status = RespCode::BAD;
            resp->errorMessage = line;
            
        } else if ((line.find(kUntaggedStr + " " + kNOStr) == 0)|| (line.find(kUntaggedStr + " " + kBADStr) == 0)) {
            std::cerr << line << std::endl; 

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
    std::string tag = commandLine.substr(0, commandLine.find_first_of(' '));

    resp->command = commandLine.substr(commandLine.find_first_of(' ') + 1);
    resp->command = resp->command.substr(0, resp->command.find_first_of(' '));

    for (std::string line; std::getline(responseStream, line, '\n');) {

        line.pop_back();

        if (line.find(kFETCHStr) != std::string::npos) {
            storeData.flags = contentsBetween(line.substr(line.find_first_of('(') + 1), '(', ')');
            storeData.index = std::strtoull(cutOut(line, ' ').c_str(), nullptr, 10);
            resp->storeList.push_back(storeData);

        } else if (line.find(tag + " " + kOKStr) == 0) {
            resp->status = RespCode::OK;

        } else if (line.find(tag + " " + kNOStr) == 0) {
            resp->status = RespCode::NO;
            resp->errorMessage = line;
            
        } else if (line.find(tag + " " + kBADStr) == 0) {
            resp->status = RespCode::BAD;
            resp->errorMessage = line;
            
        } else {
            std::cerr << "UKNOWN RESPONSE TYPE = [" << line << "]" << std::endl;
        }

    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));

}

//
// Default Decode Response
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeDefault(const std::string& commandLine, std::istringstream& responseStream) {

    CMailIMAP::BASERESPONSE resp(new CMailIMAP::BResponse);

    std::string tag = commandLine.substr(0, commandLine.find_first_of(' '));

    resp->command = commandLine.substr(commandLine.find_first_of(' ') + 1);
    resp->command = resp->command.substr(0, resp->command.find_first_of(' '));

    for (std::string line; std::getline(responseStream, line, '\n');) {

        line.pop_back();

        if (line.find(tag + " " + kOKStr) == 0) {
            resp->status = RespCode::OK;

        } else if (line.find(tag + " " + kNOStr) == 0) {
            resp->status = RespCode::NO;
            resp->errorMessage = line;
            
        } else if (line.find(tag + " " + kBADStr) == 0) {
            resp->status = RespCode::BAD;
            resp->errorMessage = line;
            
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
    
    CMailIMAP::DecodeFunction decodeFn;
 
    std::string command = commandLine.substr(commandLine.find_first_of(' ') + 1);

    command = command.substr(0, command.find_first_of(' '));
    
    decodeFn = CMailIMAP::decodeCommmandMap[command];
    if (!decodeFn) {
        decodeFn = decodeDefault;
    }
    
    return (decodeFn(commandLine, responseStream));

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

        this->generateTag();
        
        this->sendCommandDirect(this->currentTag + " " + kSELECTStr + " \"" + imapMailBox + "\"" + kEOLStr);
        this->waitForCommandResponse(this->currentTag, this->commandResponse);
        
        this->generateTag();  
        
        this->sendCommandDirect( this->currentTag + " " + kIDLEStr + kEOLStr);
        this->waitForCommandResponse(kContinuationStr, this->commandResponse);
        this->waitForCommandResponse(kUntaggedStr, this->commandResponse);

        
        this->sendCommandDirect(kDONEStr + kEOLStr);

        this->waitForCommandResponse(this->currentTag, this->commandResponse);
 
    }


}

//
// Setup connection to server
//

void CMailIMAP::connect(void) {


    if (this->curl) {

        curl_easy_setopt(this->curl, CURLOPT_USERNAME, this->userName.c_str());
        curl_easy_setopt(this->curl, CURLOPT_PASSWORD, this->userPassword.c_str());

        curl_easy_setopt(this->curl, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(this->curl, CURLOPT_URL, this->serverURL.c_str());

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
            throw std::runtime_error(std::string("curl_easy_perform() failed: ") + errMsg);
        }

    }

}

//
// Disconnect from server
//

void CMailIMAP::disconnect() {
    if (this->curl) {
        curl_easy_cleanup(this->curl);
        this->curl = nullptr;
        this->tagCount = 1;
    }
}

//
// Send single IMAP command and decode/return response.
//

CMailIMAP::BASERESPONSE CMailIMAP::sendCommand(const std::string& commandLine) {

    this->generateTag();

    this->sendCommandDirect( this->currentTag + " " + commandLine + kEOLStr);
    this->waitForCommandResponse( this->currentTag, this->commandResponse);

    return (CMailIMAP::decodeResponse( this->currentTag + " " + commandLine, this->commandResponse));

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

    CMailIMAP::decodeCommmandMap.insert({kLISTStr, decodeLIST});
    CMailIMAP::decodeCommmandMap.insert({kLSUBStr, decodeLIST});
    CMailIMAP::decodeCommmandMap.insert({kSEARCHStr, decodeSEARCH});
    CMailIMAP::decodeCommmandMap.insert({kSELECTStr, decodeSELECT});
    CMailIMAP::decodeCommmandMap.insert({kEXAMINEStr, decodeSELECT});
    CMailIMAP::decodeCommmandMap.insert({kSTATUSStr, decodeSTATUS});
    CMailIMAP::decodeCommmandMap.insert({kEXPUNGEStr, decodeEXPUNGE});
    CMailIMAP::decodeCommmandMap.insert({kSTOREStr, decodeSTORE});
 
}

//
// CMailIMAP closedown
//

void CMailIMAP::closedown(void) {

    curl_global_cleanup();

}

