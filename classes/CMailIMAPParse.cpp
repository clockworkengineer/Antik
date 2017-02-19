/*
 * File:   CMailIMAPParse.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on January 24, 2017, 2:33 PM
 *
 * Copyright 2016.
 *
 */

//
// Class: CMailIMAPParse
// 
// Description: A class to parse CMailIMAP command responses. It has
// purely static data and functions and cannot be instantiated (ie. 
// it contains no constructor/destructor.
//
// NOTE: IMAP commands sent can be any in combination of case and this 
// is mirrored back in the response. So perform case-insensitive compares 
// for any commands in responses.
//
// Dependencies:   C11++     - Language standard features used.

//

// =================
// CLASS DEFINITIONS
// =================

#include "CMailIMAP.hpp"
#include "CMailIMAPParse.hpp"

// ====================
// CLASS IMPLEMENTATION
// ====================

//
// C++ STL definitions
//

#include <iostream>
#include <sstream>

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
// IMAP command string to parse response mapping table
//

std::unordered_map<std::string, CMailIMAPParse::ParseFunction> CMailIMAPParse::parseCommmandMap = {

    {CMailIMAP::kLISTStr, parseLIST},
    {CMailIMAP::kLSUBStr, parseLIST},
    {CMailIMAP::kSEARCHStr, parseSEARCH},
    {CMailIMAP::kSELECTStr, parseSELECT},
    {CMailIMAP::kEXAMINEStr, parseSELECT},
    {CMailIMAP::kSTATUSStr, parseSTATUS},
    {CMailIMAP::kEXPUNGEStr, parseEXPUNGE},
    {CMailIMAP::kSTOREStr, parseSTORE},
    {CMailIMAP::kCAPABILITYStr, parseCAPABILITY},
    {CMailIMAP::kFETCHStr, parseFETCH},
    {CMailIMAP::kNOOPStr, parseNOOP},
    {CMailIMAP::kIDLEStr, parseNOOP}

};

//
// IMAP command string to internal enum code map table
//

std::unordered_map<std::string, CMailIMAPParse::Commands> CMailIMAPParse::stringToCodeMap = {
    { CMailIMAP::kSTARTTLSStr, Commands::STARTTLS},
    { CMailIMAP::kAUTHENTICATEStr, Commands::AUTHENTICATE},
    { CMailIMAP::kLOGINStr, Commands::LOGIN},
    { CMailIMAP::kCAPABILITYStr, Commands::CAPABILITY},
    { CMailIMAP::kSELECTStr, Commands::SELECT},
    { CMailIMAP::kEXAMINEStr, Commands::EXAMINE},
    { CMailIMAP::kCREATEStr, Commands::CREATE},
    { CMailIMAP::kDELETEStr, Commands::DELETE},
    { CMailIMAP::kRENAMEStr, Commands::RENAME},
    { CMailIMAP::kSUBSCRIBEStr, Commands::SUBSCRIBE},
    { CMailIMAP::kUNSUBSCRIBEStr, Commands::UNSUBSCRIBE},
    { CMailIMAP::kLISTStr, Commands::LIST},
    { CMailIMAP::kLSUBStr, Commands::LSUB},
    { CMailIMAP::kSTATUSStr, Commands::STATUS},
    { CMailIMAP::kAPPENDStr, Commands::APPEND},
    { CMailIMAP::kCHECKStr, Commands::CHECK},
    { CMailIMAP::kCLOSEStr, Commands::CLOSE},
    { CMailIMAP::kEXPUNGEStr, Commands::EXPUNGE},
    { CMailIMAP::kSEARCHStr, Commands::SEARCH},
    { CMailIMAP::kFETCHStr, Commands::FETCH},
    { CMailIMAP::kSTOREStr, Commands::STORE},
    { CMailIMAP::kCOPYStr, Commands::COPY},
    { CMailIMAP::kUIDStr, Commands::UID},
    { CMailIMAP::kNOOPStr, Commands::NOOP},
    { CMailIMAP::kLOGOUTStr, Commands::LOGOUT},
    { CMailIMAP::kIDLEStr, Commands::IDLE}

};

// =======================
// PUBLIC STATIC VARIABLES
// =======================

// ===============
// PRIVATE METHODS
// ===============

