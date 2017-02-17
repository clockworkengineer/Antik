/*
 * File:   CMailIMAPDecode.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on Januray 24, 2017, 2:33 PM
 *
 * Copyright 2016.
 
 */

#ifndef CMAILIMAPDECODE_HPP
#define CMAILIMAPDECODE_HPP

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

class CMailIMAPDecode {
public:

    // ==========================
    // PUBLIC TYPES AND CONSTANTS
    // ==========================
    
    //
    // Class exception
    //
    
    struct Exception : public std::runtime_error {

        Exception(std::string const& message)
        : std::runtime_error("CMailIMAPDecode Failure: "+ message) { }
        
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
    // Decoded command structures.
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
    // Decode IMAP command response and return decoded response structure.
    //
    
    static BASERESPONSE decodeResponse(const std::string& commandResponseStr);

    // ================
    // PUBLIC VARIABLES
    // ================

private:

    // ===========================
    // PRIVATE TYPES AND CONSTANTS
    // ===========================
    
    //
    // Decode function pointer
    //
    
    typedef std::function<BASERESPONSE  (CommandData& commandData)> DecodeFunction;
    
    // =====================
    // DISABLED CONSTRUCTORS
    // =====================
    
    CMailIMAPDecode() = delete;
    CMailIMAPDecode(const CMailIMAPDecode & orig) = delete;
    CMailIMAPDecode(const CMailIMAPDecode && orig) = delete;
    virtual ~CMailIMAPDecode() = delete;
    CMailIMAPDecode& operator=(CMailIMAPDecode other) = delete;
    
    // ===============
    // PRIVATE METHODS
    // ===============
     
    //
    // Convert string to uppercase and string case-insensitive compare
    //
    
    static std::string stringToUpper(const std::string& lineStr);
    static bool stringEqual(const std::string& lineStr, const std::string& compareStr);
    
    //
    // Command response decode utility methods
    //
    
    static std::string extractBetween(const std::string& lineStr, const char first, const char last);
    static std::string extractBetweenDelimeter(const std::string& lineStr, const char delimeter);
    static std::string extractTag(const std::string& lineStr);
    static std::string extractCommand(const std::string& lineStr);
    static std::string extractList(const std::string& lineStr);
    static std::string extractUntaggedNumber(const std::string& lineStr);
    
    static BASERESPONSE decodeStatus(const std::string& tagStr, const std::string& lineStr);
    static void decodeOctets(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr, std::istringstream& responseStream);
    static void decodeList(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr);
    static void decodeString(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr);
    static void decodeNumber(const std::string& itemStr, FetchRespData& fetchData, std::string& lineStr);

    //
    // Command response decode methods
    //
    
    static BASERESPONSE decodeFETCH(CommandData& commandData);
    static BASERESPONSE decodeLIST(CommandData& commandData);
    static BASERESPONSE decodeSEARCH(CommandData& commandData);
    static BASERESPONSE decodeSELECT(CommandData& commandData);
    static BASERESPONSE decodeSTATUS(CommandData& commandData);
    static BASERESPONSE decodeEXPUNGE(CommandData& commandData);
    static BASERESPONSE decodeSTORE(CommandData& commandData);
    static BASERESPONSE decodeCAPABILITY(CommandData& commandData);
    static BASERESPONSE decodeNOOP(CommandData& commandData);
    static BASERESPONSE decodeLOGOUT(CommandData& commandData);
    static BASERESPONSE decodeDefault(CommandData& commandData);

    // =================
    // PRIVATE VARIABLES
    // =================

    //
    // IMAP command to decode response function mapping table
    //
    
    static std::unordered_map<std::string, DecodeFunction> decodeCommmandMap;
    
    //
    // IMAP command string to code mapping table
    //
    
    static std::unordered_map<std::string, Commands> stringToCodeMap; 

};
#endif /* CMAILIMAPDECODE_HPP */

