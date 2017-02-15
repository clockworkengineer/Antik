/*
 * File:   CMailIMAPDecode.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on January 24, 2017, 2:33 PM
 *
 * Copyright 2016.
 *
 */

//
// Class: CMailIMAPDecode
// 
// Description: A class to decode CMailIMAP command responses. It has
// purely static data and functions and cannot be instantiated (ie. it contains 
// no constructor/destructor.
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
#include "CMailIMAPDecode.hpp"

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
// IMAP command string to decode response mapping table
//

std::unordered_map<std::string, CMailIMAPDecode::DecodeFunction> CMailIMAPDecode::decodeCommmandMap ={

    {CMailIMAP::kLISTStr, decodeLIST},
    {CMailIMAP::kLSUBStr, decodeLIST},
    {CMailIMAP::kSEARCHStr, decodeSEARCH},
    {CMailIMAP::kSELECTStr, decodeSELECT},
    {CMailIMAP::kEXAMINEStr, decodeSELECT},
    {CMailIMAP::kSTATUSStr, decodeSTATUS},
    {CMailIMAP::kEXPUNGEStr, decodeEXPUNGE},
    {CMailIMAP::kSTOREStr, decodeSTORE},
    {CMailIMAP::kCAPABILITYStr, decodeCAPABILITY},
    {CMailIMAP::kFETCHStr, decodeFETCH},
    {CMailIMAP::kNOOPStr, decodeNOOP},
    {CMailIMAP::kIDLEStr, decodeNOOP}

};

//
// IMAP command string to internal enum code map table
//

