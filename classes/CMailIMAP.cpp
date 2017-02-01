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
const std::string CMailIMAP::kDONEStr("DONE");
const std::string CMailIMAP::kContinuationStr("+");
const std::string CMailIMAP::kENVELOPEStr("ENVELOPE");

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
const std::string CMailIMAP::kCAPABILITYStr("CAPABILITY");


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
// we have a full line. Also if we run out of buffer space then append current buffer to response
// and start at front of rxBuffer.
//

void CMailIMAP::waitForCommandResponse(const std::string& commandTag, std::string& commandResponse) {

    size_t len = 0;
    size_t currPos = 0;
    char *tagptr;

    commandResponse.clear();

    do {

        this->errMsgBuffer[0];
        this->res = curl_easy_recv(this->curl, &this->rxBuffer[currPos], sizeof (this->rxBuffer) - currPos, &len);

        if (this->res == CURLE_OK) {

            this->rxBuffer[currPos + len] = '\0';

            if ((tagptr = strstr(this->rxBuffer, commandTag.c_str())) != nullptr) {
                if ((this->rxBuffer[currPos + len - 2] == '\r') &&
                        (this->rxBuffer[currPos + len - 1] == '\n')) {
                    commandResponse.append(this->rxBuffer);
                    break;
                }
            } else if (len == 0) {
                commandResponse.append(this->rxBuffer);
                currPos = 0;
                continue;
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
// Generate next command tag
//

inline void CMailIMAP::generateTag() {
    this->currentTag = "A" + std::to_string(this->tagCount++);
}


//
// Extract the contents between to delimeters
//

inline std::string CMailIMAP::contentsBetween(const std::string& line, const char first, const char last) {
    return (line.substr(line.find_first_of(first) + 1, line.find_first_of(last) - line.find_first_of(first) - 1));
}

//
// Extract number between two characters (ie. number and spaces)
//

inline std::string CMailIMAP::extractNumber(const std::string& line, const char seperator) {
    
    return (line.substr(line.find_first_of(seperator) + 1, line.find_last_of(seperator) - line.find_first_of(seperator) - 1));
    
}

//
// Extract tag from command line
//

inline std::string CMailIMAP::extractTag(const std::string& line) {
    return (line.substr(0, line.find_first_of(' ')));
}

//
// Extract command string from command line
//

inline std::string CMailIMAP::extractCommand(const std::string& line) {
    
   std::string command;
   command = line.substr(line.find_first_of(' ') + 1);
   return(command.substr(0, command.find_first_of(' ')));
   
}

//
// Decode command response status
//

void CMailIMAP::decodeStatus(const std::string& tag, const std::string& line, CMailIMAP::BASERESPONSE resp) {

    if (line.find(tag + " " + kOKStr) == 0) {
        resp->status = RespCode::OK;

    } else if (line.find(tag + " " + kNOStr) == 0) {
        resp->status = RespCode::NO;
        resp->errorMessage = line;

    } else if (line.find(tag + " " + kBADStr) == 0) {
        resp->status = RespCode::BAD;
        resp->errorMessage = line;
        
    } else if ((line.find(kUntaggedStr + " " + kNOStr) == 0) 
            || (line.find(kUntaggedStr + " " + kBADStr) == 0)) {
            std::cerr << line << std::endl;
        
    } else {
        std::cerr << "UKNOWN RESPONSE TYPE = [" << line << "]" << std::endl;
    }


}

//
// Decode SELECT/EXAMINE Response
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeSELECT(CMailIMAP::CommandData& commandData, std::istringstream& responseStream) {

    CMailIMAP::SELECTRESPONSE resp { new CMailIMAP::SelectResponse };

    resp->command = commandData.command;
    
    resp->mailBoxName = commandData.commandLine.substr(commandData.commandLine.find_last_of(' ') + 1);
    if (resp->mailBoxName.back() == '\"') resp->mailBoxName.pop_back();
    if (resp->mailBoxName.front() == '\"') resp->mailBoxName = resp->mailBoxName.substr(1);

    for (std::string line; std::getline(responseStream, line, '\n');) {
            
        line.pop_back();

        if (line.find(kUntaggedStr + " " + kOKStr + " [") == 0){
            line = contentsBetween(line, '[', ']');
        }
        
        if (line.find(kUntaggedStr + " " + kFLAGSStr) == 0) {
            resp->responseMap.insert({kFLAGSStr, contentsBetween(line, '(', ')')});

        } else if (line.find(kPERMANENTFLAGSStr) == 0) {
            resp->responseMap.insert({kPERMANENTFLAGSStr, contentsBetween(line, '(', ')')});

        } else if (line.find(kUIDVALIDITYStr) == 0) {
            resp->responseMap.insert({kUIDVALIDITYStr, line.substr(line.find_first_of(' ') + 1)});

        } else if (line.find(kUIDNEXTStr) == 0) {
            resp->responseMap.insert({kUIDNEXTStr, line.substr(line.find_first_of(' ') + 1)});

        } else if (line.find(kHIGHESTMODSEQStr) == 0) {
            resp->responseMap.insert({kHIGHESTMODSEQStr, line.substr(line.find_first_of(' ') + 1)});

        } else if (line.find(kUNSEENStr) == 0) {
            resp->responseMap.insert({kUNSEENStr, line.substr(line.find_first_of(' ') + 1)});

        } else if (line.find(kEXISTSStr) != std::string::npos) {
            resp->responseMap.insert({kEXISTSStr, extractNumber(line, ' ')});

        } else if (line.find(kRECENTStr) != std::string::npos) {
            resp->responseMap.insert({kRECENTStr, extractNumber(line, ' ')});

        } else if (line.find(kUntaggedStr + " " + kCAPABILITYStr) == 0) {
            line = line.substr((std::string(kUntaggedStr + " " + kCAPABILITYStr).length()) + 1);
            resp->responseMap.insert({kCAPABILITYStr, line});

        } else if ((line.find("] " + resp->mailBoxName) != std::string::npos) ||
                (line.find("] " + resp->command + " completed.") != std::string::npos)) {
            resp->mailBoxAccess = contentsBetween(line, '[', ']');
        } else {
            decodeStatus(commandData.tag, line, resp);
        }          
        
        
    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));

}

//
// Decode SEARCH Response
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeSEARCH(CMailIMAP::CommandData& commandData, std::istringstream& responseStream) {

    CMailIMAP::SEARCHRESPONSE resp { new CMailIMAP::SearchResponse };

    resp->command = commandData.command;

    for (std::string line; std::getline(responseStream, line, '\n');) {

        line.pop_back();

        if (line.find(kUntaggedStr + " " + resp->command) == 0) {

            line = line.substr(line.find_first_of(' ') + 1);
            line = line.substr(line.find_first_of(' ') + 1);

            std::istringstream listStream(line);
            while (listStream.good()) {
                uint64_t index;
                listStream >> index;
                resp->indexes.push_back(index);
            }

        } else {
            decodeStatus(commandData.tag, line, resp);
        }      

    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));

}

//
// Decode LIST/LSUB Response
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeLIST(CMailIMAP::CommandData& commandData, std::istringstream& responseStream) {

    CMailIMAP::LISTRESPONSE resp { new CMailIMAP::ListResponse };

    resp->command = commandData.command;

    for (std::string line; std::getline(responseStream, line, '\n');) {

        CMailIMAP::ListRespData mailBoxEntry;

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

        } else {
            decodeStatus(commandData.tag, line, resp);
        }      
        
    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));


}

