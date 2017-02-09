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
// Description: A class to connect to an IMAP server and send commands
// and recieve and decode responses to them. The class uses the libcurl 
// to provide its functionality. NOTE: This class does not at present cater 
// for the fact that any IMAP commands can be upper or lower case and any 
// protocol parsing should be case insensitive to  allow for this 
// (ON TODO LIST).
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

#include <iostream>
#include <cstring>
#include <string>
#include <stdexcept>
#include <sstream>
#include <iomanip>

// ===========================
// PRIVATE TYPES AND CONSTANTS
// ===========================

//
// Line terminator
//

const std::string CMailIMAP::kEOLStr("\r\n");

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
const std::string CMailIMAP::kUIDStr("UID");
const std::string CMailIMAP::kBYEStr("BYE");

// ==========================
// PUBLIC TYPES AND CONSTANTS
// ==========================

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

// ========================
// PRIVATE STATIC VARIABLES
// ========================

//
// IMAP command string to decode response mapping table
//

std::unordered_map<std::string, CMailIMAP::DecodeFunction> CMailIMAP::decodeCommmandMap ={

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

std::unordered_map<std::string, CMailIMAP::Commands> CMailIMAP::stringToCodeMap = {
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
// Extract the contents between two delimeters in command response line.
//

inline std::string CMailIMAP::extractBetween(const std::string& lineStr, const char first, const char last) {
    int firstDel = lineStr.find_first_of(first);
    int lastDel = lineStr.find_first_of(last, firstDel);
    return (lineStr.substr(firstDel + 1, (lastDel - firstDel - 1)));
}

//
// Extract string between two occurrences of a delimeter character (ie. number and spaces).
//

inline std::string CMailIMAP::extractBetweenDelimeter(const std::string& lineStr, const char delimeter) {

    int firstDel = lineStr.find_first_of(delimeter) + 1;
    int lastDel = lineStr.find_first_of(delimeter, firstDel);
    return (lineStr.substr(firstDel, lastDel - firstDel));

}

//
// Extract number that may follow an un-tagged command response.
//

inline std::string CMailIMAP::extractUntaggedNumber(const std::string& lineStr) {

    int startNumber = lineStr.find_first_not_of(' ', 1);
    int endNumber = lineStr.find_first_of(' ', startNumber);
    return (lineStr.substr(startNumber, endNumber - startNumber));

}

//
// Extract tag from command response line.
//

inline std::string CMailIMAP::extractTag(const std::string& lineStr) {
    return (lineStr.substr(0, lineStr.find_first_of(' ')));
}

//
// Extract command string from command line. If the command has the UID 
// prefix then this is skipped over.
//

inline std::string CMailIMAP::extractCommand(const std::string& lineStr) {

    int startOfCommand = lineStr.find_first_of(' ') + 1;
    int endOfCommand = lineStr.find_first_of(' ', startOfCommand);

    if (lineStr.compare(startOfCommand, endOfCommand - startOfCommand, kUIDStr) == 0) {
        startOfCommand = lineStr.find_first_of(' ', startOfCommand) + 1;
        endOfCommand = lineStr.find_first_of(' ', startOfCommand);
    }

    return (lineStr.substr(startOfCommand, endOfCommand - startOfCommand));

}

//
// Extract list  from command response line. Note only check until 
// the end of line.
//

inline std::string CMailIMAP::extractList(const std::string& lineStr) {

    int bracketCount = 0, currentIndex = 0, lineLength = lineStr.length();

    do {
        if (lineStr[currentIndex] == '(') bracketCount++;
        if (lineStr[currentIndex] == ')') bracketCount--;
        currentIndex++;
    } while (bracketCount && (--lineLength > 0));

    return (lineStr.substr(0, currentIndex));

}

//
// Decode item/number pair in command response and add to response map. Note the current line is 
// updated to remove the pair and also this function is only used in FETCH command decode as the 
// response is processed over multiple lines and not line by line.
//

void CMailIMAP::decodeNumber(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr) {
    std::string numberStr;
    lineStr = lineStr.substr(lineStr.find(itemStr) + itemStr.length());
    numberStr = extractBetweenDelimeter(lineStr, ' ');
    lineStr = lineStr.substr(numberStr.length() + 2);
    fetchData.responseMap.insert({itemStr, numberStr});

}

//
// Decode item/string pair in response and add to response map.Note the current line is 
// updated to remove the pair and also this function is only used in FETCH command decode as the 
// response is processed over multiple lines and not line by line.

void CMailIMAP::decodeString(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr) {
    std::string quotedString;
    lineStr = lineStr.substr(lineStr.find(itemStr) + itemStr.length() + 1);
    quotedString = "\"" + extractBetweenDelimeter(lineStr, '\"') + "\"";
    lineStr = lineStr.substr(quotedString.length());
    fetchData.responseMap.insert({itemStr, quotedString});

}

//
// Decode item list in response and add to response map.Note the current line is 
// updated to remove the pair and also this function is only used in FETCH 
// command decode as the response is processed over multiple lines and not line 
// by line.
//

void CMailIMAP::decodeList(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr) {

    std::string list;
    lineStr = lineStr.substr(lineStr.find(itemStr) + itemStr.length() + 1);
    list = extractList(lineStr);
    lineStr = lineStr.substr(list.length());
    fetchData.responseMap.insert({itemStr, list});

}

//
// Decode item octet string in response and add to response map. This involves decoding 
// the octet string length and reading the string into a buffer, leaving line containing 
// the next part of the command response to be processed. Note: The command response before
// the octet string is used as the first item when inserting the octet string into the 
// response map to distinguish between multiple octet fetches that might occur.
//

void CMailIMAP::decodeOctets(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr, std::istringstream& responseStream) {

    std::string octetStr, octectBuffer, commandLabel{ lineStr};
    int numberOfOctets;

    if (commandLabel.back() == '\r') commandLabel.pop_back();

    lineStr = lineStr.substr(lineStr.find(itemStr) + itemStr.length());
    octetStr = extractBetween(lineStr, '{', '}');
    numberOfOctets = std::strtoull(octetStr.c_str(), nullptr, 10);
    lineStr = lineStr.substr(octetStr.length() + 2);
    octectBuffer.resize(numberOfOctets);
    responseStream.read(&octectBuffer[0], numberOfOctets);
    std::getline(responseStream, lineStr, '\n');
    fetchData.responseMap.insert({commandLabel, octectBuffer});

}

//
// Decode command response status. At present an un-tagged BAD/NO gets sent to std::cerr.
// Note: Any optional string that the server provides with a status is stored away in the
// response for use by the caller.
//

void CMailIMAP::decodeStatus(const std::string& tagStr, const std::string& lineStr, CMailIMAP::BASERESPONSE resp) {

    if (lineStr.find(tagStr + " " + kOKStr) == 0) {
        resp->status = RespCode::OK;
        resp->errorMessage = "";

    } else if (lineStr.find(tagStr + " " + kNOStr) == 0) {
        resp->status = RespCode::NO;
        resp->errorMessage = lineStr;

    } else if (lineStr.find(tagStr + " " + kBADStr) == 0) {
        resp->status = RespCode::BAD;
        resp->errorMessage = lineStr;

    } else if (lineStr.find(kUntaggedStr + " " + kBYEStr) == 0) {
        resp->status = RespCode::BAD;
        resp->errorMessage = lineStr;

    } else if ((lineStr.find(kUntaggedStr + " " + kNOStr) == 0)
            || (lineStr.find(kUntaggedStr + " " + kBADStr) == 0)) {
        std::cerr << lineStr << std::endl;

    } else {
        std::cerr << "UKNOWN RESPONSE LINE = [" << lineStr << "]" << std::endl;
    }


}

//
// Decode SELECT/EXAMINE Response. Note: The mailbox name is extracted from the 
// command line and used when decoding the response to find the mailbox access 
// privileges (READ-ONLY or READ-WRITE).
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeSELECT(CMailIMAP::CommandData& commandData) {

    CMailIMAP::SELECTRESPONSE resp{ new CMailIMAP::SelectResponse};

    resp->command = CMailIMAP::stringToCodeMap[commandData.commandStr];

    // Extract mailbox name from command (stripping any quotes).

    resp->mailBoxName = commandData.commandLineStr.substr(commandData.commandLineStr.find_last_of(' ') + 1);
    if (resp->mailBoxName.back() == '\"') resp->mailBoxName.pop_back();
    if (resp->mailBoxName.front() == '\"') resp->mailBoxName = resp->mailBoxName.substr(1);

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        lineStr.pop_back(); // Remove linefeed

        if (lineStr.find(kUntaggedStr + " " + kOKStr + " [") == 0) {
            lineStr = extractBetween(lineStr, '[', ']');
        }

        if (lineStr.find(kUntaggedStr + " " + kFLAGSStr) == 0) {
            resp->responseMap.insert({kFLAGSStr, extractList(lineStr.substr(lineStr.find_first_of('(')))});

        } else if (lineStr.find(kPERMANENTFLAGSStr) == 0) {
            resp->responseMap.insert({kPERMANENTFLAGSStr, extractList(lineStr.substr(lineStr.find_first_of('(')))});

        } else if (lineStr.find(kUIDVALIDITYStr) == 0) {
            resp->responseMap.insert({kUIDVALIDITYStr, lineStr.substr(lineStr.find_first_of(' ') + 1)});

        } else if (lineStr.find(kUIDNEXTStr) == 0) {
            resp->responseMap.insert({kUIDNEXTStr, lineStr.substr(lineStr.find_first_of(' ') + 1)});

        } else if (lineStr.find(kHIGHESTMODSEQStr) == 0) {
            resp->responseMap.insert({kHIGHESTMODSEQStr, lineStr.substr(lineStr.find_first_of(' ') + 1)});

        } else if (lineStr.find(kUNSEENStr) == 0) {
            resp->responseMap.insert({kUNSEENStr, lineStr.substr(lineStr.find_first_of(' ') + 1)});

        } else if (lineStr.find(kEXISTSStr) != std::string::npos) {
            resp->responseMap.insert({kEXISTSStr, extractUntaggedNumber(lineStr)});

        } else if (lineStr.find(kRECENTStr) != std::string::npos) {
            resp->responseMap.insert({kRECENTStr, extractUntaggedNumber(lineStr)});

        } else if (lineStr.find(kUntaggedStr + " " + kCAPABILITYStr) == 0) {
            lineStr = lineStr.substr((std::string(kUntaggedStr + " " + kCAPABILITYStr).length()) + 1);
            resp->responseMap.insert({kCAPABILITYStr, lineStr});

        } else if ((lineStr.find("] " + resp->mailBoxName) != std::string::npos) ||
                (lineStr.find("] " + commandData.commandStr + " completed.") != std::string::npos)) {
            resp->mailBoxAccess = extractBetween(lineStr, '[', ']');
        } else {
            decodeStatus(commandData.tagStr, lineStr, resp);
        }

    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));

}

