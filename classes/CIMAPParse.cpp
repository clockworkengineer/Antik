#include "HOST.hpp"
/*
 * File:   CIMAPParse.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on January 24, 2017, 2:33 PM
 *
 * Copyright 2016.
 *
 */

//
// Class: CIMAPParse
// 
// Description: A class to parse CIMAP command responses. It is designed 
// to expect syntactically correct command responses from a server and 
// not report specific errors; but if any occur to report so through 
// an exception.
//
// NOTE: IMAP commands sent can be in any combination of case (upper/lower) and 
// this is mirrored back in the response. So perform case-insensitive compares 
// for any commands in responses.
//
// Dependencies:   C11++     - Language standard features used.

//

// =================
// CLASS DEFINITIONS
// =================

#include "CIMAP.hpp"
#include "CIMAPParse.hpp"

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

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace Mail {

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

        std::unordered_map<int, CIMAPParse::ParseFunction> CIMAPParse::parseCommmandMap
        {
            { static_cast<int> (Commands::LIST), parseLIST},
            { static_cast<int> (Commands::LSUB), parseLIST},
            { static_cast<int> (Commands::SEARCH), parseSEARCH},
            { static_cast<int> (Commands::SELECT), parseSELECT},
            { static_cast<int> (Commands::EXAMINE), parseSELECT},
            { static_cast<int> (Commands::STATUS), parseSTATUS},
            { static_cast<int> (Commands::EXPUNGE), parseEXPUNGE},
            { static_cast<int> (Commands::STORE), parseSTORE},
            { static_cast<int> (Commands::CAPABILITY), parseCAPABILITY},
            { static_cast<int> (Commands::FETCH), parseFETCH},
            { static_cast<int> (Commands::NOOP), parseNOOP},
            { static_cast<int> (Commands::IDLE), parseNOOP},
            { static_cast<int> (Commands::LOGOUT), parseLOGOUT}};

        //
        // IMAP command string to internal enum code map table.
        //

        std::unordered_map<std::string, CIMAPParse::Commands> CIMAPParse::stringToCodeMap
        {
            { CIMAP::kSTARTTLSStr, Commands::STARTTLS},
            { CIMAP::kAUTHENTICATEStr, Commands::AUTHENTICATE},
            { CIMAP::kLOGINStr, Commands::LOGIN},
            { CIMAP::kCAPABILITYStr, Commands::CAPABILITY},
            { CIMAP::kSELECTStr, Commands::SELECT},
            { CIMAP::kEXAMINEStr, Commands::EXAMINE},
            { CIMAP::kCREATEStr, Commands::CREATE},
            { CIMAP::kDELETEStr, Commands::DELETE},
            { CIMAP::kRENAMEStr, Commands::RENAME},
            { CIMAP::kSUBSCRIBEStr, Commands::SUBSCRIBE},
            { CIMAP::kUNSUBSCRIBEStr, Commands::UNSUBSCRIBE},
            { CIMAP::kLISTStr, Commands::LIST},
            { CIMAP::kLSUBStr, Commands::LSUB},
            { CIMAP::kSTATUSStr, Commands::STATUS},
            { CIMAP::kAPPENDStr, Commands::APPEND},
            { CIMAP::kCHECKStr, Commands::CHECK},
            { CIMAP::kCLOSEStr, Commands::CLOSE},
            { CIMAP::kEXPUNGEStr, Commands::EXPUNGE},
            { CIMAP::kSEARCHStr, Commands::SEARCH},
            { CIMAP::kFETCHStr, Commands::FETCH},
            { CIMAP::kSTOREStr, Commands::STORE},
            { CIMAP::kCOPYStr, Commands::COPY},
            { CIMAP::kUIDStr, Commands::UID},
            { CIMAP::kNOOPStr, Commands::NOOP},
            { CIMAP::kLOGOUTStr, Commands::LOGOUT},
            { CIMAP::kIDLEStr, Commands::IDLE}};

        // =======================
        // PUBLIC STATIC VARIABLES
        // =======================

        // ===============
        // PRIVATE METHODS
        // ===============

        //
        // Read next line to parse. If the stream is no longer good then throw an exception.
        //

        bool CIMAPParse::parseGetNextLine(std::istringstream& responseStream, std::string& lineStr) {

            if (responseStream.good()) {
                bool bLineRead = static_cast<bool> (std::getline(responseStream, lineStr, '\n'));
                if (bLineRead) lineStr.pop_back();
                return (bLineRead);
            } else {
                throw Exception("error parsing command response (run out of input).");
            }
        }

        //
        // Parse item/number pair in command response and add to response map. Note the current line is 
        // updated to remove the pair and also this function is only used in FETCH command parse as the 
        // response is processed over multiple lines and not line by line.
        //

        void CIMAPParse::parseNumber(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr) {
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

        void CIMAPParse::parseString(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr) {
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

        void CIMAPParse::parseList(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr) {

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

        void CIMAPParse::parseOctets(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr, std::istringstream& responseStream) {

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

        void CIMAPParse::parseCommon(const std::string& tagStr, const std::string& lineStr, CommandResponse * resp) {


            if ((lineStr[0] == CIMAP::kUntaggedStr[0]) &&
                    (lineStr.find(CIMAP::kRECENTStr) != std::string::npos)) {

                if (resp->responseMap.find(CIMAP::kRECENTStr) == resp->responseMap.end()) {
                    resp->responseMap.insert({CIMAP::kRECENTStr, stringUntaggedNumber(lineStr)});
                } else {
                    resp->responseMap[CIMAP::kRECENTStr] += " " + stringUntaggedNumber(lineStr);
                }

            } else if ((lineStr[0] == CIMAP::kUntaggedStr[0]) &&
                    (lineStr.find(CIMAP::kEXISTSStr) != std::string::npos)) {

                if (resp->responseMap.find(CIMAP::kEXISTSStr) == resp->responseMap.end()) {
                    resp->responseMap.insert({CIMAP::kEXISTSStr, stringUntaggedNumber(lineStr)});
                } else {
                    resp->responseMap[CIMAP::kEXISTSStr] += " " + stringUntaggedNumber(lineStr);
                }

            } else if ((lineStr[0] == CIMAP::kUntaggedStr[0]) &&
                    (lineStr.find(CIMAP::kEXPUNGEStr) != std::string::npos)) {

                if (resp->responseMap.find(CIMAP::kEXPUNGEStr) == resp->responseMap.end()) {
                    resp->responseMap.insert({CIMAP::kEXPUNGEStr, stringUntaggedNumber(lineStr)});
                } else {
                    resp->responseMap[CIMAP::kEXPUNGEStr] += " " + stringUntaggedNumber(lineStr);
                }

            } else if (stringEqual(lineStr, tagStr + " " + CIMAP::kOKStr)) {
                resp->status = RespCode::OK;

            } else if (stringEqual(lineStr, tagStr + " " + CIMAP::kNOStr)) {
                resp->status = RespCode::NO;
                resp->errorMessageStr = lineStr;

            } else if (stringEqual(lineStr, tagStr + " " + CIMAP::kBADStr)) {
                resp->status = RespCode::BAD;
                resp->errorMessageStr = lineStr;

            } else if (stringEqual(lineStr, static_cast<std::string> (CIMAP::kUntaggedStr) + " " + CIMAP::kBYEStr)) {
                if (!resp->bBYESent) {
                    resp->bBYESent = true;
                }
                resp->errorMessageStr = lineStr;

            } else if ((stringEqual(lineStr, static_cast<std::string> (CIMAP::kUntaggedStr) + " " + CIMAP::kNOStr))
                    || (stringEqual(lineStr, static_cast<std::string> (CIMAP::kUntaggedStr) + " " + CIMAP::kBADStr))) {
                std::cerr << lineStr << std::endl;

            } else if (stringEqual(lineStr, CIMAP::kUntaggedStr)) {
                std::cerr << "WARNING: un-handled response: " << lineStr << std::endl; // WARN of any untagged that should be processed.

            } else {
                throw Exception("error while parsing IMAP command [" + lineStr + "]");
            }

        }

        //
        // Parse SELECT/EXAMINE Response.
        //

        void CIMAPParse::parseSELECT(CommandData& commandData) {

            // Extract mailbox name from command (stripping any quotes).

            std::string mailBoxNameStr = commandData.commandLineStr.substr(commandData.commandLineStr.find_last_of(' ') + 1);
            if (mailBoxNameStr.back() == '\"') mailBoxNameStr.pop_back();
            if (mailBoxNameStr.front() == '\"') mailBoxNameStr = mailBoxNameStr.substr(1);

            commandData.resp->responseMap.insert({CIMAP::kMAILBOXNAMEStr, mailBoxNameStr});

            for (std::string lineStr; parseGetNextLine(commandData.commandRespStream, lineStr);) {

                if (stringEqual(lineStr, static_cast<std::string> (CIMAP::kUntaggedStr) + " " + CIMAP::kOKStr + " [")) {
                    lineStr = stringBetween(lineStr, '[', ']');
                }

                if (stringEqual(lineStr, static_cast<std::string> (CIMAP::kUntaggedStr) + " " + CIMAP::kFLAGSStr)) {
                    commandData.resp->responseMap.insert({CIMAP::kFLAGSStr, stringList(lineStr)});

                } else if (stringEqual(lineStr, CIMAP::kPERMANENTFLAGSStr)) {
                    commandData.resp->responseMap.insert({CIMAP::kPERMANENTFLAGSStr, stringList(lineStr)});

                } else if (stringEqual(lineStr, CIMAP::kUIDVALIDITYStr)) {
                    commandData.resp->responseMap.insert({CIMAP::kUIDVALIDITYStr, stringBetween(lineStr, ' ', ']')});

                } else if (stringEqual(lineStr, CIMAP::kUIDNEXTStr)) {
                    commandData.resp->responseMap.insert({CIMAP::kUIDNEXTStr, stringBetween(lineStr, ' ', ']')});

                } else if (stringEqual(lineStr, CIMAP::kHIGHESTMODSEQStr)) {
                    commandData.resp->responseMap.insert({CIMAP::kHIGHESTMODSEQStr, stringBetween(lineStr, ' ', ']')});

                } else if (stringEqual(lineStr, static_cast<std::string> (CIMAP::kUntaggedStr) + " " + CIMAP::kCAPABILITYStr)) {
                    lineStr = lineStr.substr(((static_cast<std::string> (CIMAP::kUntaggedStr) + " " + CIMAP::kCAPABILITYStr).length()) + 1);
                    commandData.resp->responseMap.insert({CIMAP::kCAPABILITYStr, lineStr});

                } else if (lineStr.find(CIMAP::kUNSEENStr) == 0) {
                    commandData.resp->responseMap.insert({CIMAP::kUNSEENStr, stringBetween(lineStr, ' ', ']')});

                } else {
                    parseCommon(commandData.tagStr, lineStr, static_cast<CommandResponse *> (commandData.resp.get()));
                    if (commandData.resp->status == RespCode::OK) {
                        commandData.resp->responseMap.insert({CIMAP::kMAILBOXACCESSStr, stringBetween(lineStr, '[', ']')});
                    }
                }

            }

        }

        //
        // Parse SEARCH Response.
        //

        void CIMAPParse::parseSEARCH(CommandData& commandData) {

            for (std::string lineStr; parseGetNextLine(commandData.commandRespStream, lineStr);) {

                if (stringEqual(lineStr, static_cast<std::string> (CIMAP::kUntaggedStr) + " " + CIMAP::kSEARCHStr)) {

                    lineStr = lineStr.substr((static_cast<std::string> (CIMAP::kUntaggedStr) + " " + CIMAP::kSEARCHStr).length());

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

        void CIMAPParse::parseLIST(CommandData& commandData) {

            for (std::string lineStr; parseGetNextLine(commandData.commandRespStream, lineStr);) {

                ListRespData mailBoxEntry;

                if ((stringEqual(lineStr, static_cast<std::string> (CIMAP::kUntaggedStr) + " " + CIMAP::kLISTStr)) ||
                        (stringEqual(lineStr, static_cast<std::string> (CIMAP::kUntaggedStr) + " " + CIMAP::kLSUBStr))) {
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

        void CIMAPParse::parseSTATUS(CommandData& commandData) {

            for (std::string lineStr; parseGetNextLine(commandData.commandRespStream, lineStr);) {

                if (stringEqual(lineStr, static_cast<std::string> (CIMAP::kUntaggedStr) + " " + CIMAP::kSTATUSStr)) {

                    lineStr = lineStr.substr((static_cast<std::string> (CIMAP::kUntaggedStr) + " " + CIMAP::kSTATUSStr).length() + 1);

                    commandData.resp->responseMap.insert({CIMAP::kMAILBOXNAMEStr, lineStr.substr(0, lineStr.find_first_of(' '))});

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

        void CIMAPParse::parseEXPUNGE(CommandData& commandData) {

            for (std::string lineStr; parseGetNextLine(commandData.commandRespStream, lineStr);) {
                parseCommon(commandData.tagStr, lineStr, static_cast<CommandResponse *> (commandData.resp.get()));
            }

        }

        //
        // Parse STORE Response.
        //

        void CIMAPParse::parseSTORE(CommandData& commandData) {

            for (std::string lineStr; parseGetNextLine(commandData.commandRespStream, lineStr);) {

                StoreRespData storeData;

                if (lineStr.find(CIMAP::kFETCHStr) != std::string::npos) {
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

        void CIMAPParse::parseCAPABILITY(CommandData& commandData) {

            for (std::string lineStr; parseGetNextLine(commandData.commandRespStream, lineStr);) {

                if (stringEqual(lineStr, static_cast<std::string> (CIMAP::kUntaggedStr) + " " + CIMAP::kCAPABILITYStr)) {
                    commandData.resp->responseMap.insert({CIMAP::kCAPABILITYStr, lineStr.substr((static_cast<std::string> (CIMAP::kUntaggedStr) + " " + CIMAP::kCAPABILITYStr).length() + 1)});
                } else {
                    parseCommon(commandData.tagStr, lineStr, static_cast<CommandResponse *> (commandData.resp.get()));
                }

            }

        }

        //
        // Parse NOOP/IDLE Response.
        //

        void CIMAPParse::parseNOOP(CommandData& commandData) {

            for (std::string lineStr; parseGetNextLine(commandData.commandRespStream, lineStr);) {
                parseCommon(commandData.tagStr, lineStr, static_cast<CommandResponse *> (commandData.resp.get()));
            }

        }

        //
        // Parse FETCH Response
        //

        void CIMAPParse::parseFETCH(CommandData& commandData) {

            for (std::string lineStr; parseGetNextLine(commandData.commandRespStream, lineStr);) {

                FetchRespData fetchData;

                int lineLength = lineStr.length() + std::strlen(CIMAP::kEOLStr);

                if (lineStr.find(static_cast<std::string> (CIMAP::kFETCHStr) + " (") != std::string::npos) {

                    fetchData.index = std::strtoull(stringUntaggedNumber(lineStr).c_str(), nullptr, 10);
                    lineStr = lineStr.substr(lineStr.find_first_of('(') + 1);

                    bool endOfFetch = false;

                    do {

                        if (stringEqual(lineStr, static_cast<std::string> (CIMAP::kBODYSTRUCTUREStr) + " ")) {
                            parseList(CIMAP::kBODYSTRUCTUREStr, fetchData, lineStr);
                        } else if (stringEqual(lineStr, static_cast<std::string> (CIMAP::kENVELOPEStr) + " ")) {
                            parseList(CIMAP::kENVELOPEStr, fetchData, lineStr);
                        } else if (stringEqual(lineStr, static_cast<std::string> (CIMAP::kFLAGSStr) + " ")) {
                            parseList(CIMAP::kFLAGSStr, fetchData, lineStr);
                        } else if (stringEqual(lineStr, static_cast<std::string> (CIMAP::kBODYStr) + " ")) {
                            parseList(CIMAP::kBODYStr, fetchData, lineStr);
                        } else if (stringEqual(lineStr, static_cast<std::string> (CIMAP::kINTERNALDATEStr) + " ")) {
                            parseString(CIMAP::kINTERNALDATEStr, fetchData, lineStr);
                        } else if (stringEqual(lineStr, static_cast<std::string> (CIMAP::kRFC822SIZEStr) + " ")) {
                            parseNumber(CIMAP::kRFC822SIZEStr, fetchData, lineStr);
                        } else if (stringEqual(lineStr, static_cast<std::string> (CIMAP::kUIDStr) + " ")) {
                            parseNumber(CIMAP::kUIDStr, fetchData, lineStr);
                        } else if (stringEqual(lineStr, static_cast<std::string> (CIMAP::kRFC822HEADERStr) + " ")) {
                            parseOctets(CIMAP::kRFC822HEADERStr, fetchData, lineStr, commandData.commandRespStream);
                        } else if (stringEqual(lineStr, static_cast<std::string> (CIMAP::kBODYStr) + "[")) {
                            parseOctets(CIMAP::kBODYStr, fetchData, lineStr, commandData.commandRespStream);
                        } else if (stringEqual(lineStr, static_cast<std::string> (CIMAP::kRFC822Str) + " ")) {
                            parseOctets(CIMAP::kRFC822Str, fetchData, lineStr, commandData.commandRespStream);
                        } else {
                            throw Exception("error while parsing FETCH command [" + lineStr + "]");
                        }

                        // Still data to process

                        if (lineStr.length() != 0) {
                            lineStr = lineStr.substr(lineStr.find_first_not_of(' ')); // Next non space.
                            if (lineStr[0] == ')') { // End of FETCH List
                                endOfFetch = true;
                            } else if (lineStr.length() == std::strlen(CIMAP::kEOLStr) - 1) { // No data left on line
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

        void CIMAPParse::parseLOGOUT(CommandData& commandData) {

            for (std::string lineStr; parseGetNextLine(commandData.commandRespStream, lineStr);) {
                parseCommon(commandData.tagStr, lineStr, static_cast<CommandResponse *> (commandData.resp.get()));
            }

        }

        //
        // Default Parse Response
        //

        void CIMAPParse::parseDefault(CommandData& commandData) {

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

        std::string CIMAPParse::stringToUpper(const std::string& lineStr) {

            std::string upperCaseStr(lineStr);
            for (auto &c : upperCaseStr) c = std::toupper(c);
            return (upperCaseStr);

        }

        //
        // Perform case-insensitive string compare (return true strings are equal, false otherwise)
        //

        bool CIMAPParse::stringEqual(const std::string& lineStr, const std::string& compareStr) {

            int cnt01 = 0;
            if (lineStr.length() < compareStr.length()) return (false);
            for (auto &c : compareStr) if (std::toupper(c) != std::toupper(lineStr[cnt01++])) return (false);
            return (true);

        }

        //
        // Extract the contents between two delimeters in command response line.
        //

        std::string CIMAPParse::stringBetween(const std::string& lineStr, const char first, const char last) {
            int firstDel = lineStr.find_first_of(first);
            int lastDel = lineStr.find_first_of(last, firstDel + 1);
            return (lineStr.substr(firstDel + 1, (lastDel - firstDel - 1)));
        }

        //
        // Extract number that may follow an un-tagged command response.
        //

        std::string CIMAPParse::stringUntaggedNumber(const std::string& lineStr) {

            int startNumber = lineStr.find_first_not_of(' ', 1);
            int endNumber = lineStr.find_first_of(' ', startNumber);
            return (lineStr.substr(startNumber, endNumber - startNumber));

        }

        //
        // Extract tag from command response line.
        //

        std::string CIMAPParse::stringTag(const std::string& lineStr) {
            return (lineStr.substr(0, lineStr.find_first_of(' ')));
        }

        //
        // Extract command string from command line. If the command has the UID 
        // prefix then this is skipped over.
        //

        std::string CIMAPParse::stringCommand(const std::string& lineStr) {

            int startOfCommand = lineStr.find_first_of(' ') + 1;
            int endOfCommand = lineStr.find_first_of(' ', startOfCommand);

            if (stringEqual(lineStr.substr(startOfCommand, endOfCommand - startOfCommand), CIMAP::kUIDStr)) {
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

        std::string CIMAPParse::stringList(const std::string& lineStr) {

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

        CIMAPParse::COMMANDRESPONSE CIMAPParse::parseResponse(const std::string & commandResponseStr) {

            std::istringstream responseStream(commandResponseStr);
            std::string commandLineStr;

            parseGetNextLine(responseStream, commandLineStr);

            ParseFunction parseFn;
            CommandData commandData{ stringTag(commandLineStr),
                stringToCodeMap[stringCommand(commandLineStr)],
                commandLineStr, responseStream};

            commandData.resp.reset({new CommandResponse
                { commandData.commandCode}});

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

        std::string CIMAPParse::commandCodeString(Commands commandCode) {

            for (auto commandEntry : stringToCodeMap) {
                if (commandEntry.second == commandCode) {
                    return (commandEntry.first);
                }
            }

            Exception("commandCodeString() : Invalid command code.");

            return (""); // Never reached.

        }

    } // namespace Mail
} // namespace Antik