//
// Convert any lowercase characters in string to upper.
//

inline std::string CMailIMAPParse::stringToUpper(const std::string& lineStr) {

    std::string upperCase(lineStr);
    for (auto &c : upperCase) c = std::toupper(c);
    return (upperCase);

}

//
// Perform case-insensitive string compare (return true strings are equal, false otherwise)
//

inline bool CMailIMAPParse::stringEqual(const std::string& lineStr, const std::string& compareStr) {

    int cnt01 = 0;
    if (lineStr.length() < compareStr.length()) return (false);
    for (auto &c : compareStr) if (std::toupper(c) != std::toupper(lineStr[cnt01++])) return (false);
    return (true);

}

//
// Extract the contents between two delimeters in command response line.
//

inline std::string CMailIMAPParse::extractBetween(const std::string& lineStr, const char first, const char last) {
    int firstDel = lineStr.find_first_of(first);
    int lastDel = lineStr.find_first_of(last, firstDel);
    return (lineStr.substr(firstDel + 1, (lastDel - firstDel - 1)));
}

//
// Extract string between two occurrences of a delimeter character (ie. number and spaces).
//

inline std::string CMailIMAPParse::extractBetweenDelimeter(const std::string& lineStr, const char delimeter) {

    int firstDel = lineStr.find_first_of(delimeter) + 1;
    int lastDel = lineStr.find_first_of(delimeter, firstDel);
    return (lineStr.substr(firstDel, lastDel - firstDel));

}

//
// Extract number that may follow an un-tagged command response.
//

inline std::string CMailIMAPParse::extractUntaggedNumber(const std::string& lineStr) {

    int startNumber = lineStr.find_first_not_of(' ', 1);
    int endNumber = lineStr.find_first_of(' ', startNumber);
    return (lineStr.substr(startNumber, endNumber - startNumber));

}

//
// Extract tag from command response line.
//

inline std::string CMailIMAPParse::extractTag(const std::string& lineStr) {
    return (lineStr.substr(0, lineStr.find_first_of(' ')));
}

//
// Extract command string from command line. If the command has the UID 
// prefix then this is skipped over.
//

inline std::string CMailIMAPParse::extractCommand(const std::string& lineStr) {

    int startOfCommand = lineStr.find_first_of(' ') + 1;
    int endOfCommand = lineStr.find_first_of(' ', startOfCommand);

    if (stringEqual(lineStr.substr(startOfCommand, endOfCommand - startOfCommand), CMailIMAP::kUIDStr)) {
        startOfCommand = lineStr.find_first_of(' ', startOfCommand) + 1;
        endOfCommand = lineStr.find_first_of(' ', startOfCommand);
    }

    return (stringToUpper(lineStr.substr(startOfCommand, endOfCommand - startOfCommand)));

}

//
// Extract list  from command response line. Note: only check until 
// the end of line; the first character in linsStr is the start 
// of the list ie. a '('.
//

inline std::string CMailIMAPParse::extractList(const std::string& lineStr) {

    int bracketCount = 0, currentIndex = 0, lineLength = lineStr.length();

    do {
        if (lineStr[currentIndex] == '(') bracketCount++;
        if (lineStr[currentIndex] == ')') bracketCount--;
        currentIndex++;
    } while (bracketCount && (--lineLength > 0));

    return (lineStr.substr(0, currentIndex));

}

//
// Parse item/number pair in command response and add to response map. Note the current line is 
// updated to remove the pair and also this function is only used in FETCH command parse as the 
// response is processed over multiple lines and not line by line.
//

void CMailIMAPParse::parseNumber(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr) {
    int numberPos=0;
    std::string numberStr;
    lineStr = lineStr.substr(itemStr.length()+1);
    while(std::isdigit(lineStr[numberPos])) {
        numberStr.append(1,lineStr[numberPos++]);
    }
    lineStr = lineStr.substr(numberStr.length());
    fetchData.responseMap.insert({itemStr, numberStr});

}

//
// Parse item/string pair in response and add to response map.Note the current line is 
// updated to remove the pair and also this function is only used in FETCH command parse as the 
// response is processed over multiple lines and not line by line.

