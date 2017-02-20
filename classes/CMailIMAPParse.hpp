/*
 * File:   CMailIMAPParse.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on January 24, 2017, 2:33 PM
 *
 * Copyright 2016.
 
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

        Exception(std::string const& message)
        : std::runtime_error("CMailIMAPParse Failure: "+ message) { }
        
    };
    
    //
    // Command data structure
    //
    
    struct CommandData {
        std::string tagStr;                      // Command tag
        std::string commandStr;                  // Command string
        std::string commandLineStr;              // Full command line
        std::istringstream& commandRespStream;   // Command response stream (Note reference)
    };
    
    //
    // Enumeration of command codes.
    //

    enum class Commands {
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
        OK = 0,
        NO,
        BAD
    };

       //
    // FETCH response data
    //
    
    struct FetchRespData {
       uint64_t index;                                              // EMail Index/UID
       std::unordered_map<std::string, std::string> responseMap;    // Fetch command response map
    };
    
    //
    // LIST response data
    //
    
    struct ListRespData {
        uint8_t     hierDel;       // Hierarchy Delimeter
        std::string attributes;    // Mailbox attributes
        std::string name;          // Mailbox name
    };
    
    //
    // STORE response data
    //
    
    struct StoreRespData {
        uint64_t index;      // EMail Index/UID
        std::string flags;   // EMail flags
    };

    //
    // Parsed command structures.
    // 
    
    struct BaseResponse {
        Commands command;
        RespCode status;
        std::string errorMessage;
    };

    struct SearchResponse : public BaseResponse {
        std::vector<uint64_t> indexes;
    };

    struct SelectResponse : public BaseResponse {
        std::string mailBoxName;
        std::string mailBoxAccess;
        std::unordered_map<std::string, std::string> responseMap;
    };

    struct ExamineResponse : public SelectResponse {
    };

    struct ListResponse : public BaseResponse {
        std::vector<ListRespData> mailBoxList;
    };
    
    struct LSubResponse : public ListResponse {
    };
    
    struct StatusResponse : public BaseResponse {
        std::string mailBoxName;
        std::unordered_map<std::string, std::string> responseMap;
    };
    
    struct ExpungeResponse : public BaseResponse {
        std::vector<uint64_t> exists;
        std::vector<uint64_t> expunged;
    };
   
    struct StoreResponse : public BaseResponse {
        std::vector<StoreRespData> storeList;
    };
    
    struct CapabilityResponse : public BaseResponse {
        std::string capabilityList;
    };
    
    struct FetchResponse : public BaseResponse {
        std::vector<FetchRespData> fetchList;
    };
    
    struct NoOpResponse : public BaseResponse {
        std::vector<std::string> rawResponse;
    };
    
    struct LogOutResponse : public NoOpResponse {
    };
    
    struct IdleResponse : NoOpResponse {
    };
  
    //
    // Command response structure unique pointer wrapper.
    //
    
    typedef  std::unique_ptr<BaseResponse> BASERESPONSE; 
    typedef  std::unique_ptr<SearchResponse> SEARCHRESPONSE;
    typedef  std::unique_ptr<SelectResponse> SELECTRESPONSE;
    typedef  std::unique_ptr<ExamineResponse> EXAMINERESPONSE;
    typedef  std::unique_ptr<ListResponse> LISTRESPONSE;
    typedef  std::unique_ptr<LSubResponse> LSUBRESPONSE;
    typedef  std::unique_ptr<StatusResponse> STATUSRESPONSE;
    typedef  std::unique_ptr<ExpungeResponse> EXPUNGERESPONSE;
    typedef  std::unique_ptr<StoreResponse> STORERESPONSE;
    typedef  std::unique_ptr<CapabilityResponse> CAPABILITYRESPONSE;
    typedef  std::unique_ptr<FetchResponse> FETCHRESPONSE;
    typedef  std::unique_ptr<NoOpResponse> NOOPRESPONSE;
    typedef  std::unique_ptr<LogOutResponse> LOGOUTRESPONSE;
    typedef  std::unique_ptr<IdleResponse> IDLERESPONSE;

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
    
    static BASERESPONSE parseResponse(const std::string& commandResponseStr);
    
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
    
    typedef std::function<BASERESPONSE  (CommandData& commandData)> ParseFunction;
    
    // ===========================================
    // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
    // ===========================================
    
    CMailIMAPParse() = delete;
    CMailIMAPParse(const CMailIMAPParse & orig) = delete;
    CMailIMAPParse(const CMailIMAPParse && orig) = delete;
    virtual ~CMailIMAPParse() = delete;
    CMailIMAPParse& operator=(CMailIMAPParse other) = delete;
    
    // ===============
    // PRIVATE METHODS
    // ===============
      
    //
    // Command response parse utility methods
    //
      
    static BASERESPONSE parseStatus(const std::string& tagStr, const std::string& lineStr);
    static void parseOctets(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr, std::istringstream& responseStream);
    static void parseList(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr);
    static void parseString(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr);
    static void parseNumber(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr);

    //
    // Command response parse methods
    //
    
    static BASERESPONSE parseFETCH(CommandData& commandData);
    static BASERESPONSE parseLIST(CommandData& commandData);
    static BASERESPONSE parseSEARCH(CommandData& commandData);
    static BASERESPONSE parseSELECT(CommandData& commandData);
    static BASERESPONSE parseSTATUS(CommandData& commandData);
    static BASERESPONSE parseEXPUNGE(CommandData& commandData);
    static BASERESPONSE parseSTORE(CommandData& commandData);
    static BASERESPONSE parseCAPABILITY(CommandData& commandData);
    static BASERESPONSE parseNOOP(CommandData& commandData);
    static BASERESPONSE parseLOGOUT(CommandData& commandData);
    static BASERESPONSE parseDefault(CommandData& commandData);

    // =================
    // PRIVATE VARIABLES
    // =================

    //
    // IMAP command to parse response function mapping table
    //
    
    static std::unordered_map<std::string, ParseFunction> parseCommmandMap;
    
    //
    // IMAP command string to code mapping table
    //
    
    static std::unordered_map<std::string, Commands> stringToCodeMap; 

};
#endif /* CMAILIMAPPARSE_HPP */