std::unordered_map<std::string, CMailIMAPDecode::Commands> CMailIMAPDecode::stringToCodeMap = {
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

inline std::string CMailIMAPDecode::stringToUpper(const std::string& lineStr) {
    
    std::string upperCase(lineStr);
    for (auto  &c: upperCase) c = std::toupper(c);
    return(upperCase);
    
}

//
// Perform case-insensitive string compare (return true strings are equal, false otherwise)
//

inline bool CMailIMAPDecode::stringEqual(const std::string& lineStr, const std::string& compareStr) {
    
    int cnt01=0;
    if (lineStr.length() < compareStr.length()) return (false);
    for (auto  &c: compareStr) if (std::toupper(c) != std::toupper(lineStr[cnt01++])) return (false);
    return(true);
    
}

//
// Extract the contents between two delimeters in command response line.
//

inline std::string CMailIMAPDecode::extractBetween(const std::string& lineStr, const char first, const char last) {
    int firstDel = lineStr.find_first_of(first);
    int lastDel = lineStr.find_first_of(last, firstDel);
    return (lineStr.substr(firstDel + 1, (lastDel - firstDel - 1)));
}

//
// Extract string between two occurrences of a delimeter character (ie. number and spaces).
//

inline std::string CMailIMAPDecode::extractBetweenDelimeter(const std::string& lineStr, const char delimeter) {

    int firstDel = lineStr.find_first_of(delimeter) + 1;
    int lastDel = lineStr.find_first_of(delimeter, firstDel);
    return (lineStr.substr(firstDel, lastDel - firstDel));

}

//
// Extract number that may follow an un-tagged command response.
//

inline std::string CMailIMAPDecode::extractUntaggedNumber(const std::string& lineStr) {

    int startNumber = lineStr.find_first_not_of(' ', 1);
    int endNumber = lineStr.find_first_of(' ', startNumber);
    return (lineStr.substr(startNumber, endNumber - startNumber));

}

//
// Extract tag from command response line.
//

inline std::string CMailIMAPDecode::extractTag(const std::string& lineStr) {
    return (lineStr.substr(0, lineStr.find_first_of(' ')));
}

//
// Extract command string from command line. If the command has the UID 
// prefix then this is skipped over.
//

inline std::string CMailIMAPDecode::extractCommand(const std::string& lineStr) {

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

inline std::string CMailIMAPDecode::extractList(const std::string& lineStr) {

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

void CMailIMAPDecode::decodeNumber(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr) {
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

void CMailIMAPDecode::decodeString(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr) {
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

void CMailIMAPDecode::decodeList(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr) {

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

void CMailIMAPDecode::decodeOctets(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr, std::istringstream& responseStream) {

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

void CMailIMAPDecode::decodeStatus (const std::string& tagStr, const std::string& lineStr, CMailIMAPDecode::BASERESPONSE resp) {

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


}

//
// Decode SELECT/EXAMINE Response. Note: The mailbox name is extracted from the 
// command line and used when decoding the response to find the mailbox access 
// privileges (READ-ONLY or READ-WRITE).
//

CMailIMAPDecode::BASERESPONSE CMailIMAPDecode::decodeSELECT(CMailIMAPDecode::CommandData& commandData) {

    CMailIMAPDecode::SELECTRESPONSE resp{ new CMailIMAPDecode::SelectResponse};

    resp->command = CMailIMAPDecode::stringToCodeMap[commandData.commandStr];

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
            decodeStatus(commandData.tagStr, lineStr, resp);
        }

    }

    return (static_cast<CMailIMAPDecode::BASERESPONSE> (resp));

}

//
// Decode SEARCH Response.
//

CMailIMAPDecode::BASERESPONSE CMailIMAPDecode::decodeSEARCH(CMailIMAPDecode::CommandData& commandData) {

    CMailIMAPDecode::SEARCHRESPONSE resp{ new CMailIMAPDecode::SearchResponse};

    resp->command = CMailIMAPDecode::stringToCodeMap[commandData.commandStr];

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
            decodeStatus(commandData.tagStr, lineStr, resp);
        }

    }

    return (static_cast<CMailIMAPDecode::BASERESPONSE> (resp));

}

//
// Decode LIST/LSUB Response.
//

CMailIMAPDecode::BASERESPONSE CMailIMAPDecode::decodeLIST(CMailIMAPDecode::CommandData& commandData) {

    CMailIMAPDecode::LISTRESPONSE resp{ new CMailIMAPDecode::ListResponse};

    resp->command = CMailIMAPDecode::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        CMailIMAPDecode::ListRespData mailBoxEntry;

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
            decodeStatus(commandData.tagStr, lineStr, resp);
        }

    }

    return (static_cast<CMailIMAPDecode::BASERESPONSE> (resp));


}

//
// Decode STATUS Response.
//

CMailIMAPDecode::BASERESPONSE CMailIMAPDecode::decodeSTATUS(CMailIMAPDecode::CommandData& commandData) {

    CMailIMAPDecode::STATUSRESPONSE resp{ new CMailIMAPDecode::StatusResponse};

    resp->command = CMailIMAPDecode::stringToCodeMap[commandData.commandStr];

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
            decodeStatus(commandData.tagStr, lineStr, resp);
        }

    }

    return (static_cast<CMailIMAPDecode::BASERESPONSE> (resp));

}

//
// Decode EXPUNGE Response.
//

CMailIMAPDecode::BASERESPONSE CMailIMAPDecode::decodeEXPUNGE(CMailIMAPDecode::CommandData& commandData) {

    CMailIMAPDecode::EXPUNGERESPONSE resp{ new CMailIMAPDecode::ExpungeResponse};

    resp->command = CMailIMAPDecode::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        lineStr.pop_back(); // Remove linefeed

        if (lineStr.find(CMailIMAP::kEXISTSStr) != std::string::npos) {
            lineStr = extractUntaggedNumber(lineStr);
            resp->exists.push_back(std::strtoull(lineStr.c_str(), nullptr, 10));

        } else if (lineStr.find(CMailIMAP::kEXPUNGEStr) != std::string::npos) {
            lineStr = extractUntaggedNumber(lineStr);
            resp->expunged.push_back(std::strtoull(lineStr.c_str(), nullptr, 10));

        } else {
            decodeStatus(commandData.tagStr, lineStr, resp);
        }

    }

    return (static_cast<CMailIMAPDecode::BASERESPONSE> (resp));

}

//
// Decode STORE Response.
//

