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
    // Command data structure
    //
    
    struct CommandData {
        std::string tagStr;                      // Command tag
        Commands    commandCode;                 // Command code
        std::string commandLineStr;              // Full command line
        std::istringstream& commandRespStream;   // Command response stream (Note reference)
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
    // Command response map (item/value string pairs)
    //
    
    typedef std::unordered_map<std::string, std::string> CommandRespMap;
    
    //
    // FETCH response data
    //
    
    struct FetchRespData {
       uint64_t index=0;              // EMail Index/UID
       CommandRespMap responseMap;    // Fetch command response map
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
        uint64_t index=0;             // EMail Index/UID
        std::string flagsListStr;   // EMail flags list
    };

    //
    // Parsed command structures.
    // 
    
    // BASE
    
    struct BaseResponse {

        BaseResponse(Commands command) : command {command} {}
        
        BaseResponse(Commands command, RespCode status, std::string  errorMessageStr, bool  bBYESent) 
        : command {command}, status {status}, errorMessageStr { errorMessageStr}, bBYESent {bBYESent} {}
        
        Commands command;               // Command enum code
        RespCode status=RespCode::OK;   // Command enum status
        std::string errorMessageStr;    // Command error string
        bool bBYESent=false;            // ==true then BYE sent as part of response

    };

    // SEARCH
    
    struct SearchResponse : public BaseResponse {
    
        SearchResponse(Commands command) : BaseResponse(command) {} 
        
        std::vector<uint64_t> indexes;  // Email index(s)/UID(s) returned
        
    };

    // SELECT / EXAMINE
    
    struct SelectResponse : public BaseResponse {
      
        SelectResponse(Commands command) : BaseResponse(command) {} 
  
        std::string mailBoxNameStr;    // Mailbox name
        std::string mailBoxAccessStr;  // Mailbox access permissions
        CommandRespMap responseMap;    // Command response map 

    };
    
    struct ExamineResponse : public SelectResponse {

        ExamineResponse(Commands command) : SelectResponse(command) {}

    };
 
    // LIST / LSUB
    
    struct ListResponse : public BaseResponse {
    
        ListResponse(Commands command) : BaseResponse(command) {}
          
        std::vector<ListRespData> mailBoxList;  // Vector of mailbox list response data
        
    };
    
    struct LSubResponse : public ListResponse {
        
        LSubResponse(Commands command) : ListResponse(command) {}
    
    };
    
    // STATUS
    
    struct StatusResponse : public BaseResponse {
    
       StatusResponse(Commands command) : BaseResponse(command) {}
 
       std::string mailBoxNameStr;     // Mailbox name
       CommandRespMap responseMap;     // Command response map
       
    };
    
    // EXPUNGE
    
    struct ExpungeResponse : public BaseResponse {
        
        ExpungeResponse(Commands command) : BaseResponse(command) {}
 
        std::vector<uint64_t> exists;       // Vector of exists responses
        std::vector<uint64_t> expunged;     // Vector of expunged responses
    
    };
   
    // STORE
    
    struct StoreResponse : public BaseResponse {
        
        StoreResponse(Commands command) : BaseResponse(command) {}

        std::vector<StoreRespData> storeList;   // Vector of STORE response data
    };
    
    // CAPABILITY
    
    struct CapabilityResponse : public BaseResponse {
        
        CapabilityResponse(Commands command) : BaseResponse(command) {}

        std::string capabilitiesStr;    // Capabilities
    
    };
    
    // FETCH
    
    struct FetchResponse : public BaseResponse {
        
        FetchResponse(Commands command) : BaseResponse(command) {}
    
        std::vector<FetchRespData> fetchList;   // Vector of FETCH response data
    
    };
    
    // NOOP / LOGOUT / IDLE
    
    struct NoOpResponse : public BaseResponse {
        
        NoOpResponse(Commands command) : BaseResponse(command) {}
              
        std::vector<std::string> rawResponse;   // Raw IDLE response data
    
    };
    
    struct LogOutResponse : public NoOpResponse {
    
        LogOutResponse(Commands command) : NoOpResponse(command) {}
              
    };
    
    struct IdleResponse : NoOpResponse {
    
        IdleResponse(Commands command) : NoOpResponse(command) {}
              
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
    
    CMailIMAPParse();

    // ==========
    // DESTRUCTOR
    // ==========
    
    virtual ~CMailIMAPParse();
    
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
   
    static void parseStatus(const std::string& tagStr, const std::string& lineStr, BaseResponse* statusResponse);
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
    // IMAP command code to parse response function mapping table
    //
    
    static std::unordered_map<int, ParseFunction> parseCommmandMap;
    
    //
    // IMAP command string to code mapping table
    //
    
    static std::unordered_map<std::string, Commands> stringToCodeMap; 

};
#endif /* CMAILIMAPPARSE_HPP */

