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
// its functionality. NOTE: This class does not at present cater for the
// fact that any IMAP commands can be upper or lower case and any protocol
// parsing should be case insensitive to  allow for this (ON TODO LIST).
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
const std::string CMailIMAP::kBODYSTRUCTUREStr("BODYSTRUCTURE");
const std::string CMailIMAP::kBODYStr("BODY");
const std::string CMailIMAP::kRFC822Str("RFC822");
const std::string CMailIMAP::kINTERNALDATEStr("INTERNALDATE");
const std::string CMailIMAP::kRFC822HEADERStr("RFC822.HEADER");
const std::string CMailIMAP::kRFC822SIZEStr("RFC822.SIZE");
const std::string CMailIMAP::kRFC822TEXTStr("RFC822.TEXT");
const std::string CMailIMAP::kUIDStr("UID");
const std::string CMailIMAP::kBYEStr("BYE");

// ==========================
// PUBLIC TYPES AND CONSTANTS
// ==========================

//
// Command strings
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

// ========================
// PRIVATE STATIC VARIABLES
// ========================

//
// IMAP command to decode response mapping table
//

std::unordered_map<std::string, CMailIMAP::DecodeFunction> CMailIMAP::decodeCommmandMap = 
{

    {kLISTStr, decodeLIST},
    {kLSUBStr, decodeLIST},
    {kSEARCHStr, decodeSEARCH},
    {kSELECTStr, decodeSELECT},
    {kEXAMINEStr, decodeSELECT},
    {kSTATUSStr, decodeSTATUS},
    {kEXPUNGEStr, decodeEXPUNGE},
    {kSTOREStr, decodeSTORE},
    {kCAPABILITYStr, decodeCAPABILITY},
    {kFETCHStr, decodeFETCH},
    {kNOOPStr, decodeNOOP},
    {kIDLEStr, decodeNOOP} 

};

//
// IMAP command string to internal enum code map table
//

std::unordered_map<std::string, CMailIMAP::Commands> CMailIMAP::stringToCodeMap ={
    { kSTARTTLSStr, Commands::STARTTLS},
    { kAUTHENTICATEStr, Commands::AUTHENTICATE},
    { kLOGINStr, Commands::LOGIN},
    { kCAPABILITYStr, Commands::CAPABILITY},
    { kSELECTStr, Commands::SELECT},
    { kEXAMINEStr, Commands::EXAMINE},
    { kCREATEStr, Commands::CREATE},
    { kDELETEStr, Commands::DELETE},
    { kRENAMEStr, Commands::RENAME},
    { kSUBSCRIBEStr, Commands::SUBSCRIBE},
    { kUNSUBSCRIBEStr, Commands::UNSUBSCRIBE},
    { kLISTStr, Commands::LIST},
    { kLSUBStr, Commands::LSUB},
    { kSTATUSStr, Commands::STATUS},
    { kAPPENDStr, Commands::APPEND},
    { kCHECKStr, Commands::CHECK},
    { kCLOSEStr, Commands::CLOSE},
    { kEXPUNGEStr, Commands::EXPUNGE},
    { kSEARCHStr, Commands::SEARCH},
    { kFETCHStr, Commands::FETCH},
    { kSTOREStr, Commands::STORE},
    { kCOPYStr, Commands::COPY},
    { kUIDStr, Commands::UID},
    { kNOOPStr, Commands::NOOP},
    { kLOGOUTStr, Commands::LOGOUT},
    { kIDLEStr, Commands::IDLE}

};

// =======================
// PUBLIC STATIC VARIABLES
// =======================

// ===============
// PRIVATE METHODS
// ===============

//
// Send IMAP command direct to server. The maximum buffre size is CURL_MAX_WRITE_SIZE
// so split up message in chunks.
//