CMailIMAPDecode::BASERESPONSE CMailIMAPDecode::decodeSTORE(CMailIMAPDecode::CommandData& commandData) {

    CMailIMAPDecode::STORERESPONSE resp{ new CMailIMAPDecode::StoreResponse};

    resp->command = CMailIMAPDecode::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        StoreRespData storeData;

        lineStr.pop_back(); // Remove linefeed

        if (lineStr.find(CMailIMAP::kFETCHStr) != std::string::npos) {
            storeData.flags = "(" + extractBetween(lineStr.substr(lineStr.find_first_of('(') + 1), '(', ')') + ")";
            storeData.index = std::strtoull(extractUntaggedNumber(lineStr).c_str(), nullptr, 10);
            resp->storeList.push_back(std::move(storeData));

        } else {
            decodeStatus(commandData.tagStr, lineStr, resp);
        }

    }

    return (static_cast<CMailIMAPDecode::BASERESPONSE> (resp));

}

//
// Decode CAPABILITY Response.
//

CMailIMAPDecode::BASERESPONSE CMailIMAPDecode::decodeCAPABILITY(CMailIMAPDecode::CommandData& commandData) {

    CMailIMAPDecode::CAPABILITYRESPONSE resp{ new CMailIMAPDecode::CapabilityResponse};

    resp->command = CMailIMAPDecode::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        lineStr.pop_back(); // Remove linefeed

        if (stringEqual(lineStr, CMailIMAP::kUntaggedStr + " " + CMailIMAP::kCAPABILITYStr)) {
            lineStr = lineStr.substr(lineStr.find_first_of(' ') + 1);
            resp->capabilityList = lineStr.substr(lineStr.find_first_of(' ') + 1);

        } else {
            decodeStatus(commandData.tagStr, lineStr, resp);
        }

    }

    return (static_cast<CMailIMAPDecode::BASERESPONSE> (resp));

}

//
// Decode NOOP/IDLE Response.
//

CMailIMAPDecode::BASERESPONSE CMailIMAPDecode::decodeNOOP(CMailIMAPDecode::CommandData& commandData) {

    CMailIMAPDecode::NOOPRESPONSE resp{ new CMailIMAPDecode::NoOpResponse};

    resp->command = CMailIMAPDecode::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        lineStr.pop_back(); // Remove linefeed

        if (lineStr.find(CMailIMAP::kUntaggedStr) == 0) {
            resp->rawResponse.push_back(std::move(lineStr));
        } else {
            decodeStatus(commandData.tagStr, lineStr, resp);
        }

    }

    return (static_cast<CMailIMAPDecode::BASERESPONSE> (resp));

}

//
// Decode FETCH Response
//