void CMailIMAPParse::parseString(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr) {
    std::string quotedString;
    lineStr = lineStr.substr(lineStr.find(itemStr) + itemStr.length() + 1);
    quotedString = "\"" + extractBetweenDelimeter(lineStr, '\"') + "\"";
    lineStr = lineStr.substr(quotedString.length());
    fetchData.responseMap.insert({itemStr, quotedString});

}

//
// Parse item list in response and add to response map.Note the current line is 
// updated to remove the pair and also this function is only used in FETCH 
// command parse as the response is processed over multiple lines and not line 
// by line.
//

void CMailIMAPParse::parseList(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr) {

    std::string list;
    lineStr = lineStr.substr(lineStr.find(itemStr) + itemStr.length() + 1);
    list = extractList(lineStr);
    lineStr = lineStr.substr(list.length());
    fetchData.responseMap.insert({itemStr, list});

}

//
// Parse item octet string in response and add to response map. This involves decoding 
// the octet string length and reading the string into a buffer, leaving line containing 
// the next part of the command response to be processed. Note: The command response before
// the octet string is used as the first item when inserting the octet string into the 
// response map to distinguish between multiple octet fetches that might occur.
//

void CMailIMAPParse::parseOctets(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr, std::istringstream& responseStream) {

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
// Parse command response status and return response. At present an un-tagged BAD/NO gets 
// sent to std::cerr. Note: Any optional string that the server provides with a status is 
// stored away in the aresponse for use by the caller.
//

CMailIMAPParse::BASERESPONSE CMailIMAPParse::parseStatus(const std::string& tagStr, const std::string& lineStr) {

    CMailIMAPParse::BASERESPONSE resp{ new CMailIMAPParse::BaseResponse};

    if (stringEqual(lineStr, tagStr + " " + CMailIMAP::kOKStr)) {
        resp->status = RespCode::OK;
        resp->errorMessage = "";

    } else if (stringEqual(lineStr, tagStr + " " + CMailIMAP::kNOStr)) {
        resp->status = RespCode::NO;
        resp->errorMessage = lineStr;

    } else if (stringEqual(lineStr, tagStr + " " + CMailIMAP::kBADStr)) {
        resp->status = RespCode::BAD;
        resp->errorMessage = lineStr;

    } else if (stringEqual(lineStr, CMailIMAP::kUntaggedStr + " " + CMailIMAP::kBYEStr)) {
        resp->status = RespCode::BAD;
        resp->errorMessage = lineStr;

    } else if ((stringEqual(lineStr, CMailIMAP::kUntaggedStr + " " + CMailIMAP::kNOStr))
            || (stringEqual(lineStr, CMailIMAP::kUntaggedStr + " " + CMailIMAP::kBADStr))) {
        std::cerr << lineStr << std::endl;

    } else {
        std::cerr << "UKNOWN RESPONSE LINE = [" << lineStr << "]" << std::endl;
    }

    return (resp);

}

//
// Parse SELECT/EXAMINE Response. Note: The mailbox name is extracted from the 
// command line and used when decoding the response to find the mailbox access 
// privileges (READ-ONLY or READ-WRITE).
//

CMailIMAPParse::BASERESPONSE CMailIMAPParse::parseSELECT(CMailIMAPParse::CommandData& commandData) {

    CMailIMAPParse::SELECTRESPONSE resp{ new CMailIMAPParse::SelectResponse};

    resp->command = CMailIMAPParse::stringToCodeMap[commandData.commandStr];

    // Extract mailbox name from command (stripping any quotes).

    resp->mailBoxName = commandData.commandLineStr.substr(commandData.commandLineStr.find_last_of(' ') + 1);
    if (resp->mailBoxName.back() == '\"') resp->mailBoxName.pop_back();
    if (resp->mailBoxName.front() == '\"') resp->mailBoxName = resp->mailBoxName.substr(1);

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        lineStr.pop_back(); // Remove linefeed

        if (stringEqual(lineStr, CMailIMAP::kUntaggedStr + " " + CMailIMAP::kOKStr + " [")) {
            lineStr = extractBetween(lineStr, '[', ']');
        }

        if (stringEqual(lineStr, CMailIMAP::kUntaggedStr + " " + CMailIMAP::kFLAGSStr)) {
            resp->responseMap.insert({CMailIMAP::kFLAGSStr, extractList(lineStr.substr(lineStr.find_first_of('(')))});

        } else if (stringEqual(lineStr, CMailIMAP::kPERMANENTFLAGSStr)) {
            resp->responseMap.insert({CMailIMAP::kPERMANENTFLAGSStr, extractList(lineStr.substr(lineStr.find_first_of('(')))});

        } else if (stringEqual(lineStr, CMailIMAP::kUIDVALIDITYStr)) {
            resp->responseMap.insert({CMailIMAP::kUIDVALIDITYStr, lineStr.substr(lineStr.find_first_of(' ') + 1)});

        } else if (stringEqual(lineStr, CMailIMAP::kUIDNEXTStr)) {
            resp->responseMap.insert({CMailIMAP::kUIDNEXTStr, lineStr.substr(lineStr.find_first_of(' ') + 1)});

        } else if (stringEqual(lineStr, CMailIMAP::kHIGHESTMODSEQStr)) {
            resp->responseMap.insert({CMailIMAP::kHIGHESTMODSEQStr, lineStr.substr(lineStr.find_first_of(' ') + 1)});

        } else if (stringEqual(lineStr, CMailIMAP::kUntaggedStr + " " + CMailIMAP::kCAPABILITYStr)) {
            lineStr = lineStr.substr((std::string(CMailIMAP::kUntaggedStr + " " + CMailIMAP::kCAPABILITYStr).length()) + 1);
            resp->responseMap.insert({CMailIMAP::kCAPABILITYStr, lineStr});

        } else if (lineStr.find(CMailIMAP::kUNSEENStr) == 0) {
            resp->responseMap.insert({CMailIMAP::kUNSEENStr, lineStr.substr(lineStr.find_first_of(' ') + 1)});

        } else if (lineStr.find(CMailIMAP::kEXISTSStr) != std::string::npos) {
            resp->responseMap.insert({CMailIMAP::kEXISTSStr, extractUntaggedNumber(lineStr)});

        } else if (lineStr.find(CMailIMAP::kRECENTStr) != std::string::npos) {
            resp->responseMap.insert({CMailIMAP::kRECENTStr, extractUntaggedNumber(lineStr)});

        } else if ((lineStr.find("] " + resp->mailBoxName) != std::string::npos) ||
                (lineStr.find("] " + commandData.commandStr + " completed.") != std::string::npos)) {
            resp->mailBoxAccess = extractBetween(lineStr, '[', ']');

        } else {
            CMailIMAPParse::BASERESPONSE responseStatus = parseStatus(commandData.tagStr, lineStr);
            resp->status = responseStatus->status;
            resp->errorMessage = responseStatus->errorMessage;
        }

    }

    return (static_cast<CMailIMAPParse::BASERESPONSE> (std::move(resp)));

}

