/*
 * File:   CMailIMAPParse.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on January 24, 2017, 2:33 PM
 *
 * Copyright 2016.
 *
 */

#ifndef CMAILIMAPPARSE_HPP
#define CMAILIMAPPARSE_HPP

//
// C++ STL definitions
//

#include <vector>
#include <memory>
#include <unordered_map>
#include <stdexcept>

// ================
// CLASS DEFINITION
// ================

class CMailIMAPParse {
public:

    // ==========================
    // PUBLIC TYPES AND CONSTANTS
    // ==========================
    
    //
    // Class exception
    //
    
    struct Exception : public std::runtime_error {

        Exception(std::string const& messageStr)
        : std::runtime_error("CMailIMAPParse Failure: "+ messageStr) { }
        
    };
        
    //
    // Enumeration of command codes.
    //

    enum class Commands {
        NONE = -1,
        STARTTLS = 0,   // Supported (through curl connect)
        AUTHENTICATE,   // Supported (through curl connect)
        LOGIN,          // Supported (through curl connect)
        CAPABILITY,     // Supported
        SELECT,         // Supported
        EXAMINE,        // Supported
        CREATE,         // Supported
        DELETE,         // Supported
        RENAME,         // Supported
        SUBSCRIBE,      // Supported
        UNSUBSCRIBE,    // Supported
        LIST,           // Supported
        LSUB,           // Supported
        STATUS,         // Supported
        APPEND,         // Supported
        CHECK,          // Supported
        CLOSE,          // Supported
        EXPUNGE,        // Supported
        SEARCH,         // Supported
        FETCH,          // Supported
        STORE,          // Supported
        COPY,           // Supported
        UID,            // Supported
        NOOP,           // Supported
        LOGOUT,         // Supported
        IDLE            // Supported
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
       uint64_t index=0;                // EMail Index/UID
       CommandResponseMap responseMap;  // Fetch command response map
    };
    
    //
    // LIST response data
    //
    
    struct ListRespData {
        uint8_t     hierDel=' ';      // Hierarchy Delimeter
        std::string attributesStr;    // Mailbox attributes
        std::string mailBoxNameStr;   // Mailbox name
    };
    
    //
    // STORE response data
    //
    
    struct StoreRespData {
        uint64_t index=0;           // EMail Index/UID
        std::string flagsListStr;   // EMail flags list
    };

    //
    // Parsed command response structure.
    // 
      
    struct CommandResponse {

        CommandResponse(Commands command) : command {command} {}
             
        Commands command=Commands::NONE; // Command enum code
        RespCode status=RespCode::NONE;  // Command enum status
        std::string errorMessageStr;     // Command error string
        bool bBYESent=false;             // ==true then BYE sent as part of response
        CommandResponseMap responseMap;  // Command response map 

        std::vector<uint64_t> indexes;          // Vector of SEARCH index(s)/UID(s)
        std::vector<ListRespData> mailBoxList;  // Vector of LIST response data
        std::vector<StoreRespData> storeList;   // Vector of STORE response data
        std::vector<FetchRespData> fetchList;   // Vector of FETCH response data
        
    };
    
    typedef  std::unique_ptr<CommandResponse> COMMANDRESPONSE; 

    //
    // Command data structure
    //
    
    struct CommandData {
        std::string tagStr;                      // Command tag
        Commands    commandCode;                 // Command code
        std::string commandLineStr;              // Full command line
        std::istringstream& commandRespStream;   // Command response stream (Note reference)
        COMMANDRESPONSE resp;                    // Parsed command response structure
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
    
    static std::string commandCodeString (Commands commandCode);

    //
    // Parse IMAP command response and return parsed response structure.
    //
    
    static COMMANDRESPONSE parseResponse(const std::string& commandResponseStr);
    
    //
    // Command response parse string utility methods
    //
    
    static std::string stringToUpper(const std::string& lineStr);
    static bool stringEqual(const std::string& lineStr, const std::string& compareStr);
  
    static std::string stringBetween(const std::string& lineStr, const char first, const char last);
    static std::string stringTag(const std::string& lineStr);
    static std::string stringCommand(const std::string& lineStr);
    static std::string stringList(const std::string& lineStr);
    static std::string stringUntaggedNumber(const std::string& lineStr);
  
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
    
    typedef std::function<void  (CommandData& commandData)> ParseFunction;
    
    // ===========================================
    // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
    // ===========================================
    
    CMailIMAPParse() = delete;
    virtual ~CMailIMAPParse() = delete;
    CMailIMAPParse(const CMailIMAPParse & orig) = delete;
    CMailIMAPParse(const CMailIMAPParse && orig) = delete;
    CMailIMAPParse& operator=(CMailIMAPParse other) = delete;
    
    // ===============
    // PRIVATE METHODS
    // ===============
    
    //
    // Get next line from response
    //
    
    static bool parseGetNextLine(std::istringstream& responseStream, std::string& lineStr);
    
    //
    // Command response parse utility methods
    //
   
    static void parseCommon(const std::string& tagStr, const std::string& lineStr, CommandResponse* statusResponse);
    static void parseOctets(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr, std::istringstream& responseStream);
    static void parseList(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr);
    static void parseString(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr);
    static void parseNumber(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr);

    //
    // Command response parse methods
    //
    
    static void parseFETCH(CommandData& commandData);
    static void parseLIST(CommandData& commandData);
    static void parseSEARCH(CommandData& commandData);
    static void parseSELECT(CommandData& commandData);
    static void parseSTATUS(CommandData& commandData);
    static void parseEXPUNGE(CommandData& commandData);
    static void parseSTORE(CommandData& commandData);
    static void parseCAPABILITY(CommandData& commandData);
    static void parseNOOP(CommandData& commandData);
    static void parseLOGOUT(CommandData& commandData);
    static void parseDefault(CommandData& commandData);
    
    // =================
    // PRIVATE VARIABLES
    // =================

    //
    // IMAP command code to parse response function mapping table
    //
    
    static std::unordered_map<int, ParseFunction> parseCommmandMap;
    
    //
    // IMAP command string to code mapping table
    //
    
    static std::unordered_map<std::string, Commands> stringToCodeMap; 

};
#endif /* CMAILIMAPPARSE_HPP */

