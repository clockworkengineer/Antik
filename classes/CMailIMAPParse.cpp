#include "HOST.hpp"
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
// Description: A class to parse CMailIMAP command responses. It is designed 
// to expect syntactically correct commands from any server and not report any
// specific errors; but if any occur to report so through an exception exit 
// processing gracefully.
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
#include <cstring>
#include <future>

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
// IMAP command code to parse response mapping table
//

std::unordered_map<int, CMailIMAPParse::ParseFunction> CMailIMAPParse::parseCommmandMap
{
    { static_cast<int> (Commands::LIST), parseLIST },
    { static_cast<int> (Commands::LSUB), parseLIST },
    { static_cast<int> (Commands::SEARCH), parseSEARCH },
    { static_cast<int> (Commands::SELECT), parseSELECT },
    { static_cast<int> (Commands::EXAMINE), parseSELECT },
    { static_cast<int> (Commands::STATUS), parseSTATUS },
    { static_cast<int> (Commands::EXPUNGE), parseEXPUNGE },
    { static_cast<int> (Commands::STORE), parseSTORE },
    { static_cast<int> (Commands::CAPABILITY), parseCAPABILITY },
    { static_cast<int> (Commands::FETCH), parseFETCH },
    { static_cast<int> (Commands::NOOP), parseNOOP },
    { static_cast<int> (Commands::IDLE), parseNOOP },
    { static_cast<int> (Commands::LOGOUT), parseLOGOUT }
};

//
// IMAP command string to internal enum code map table.
//