//
// Parse SEARCH Response.
//

CMailIMAPParse::BASERESPONSE CMailIMAPParse::parseSEARCH(CMailIMAPParse::CommandData& commandData) {

    CMailIMAPParse::SEARCHRESPONSE resp{ new CMailIMAPParse::SearchResponse};

    resp->command = CMailIMAPParse::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        lineStr.pop_back(); // Remove linefeed

        if (stringEqual(lineStr, CMailIMAP::kUntaggedStr + " " + commandData.commandStr)) {

            lineStr = lineStr.substr(lineStr.find_first_of(' ') + 1);
            lineStr = lineStr.substr(lineStr.find_first_of(' ') + 1);

            std::istringstream listStream(lineStr); // Read indexes/UIDs
            while (listStream.good()) {
                uint64_t index;
                listStream >> index;
                resp->indexes.push_back(index);
            }

        } else {
            CMailIMAPParse::BASERESPONSE responseStatus = parseStatus(commandData.tagStr, lineStr);
            resp->status = responseStatus->status;
            resp->errorMessage = responseStatus->errorMessage;
        }

    }

    return (static_cast<CMailIMAPParse::BASERESPONSE> (std::move(resp)));

}

//
// Parse LIST/LSUB Response.
//

CMailIMAPParse::BASERESPONSE CMailIMAPParse::parseLIST(CMailIMAPParse::CommandData& commandData) {

    CMailIMAPParse::LISTRESPONSE resp{ new CMailIMAPParse::ListResponse};

    resp->command = CMailIMAPParse::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        CMailIMAPParse::ListRespData mailBoxEntry;

        lineStr.pop_back(); // Remove linefeed

        if (stringEqual(lineStr, CMailIMAP::kUntaggedStr + " " + commandData.commandStr)) {
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
            CMailIMAPParse::BASERESPONSE responseStatus = parseStatus(commandData.tagStr, lineStr);
            resp->status = responseStatus->status;
            resp->errorMessage = responseStatus->errorMessage;
        }

    }

    return (static_cast<CMailIMAPParse::BASERESPONSE> (std::move(resp)));

}