//
// Decode STATUS Response
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeSTATUS(CMailIMAP::CommandData& commandData, std::istringstream& responseStream) {

    CMailIMAP::STATUSRESPONSE resp { new CMailIMAP::StatusResponse };

    resp->command = commandData.command;

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

        } else {
            decodeStatus(commandData.tag, line, resp);
        }      

    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));

}

//
// Decode EXPUNGE Response
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeEXPUNGE(CMailIMAP::CommandData& commandData, std::istringstream& responseStream) {

    CMailIMAP::EXPUNGERESPONSE resp { new CMailIMAP::ExpungeResponse };

    resp->command = commandData.command;

    for (std::string line; std::getline(responseStream, line, '\n');) {

        line.pop_back();

        if (line.find(kEXISTSStr) != std::string::npos) {
            line = extractNumber(line, ' ');
            resp->exists.push_back(std::strtoull(line.c_str(), nullptr, 10));

        } else if (line.find(kEXPUNGEStr) != std::string::npos) {
            line = extractNumber(line, ' ');
            resp->expunged.push_back(std::strtoull(line.c_str(), nullptr, 10));

        } else {
            decodeStatus(commandData.tag, line, resp);
        }      

    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));

}

//
// Decode STORE Response
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeSTORE(CMailIMAP::CommandData& commandData, std::istringstream& responseStream) {

    CMailIMAP::STORERESPONSE resp { new CMailIMAP::StoreResponse };

    resp->command = commandData.command;

    for (std::string line; std::getline(responseStream, line, '\n');) {

        StoreRespData storeData;
            
        line.pop_back();

        if (line.find(kFETCHStr) != std::string::npos) {
            storeData.flags = contentsBetween(line.substr(line.find_first_of('(') + 1), '(', ')');
            storeData.index = std::strtoull(extractNumber(line, ' ').c_str(), nullptr, 10);
            resp->storeList.push_back(storeData);

        } else {
            decodeStatus(commandData.tag, line, resp);
        }      

    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));

}

//
// Decode CAPABILITY Response
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeCAPABILITY(CMailIMAP::CommandData& commandData, std::istringstream& responseStream) {

    CMailIMAP::CAPABILITYRESPONSE resp { new CMailIMAP::CapabilityResponse };

    resp->command = commandData.command;

    for (std::string line; std::getline(responseStream, line, '\n');) {

        line.pop_back();

        if (line.find(kUntaggedStr + " " + kCAPABILITYStr) == 0) {
            line = line.substr(line.find_first_of(' ') + 1);
            resp->capabilityList = line.substr(line.find_first_of(' ') + 1);

        } else {
            decodeStatus(commandData.tag, line, resp);
        }      

    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));

}

