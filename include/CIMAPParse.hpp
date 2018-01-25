/*
 * File:   ClIMAPParse.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on January 24, 2017, 2:33 PM
 *
 * Copyright 2016.
 *
 */

#ifndef CIMAPPARSE_HPP
#define CIMAPPARSE_HPP

//
// C++ STL
//

#include <vector>
#include <memory>
#include <unordered_map>
#include <stdexcept>

//
// Antik classes
//

#include "CommonAntik.hpp"

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace IMAP {

        // ================
        // CLASS DEFINITION
        // ================

        class CIMAPParse {
        public:

            // ==========================
            // PUBLIC TYPES AND CONSTANTS
            // ==========================

            //
            // Class exception
            //

            struct Exception : public std::runtime_error {

                Exception(std::string const& message)
                : std::runtime_error("ClIMAPParse Failure: " + message) {
                }

            };

            //
            // Enumeration of command codes.
            //

            enum class Commands {
                NONE = -1,
                STARTTLS = 0, // Un-Supported (connect does tls handshakes automatically).
                AUTHENTICATE, // Un-Supported
                LOGIN, // Un-Supported (connect logs user in)
                CAPABILITY, // Supported
                SELECT, // Supported
                EXAMINE, // Supported
                CREATE, // Supported
                DELETE, // Supported
                RENAME, // Supported
                SUBSCRIBE, // Supported
                UNSUBSCRIBE, // Supported
                LIST, // Supported
                LSUB, // Supported
                STATUS, // Supported
                APPEND, // Supported
                CHECK, // Supported
                CLOSE, // Supported
                EXPUNGE, // Supported
                SEARCH, // Supported
                FETCH, // Supported
                STORE, // Supported
                COPY, // Supported
                UID, // Supported
                NOOP, // Supported
                LOGOUT, // Supported
                IDLE // Supported
            };


            //
            // Command response code enumeration.
            //

            enum class RespCode {
                NONE = -1,
                OK = 0,
                NO,
                BAD
            };

            //
            // Command response map (item/value string pairs)
            //

            typedef std::unordered_map<std::string, std::string> CommandResponseMap;

            //
            // FETCH response data
            //

            struct FetchRespData {
                std::uint64_t index{ 0}; // EMail Index/UID
                CommandResponseMap responseMap; // Fetch command response map
            };

            //
            // LIST response data
            //

            struct ListRespData {
                std::uint8_t hierDel{ ' '}; // Hierarchy Delimeter
                std::string attributes; // Mailbox attributes
                std::string mailBoxName; // Mailbox name
            };

            //
            // STORE response data
            //

            struct StoreRespData {
                std::uint64_t index{ 0}; // EMail Index/UID
                std::string flagsList; // EMail flags list
            };

            //
            // Parsed command response structure.
            // 

            struct CommandResponse {
                CommandResponse(Commands command) : command{command}
                {
                }

                Commands command{ Commands::NONE}; // Command code
                RespCode status{ RespCode::NONE};  // Command status
                std::string errorMessage;          // Command error string
                bool byeSent{ false};              // ==true then BYE sent as part of response
                CommandResponseMap responseMap;    // Command response map 

                std::vector<std::uint64_t> indexes; // Vector of SEARCH index(s)/UID(s)
                std::vector<ListRespData> mailBoxList; // Vector of LIST response data
                std::vector<StoreRespData> storeList; // Vector of STORE response data
                std::vector<FetchRespData> fetchList; // Vector of FETCH response data

            };

            typedef std::unique_ptr<CommandResponse> COMMANDRESPONSE;

            //
            // Command data structure
            //

            struct CommandData {
                std::string tag;                       // Command tag
                std::string commandLine;               // Full command line
                std::istringstream& commandRespStream; // Command response stream (Note reference)
                COMMANDRESPONSE resp;                  // Parsed command response structure
            };

            // ============
            // CONSTRUCTORS
            // ============

            // ==========
            // DESTRUCTOR
            // ==========

            // ==============
            // PUBLIC METHODS
            // ==============

            //
            // Get command string representation from internal code.
            //

            static std::string commandCodeString(Commands commandCode);

            //
            // Parse IMAP command response and return parsed response structure.
            //

            static COMMANDRESPONSE parseResponse(const std::string& commandResponse);

            //
            // Command response parse string utility methods
            //

            static std::string stringToUpper(std::string line);
            static bool stringStartsWith(const std::string& line, const std::string& start);
            static std::string stringBetween(const std::string& line, const char first, const char last);
            static std::string stringTag(const std::string& line);
            static std::string stringCommand(const std::string& line);
            static std::string stringList(const std::string& line);
            static std::string stringUntaggedNumber(const std::string& line);

            // ================
            // PUBLIC VARIABLES
            // ================

        private:

            // ===========================
            // PRIVATE TYPES AND CONSTANTS
            // ===========================

            //
            // Parse function pointer
            //

            typedef std::function<void (CommandData& commandData) > ParseFunction;

            // ===========================================
            // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
            // ===========================================

            CIMAPParse() = delete;
            virtual ~CIMAPParse() = delete;
            CIMAPParse(const CIMAPParse & orig) = delete;
            CIMAPParse(const CIMAPParse && orig) = delete;
            CIMAPParse& operator=(CIMAPParse other) = delete;

            // ===============
            // PRIVATE METHODS
            // ===============

            //
            // Get next line from response
            //

            static bool parseGetNextLine(std::istringstream& responseStream, std::string& line);

            //
            // Command response parse utility methods
            //

            static void parseOctets(const std::string& item, FetchRespData& fetchData, std::string& line, std::istringstream& responseStream);
            static void parseList(const std::string& item, FetchRespData& fetchData, std::string& line);
            static void parseString(const std::string& item, FetchRespData& fetchData, std::string& line);
            static void parseNumber(const std::string& item, FetchRespData& fetchData, std::string& line);

            //
            // Command response common parsing
            
            //
            static bool parseCommonUntaggedNumeric(const std::string& item, const std::string& line, CommandResponse * resp);
            static bool parseCommonStatus(const std::string& tag, const std::string& line, CommandResponse * resp);
            static void parseCommon(const std::string& tag, const std::string& line, CommandResponse* statusResponse);

            //
            // Command response parse methods
            //

            static void parseFETCH(CommandData& commandData);
            static void parseLIST(CommandData& commandData);
            static void parseSEARCH(CommandData& commandData);
            static void parseSELECT(CommandData& commandData);
            static void parseSTATUS(CommandData& commandData);
            static void parseSTORE(CommandData& commandData);
            static void parseCAPABILITY(CommandData& commandData);
            static void parseDefault(CommandData& commandData);

            // =================
            // PRIVATE VARIABLES
            // =================

            //
            // IMAP command code to parse response function mapping table
            //

            static std::unordered_map<int, ParseFunction> m_parseCommmandMap;

            //
            // IMAP command string to code mapping table
            //

            static std::unordered_map<std::string, Commands> m_stringToCodeMap;

        };

    } // namespace IMAP
} // namespace Antik

#endif /* CIMAPPARSE_HPP */