//
// Parse STATUS Response.
//

CMailIMAPParse::BASERESPONSE CMailIMAPParse::parseSTATUS(CMailIMAPParse::CommandData& commandData) {

    CMailIMAPParse::STATUSRESPONSE resp{ new CMailIMAPParse::StatusResponse};

    resp->command = CMailIMAPParse::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        lineStr.pop_back(); // Remove linefeed

        if (stringEqual(lineStr, CMailIMAP::kUntaggedStr + " " + commandData.commandStr)) {

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
            CMailIMAPParse::BASERESPONSE responseStatus = parseStatus(commandData.tagStr, lineStr);
            resp->status = responseStatus->status;
            resp->errorMessage = responseStatus->errorMessage;
        }

    }

    return (static_cast<CMailIMAPParse::BASERESPONSE> (std::move(resp)));

}

//
// Parse EXPUNGE Response.
//

CMailIMAPParse::BASERESPONSE CMailIMAPParse::parseEXPUNGE(CMailIMAPParse::CommandData& commandData) {

    CMailIMAPParse::EXPUNGERESPONSE resp{ new CMailIMAPParse::ExpungeResponse};

    resp->command = CMailIMAPParse::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        lineStr.pop_back(); // Remove linefeed

        if (lineStr.find(CMailIMAP::kEXISTSStr) != std::string::npos) {
            lineStr = extractUntaggedNumber(lineStr);
            resp->exists.push_back(std::strtoull(lineStr.c_str(), nullptr, 10));

        } else if (lineStr.find(CMailIMAP::kEXPUNGEStr) != std::string::npos) {
            lineStr = extractUntaggedNumber(lineStr);
            resp->expunged.push_back(std::strtoull(lineStr.c_str(), nullptr, 10));

        } else {
            CMailIMAPParse::BASERESPONSE responseStatus = parseStatus(commandData.tagStr, lineStr);
            resp->status = responseStatus->status;
            resp->errorMessage = responseStatus->errorMessage;
        }

    }

    return (static_cast<CMailIMAPParse::BASERESPONSE> (std::move(resp)));

}

//
// Parse STORE Response.
//

CMailIMAPParse::BASERESPONSE CMailIMAPParse::parseSTORE(CMailIMAPParse::CommandData& commandData) {

    CMailIMAPParse::STORERESPONSE resp{ new CMailIMAPParse::StoreResponse};

    resp->command = CMailIMAPParse::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        StoreRespData storeData;

        lineStr.pop_back(); // Remove linefeed

        if (lineStr.find(CMailIMAP::kFETCHStr) != std::string::npos) {
            storeData.flags = "(" + extractBetween(lineStr.substr(lineStr.find_first_of('(') + 1), '(', ')') + ")";
            storeData.index = std::strtoull(extractUntaggedNumber(lineStr).c_str(), nullptr, 10);
            resp->storeList.push_back(std::move(storeData));

        } else {
            CMailIMAPParse::BASERESPONSE responseStatus = parseStatus(commandData.tagStr, lineStr);
            resp->status = responseStatus->status;
            resp->errorMessage = responseStatus->errorMessage;
        }

    }

    return (static_cast<CMailIMAPParse::BASERESPONSE> (std::move(resp)));

}

//
// Parse CAPABILITY Response.
//