CMailIMAPDecode::BASERESPONSE CMailIMAPDecode::decodeFETCH(CMailIMAPDecode::CommandData& commandData) {

    CMailIMAPDecode::FETCHRESPONSE resp{ new CMailIMAPDecode::FetchResponse};

    resp->command = CMailIMAPDecode::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        FetchRespData fetchData;

        lineStr.pop_back(); // Add back '\n' as '\r\n' will be part of octet count

        if (lineStr.find(CMailIMAP::kFETCHStr + " (") != std::string::npos) {

            fetchData.index = std::strtoull(extractUntaggedNumber(lineStr).c_str(), nullptr, 10);
            lineStr = lineStr.substr(lineStr.find_first_of('(') + 1);

            bool endOfFetch = false;

            do {

                if (stringEqual(lineStr, CMailIMAP::kBODYSTRUCTUREStr + " ")) {
                    decodeList(CMailIMAP::kBODYSTRUCTUREStr, fetchData, lineStr);
                } else if (stringEqual(lineStr, CMailIMAP::kENVELOPEStr + " ")) {
                    decodeList(CMailIMAP::kENVELOPEStr, fetchData, lineStr);
                } else if (stringEqual(lineStr, CMailIMAP::kFLAGSStr + " ")) {
                    decodeList(CMailIMAP::kFLAGSStr, fetchData, lineStr);
                } else if (stringEqual(lineStr, CMailIMAP::kBODYStr + " ")) {
                    decodeList(CMailIMAP::kBODYStr, fetchData, lineStr);
                } else if (stringEqual(lineStr, CMailIMAP::kINTERNALDATEStr + " ")) {
                    decodeString(CMailIMAP::kINTERNALDATEStr, fetchData, lineStr);
                } else if (stringEqual(lineStr, CMailIMAP::kRFC822SIZEStr + " ")) {
                    decodeNumber(CMailIMAP::kRFC822SIZEStr, fetchData, lineStr);
                } else if (stringEqual(lineStr, CMailIMAP::kUIDStr + " ")) {
                    decodeNumber(CMailIMAP::kUIDStr, fetchData, lineStr);
                } else if (stringEqual(lineStr, CMailIMAP::kRFC822HEADERStr + " ")) {
                    decodeOctets(CMailIMAP::kRFC822HEADERStr, fetchData, lineStr, commandData.commandRespStream);
                } else if (stringEqual(lineStr, CMailIMAP::kBODYStr + "[")) {
                    decodeOctets(CMailIMAP::kBODYStr, fetchData, lineStr, commandData.commandRespStream);
                } else if (stringEqual(lineStr, CMailIMAP::kRFC822Str + " ")) {
                    decodeOctets(CMailIMAP::kRFC822Str, fetchData, lineStr, commandData.commandRespStream);
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
            decodeStatus(commandData.tagStr, lineStr, resp);
        }


    }

    return (static_cast<CMailIMAPDecode::BASERESPONSE> (resp));

}

//
// Decode LOGOUT Response
//

CMailIMAPDecode::BASERESPONSE CMailIMAPDecode::decodeLOGOUT(CMailIMAPDecode::CommandData& commandData) {

    CMailIMAPDecode::LOGOUTRESPONSE resp{ new CMailIMAPDecode::LogOutResponse};

    resp->command = CMailIMAPDecode::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {

        lineStr.pop_back(); // Remove linefeed

        if (stringEqual(lineStr, CMailIMAP::kUntaggedStr + " " + CMailIMAP::kBYEStr)) {
            continue;
        } else {
            decodeStatus(commandData.tagStr, lineStr, resp);
        }
    }

    return (static_cast<CMailIMAPDecode::BASERESPONSE> (resp));

}

//
// Default Decode Response
//

CMailIMAPDecode::BASERESPONSE CMailIMAPDecode::decodeDefault(CMailIMAPDecode::CommandData& commandData) {

    CMailIMAPDecode::BASERESPONSE resp{ new CMailIMAPDecode::BaseResponse};

    resp->command = CMailIMAPDecode::stringToCodeMap[commandData.commandStr];

    for (std::string lineStr; std::getline(commandData.commandRespStream, lineStr, '\n');) {
        lineStr.pop_back(); // Remove lineStrfeed
        decodeStatus(commandData.tagStr, lineStr, resp);
    }

    return (static_cast<CMailIMAPDecode::BASERESPONSE> (resp));

}

// ==============
// PUBLIC METHODS
// ==============

//
// Decode Command Response. The response string is one long string containing "\r\n"s to
// signal end of lines. The string is read line by line converting the response to a istringstream 
// and using getline() and '\n' to signal the end of line character (except FETCH which is dealt 
// with differently as it has to cater for octet strings that can span multiple lines.
//

CMailIMAPDecode::BASERESPONSE CMailIMAPDecode::decodeResponse(const std::string & commandResponseStr) {

    std::istringstream responseStream(commandResponseStr);
    std::string commandLineStr;
    
    std::getline(responseStream, commandLineStr, '\n');
    commandLineStr.pop_back();
    
    CMailIMAPDecode::DecodeFunction decodeFn;
    CommandData commandData{ extractTag(commandLineStr), extractCommand(commandLineStr), commandLineStr, responseStream};

    decodeFn = CMailIMAPDecode::decodeCommmandMap[commandData.commandStr];
    if (!decodeFn) {
        CMailIMAPDecode::decodeCommmandMap[commandData.commandStr]=decodeDefault;
        decodeFn = decodeDefault;
    }

    return (decodeFn(commandData));

}

//
// Return string for IMAP command code
//

std::string CMailIMAPDecode::commandCodeString(CMailIMAPDecode::Commands commandCode) {

    for (auto commandEntry : CMailIMAPDecode::stringToCodeMap) {
        if (commandEntry.second == commandCode) {
            return (commandEntry.first);
        }
    }

    CMailIMAPDecode::Exception("commandCodeString() : Invalid command code.");

    return (""); // Never reached.

}