std::unordered_map<std::string, CMailIMAPParse::Commands> CMailIMAPParse::stringToCodeMap
{
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
// Read next line to parse. If the stream is no longer good then throw an exception.
//

bool CMailIMAPParse::parseGetNextLine(std::istringstream& responseStream, std::string& lineStr) {

    if (responseStream.good()) {
        bool bLineRead = static_cast<bool> (std::getline(responseStream, lineStr, '\n'));
        if (bLineRead) lineStr.pop_back();
        return (bLineRead);
    } else {
        throw Exception("error parsing command response (run out of input).");
    }}

//
// Parse item/number pair in command response and add to response map. Note the current line is 
// updated to remove the pair and also this function is only used in FETCH command parse as the 
// response is processed over multiple lines and not line by line.
//

void CMailIMAPParse::parseNumber(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr) {
    int numberPos = 0;
    std::string numberStr;
    lineStr = lineStr.substr(itemStr.length() + 1);
    while (std::isdigit(lineStr[numberPos])) {
        numberStr.append(1, lineStr[numberPos++]);
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
    quotedString = "\"" + stringBetween(lineStr, '\"', '\"') + "\"";
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

    std::string listStr;
    lineStr = lineStr.substr(lineStr.find(itemStr) + itemStr.length() + 1);
    listStr = stringList(lineStr);
    lineStr = lineStr.substr(listStr.length());
    fetchData.responseMap.insert({itemStr, listStr});

}

//
// Parse item octet string in response and add to response map. This involves decoding 
// the octet string length and reading the string into a buffer, leaving line containing 
// the next part of the command response to be processed. Note: The command response before
// the octet string is used as the first item when inserting the octet string into the 
// response map to distinguish between multiple octet fetches that might occur.
//

void CMailIMAPParse::parseOctets(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr, std::istringstream& responseStream) {

    std::string octetStr, octectBuffer, commandLabel{lineStr};
    int numberOfOctets;

    octetStr = stringBetween(lineStr, '{', '}');
    numberOfOctets = std::strtoull(octetStr.c_str(), nullptr, 10);
    octectBuffer.resize(numberOfOctets);
    responseStream.read(&octectBuffer[0], numberOfOctets);
    parseGetNextLine(responseStream, lineStr);
    fetchData.responseMap.insert({commandLabel, octectBuffer});

}

//
// Parse command response common fields including status and return response. This may include
// un-tagged EXISTS/EXPUNGED/RECENT replies to the current command or server replies to changes
// in mailbox status.
//

void CMailIMAPParse::parseCommon(const std::string& tagStr, const std::string& lineStr, CommandResponse * resp) {


    if ((lineStr[0] == CMailIMAP::kUntaggedStr[0]) &&
            (lineStr.find(CMailIMAP::kRECENTStr) != std::string::npos)) {

        if (resp->responseMap.find(CMailIMAP::kRECENTStr) == resp->responseMap.end()) {
            resp->responseMap.insert({CMailIMAP::kRECENTStr, stringUntaggedNumber(lineStr)});
        } else {
            resp->responseMap[CMailIMAP::kRECENTStr] += " " + stringUntaggedNumber(lineStr);
        }

    } else if ((lineStr[0] == CMailIMAP::kUntaggedStr[0]) &&
            (lineStr.find(CMailIMAP::kEXISTSStr) != std::string::npos)) {

        if (resp->responseMap.find(CMailIMAP::kEXISTSStr) == resp->responseMap.end()) {
            resp->responseMap.insert({CMailIMAP::kEXISTSStr, stringUntaggedNumber(lineStr)});
        } else {
            resp->responseMap[CMailIMAP::kEXISTSStr] += " " + stringUntaggedNumber(lineStr);
        }

    } else if ((lineStr[0] == CMailIMAP::kUntaggedStr[0]) &&
            (lineStr.find(CMailIMAP::kEXPUNGEStr) != std::string::npos)) {

        if (resp->responseMap.find(CMailIMAP::kEXPUNGEStr) == resp->responseMap.end()) {
            resp->responseMap.insert({CMailIMAP::kEXPUNGEStr, stringUntaggedNumber(lineStr)});
        } else {
            resp->responseMap[CMailIMAP::kEXPUNGEStr] += " " + stringUntaggedNumber(lineStr);
        }

    } else if (stringEqual(lineStr, tagStr + " " + CMailIMAP::kOKStr)) {
        resp->status = RespCode::OK;

    } else if (stringEqual(lineStr, tagStr + " " + CMailIMAP::kNOStr)) {
        resp->status = RespCode::NO;
        resp->errorMessageStr = lineStr;

    } else if (stringEqual(lineStr, tagStr + " " + CMailIMAP::kBADStr)) {
        resp->status = RespCode::BAD;
        resp->errorMessageStr = lineStr;

    } else if (stringEqual(lineStr, static_cast<std::string> (CMailIMAP::kUntaggedStr) + " " + CMailIMAP::kBYEStr)) {
        if (!resp->bBYESent) {
            resp->bBYESent = true;
        }
        resp->errorMessageStr = lineStr;

    } else if ((stringEqual(lineStr, static_cast<std::string> (CMailIMAP::kUntaggedStr) + " " + CMailIMAP::kNOStr))
            || (stringEqual(lineStr, static_cast<std::string> (CMailIMAP::kUntaggedStr) + " " + CMailIMAP::kBADStr))) {
        std::cerr << lineStr << std::endl;

    } else if (stringEqual(lineStr, CMailIMAP::kUntaggedStr)) {
         std::cerr << "ERROR: un-handled response: " <<lineStr << std::endl; // WARN of any untagged that should be processed.

    } else {
        throw Exception("error while parsing IMAP command [" + lineStr + "]");
    }

}

//
// Parse SELECT/EXAMINE Response.
//

void CMailIMAPParse::parseSELECT(CommandData& commandData) {

    // Extract mailbox name from command (stripping any quotes).
    
    std::string mailBoxNameStr = commandData.commandLineStr.substr(commandData.commandLineStr.find_last_of(' ') + 1);
    if (mailBoxNameStr.back() == '\"') mailBoxNameStr.pop_back();
    if (mailBoxNameStr.front() == '\"') mailBoxNameStr = mailBoxNameStr.substr(1);
    
    commandData.resp->responseMap.insert({CMailIMAP::kMAILBOXNAMEStr, mailBoxNameStr});

    for (std::string lineStr; parseGetNextLine(commandData.commandRespStream, lineStr);) {

        if (stringEqual(lineStr, static_cast<std::string>(CMailIMAP::kUntaggedStr) + " " + CMailIMAP::kOKStr + " [")) {
            lineStr = stringBetween(lineStr, '[', ']');
        }

        if (stringEqual(lineStr, static_cast<std::string>(CMailIMAP::kUntaggedStr) + " " + CMailIMAP::kFLAGSStr)) {
            commandData.resp->responseMap.insert({CMailIMAP::kFLAGSStr, stringList(lineStr)});

        } else if (stringEqual(lineStr, CMailIMAP::kPERMANENTFLAGSStr)) {
            commandData.resp->responseMap.insert({CMailIMAP::kPERMANENTFLAGSStr, stringList(lineStr)});

        } else if (stringEqual(lineStr, CMailIMAP::kUIDVALIDITYStr)) {
            commandData.resp->responseMap.insert({CMailIMAP::kUIDVALIDITYStr, stringBetween(lineStr, ' ', ']')});

        } else if (stringEqual(lineStr, CMailIMAP::kUIDNEXTStr)) {
            commandData.resp->responseMap.insert({CMailIMAP::kUIDNEXTStr, stringBetween(lineStr, ' ', ']')});

        } else if (stringEqual(lineStr, CMailIMAP::kHIGHESTMODSEQStr)) {
            commandData.resp->responseMap.insert({CMailIMAP::kHIGHESTMODSEQStr, stringBetween(lineStr, ' ', ']')});

        } else if (stringEqual(lineStr, static_cast<std::string>(CMailIMAP::kUntaggedStr) + " " + CMailIMAP::kCAPABILITYStr)) {
            lineStr = lineStr.substr(((static_cast<std::string>(CMailIMAP::kUntaggedStr) + " " + CMailIMAP::kCAPABILITYStr).length()) + 1);
            commandData.resp->responseMap.insert({CMailIMAP::kCAPABILITYStr, lineStr});

        } else if (lineStr.find(CMailIMAP::kUNSEENStr) == 0) {
            commandData.resp->responseMap.insert({CMailIMAP::kUNSEENStr, stringBetween(lineStr, ' ', ']')});

        } else {
            parseCommon(commandData.tagStr, lineStr, static_cast<CommandResponse *> (commandData.resp.get()));
            if (commandData.resp->status == RespCode::OK) {
                commandData.resp->responseMap.insert({CMailIMAP::kMAILBOXACCESSStr,  stringBetween(lineStr, '[', ']')});
            }
        }

    }

}

//
// Parse SEARCH Response.
//

void CMailIMAPParse::parseSEARCH(CommandData& commandData) {

    for (std::string lineStr; parseGetNextLine(commandData.commandRespStream, lineStr);) {

        if (stringEqual(lineStr, static_cast<std::string>(CMailIMAP::kUntaggedStr) + " " + CMailIMAP::kSEARCHStr)) {

            lineStr = lineStr.substr((static_cast<std::string>(CMailIMAP::kUntaggedStr) + " " + CMailIMAP::kSEARCHStr).length());

            if (!lineStr.empty()) {
                std::istringstream listStream(lineStr); // Read indexes/UIDs
                while (listStream.good()) {
                    uint64_t index = 0;
                    listStream >> index;
                    if (!listStream.fail()) {
                        commandData.resp->indexes.push_back(index);
                    }
                }
            }

        } else {
            parseCommon(commandData.tagStr, lineStr, static_cast<CommandResponse *> (commandData.resp.get()));
        }

    }

}

//
// Parse LIST/LSUB Response.
//

void CMailIMAPParse::parseLIST(CommandData& commandData) {

    for (std::string lineStr; parseGetNextLine(commandData.commandRespStream, lineStr);) {

        ListRespData mailBoxEntry;

        if ((stringEqual(lineStr, static_cast<std::string>(CMailIMAP::kUntaggedStr) + " " + CMailIMAP::kLISTStr)) ||
                (stringEqual(lineStr, static_cast<std::string>(CMailIMAP::kUntaggedStr) + " " + CMailIMAP::kLSUBStr))) {
            mailBoxEntry.attributesStr = stringList(lineStr);
            mailBoxEntry.hierDel = stringBetween(lineStr, '\"', '\"').front();
            if (lineStr.back() != '\"') {
                mailBoxEntry.mailBoxNameStr = lineStr.substr(lineStr.find_last_of(' '));
            } else {
                lineStr.pop_back();
                mailBoxEntry.mailBoxNameStr = lineStr.substr(lineStr.find_last_of('\"'));
                mailBoxEntry.mailBoxNameStr += '\"';
            }
            commandData.resp->mailBoxList.push_back(std::move(mailBoxEntry));

        } else {
            parseCommon(commandData.tagStr, lineStr, static_cast<CommandResponse *> (commandData.resp.get()));
        }

    }

}

//
// Parse STATUS Response.
//

void CMailIMAPParse::parseSTATUS(CommandData& commandData) {

    for (std::string lineStr; parseGetNextLine(commandData.commandRespStream, lineStr);) {

        if (stringEqual(lineStr, static_cast<std::string>(CMailIMAP::kUntaggedStr) + " " + CMailIMAP::kSTATUSStr)) {

            lineStr = lineStr.substr((static_cast<std::string>(CMailIMAP::kUntaggedStr) + " " + CMailIMAP::kSTATUSStr).length() + 1);
            
            commandData.resp->responseMap.insert({CMailIMAP::kMAILBOXNAMEStr,  lineStr.substr(0, lineStr.find_first_of(' '))});
   
            lineStr = stringBetween(lineStr, '(', ')');

            if (!lineStr.empty()) {
                std::istringstream listStream(lineStr);
                while (listStream.good()) {
                    std::string itemStr, valueStr;
                    listStream >> itemStr >> valueStr;
                    if (!listStream.fail()) {
                        commandData.resp->responseMap.insert({itemStr, valueStr});
                    }
                }
            }

        } else {
            parseCommon(commandData.tagStr, lineStr, static_cast<CommandResponse *> (commandData.resp.get()));
        }

    }

}

//
// Parse EXPUNGE Response.
//

void CMailIMAPParse::parseEXPUNGE(CommandData& commandData) {

    for (std::string lineStr; parseGetNextLine(commandData.commandRespStream, lineStr);) {
        parseCommon(commandData.tagStr, lineStr, static_cast<CommandResponse *> (commandData.resp.get()));
    }

}

//
// Parse STORE Response.
//

void CMailIMAPParse::parseSTORE(CommandData& commandData) {

    for (std::string lineStr; parseGetNextLine(commandData.commandRespStream, lineStr);) {

        StoreRespData storeData;

        if (lineStr.find(CMailIMAP::kFETCHStr) != std::string::npos) {
            storeData.index = std::strtoull(stringUntaggedNumber(lineStr).c_str(), nullptr, 10);
            storeData.flagsListStr = stringList(stringList(lineStr).substr(1));
            commandData.resp->storeList.push_back(std::move(storeData));

        } else {
            parseCommon(commandData.tagStr, lineStr, static_cast<CommandResponse *> (commandData.resp.get()));
        }

    }

}

//
// Parse CAPABILITY Response.
//

void CMailIMAPParse::parseCAPABILITY(CommandData& commandData) {

    for (std::string lineStr; parseGetNextLine(commandData.commandRespStream, lineStr);) {

        if (stringEqual(lineStr, static_cast<std::string>(CMailIMAP::kUntaggedStr) + " " + CMailIMAP::kCAPABILITYStr)) {
            commandData.resp->responseMap.insert({ CMailIMAP::kCAPABILITYStr,  lineStr.substr((static_cast<std::string>(CMailIMAP::kUntaggedStr) + " " + CMailIMAP::kCAPABILITYStr).length() + 1)});
        } else {
            parseCommon(commandData.tagStr, lineStr, static_cast<CommandResponse *> (commandData.resp.get()));
        }

    }

}

//
// Parse NOOP/IDLE Response.
//

void CMailIMAPParse::parseNOOP(CommandData& commandData) {

    for (std::string lineStr; parseGetNextLine(commandData.commandRespStream, lineStr);) {
        parseCommon(commandData.tagStr, lineStr, static_cast<CommandResponse *> (commandData.resp.get()));
    }

}

//
// Parse FETCH Response
//

void CMailIMAPParse::parseFETCH(CommandData& commandData) {

    for (std::string lineStr; parseGetNextLine(commandData.commandRespStream, lineStr);) {

        FetchRespData fetchData;

        int lineLength = lineStr.length() + std::strlen(CMailIMAP::kEOLStr);

        if (lineStr.find(static_cast<std::string>(CMailIMAP::kFETCHStr) + " (") != std::string::npos) {

            fetchData.index = std::strtoull(stringUntaggedNumber(lineStr).c_str(), nullptr, 10);
            lineStr = lineStr.substr(lineStr.find_first_of('(') + 1);

            bool endOfFetch = false;

            do {

                if (stringEqual(lineStr, static_cast<std::string>(CMailIMAP::kBODYSTRUCTUREStr) + " ")) {
                    parseList(CMailIMAP::kBODYSTRUCTUREStr, fetchData, lineStr);
                } else if (stringEqual(lineStr, static_cast<std::string>(CMailIMAP::kENVELOPEStr) + " ")) {
                    parseList(CMailIMAP::kENVELOPEStr, fetchData, lineStr);
                } else if (stringEqual(lineStr, static_cast<std::string>(CMailIMAP::kFLAGSStr) + " ")) {
                    parseList(CMailIMAP::kFLAGSStr, fetchData, lineStr);
                } else if (stringEqual(lineStr, static_cast<std::string>(CMailIMAP::kBODYStr) + " ")) {
                    parseList(CMailIMAP::kBODYStr, fetchData, lineStr);
                } else if (stringEqual(lineStr, static_cast<std::string>(CMailIMAP::kINTERNALDATEStr) + " ")) {
                    parseString(CMailIMAP::kINTERNALDATEStr, fetchData, lineStr);
                } else if (stringEqual(lineStr, static_cast<std::string>(CMailIMAP::kRFC822SIZEStr) + " ")) {
                    parseNumber(CMailIMAP::kRFC822SIZEStr, fetchData, lineStr);
                } else if (stringEqual(lineStr, static_cast<std::string>(CMailIMAP::kUIDStr) + " ")) {
                    parseNumber(CMailIMAP::kUIDStr, fetchData, lineStr);
                } else if (stringEqual(lineStr, static_cast<std::string>(CMailIMAP::kRFC822HEADERStr) + " ")) {
                    parseOctets(CMailIMAP::kRFC822HEADERStr, fetchData, lineStr, commandData.commandRespStream);
                } else if (stringEqual(lineStr, static_cast<std::string>(CMailIMAP::kBODYStr) + "[")) {
                    parseOctets(CMailIMAP::kBODYStr, fetchData, lineStr, commandData.commandRespStream);
                } else if (stringEqual(lineStr, static_cast<std::string>(CMailIMAP::kRFC822Str) + " ")) {
                    parseOctets(CMailIMAP::kRFC822Str, fetchData, lineStr, commandData.commandRespStream);
                } else {
                    throw Exception("error while parsing FETCH command [" + lineStr + "]");
                }

                // Still data to process

                if (lineStr.length() != 0) {
                    lineStr = lineStr.substr(lineStr.find_first_not_of(' ')); // Next non space.
                    if (lineStr[0] == ')') { // End of FETCH List
                        endOfFetch = true;
                    } else if (lineStr.length() == std::strlen(CMailIMAP::kEOLStr) - 1) { // No data left on line
                        parseGetNextLine(commandData.commandRespStream, lineStr); // Move to next
                    }
                } else {
                    commandData.commandRespStream.seekg(-lineLength, std::ios_base::cur); // Rewind read to get line
                    parseGetNextLine(commandData.commandRespStream, lineStr);
                    throw Exception("error while parsing FETCH command [" + lineStr + "]");
                }


            } while (!endOfFetch);

            commandData.resp->fetchList.push_back(std::move(fetchData));

        } else {
            parseCommon(commandData.tagStr, lineStr, static_cast<CommandResponse *> (commandData.resp.get()));
        }

    }

}

//
// Parse LOGOUT Response
//

void CMailIMAPParse::parseLOGOUT(CommandData& commandData) {

    for (std::string lineStr; parseGetNextLine(commandData.commandRespStream, lineStr);) {
        parseCommon(commandData.tagStr, lineStr, static_cast<CommandResponse *> (commandData.resp.get()));
    }

}

//
// Default Parse Response
//

void CMailIMAPParse::parseDefault(CommandData& commandData) {

    for (std::string lineStr; parseGetNextLine(commandData.commandRespStream, lineStr);) {
        parseCommon(commandData.tagStr, lineStr, static_cast<CommandResponse *> (commandData.resp.get()));
    }

}

// ==============
// PUBLIC METHODS
// ==============

//
// Convert any lowercase characters in string to upper.
//

std::string CMailIMAPParse::stringToUpper(const std::string& lineStr) {

    std::string upperCaseStr(lineStr);
    for (auto &c : upperCaseStr) c = std::toupper(c);
    return (upperCaseStr);

}

//
// Perform case-insensitive string compare (return true strings are equal, false otherwise)
//

bool CMailIMAPParse::stringEqual(const std::string& lineStr, const std::string& compareStr) {

    int cnt01 = 0;
    if (lineStr.length() < compareStr.length()) return (false);
    for (auto &c : compareStr) if (std::toupper(c) != std::toupper(lineStr[cnt01++])) return (false);
    return (true);

}

//
// Extract the contents between two delimeters in command response line.
//

std::string CMailIMAPParse::stringBetween(const std::string& lineStr, const char first, const char last) {
    int firstDel = lineStr.find_first_of(first);
    int lastDel = lineStr.find_first_of(last, firstDel + 1);
    return (lineStr.substr(firstDel + 1, (lastDel - firstDel - 1)));
}

//
// Extract number that may follow an un-tagged command response.
//

std::string CMailIMAPParse::stringUntaggedNumber(const std::string& lineStr) {

    int startNumber = lineStr.find_first_not_of(' ', 1);
    int endNumber = lineStr.find_first_of(' ', startNumber);
    return (lineStr.substr(startNumber, endNumber - startNumber));

}

//
// Extract tag from command response line.
//

std::string CMailIMAPParse::stringTag(const std::string& lineStr) {
    return (lineStr.substr(0, lineStr.find_first_of(' ')));
}

//
// Extract command string from command line. If the command has the UID 
// prefix then this is skipped over.
//

std::string CMailIMAPParse::stringCommand(const std::string& lineStr) {

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

std::string CMailIMAPParse::stringList(const std::string& lineStr) {

    int bracketCount = 0, startPosition = 0, currentIndex = 0, lineLength = lineStr.length();

    startPosition = lineStr.find_first_of('(');
    lineLength -= startPosition;

    currentIndex = startPosition;
    do {
        if (lineStr[currentIndex] == '(') bracketCount++;
        if (lineStr[currentIndex] == ')') bracketCount--;
        currentIndex++;
    } while (bracketCount && (--lineLength > 0));

    return (lineStr.substr(startPosition, currentIndex - startPosition));

}

//
// Parse Command Response. The response string is one long string containing "\r\n"s to
// signal end of lines. The string is read line by line converting the response to a istringstream 
// and using getline() and '\n' to signal the end of line character (except FETCH which is dealt 
// with differently as it has to cater for octet strings that can span multiple lines.
//

CMailIMAPParse::COMMANDRESPONSE CMailIMAPParse::parseResponse(const std::string & commandResponseStr) {

    std::istringstream responseStream(commandResponseStr);
    std::string commandLineStr;

    parseGetNextLine(responseStream, commandLineStr);

    ParseFunction parseFn;
    CommandData commandData{ stringTag(commandLineStr),
        stringToCodeMap[stringCommand(commandLineStr)],
        commandLineStr, responseStream};

    commandData.resp.reset({ new CommandResponse { commandData.commandCode}});
          
    parseFn = parseCommmandMap[static_cast<int> (commandData.commandCode)];
    if (!parseFn) {
        parseCommmandMap[static_cast<int> (commandData.commandCode)] = parseDefault;
        parseFn = parseDefault;
    }

    parseFn(commandData);
    
    return (std::move(commandData.resp));

}

//
// Return string for IMAP command code
//

std::string CMailIMAPParse::commandCodeString(Commands commandCode) {

    for (auto commandEntry : stringToCodeMap) {
        if (commandEntry.second == commandCode) {
            return (commandEntry.first);
        }
    }

    Exception("commandCodeString() : Invalid command code.");

    return (""); // Never reached.

}