CMailIMAPParse::BASERESPONSE CMailIMAPParse::parseCAPABILITY(CMailIMAPParse::CommandData& commandData) {

    CMailIMAPParse::CAPABILITYRESPONSE resp{ new CMailIMAPParse::CapabilityResponse};

    resp->command = CMailIMAPParse::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        lineStr.pop_back(); // Remove linefeed

        if (stringEqual(lineStr, CMailIMAP::kUntaggedStr + " " + CMailIMAP::kCAPABILITYStr)) {
            lineStr = lineStr.substr(lineStr.find_first_of(' ') + 1);
            resp->capabilityList = lineStr.substr(lineStr.find_first_of(' ') + 1);

        } else {
            CMailIMAPParse::BASERESPONSE responseStatus = parseStatus(commandData.tagStr, lineStr);
            resp->status = responseStatus->status;
            resp->errorMessage = responseStatus->errorMessage;
        }

    }

    return (static_cast<CMailIMAPParse::BASERESPONSE> (std::move(resp)));

}

//
// Parse NOOP/IDLE Response.
//

CMailIMAPParse::BASERESPONSE CMailIMAPParse::parseNOOP(CMailIMAPParse::CommandData& commandData) {

    CMailIMAPParse::NOOPRESPONSE resp{ new CMailIMAPParse::NoOpResponse};

    resp->command = CMailIMAPParse::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        lineStr.pop_back(); // Remove linefeed

        if (lineStr.find(CMailIMAP::kUntaggedStr) == 0) {
            resp->rawResponse.push_back(std::move(lineStr));
        } else {
            CMailIMAPParse::BASERESPONSE responseStatus = parseStatus(commandData.tagStr, lineStr);
            resp->status = responseStatus->status;
            resp->errorMessage = responseStatus->errorMessage;
        }

    }

    return (static_cast<CMailIMAPParse::BASERESPONSE> (std::move(resp)));

}

//
// Parse FETCH Response
//

CMailIMAPParse::BASERESPONSE CMailIMAPParse::parseFETCH(CMailIMAPParse::CommandData& commandData) {

    CMailIMAPParse::FETCHRESPONSE resp{ new CMailIMAPParse::FetchResponse};

    resp->command = CMailIMAPParse::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        FetchRespData fetchData;

        lineStr.pop_back(); // Add back '\n' as '\r\n' will be part of octet count

        if (lineStr.find(CMailIMAP::kFETCHStr + " (") != std::string::npos) {

            fetchData.index = std::strtoull(extractUntaggedNumber(lineStr).c_str(), nullptr, 10);
            lineStr = lineStr.substr(lineStr.find_first_of('(') + 1);

            bool endOfFetch = false;

            do {

                if (stringEqual(lineStr, CMailIMAP::kBODYSTRUCTUREStr + " ")) {
                    parseList(CMailIMAP::kBODYSTRUCTUREStr, fetchData, lineStr);
                } else if (stringEqual(lineStr, CMailIMAP::kENVELOPEStr + " ")) {
                    parseList(CMailIMAP::kENVELOPEStr, fetchData, lineStr);
                } else if (stringEqual(lineStr, CMailIMAP::kFLAGSStr + " ")) {
                    parseList(CMailIMAP::kFLAGSStr, fetchData, lineStr);
                } else if (stringEqual(lineStr, CMailIMAP::kBODYStr + " ")) {
                    parseList(CMailIMAP::kBODYStr, fetchData, lineStr);
                } else if (stringEqual(lineStr, CMailIMAP::kINTERNALDATEStr + " ")) {
                    parseString(CMailIMAP::kINTERNALDATEStr, fetchData, lineStr);
                } else if (stringEqual(lineStr, CMailIMAP::kRFC822SIZEStr + " ")) {
                    parseNumber(CMailIMAP::kRFC822SIZEStr, fetchData, lineStr);
                } else if (stringEqual(lineStr, CMailIMAP::kUIDStr + " ")) {
                    parseNumber(CMailIMAP::kUIDStr, fetchData, lineStr);
                } else if (stringEqual(lineStr, CMailIMAP::kRFC822HEADERStr + " ")) {
                    parseOctets(CMailIMAP::kRFC822HEADERStr, fetchData, lineStr, commandData.commandRespStream);
                } else if (stringEqual(lineStr, CMailIMAP::kBODYStr + "[")) {
                    parseOctets(CMailIMAP::kBODYStr, fetchData, lineStr, commandData.commandRespStream);
                } else if (stringEqual(lineStr, CMailIMAP::kRFC822Str + " ")) {
                    parseOctets(CMailIMAP::kRFC822Str, fetchData, lineStr, commandData.commandRespStream);
                } else {
                    std::cerr << "Error parsing" << std::endl; // throw here ?
                }

                lineStr = lineStr.substr(lineStr.find_first_not_of(' '));
                if (lineStr[0] == ')') {
                    endOfFetch = true;
                } else if (lineStr.length() == CMailIMAP::kEOLStr.length() - 1) {
                    std::getline(commandData.commandRespStream, lineStr, '\n');
                }

            } while (!endOfFetch);

            resp->fetchList.push_back(std::move(fetchData));

        } else {
            CMailIMAPParse::BASERESPONSE responseStatus = parseStatus(commandData.tagStr, lineStr);
            resp->status = responseStatus->status;
            resp->errorMessage = responseStatus->errorMessage;
        }


    }

    return (static_cast<CMailIMAPParse::BASERESPONSE> (std::move(resp)));

}