//
// Decode SEARCH Response.
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeSEARCH(CMailIMAP::CommandData& commandData) {

    CMailIMAP::SEARCHRESPONSE resp{ new CMailIMAP::SearchResponse};

    resp->command = CMailIMAP::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        lineStr.pop_back(); // Remove linefeed

        if (lineStr.find(kUntaggedStr + " " + commandData.commandStr) == 0) {

            lineStr = lineStr.substr(lineStr.find_first_of(' ') + 1);
            lineStr = lineStr.substr(lineStr.find_first_of(' ') + 1);

            std::istringstream listStream(lineStr); // Read indexes/UIDs
            while (listStream.good()) {
                uint64_t index;
                listStream >> index;
                resp->indexes.push_back(index);
            }

        } else {
            decodeStatus(commandData.tagStr, lineStr, resp);
        }

    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));

}

//
// Decode LIST/LSUB Response.
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeLIST(CMailIMAP::CommandData& commandData) {

    CMailIMAP::LISTRESPONSE resp{ new CMailIMAP::ListResponse};

    resp->command = CMailIMAP::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        CMailIMAP::ListRespData mailBoxEntry;

        lineStr.pop_back(); // Remove linefeed

        if (lineStr.find(kUntaggedStr + " " + commandData.commandStr) != std::string::npos) {
            mailBoxEntry.attributes = extractList(lineStr.substr(lineStr.find_first_of('(')));
            mailBoxEntry.hierDel = extractBetween(lineStr, '\"', '\"').front();
            if (lineStr.back() != '\"') {
                mailBoxEntry.name = lineStr.substr(lineStr.find_last_of(' '));
            } else {
                lineStr.pop_back();
                mailBoxEntry.name = lineStr.substr(lineStr.find_last_of('\"'));
                mailBoxEntry.name += '\"';
            }
            resp->mailBoxList.push_back(std::move(mailBoxEntry));

        } else {
            decodeStatus(commandData.tagStr, lineStr, resp);
        }

    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));


}