//
// Decode FETCH Response
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeFETCH(CMailIMAP::CommandData& commandData, std::istringstream& responseStream) {

    CMailIMAP::FETCHRESPONSE resp { new CMailIMAP::FetchResponse };

    resp->command = commandData.command;

    for (std::string line; std::getline(responseStream, line, '\n');) {

        FetchRespData fetchData;
            
        line.pop_back();

        if (line.find(kFETCHStr) != std::string::npos) {
            int bodyLength = 0;
            fetchData.index = std::strtoull(extractNumber(line, ' ').c_str(), nullptr, 10);
            if (line.find(kENVELOPEStr) != std::string::npos) {
                line = line.substr(line.find_first_of('(' + 1));
                fetchData.envelope = contentsBetween(line, '(', ')');
                fetchData.bodyLength = 0;
            } else {
                if (line.find(kFLAGSStr) != std::string::npos) {
                    fetchData.flags = contentsBetween(line.substr(line.find_first_of('(') + 1), '(', ')');
                }
                bodyLength = fetchData.bodyLength = std::strtoull(contentsBetween(line, '{', '}').c_str(), nullptr, 10);
                if (bodyLength) {
                    for (std::string body; std::getline(responseStream, body, '\n');) {
                        body.push_back('\n');
                        if (bodyLength < body.length()) {
                            body.resize(bodyLength);
                        }
                        fetchData.body.push_back(body);
                        bodyLength -= body.length();
                        if (bodyLength == 0)break;
                    }
               }
            }
            resp->fetchList.push_back(fetchData);

       } else if ((line.find(")") == 0) && line.length()==1) {
           continue;
                    
        } else {
            decodeStatus(commandData.tag, line, resp);
        }      
    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));

}

//
// Default Decode Response
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeDefault(CMailIMAP::CommandData& commandData, std::istringstream& responseStream) {

    CMailIMAP::BASERESPONSE resp { new CMailIMAP::BResponse };

    resp->command = commandData.command;

    for (std::string line; std::getline(responseStream, line, '\n');) {
        line.pop_back();
        decodeStatus(commandData.tag, line, resp);
    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));

}

//
// Decode Command Response
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeResponse(const std::string& commandLine, const std::string & commandResponse) {

    std::istringstream responseStream(commandResponse);
    CMailIMAP::DecodeFunction decodeFn;
    CommandData commandData { extractTag(commandLine) , extractCommand(commandLine), commandLine };
     
    decodeFn = CMailIMAP::decodeCommmandMap[commandData.command];
    if (!decodeFn) {
        decodeFn = decodeDefault;
    }
    
    return (decodeFn(commandData, responseStream));

}

//
// Send IDLE command (requires a special handler).
//

void CMailIMAP::sendCommandIDLE() {

    this->generateTag();

    this->sendCommandDirect(this->currentTag + " " + kIDLEStr + kEOLStr);
    this->waitForCommandResponse(kContinuationStr, this->commandResponse);

    this->waitForCommandResponse(kUntaggedStr, this->commandResponse);

    this->sendCommandDirect(kDONEStr + kEOLStr);

    this->waitForCommandResponse(this->currentTag, this->commandResponse);


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

    if (commandLine.compare(kIDLEStr) == 0) {
        sendCommandIDLE();
    } else {
        this->sendCommandDirect(this->currentTag + " " + commandLine + kEOLStr);
        this->waitForCommandResponse(this->currentTag, this->commandResponse);
                std::cout << this->commandResponse <<std::endl;
    }

    return (CMailIMAP::decodeResponse(this->currentTag + " " + commandLine, this->commandResponse));

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
        throw std::runtime_error(std::string("curl_global_init() : failure to initialize libcurl."));
    }

    //
    // Setup command response decode functions
    //
        
    CMailIMAP::decodeCommmandMap.insert({kLISTStr, decodeLIST});
    CMailIMAP::decodeCommmandMap.insert({kLSUBStr, decodeLIST});
    CMailIMAP::decodeCommmandMap.insert({kSEARCHStr, decodeSEARCH});
    CMailIMAP::decodeCommmandMap.insert({kSELECTStr, decodeSELECT});
    CMailIMAP::decodeCommmandMap.insert({kEXAMINEStr, decodeSELECT});
    CMailIMAP::decodeCommmandMap.insert({kSTATUSStr, decodeSTATUS});
    CMailIMAP::decodeCommmandMap.insert({kEXPUNGEStr, decodeEXPUNGE});
    CMailIMAP::decodeCommmandMap.insert({kSTOREStr, decodeSTORE});
    CMailIMAP::decodeCommmandMap.insert({kCAPABILITYStr, decodeCAPABILITY});
    CMailIMAP::decodeCommmandMap.insert({kFETCHStr, decodeFETCH});

}

//
// CMailIMAP closedown
//

void CMailIMAP::closedown(void) {

    curl_global_cleanup();

}