//
// Parse LOGOUT Response
//

CMailIMAPParse::BASERESPONSE CMailIMAPParse::parseLOGOUT(CMailIMAPParse::CommandData& commandData) {

    CMailIMAPParse::LOGOUTRESPONSE resp{ new CMailIMAPParse::LogOutResponse};

    resp->command = CMailIMAPParse::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        lineStr.pop_back(); // Remove linefeed

        if (stringEqual(lineStr, CMailIMAP::kUntaggedStr + " " + CMailIMAP::kBYEStr)) {
            continue;
        } else {
            CMailIMAPParse::BASERESPONSE responseStatus = parseStatus(commandData.tagStr, lineStr);
            resp->status = responseStatus->status;
            resp->errorMessage = responseStatus->errorMessage;
        }
    }

    return (static_cast<CMailIMAPParse::BASERESPONSE> (std::move(resp)));

}

//
// Default Parse Response
//

CMailIMAPParse::BASERESPONSE CMailIMAPParse::parseDefault(CMailIMAPParse::CommandData& commandData) {

    CMailIMAPParse::BASERESPONSE resp{ new CMailIMAPParse::BaseResponse};

    resp->command = CMailIMAPParse::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {
        lineStr.pop_back(); // Remove lineStrfeed
        CMailIMAPParse::BASERESPONSE responseStatus = parseStatus(commandData.tagStr, lineStr);
        resp->status = responseStatus->status;
        resp->errorMessage = responseStatus->errorMessage;
    }

    return (static_cast<CMailIMAPParse::BASERESPONSE> (std::move(resp)));

}

// ==============
// PUBLIC METHODS
// ==============

//
// Parse Command Response. The response string is one long string containing "\r\n"s to
// signal end of lines. The string is read line by line converting the response to a istringstream 
// and using getline() and '\n' to signal the end of line character (except FETCH which is dealt 
// with differently as it has to cater for octet strings that can span multiple lines.
//

CMailIMAPParse::BASERESPONSE CMailIMAPParse::parseResponse(const std::string & commandResponseStr) {

    std::istringstream responseStream(commandResponseStr);
    std::string commandLineStr;

    std::getline(responseStream, commandLineStr, '\n');
    commandLineStr.pop_back();

    CMailIMAPParse::ParseFunction parseFn;
    CommandData commandData{ extractTag(commandLineStr), extractCommand(commandLineStr), commandLineStr, responseStream};

    parseFn = CMailIMAPParse::parseCommmandMap[commandData.commandStr];
    if (!parseFn) {
        CMailIMAPParse::parseCommmandMap[commandData.commandStr] = parseDefault;
        parseFn = parseDefault;
    }

    return (parseFn(commandData));

}

//
// Return string for IMAP command code
//

std::string CMailIMAPParse::commandCodeString(CMailIMAPParse::Commands commandCode) {

    for (auto commandEntry : CMailIMAPParse::stringToCodeMap) {
        if (commandEntry.second == commandCode) {
            return (commandEntry.first);
        }
    }

    CMailIMAPParse::Exception("commandCodeString() : Invalid command code.");

    return (""); // Never reached.

}