//
// Decode STATUS Response.
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeSTATUS(CMailIMAP::CommandData& commandData) {

    CMailIMAP::STATUSRESPONSE resp{ new CMailIMAP::StatusResponse};

    resp->command = CMailIMAP::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        lineStr.pop_back(); // Remove linefeed

        if (lineStr.find(kUntaggedStr + " " + commandData.commandStr) == 0) {

            lineStr = lineStr.substr(lineStr.find_first_of(' ') + 1);
            lineStr = lineStr.substr(lineStr.find_first_of(' ') + 1);
            resp->mailBoxName = lineStr.substr(0, lineStr.find_first_of(' '));

            lineStr = extractBetween(lineStr, '(', ')');

            std::istringstream listStream(lineStr);
            std::string item, value;

            while (listStream.good()) {
                listStream >> item >> value;
                resp->responseMap.insert({item, value});
            }

        } else {
            decodeStatus(commandData.tagStr, lineStr, resp);
        }

    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));

}

//
// Decode EXPUNGE Response.
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeEXPUNGE(CMailIMAP::CommandData& commandData) {

    CMailIMAP::EXPUNGERESPONSE resp{ new CMailIMAP::ExpungeResponse};

    resp->command = CMailIMAP::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        lineStr.pop_back(); // Remove linefeed

        if (lineStr.find(kEXISTSStr) != std::string::npos) {
            lineStr = extractUntaggedNumber(lineStr);
            resp->exists.push_back(std::strtoull(lineStr.c_str(), nullptr, 10));

        } else if (lineStr.find(kEXPUNGEStr) != std::string::npos) {
            lineStr = extractUntaggedNumber(lineStr);
            resp->expunged.push_back(std::strtoull(lineStr.c_str(), nullptr, 10));

        } else {
            decodeStatus(commandData.tagStr, lineStr, resp);
        }

    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));

}

