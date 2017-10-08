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
// C++ STL
//

#include <iostream>
#include <sstream>
#include <cstring>
#include <future>

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace IMAP {

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

        std::unordered_map<int, CIMAPParse::ParseFunction> CIMAPParse::m_parseCommmandMap
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

        std::unordered_map<std::string, CIMAPParse::Commands> CIMAPParse::m_stringToCodeMap
        {
            { kSTARTTLS, Commands::STARTTLS},
            { kAUTHENTICATE, Commands::AUTHENTICATE},
            { kLOGIN, Commands::LOGIN},
            { kCAPABILITY, Commands::CAPABILITY},
            { kSELECT, Commands::SELECT},
            { kEXAMINE, Commands::EXAMINE},
            { kCREATE, Commands::CREATE},
            { kDELETE, Commands::DELETE},
            { kRENAME, Commands::RENAME},
            { kSUBSCRIBE, Commands::SUBSCRIBE},
            { kUNSUBSCRIBE, Commands::UNSUBSCRIBE},
            { kLIST, Commands::LIST},
            { kLSUB, Commands::LSUB},
            { kSTATUS, Commands::STATUS},
            { kAPPEND, Commands::APPEND},
            { kCHECK, Commands::CHECK},
            { kCLOSE, Commands::CLOSE},
            { kEXPUNGE, Commands::EXPUNGE},
            { kSEARCH, Commands::SEARCH},
            { kFETCH, Commands::FETCH},
            { kSTORE, Commands::STORE},
            { kCOPY, Commands::COPY},
            { kUID, Commands::UID},
            { kNOOP, Commands::NOOP},
            { kLOGOUT, Commands::LOGOUT},
            { kIDLE, Commands::IDLE}};

        // =======================
        // PUBLIC STATIC VARIABLES
        // =======================

        // ===============
        // PRIVATE METHODS
        // ===============

        //
        // Read next line to parse. If the stream is no longer good then throw an exception.
        //

        bool CIMAPParse::parseGetNextLine(std::istringstream& responseeam, std::string& line) {

            if (responseeam.good()) {
                bool bLineRead = static_cast<bool> (std::getline(responseeam, line, '\n'));
                if (bLineRead) line.pop_back();
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

        void CIMAPParse::parseNumber(const std::string& item, FetchRespData& fetchData, std::string& line) {
            int numberPos { 0 };
            std::string number;
            line = line.substr(item.length() + 1);
            while (std::isdigit(line[numberPos])) {
                number.append(1, line[numberPos++]);
            }
            line = line.substr(number.length());
            fetchData.responseMap.insert({item, number});

        }

        //
        // Parse item/string pair in response and add to response map.Note the current line is 
        // updated to remove the pair and also this function is only used in FETCH command parse as the 
        // response is processed over multiple lines and not line by line.

        void CIMAPParse::parseing(const std::string& item, FetchRespData& fetchData, std::string& line) {
            std::string quoteding;
            line = line.substr(line.find(item) + item.length() + 1);
            quoteding = "\"" + stringBetween(line, '\"', '\"') + "\"";
            line = line.substr(quoteding.length());
            fetchData.responseMap.insert({item, quoteding});

        }

        //
        // Parse item list in response and add to response map.Note the current line is 
        // updated to remove the pair and also this function is only used in FETCH 
        // command parse as the response is processed over multiple lines and not line 
        // by line.
        //

        void CIMAPParse::parseList(const std::string& item, FetchRespData& fetchData, std::string& line) {

            std::string list;
            line = line.substr(line.find(item) + item.length() + 1);
            list = stringList(line);
            line = line.substr(list.length());
            fetchData.responseMap.insert({item, list});

        }

        //
        // Parse item octet string in response and add to response map. This involves decoding 
        // the octet string length and reading the string into a buffer, leaving line containing 
        // the next part of the command response to be processed. Note: The command response before
        // the octet string is used as the first item when inserting the octet string into the 
        // response map to distinguish between multiple octet fetches that might occur.
        //

        void CIMAPParse::parseOctets(const std::string& item, FetchRespData& fetchData, std::string& line, std::istringstream& responseeam) {

            std::string octet;
            std::string octectBuffer;
            std::string commandLabel { line };
            int numberOfOctets { 0 };

            octet = stringBetween(line, '{', '}');
            numberOfOctets = std::strtoull(octet.c_str(), nullptr, 10);
            octectBuffer.resize(numberOfOctets);
            responseeam.read(&octectBuffer[0], numberOfOctets);
            parseGetNextLine(responseeam, line);
            fetchData.responseMap.insert({commandLabel, octectBuffer});

        }

        //
        // Parse command response common fields including status and return response. This may include
        // un-tagged EXISTS/EXPUNGED/RECENT replies to the current command or server replies to changes
        // in mailbox status.
        //

        void CIMAPParse::parseCommon(const std::string& tag, const std::string& line, CommandResponse * resp) {


            if ((line[0] == kUntagged[0]) &&
                    (line.find(kRECENT) != std::string::npos)) {

                if (resp->responseMap.find(kRECENT) == resp->responseMap.end()) {
                    resp->responseMap.insert({kRECENT, stringUntaggedNumber(line)});
                } else {
                    resp->responseMap[kRECENT] += " " + stringUntaggedNumber(line);
                }

            } else if ((line[0] == kUntagged[0]) &&
                    (line.find(kEXISTS) != std::string::npos)) {

                if (resp->responseMap.find(kEXISTS) == resp->responseMap.end()) {
                    resp->responseMap.insert({kEXISTS, stringUntaggedNumber(line)});
                } else {
                    resp->responseMap[kEXISTS] += " " + stringUntaggedNumber(line);
                }

            } else if ((line[0] == kUntagged[0]) &&
                    (line.find(kEXPUNGE) != std::string::npos)) {

                if (resp->responseMap.find(kEXPUNGE) == resp->responseMap.end()) {
                    resp->responseMap.insert({kEXPUNGE, stringUntaggedNumber(line)});
                } else {
                    resp->responseMap[kEXPUNGE] += " " + stringUntaggedNumber(line);
                }

            } else if (stringEqual(line, tag + " " + kOK)) {
                resp->status = RespCode::OK;

            } else if (stringEqual(line, tag + " " + kNO)) {
                resp->status = RespCode::NO;
                resp->errorMessage = line;

            } else if (stringEqual(line, tag + " " + kBAD)) {
                resp->status = RespCode::BAD;
                resp->errorMessage = line;

            } else if (stringEqual(line, static_cast<std::string> (kUntagged) + " " + kBYE)) {
                if (!resp->bBYESent) {
                    resp->bBYESent = true;
                }
                resp->errorMessage = line;

            } else if ((stringEqual(line, static_cast<std::string> (kUntagged) + " " + kNO))
                    || (stringEqual(line, static_cast<std::string> (kUntagged) + " " + kBAD))) {
                std::cerr << line << std::endl;

            } else if (stringEqual(line, kUntagged)) {
                std::cerr << "WARNING: un-handled response: " << line << std::endl; // WARN of any untagged that should be processed.

            } else {
                throw Exception("error while parsing IMAP command [" + line + "]");
            }

        }

        //
        // Parse SELECT/EXAMINE Response.
        //

        void CIMAPParse::parseSELECT(CommandData& commandData) {

            // Extract mailbox name from command (stripping any quotes).

            std::string mailBoxName { commandData.commandLine.substr(commandData.commandLine.find_last_of(' ') + 1) };
            if (mailBoxName.back() == '\"') mailBoxName.pop_back();
            if (mailBoxName.front() == '\"') mailBoxName = mailBoxName.substr(1);

            commandData.resp->responseMap.insert({kMAILBOXNAME, mailBoxName});

            for (std::string line; parseGetNextLine(commandData.commandRespeam, line);) {

                if (stringEqual(line, static_cast<std::string> (kUntagged) + " " + kOK + " [")) {
                    line = stringBetween(line, '[', ']');
                }

                if (stringEqual(line, static_cast<std::string> (kUntagged) + " " + kFLAGS)) {
                    commandData.resp->responseMap.insert({kFLAGS, stringList(line)});

                } else if (stringEqual(line, kPERMANENTFLAGS)) {
                    commandData.resp->responseMap.insert({kPERMANENTFLAGS, stringList(line)});

                } else if (stringEqual(line, kUIDVALIDITY)) {
                    commandData.resp->responseMap.insert({kUIDVALIDITY, stringBetween(line, ' ', ']')});

                } else if (stringEqual(line, kUIDNEXT)) {
                    commandData.resp->responseMap.insert({kUIDNEXT, stringBetween(line, ' ', ']')});

                } else if (stringEqual(line, kHIGHESTMODSEQ)) {
                    commandData.resp->responseMap.insert({kHIGHESTMODSEQ, stringBetween(line, ' ', ']')});

                } else if (stringEqual(line, static_cast<std::string> (kUntagged) + " " + kCAPABILITY)) {
                    line = line.substr(((static_cast<std::string> (kUntagged) + " " + kCAPABILITY).length()) + 1);
                    commandData.resp->responseMap.insert({kCAPABILITY, line});

                } else if (line.find(kUNSEEN) == 0) {
                    commandData.resp->responseMap.insert({kUNSEEN, stringBetween(line, ' ', ']')});

                } else {
                    parseCommon(commandData.tag, line, static_cast<CommandResponse *> (commandData.resp.get()));
                    if (commandData.resp->status == RespCode::OK) {
                        commandData.resp->responseMap.insert({kMAILBOXACCESS, stringBetween(line, '[', ']')});
                    }
                }

            }

        }

        //
        // Parse SEARCH Response.
        //

        void CIMAPParse::parseSEARCH(CommandData& commandData) {

            for (std::string line; parseGetNextLine(commandData.commandRespeam, line);) {

                if (stringEqual(line, static_cast<std::string> (kUntagged) + " " + kSEARCH)) {

                    line = line.substr((static_cast<std::string> (kUntagged) + " " + kSEARCH).length());

                    if (!line.empty()) {
                        std::istringstream listeam(line); // Read indexes/UIDs
                        while (listeam.good()) {
                            std::uint64_t index = 0;
                            listeam >> index;
                            if (!listeam.fail()) {
                                commandData.resp->indexes.push_back(index);
                            }
                        }
                    }

                } else {
                    parseCommon(commandData.tag, line, static_cast<CommandResponse *> (commandData.resp.get()));
                }

            }

        }

        //
        // Parse LIST/LSUB Response.
        //

        void CIMAPParse::parseLIST(CommandData& commandData) {

            for (std::string line; parseGetNextLine(commandData.commandRespeam, line);) {

                ListRespData mailBoxEntry;

                if ((stringEqual(line, static_cast<std::string> (kUntagged) + " " + kLIST)) ||
                        (stringEqual(line, static_cast<std::string> (kUntagged) + " " + kLSUB))) {
                    mailBoxEntry.attributes = stringList(line);
                    mailBoxEntry.hierDel = stringBetween(line, '\"', '\"').front();
                    if (line.back() != '\"') {
                        mailBoxEntry.mailBoxName = line.substr(line.find_last_of(' ')+1);
                    } else {
                        line.pop_back();
                        mailBoxEntry.mailBoxName = line.substr(line.find_last_of('\"'));
                        mailBoxEntry.mailBoxName += '\"';
                    }
                    commandData.resp->mailBoxList.push_back(std::move(mailBoxEntry));

                } else {
                    parseCommon(commandData.tag, line, static_cast<CommandResponse *> (commandData.resp.get()));
                }

            }

        }

        //
        // Parse STATUS Response.
        //

        void CIMAPParse::parseSTATUS(CommandData& commandData) {

            for (std::string line; parseGetNextLine(commandData.commandRespeam, line);) {

                if (stringEqual(line, static_cast<std::string> (kUntagged) + " " + kSTATUS)) {

                    line = line.substr((static_cast<std::string> (kUntagged) + " " + kSTATUS).length() + 1);

                    commandData.resp->responseMap.insert({kMAILBOXNAME, line.substr(0, line.find_first_of(' '))});

                    line = stringBetween(line, '(', ')');

                    if (!line.empty()) {
                        std::istringstream listeam(line);
                        while (listeam.good()) {
                            std::string item, value;
                            listeam >> item >> value;
                            if (!listeam.fail()) {
                                commandData.resp->responseMap.insert({item, value});
                            }
                        }
                    }

                } else {
                    parseCommon(commandData.tag, line, static_cast<CommandResponse *> (commandData.resp.get()));
                }

            }

        }

        //
        // Parse EXPUNGE Response.
        //

        void CIMAPParse::parseEXPUNGE(CommandData& commandData) {

            for (std::string line; parseGetNextLine(commandData.commandRespeam, line);) {
                parseCommon(commandData.tag, line, static_cast<CommandResponse *> (commandData.resp.get()));
            }

        }

        //
        // Parse STORE Response.
        //

        void CIMAPParse::parseSTORE(CommandData& commandData) {

            for (std::string line; parseGetNextLine(commandData.commandRespeam, line);) {

                StoreRespData storeData;

                if (line.find(kFETCH) != std::string::npos) {
                    storeData.index = std::strtoull(stringUntaggedNumber(line).c_str(), nullptr, 10);
                    storeData.flagsList = stringList(stringList(line).substr(1));
                    commandData.resp->storeList.push_back(std::move(storeData));

                } else {
                    parseCommon(commandData.tag, line, static_cast<CommandResponse *> (commandData.resp.get()));
                }

            }

        }

        //
        // Parse CAPABILITY Response.
        //

        void CIMAPParse::parseCAPABILITY(CommandData& commandData) {

            for (std::string line; parseGetNextLine(commandData.commandRespeam, line);) {

                if (stringEqual(line, static_cast<std::string> (kUntagged) + " " + kCAPABILITY)) {
                    commandData.resp->responseMap.insert({kCAPABILITY, line.substr((static_cast<std::string> (kUntagged) + " " + kCAPABILITY).length() + 1)});
                } else {
                    parseCommon(commandData.tag, line, static_cast<CommandResponse *> (commandData.resp.get()));
                }

            }

        }

        //
        // Parse NOOP/IDLE Response.
        //

        void CIMAPParse::parseNOOP(CommandData& commandData) {

            for (std::string line; parseGetNextLine(commandData.commandRespeam, line);) {
                parseCommon(commandData.tag, line, static_cast<CommandResponse *> (commandData.resp.get()));
            }

        }

        //
        // Parse FETCH Response
        //

        void CIMAPParse::parseFETCH(CommandData& commandData) {

            for (std::string line; parseGetNextLine(commandData.commandRespeam, line);) {

                FetchRespData fetchData;

                int lineLength = line.length() + std::strlen(kEOL);

                if (line.find(static_cast<std::string> (kFETCH) + " (") != std::string::npos) {

                    fetchData.index = std::strtoull(stringUntaggedNumber(line).c_str(), nullptr, 10);
                    line = line.substr(line.find_first_of('(') + 1);

                    bool endOfFetch = false;

                    do {

                        if (stringEqual(line, static_cast<std::string> (kBODYSTRUCTURE) + " ")) {
                            parseList(kBODYSTRUCTURE, fetchData, line);
                        } else if (stringEqual(line, static_cast<std::string> (kENVELOPE) + " ")) {
                            parseList(kENVELOPE, fetchData, line);
                        } else if (stringEqual(line, static_cast<std::string> (kFLAGS) + " ")) {
                            parseList(kFLAGS, fetchData, line);
                        } else if (stringEqual(line, static_cast<std::string> (kBODY) + " ")) {
                            parseList(kBODY, fetchData, line);
                        } else if (stringEqual(line, static_cast<std::string> (kINTERNALDATE) + " ")) {
                            parseing(kINTERNALDATE, fetchData, line);
                        } else if (stringEqual(line, static_cast<std::string> (kRFC822SIZE) + " ")) {
                            parseNumber(kRFC822SIZE, fetchData, line);
                        } else if (stringEqual(line, static_cast<std::string> (kUID) + " ")) {
                            parseNumber(kUID, fetchData, line);
                        } else if (stringEqual(line, static_cast<std::string> (kRFC822HEADER) + " ")) {
                            parseOctets(kRFC822HEADER, fetchData, line, commandData.commandRespeam);
                        } else if (stringEqual(line, static_cast<std::string> (kBODY) + "[")) {
                            parseOctets(kBODY, fetchData, line, commandData.commandRespeam);
                        } else if (stringEqual(line, static_cast<std::string> (kRFC822) + " ")) {
                            parseOctets(kRFC822, fetchData, line, commandData.commandRespeam);
                        } else {
                            throw Exception("error while parsing FETCH command [" + line + "]");
                        }

                        // Still data to process

                        if (line.length() != 0) {
                            line = line.substr(line.find_first_not_of(' ')); // Next non space.
                            if (line[0] == ')') { // End of FETCH List
                                endOfFetch = true;
                            } else if (line.length() == std::strlen(kEOL) - 1) { // No data left on line
                                parseGetNextLine(commandData.commandRespeam, line); // Move to next
                            }
                        } else {
                            commandData.commandRespeam.seekg(-lineLength, std::ios_base::cur); // Rewind read to get line
                            parseGetNextLine(commandData.commandRespeam, line);
                            throw Exception("error while parsing FETCH command [" + line + "]");
                        }


                    } while (!endOfFetch);

                    commandData.resp->fetchList.push_back(std::move(fetchData));

                } else {
                    parseCommon(commandData.tag, line, static_cast<CommandResponse *> (commandData.resp.get()));
                }

            }

        }

        //
        // Parse LOGOUT Response
        //

        void CIMAPParse::parseLOGOUT(CommandData& commandData) {

            for (std::string line; parseGetNextLine(commandData.commandRespeam, line);) {
                parseCommon(commandData.tag, line, static_cast<CommandResponse *> (commandData.resp.get()));
            }

        }

        //
        // Default Parse Response
        //

        void CIMAPParse::parseDefault(CommandData& commandData) {

            for (std::string line; parseGetNextLine(commandData.commandRespeam, line);) {
                parseCommon(commandData.tag, line, static_cast<CommandResponse *> (commandData.resp.get()));
            }

        }

        // ==============
        // PUBLIC METHODS
        // ==============

        //
        // Convert any lowercase characters in string to upper.
        //

        std::string CIMAPParse::stringToUpper(const std::string& line) {

            std::string upperCase { line };
            for (auto &c : upperCase) c = std::toupper(c);
            return (upperCase);

        }

        //
        // Perform case-insensitive string compare (return true strings are equal, false otherwise)
        //

        bool CIMAPParse::stringEqual(const std::string& line, const std::string& compare) {

            int cnt01 { 0 };
            if (line.length() < compare.length()) return (false);
            for (auto &c : compare) if (std::toupper(c) != std::toupper(line[cnt01++])) return (false);
            return (true);

        }

        //
        // Extract the contents between two delimeters in command response line.
        //

        std::string CIMAPParse::stringBetween(const std::string& line, const char first, const char last) {
            std::size_t firstDel { line.find_first_of(first) };
            std::size_t lastDel  { line.find_first_of(last, firstDel + 1) };
            return (line.substr(firstDel + 1, (lastDel - firstDel - 1)));
        }

        //
        // Extract number that may follow an un-tagged command response.
        //

        std::string CIMAPParse::stringUntaggedNumber(const std::string& line) {

            std::size_t startNumber { line.find_first_not_of(' ', 1) };
            std::size_t endNumber { line.find_first_of(' ', startNumber) };
            return (line.substr(startNumber, endNumber - startNumber));

        }

        //
        // Extract tag from command response line.
        //

        std::string CIMAPParse::stringTag(const std::string& line) {
            return (line.substr(0, line.find_first_of(' ')));
        }

        //
        // Extract command string from command line. If the command has the UID 
        // prefix then this is skipped over.
        //

        std::string CIMAPParse::stringCommand(const std::string& line) {

            std::size_t startOfCommand { line.find_first_of(' ') + 1 };
            std::size_t endOfCommand { line.find_first_of(' ', startOfCommand) };

            if (stringEqual(line.substr(startOfCommand, endOfCommand - startOfCommand), kUID)) {
                startOfCommand = line.find_first_of(' ', startOfCommand) + 1;
                endOfCommand = line.find_first_of(' ', startOfCommand);
            }

            return (stringToUpper(line.substr(startOfCommand, endOfCommand - startOfCommand)));

        }

        //
        // Extract list  from command response line. Note: only check until 
        // the end of line; the first character in lins is the start 
        // of the list ie. a '('.
        //

        std::string CIMAPParse::stringList(const std::string& line) {

            int bracketCount { 0 };
            int startPosition { 0 };
            std::size_t currentIndex { 0 };
            std::size_t lineLength { line.length() };

            startPosition = line.find_first_of('(');
            lineLength -= startPosition;

            currentIndex = startPosition;
            do {
                if (line[currentIndex] == '(') bracketCount++;
                if (line[currentIndex] == ')') bracketCount--;
                currentIndex++;
            } while (bracketCount && (--lineLength > 0));

            return (line.substr(startPosition, currentIndex - startPosition));

        }

        //
        // Parse Command Response. The response string is one long string containing "\r\n"s to
        // signal end of lines. The string is read line by line converting the response to a istringstream 
        // and using getline() and '\n' to signal the end of line character (except FETCH which is dealt 
        // with differently as it has to cater for octet strings that can span multiple lines.
        //

        CIMAPParse::COMMANDRESPONSE CIMAPParse::parseResponse(const std::string & commandResponse) {

            std::istringstream responseeam { commandResponse };
            std::string commandLine;

            parseGetNextLine(responseeam, commandLine);

            ParseFunction parseFn;
            CommandData commandData{ stringTag(commandLine),
                m_stringToCodeMap[stringCommand(commandLine)],
                commandLine, responseeam};

            commandData.resp.reset({new CommandResponse
                { commandData.commandCode}});

            parseFn = m_parseCommmandMap[static_cast<int> (commandData.commandCode)];
            if (!parseFn) {
                m_parseCommmandMap[static_cast<int> (commandData.commandCode)] = parseDefault;
                parseFn = parseDefault;
            }

            parseFn(commandData);

            return (std::move(commandData.resp));

        }

        //
        // Return string for IMAP command code
        //

        std::string CIMAPParse::commandCodeString(Commands commandCode) {

            for (auto commandEntry : m_stringToCodeMap) {
                if (commandEntry.second == commandCode) {
                    return (commandEntry.first);
                }
            }

            Exception("commandCodeing() : Invalid command code.");

            return (""); // Never reached.

        }

    } // namespace IMAP
} // namespace Antik