void CMailIMAP::sendCommandDirect(const std::string& command) {

    size_t len = 0;
    int bytesCopied = 0;

    do {
        this->errMsgBuffer[0];
        this->res = curl_easy_send(this->curl, &command[bytesCopied], std::min(((int) command.length() - bytesCopied), CURL_MAX_WRITE_SIZE), &len);
        if (this->res == CURLE_AGAIN) {
            continue;
        } else if (this->res != CURLE_OK) {
            std::string errMsg;
            if (std::strlen(this->errMsgBuffer) != 0) {
                errMsg = this->errMsgBuffer;
            } else {
                errMsg = curl_easy_strerror(res);
            }
            throw std::runtime_error(std::string("curl_easy_send() failed: ") + errMsg);
        }
        bytesCopied += len;
    } while ((bytesCopied < command.length()));

}

//
// Wait for reply from direct command. Keep filling buffer until the commandTag is found and
// we have a full line. Also if we run out of buffer space then append current buffer to response
// and start at front of rxBuffer.
//

void CMailIMAP::waitForCommandResponse(const std::string& commandTag, std::string& commandResponse) {

    std::string searchTag{ commandTag + " "};
    size_t len = 0;
    size_t currPos = 0;
    char *tagptr;

    commandResponse.clear();

    do {

        this->errMsgBuffer[0];
        this->res = curl_easy_recv(this->curl, &this->rxBuffer[currPos], sizeof (this->rxBuffer) - currPos, &len);

        if (this->res == CURLE_OK) {

            this->rxBuffer[currPos + len] = '\0';

            if ((tagptr = strstr(this->rxBuffer, searchTag.c_str())) != nullptr) {
                if ((this->rxBuffer[currPos + len - 2] == '\r') &&
                    (this->rxBuffer[currPos + len - 1] == '\n')) {
                    commandResponse.append(this->rxBuffer);
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

        if (((sizeof (this->rxBuffer) - currPos)) == 0) {
            commandResponse.append(this->rxBuffer);
            currPos = 0;
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
// Extract the contents between two delimeters in command response line.
//

inline std::string CMailIMAP::extractBetween(const std::string& line, const char first, const char last) {
    int firstDel=line.find_first_of(first);
    int lastDel=line.find_first_of(last, firstDel );
    return (line.substr(firstDel + 1, (lastDel - firstDel - 1)));
}

//
// Extract string between two occurrences of a delimeter character (ie. number and spaces).
//

inline std::string CMailIMAP::extractBetweenDelimeter(const std::string& line, const char delimeter) {
    
    int firstDel = line.find_first_of(delimeter) + 1;
    int lastDel = line.find_first_of(delimeter, firstDel);
    return (line.substr(firstDel, lastDel-firstDel));

}

//
// Extract string between two delimeters (ie. number and spaces).
//

inline std::string CMailIMAP::extractUntaggedNumber(const std::string& line) {

    int startNumber = line.find_first_not_of(' ', 1 );
    int endNumber = line.find_first_of(' ', startNumber);
    return (line.substr(startNumber, endNumber-startNumber));

}

//
// Extract tag from command response line.
//

inline std::string CMailIMAP::extractTag(const std::string& line) {
    return (line.substr(0, line.find_first_of(' ')));
}

//
// Extract command string from command response line. If it the command
// has the UID prefix command then this is skipped over.
//

inline std::string CMailIMAP::extractCommand(const std::string& line) {

    int startComm = line.find_first_of (' ')+1;
    int endComm =  line.find_first_of (' ', startComm);
    
    if (line.compare(startComm, endComm-startComm, kUIDStr) == 0) {
        startComm = line.find_first_of (' ', startComm)+1;
        endComm =  line.find_first_of (' ', startComm);
    }
 
    return(line.substr(startComm, endComm-startComm));

}

//
// Extract list string from command response line. Note only check
// until the end of string.
//

inline std::string CMailIMAP::extractList(const std::string& line) {

    int bracketCount = 0, currIndex = 0, lineLen=line.length();

    do {
        if (line[currIndex] == '(') bracketCount++;
        if (line[currIndex] == ')') bracketCount--;
        currIndex++;
    } while (bracketCount && (--lineLen > 0));

    return (line.substr(0, currIndex));

}

//
// Decode item/number pair in response and add to response map. Note the line is updated to remove the pair.
//

void CMailIMAP::decodeNumber(const std::string& commandStr, FetchRespData& fetchData, std::string& line) {
    std::string numberStr;
    line = line.substr(line.find(commandStr) + commandStr.length());
    numberStr = extractBetweenDelimeter(line, ' ');
    line = line.substr(numberStr.length() + 2);
    fetchData.responseMap.insert({commandStr, numberStr});

}

//
// Decode item/string pair in response and add to response map. Note the line is updated to remove the pair.
//

void CMailIMAP::decodeString(const std::string& commandStr, FetchRespData& fetchData, std::string& line) {
    std::string quotedString;
    line = line.substr(line.find(commandStr) + commandStr.length() + 1);
    quotedString = "\"" + extractBetweenDelimeter(line, '\"') + "\"";
    line = line.substr(quotedString.length());
    fetchData.responseMap.insert({commandStr, quotedString});

}

//
// Decode list in response and add to response map.Note the line is updated to remove the list.
//

void CMailIMAP::decodeList(const std::string& commandStr, FetchRespData& fetchData, std::string& line) {

    std::string list;
    line = line.substr(line.find(commandStr) + commandStr.length() + 1);
    list = extractList(line);
    line = line.substr(list.length());
    fetchData.responseMap.insert({commandStr, list});
    
}

//
// Decode octet string in response and add to response map. This involves decoding 
// the octet string length and reading into a buffer.
//

void CMailIMAP::decodeOctets(const std::string& commandStr, FetchRespData& fetchData, std::string& line, std::istringstream& responseStream) {

    std::string octetStr, octectBuffer, commandLabel { line };
    int numberOfOctets;
    
    if (commandLabel.back() == '\r')  commandLabel.pop_back();
    
    line = line.substr(line.find(commandStr) + commandStr.length());
    octetStr = extractBetween(line, '{', '}');
    numberOfOctets = std::strtoull(octetStr.c_str(), nullptr, 10);
    line = line.substr(octetStr.length() + 2);
    octectBuffer.resize(numberOfOctets);
    responseStream.read(&octectBuffer[0], numberOfOctets);
    std::getline(responseStream, line, '\n');
    fetchData.responseMap.insert({commandLabel, octectBuffer});

}

//
// Decode command response status. At present an un-tagged BAD/NO gets sent to std::cerr
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
// Decode SELECT/EXAMINE Response. 
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeSELECT(CMailIMAP::CommandData& commandData, std::istringstream& responseStream) {

    CMailIMAP::SELECTRESPONSE resp{ new CMailIMAP::SelectResponse};

    resp->command = CMailIMAP::stringToCodeMap[commandData.command];

    // Extract mailbox name from command (stripping any quotes).
    
    resp->mailBoxName = commandData.commandLine.substr(commandData.commandLine.find_last_of(' ') + 1);
    if (resp->mailBoxName.back() == '\"') resp->mailBoxName.pop_back();
    if (resp->mailBoxName.front() == '\"') resp->mailBoxName = resp->mailBoxName.substr(1);

    for (std::string line; std::getline(responseStream, line, '\n');) {

        line.pop_back();    // Remove linefeed

        if (line.find(kUntaggedStr + " " + kOKStr + " [") == 0) {
            line = extractBetween(line, '[', ']');
        }

        if (line.find(kUntaggedStr + " " + kFLAGSStr) == 0) {
            resp->responseMap.insert({kFLAGSStr, extractList(line.substr(line.find_first_of('(')))});

        } else if (line.find(kPERMANENTFLAGSStr) == 0) {
            resp->responseMap.insert({kPERMANENTFLAGSStr, extractList(line.substr(line.find_first_of('(')))});

        } else if (line.find(kUIDVALIDITYStr) == 0) {
            resp->responseMap.insert({kUIDVALIDITYStr, line.substr(line.find_first_of(' ') + 1)});

        } else if (line.find(kUIDNEXTStr) == 0) {
            resp->responseMap.insert({kUIDNEXTStr, line.substr(line.find_first_of(' ') + 1)});

        } else if (line.find(kHIGHESTMODSEQStr) == 0) {
            resp->responseMap.insert({kHIGHESTMODSEQStr, line.substr(line.find_first_of(' ') + 1)});

        } else if (line.find(kUNSEENStr) == 0) {
            resp->responseMap.insert({kUNSEENStr, line.substr(line.find_first_of(' ') + 1)});

        } else if (line.find(kEXISTSStr) != std::string::npos) {
            resp->responseMap.insert({kEXISTSStr, extractUntaggedNumber(line)});

        } else if (line.find(kRECENTStr) != std::string::npos) {
            resp->responseMap.insert({kRECENTStr, extractUntaggedNumber(line)});

        } else if (line.find(kUntaggedStr + " " + kCAPABILITYStr) == 0) {
            line = line.substr((std::string(kUntaggedStr + " " + kCAPABILITYStr).length()) + 1);
            resp->responseMap.insert({kCAPABILITYStr, line});

        } else if ((line.find("] " + resp->mailBoxName) != std::string::npos) ||
                (line.find("] " + commandData.command + " completed.") != std::string::npos)) {
            resp->mailBoxAccess = extractBetween(line, '[', ']');
        } else {
            decodeStatus(commandData.tag, line, resp);
        }

    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));

}

//
// Decode SEARCH Response.
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeSEARCH(CMailIMAP::CommandData& commandData, std::istringstream& responseStream) {

    CMailIMAP::SEARCHRESPONSE resp{ new CMailIMAP::SearchResponse};

     resp->command = CMailIMAP::stringToCodeMap[commandData.command];

    for (std::string line; std::getline(responseStream, line, '\n');) {

        line.pop_back();    // Remove linefeed

        if (line.find(kUntaggedStr + " " + commandData.command) == 0) {

            line = line.substr(line.find_first_of(' ') + 1);
            line = line.substr(line.find_first_of(' ') + 1);

            std::istringstream listStream(line);    // Read indexes/UIDs
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
// Decode LIST/LSUB Response.
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeLIST(CMailIMAP::CommandData& commandData, std::istringstream& responseStream) {

    CMailIMAP::LISTRESPONSE resp{ new CMailIMAP::ListResponse};

     resp->command = CMailIMAP::stringToCodeMap[commandData.command];

    for (std::string line; std::getline(responseStream, line, '\n');) {

        CMailIMAP::ListRespData mailBoxEntry;

        line.pop_back();    // Remove linefeed

        if (line.find(kUntaggedStr + " " + commandData.command) != std::string::npos) {
            mailBoxEntry.attributes = extractList(line.substr(line.find_first_of('(')));
            mailBoxEntry.hierDel = extractBetween(line, '\"', '\"').front();
            if (line.back() != '\"') {
                mailBoxEntry.name = line.substr(line.find_last_of(' '));
            } else {
                line.pop_back();
                mailBoxEntry.name = line.substr(line.find_last_of('\"'));
                mailBoxEntry.name += '\"';
            }
            resp->mailBoxList.push_back(std::move(mailBoxEntry));

        } else {
            decodeStatus(commandData.tag, line, resp);
        }

    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));


}

//
// Decode STATUS Response.
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeSTATUS(CMailIMAP::CommandData& commandData, std::istringstream& responseStream) {

    CMailIMAP::STATUSRESPONSE resp{ new CMailIMAP::StatusResponse};

     resp->command = CMailIMAP::stringToCodeMap[commandData.command];

    for (std::string line; std::getline(responseStream, line, '\n');) {

        line.pop_back();    // Remove linefeed

        if (line.find(kUntaggedStr + " " + commandData.command) == 0) {

            line = line.substr(line.find_first_of(' ') + 1);
            line = line.substr(line.find_first_of(' ') + 1);
            resp->mailBoxName = line.substr(0, line.find_first_of(' '));

            line = extractBetween(line, '(', ')');

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
// Decode EXPUNGE Response.
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeEXPUNGE(CMailIMAP::CommandData& commandData, std::istringstream& responseStream) {

    CMailIMAP::EXPUNGERESPONSE resp{ new CMailIMAP::ExpungeResponse};

     resp->command = CMailIMAP::stringToCodeMap[commandData.command];

    for (std::string line; std::getline(responseStream, line, '\n');) {

        line.pop_back();    // Remove linefeed

        if (line.find(kEXISTSStr) != std::string::npos) {
            line = extractUntaggedNumber(line);
            resp->exists.push_back(std::strtoull(line.c_str(), nullptr, 10));

        } else if (line.find(kEXPUNGEStr) != std::string::npos) {
            line = extractUntaggedNumber(line);
            resp->expunged.push_back(std::strtoull(line.c_str(), nullptr, 10));

        } else {
            decodeStatus(commandData.tag, line, resp);
        }

    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));

}

//
// Decode STORE Response.
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeSTORE(CMailIMAP::CommandData& commandData, std::istringstream& responseStream) {

    CMailIMAP::STORERESPONSE resp{ new CMailIMAP::StoreResponse};

     resp->command = CMailIMAP::stringToCodeMap[commandData.command];

    for (std::string line; std::getline(responseStream, line, '\n');) {

        StoreRespData storeData;

        line.pop_back();    // Remove linefeed

        if (line.find(kFETCHStr) != std::string::npos) {
            storeData.flags = "("+extractBetween(line.substr(line.find_first_of('(') + 1), '(', ')')+")";
            storeData.index = std::strtoull(extractUntaggedNumber(line).c_str(), nullptr, 10);
            resp->storeList.push_back(std::move(storeData));

        } else {
            decodeStatus(commandData.tag, line, resp);
        }

    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));

}

//
// Decode CAPABILITY Response.
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeCAPABILITY(CMailIMAP::CommandData& commandData, std::istringstream& responseStream) {

    CMailIMAP::CAPABILITYRESPONSE resp{ new CMailIMAP::CapabilityResponse};

     resp->command = CMailIMAP::stringToCodeMap[commandData.command];

    for (std::string line; std::getline(responseStream, line, '\n');) {

        line.pop_back();        // Remove linefeed

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
// Decode NOOP/IDLE Response.
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeNOOP(CMailIMAP::CommandData& commandData, std::istringstream& responseStream) {

    CMailIMAP::NOOPRESPONSE resp{ new CMailIMAP::NoOpResponse};

     resp->command = CMailIMAP::stringToCodeMap[commandData.command];

    for (std::string line; std::getline(responseStream, line, '\n');) {

        line.pop_back();        // Remove linefeed

        if (line.find(kUntaggedStr) == 0) {
            resp->rawResponse.push_back(std::move(line));
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

    CMailIMAP::FETCHRESPONSE resp{ new CMailIMAP::FetchResponse};

    resp->command = CMailIMAP::stringToCodeMap[commandData.command];

    for (std::string line; std::getline(responseStream, line, '\n');) {

        FetchRespData fetchData;

        line.pop_back();    // Add back '\n' as '\r\n' will be part of octet count

        if (line.find(kFETCHStr + " (") != std::string::npos) {

            fetchData.index = std::strtoull(extractUntaggedNumber(line).c_str(), nullptr, 10);
            line = line.substr(line.find_first_of('(') + 1);

            bool endOfFetch = false;
            
            do {

                if (line.find(kBODYSTRUCTUREStr + " ") == 0) {
                    decodeList(kBODYSTRUCTUREStr, fetchData, line);
                } else if (line.find(kENVELOPEStr + " ") == 0) {
                    decodeList(kENVELOPEStr, fetchData, line);                
                } else if (line.find(kFLAGSStr + " ") == 0) {
                    decodeList(kFLAGSStr, fetchData, line);
                } else if (line.find(kBODYStr + " ") == 0) {
                    decodeList(kBODYStr, fetchData, line);
                } else if (line.find(kINTERNALDATEStr + " ") == 0) {
                    decodeString(kINTERNALDATEStr, fetchData, line);
                } else if (line.find(kRFC822SIZEStr + " ") == 0) {
                  decodeNumber(kRFC822SIZEStr, fetchData, line);
                } else if (line.find(kUIDStr + " ") == 0) {
                    decodeNumber(kUIDStr, fetchData, line);
                } else if (line.find(kRFC822HEADERStr + " ") == 0) {
                    decodeOctets(kRFC822HEADERStr, fetchData, line, responseStream);
                } else if (line.find(kBODYStr+"[") == 0) {
                    decodeOctets(kBODYStr, fetchData, line, responseStream);
                } else if (line.find(kRFC822Str + " ") == 0) {
                    decodeOctets(kRFC822Str, fetchData, line, responseStream);
                }
  
                line = line.substr(line.find_first_not_of(' '));
                if (line[0] == ')') {
                    endOfFetch = true;
                } else if (line.length() == kEOLStr.length() - 1) {
                    std::getline(responseStream, line, '\n');
                }

            } while (!endOfFetch);
            
            resp->fetchList.push_back(std::move(fetchData));
   
        } else {
            decodeStatus(commandData.tag, line, resp);
        }


    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));

}

//
// Decode LOGOUT Response
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeLOGOUT(CMailIMAP::CommandData& commandData, std::istringstream& responseStream) {

    CMailIMAP::LOGOUTRESPONSE resp{ new CMailIMAP::LogOutResponse};

     resp->command = CMailIMAP::stringToCodeMap[commandData.command];

    for (std::string line; std::getline(responseStream, line, '\n');) {
        
        line.pop_back();    // Remove linefeed
        
        if (line.find(kUntaggedStr + " " + kBYEStr) == 0) {
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

    CMailIMAP::BASERESPONSE resp{ new CMailIMAP::BaseResponse};

     resp->command = CMailIMAP::stringToCodeMap[commandData.command];

    for (std::string line; std::getline(responseStream, line, '\n');) {
        line.pop_back();    // Remove linefeed
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
    CommandData commandData{ extractTag(commandLine), extractCommand(commandLine), commandLine};

    decodeFn = CMailIMAP::decodeCommmandMap[commandData.command];
    if (!decodeFn) {
        decodeFn = decodeDefault;
    }

    return (decodeFn(commandData, responseStream));

}

//
// Send IDLE command (requires a special handler).
//

void CMailIMAP::sendCommandIDLE(const std::string& commandLine) {

    std::string response;
    
    this->generateTag();

    this->sendCommandDirect(this->currentTag + " " + kIDLEStr + kEOLStr);
    this->waitForCommandResponse(kContinuationStr, this->commandResponse);

    this->waitForCommandResponse(kUntaggedStr, response);

    this->sendCommandDirect(kDONEStr + kEOLStr);

    this->waitForCommandResponse(this->currentTag, this->commandResponse);
    
    response += this->commandResponse;
    
    this->commandResponse = response;
    


}

//
// Send APPPEND command (requires a special handler).
//

void CMailIMAP::sendCommandAPPEND(const std::string& commandLine) {

    this->sendCommandDirect(this->currentTag + " " + commandLine.substr(0, commandLine.find_first_of('}')+1) + kEOLStr);
    this->waitForCommandResponse(kContinuationStr, this->commandResponse);

    this->sendCommandDirect(commandLine.substr(commandLine.find_first_of('}')+1)); 
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
// Return string for IMAP command code
//

 std::string CMailIMAP::commandCodeString (CMailIMAP::Commands commandCode) {
     
     for (auto commandEntry : CMailIMAP::stringToCodeMap) {
         if (commandEntry.second == commandCode) {
             return (commandEntry.first);
         }
     }
     
     return("");    //Not found return empty string
     
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
        sendCommandIDLE(commandLine);
    } else if (commandLine.find(kAPPENDStr) != std::string::npos) {
         sendCommandAPPEND(commandLine);
    } else {
        this->sendCommandDirect(this->currentTag + " " + commandLine + kEOLStr);
        this->waitForCommandResponse(this->currentTag, this->commandResponse);
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

}

//
// CMailIMAP closedown
//

void CMailIMAP::closedown(void) {

    curl_global_cleanup();

}