//
// Decode STORE Response.
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeSTORE(CMailIMAP::CommandData& commandData) {

    CMailIMAP::STORERESPONSE resp{ new CMailIMAP::StoreResponse};

    resp->command = CMailIMAP::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        StoreRespData storeData;

        lineStr.pop_back(); // Remove linefeed

        if (lineStr.find(kFETCHStr) != std::string::npos) {
            storeData.flags = "(" + extractBetween(lineStr.substr(lineStr.find_first_of('(') + 1), '(', ')') + ")";
            storeData.index = std::strtoull(extractUntaggedNumber(lineStr).c_str(), nullptr, 10);
            resp->storeList.push_back(std::move(storeData));

        } else {
            decodeStatus(commandData.tagStr, lineStr, resp);
        }

    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));

}

//
// Decode CAPABILITY Response.
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeCAPABILITY(CMailIMAP::CommandData& commandData) {

    CMailIMAP::CAPABILITYRESPONSE resp{ new CMailIMAP::CapabilityResponse};

    resp->command = CMailIMAP::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        lineStr.pop_back(); // Remove linefeed

        if (lineStr.find(kUntaggedStr + " " + kCAPABILITYStr) == 0) {
            lineStr = lineStr.substr(lineStr.find_first_of(' ') + 1);
            resp->capabilityList = lineStr.substr(lineStr.find_first_of(' ') + 1);

        } else {
            decodeStatus(commandData.tagStr, lineStr, resp);
        }

    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));

}

//
// Decode NOOP/IDLE Response.
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeNOOP(CMailIMAP::CommandData& commandData) {

    CMailIMAP::NOOPRESPONSE resp{ new CMailIMAP::NoOpResponse};

    resp->command = CMailIMAP::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        lineStr.pop_back(); // Remove linefeed

        if (lineStr.find(kUntaggedStr) == 0) {
            resp->rawResponse.push_back(std::move(lineStr));
        } else {
            decodeStatus(commandData.tagStr, lineStr, resp);
        }

    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));

}

//
// Decode FETCH Response
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeFETCH(CMailIMAP::CommandData& commandData) {

    CMailIMAP::FETCHRESPONSE resp{ new CMailIMAP::FetchResponse};

    resp->command = CMailIMAP::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        FetchRespData fetchData;

        lineStr.pop_back(); // Add back '\n' as '\r\n' will be part of octet count

        if (lineStr.find(kFETCHStr + " (") != std::string::npos) {

            fetchData.index = std::strtoull(extractUntaggedNumber(lineStr).c_str(), nullptr, 10);
            lineStr = lineStr.substr(lineStr.find_first_of('(') + 1);

            bool endOfFetch = false;

            do {

                if (lineStr.find(kBODYSTRUCTUREStr + " ") == 0) {
                    decodeList(kBODYSTRUCTUREStr, fetchData, lineStr);
                } else if (lineStr.find(kENVELOPEStr + " ") == 0) {
                    decodeList(kENVELOPEStr, fetchData, lineStr);
                } else if (lineStr.find(kFLAGSStr + " ") == 0) {
                    decodeList(kFLAGSStr, fetchData, lineStr);
                } else if (lineStr.find(kBODYStr + " ") == 0) {
                    decodeList(kBODYStr, fetchData, lineStr);
                } else if (lineStr.find(kINTERNALDATEStr + " ") == 0) {
                    decodeString(kINTERNALDATEStr, fetchData, lineStr);
                } else if (lineStr.find(kRFC822SIZEStr + " ") == 0) {
                    decodeNumber(kRFC822SIZEStr, fetchData, lineStr);
                } else if (lineStr.find(kUIDStr + " ") == 0) {
                    decodeNumber(kUIDStr, fetchData, lineStr);
                } else if (lineStr.find(kRFC822HEADERStr + " ") == 0) {
                    decodeOctets(kRFC822HEADERStr, fetchData, lineStr, commandData.commandRespStream);
                } else if (lineStr.find(kBODYStr + "[") == 0) {
                    decodeOctets(kBODYStr, fetchData, lineStr, commandData.commandRespStream);
                } else if (lineStr.find(kRFC822Str + " ") == 0) {
                    decodeOctets(kRFC822Str, fetchData, lineStr, commandData.commandRespStream);
                }

                lineStr = lineStr.substr(lineStr.find_first_not_of(' '));
                if (lineStr[0] == ')') {
                    endOfFetch = true;
                } else if (lineStr.length() == kEOLStr.length() - 1) {
                    std::getline(commandData.commandRespStream, lineStr, '\n');
                }

            } while (!endOfFetch);

            resp->fetchList.push_back(std::move(fetchData));

        } else {
            decodeStatus(commandData.tagStr, lineStr, resp);
        }


    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));

}

//
// Decode LOGOUT Response
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeLOGOUT(CMailIMAP::CommandData& commandData) {

    CMailIMAP::LOGOUTRESPONSE resp{ new CMailIMAP::LogOutResponse};

    resp->command = CMailIMAP::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        lineStr.pop_back(); // Remove linefeed

        if (lineStr.find(kUntaggedStr + " " + kBYEStr) == 0) {
            continue;
        } else {
            decodeStatus(commandData.tagStr, lineStr, resp);
        }
    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));

}

//
// Default Decode Response
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeDefault(CMailIMAP::CommandData& commandData) {

    CMailIMAP::BASERESPONSE resp{ new CMailIMAP::BaseResponse};

    resp->command = CMailIMAP::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {
        lineStr.pop_back(); // Remove lineStrfeed
        decodeStatus(commandData.tagStr, lineStr, resp);
    }

    return (static_cast<CMailIMAP::BASERESPONSE> (resp));

}

//
// Decode Command Response. The response string is one long string containing "\r\n"s to
// signal end of lines. The string is read line by line converting the response to a istringstream 
// and using getline() and '\n' to signal the end of line character (except FETCH which is dealt 
// with differently as it has to cater for octet strings that can span multiple lines.
//

CMailIMAP::BASERESPONSE CMailIMAP::decodeResponse(const std::string& commandLineStr, const std::string & commandResponseStr) {

    std::istringstream responseStream(commandResponseStr);
    CMailIMAP::DecodeFunction decodeFn;
    CommandData commandData{ extractTag(commandLineStr), extractCommand(commandLineStr), commandLineStr, responseStream};


    decodeFn = CMailIMAP::decodeCommmandMap[commandData.commandStr];
    if (!decodeFn) {
        decodeFn = decodeDefault;
    }

    return (decodeFn(commandData));

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
// Return string for IMAP command code
//

std::string CMailIMAP::commandCodeString(CMailIMAP::Commands commandCode) {

    for (auto commandEntry : CMailIMAP::stringToCodeMap) {
        if (commandEntry.second == commandCode) {
            return (commandEntry.first);
        }
    }

    CMailIMAP::Exception("commandCodeString() : Invalid command code.");

    return (""); // Never reached.

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
// Send single IMAP command and decode/return response.
//

CMailIMAP::BASERESPONSE CMailIMAP::sendCommand(const std::string& commandLineStr) {

    if (!this->bConnected) {
        throw CMailIMAP::Exception("Not connected to server.");
    }

    this->generateTag();

    if (commandLineStr.compare(kIDLEStr) == 0) {
        sendCommandIDLE(this->currentTagStr + " " + commandLineStr + kEOLStr);
    } else if (commandLineStr.find(kAPPENDStr) != std::string::npos) {
        sendCommandAPPEND(this->currentTagStr + " " + commandLineStr);
    } else {
        this->sendIMAPCommand(this->currentTagStr + " " + commandLineStr + kEOLStr);
        this->waitForIMAPCommandResponse(this->currentTagStr, this->commandResponseStr);
    }

    return (CMailIMAP::decodeResponse(this->currentTagStr + " " + commandLineStr, this->commandResponseStr));

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

