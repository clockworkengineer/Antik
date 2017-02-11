/*
 * File:   CMailIMAP.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on Januray 24, 2017, 2:33 PM
 *
 * Copyright 2016.
 
 */

#ifndef CMAILIMAP_HPP
#define CMAILIMAP_HPP

//
// C++ STL definitions
//

#include <vector>
#include <memory>
#include <unordered_map>
#include <stdexcept>

//
// libcurl definitions
//

#include <curl/curl.h>

// ================
// CLASS DEFINITION
// ================

class CMailIMAP {
public:

    // ==========================
    // PUBLIC TYPES AND CONSTANTS
    // ==========================
    
    //
    // Class exception
    //
    
    struct Exception : public std::runtime_error {

        Exception(std::string const& message)
        : std::runtime_error("CMailIMAP Failure: "+ message) { }
        
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
    // Command string constants
    //
    
    static const std::string kSTARTTLSStr;
    static const std::string kAUTHENTICATEStr;
    static const std::string kSEARCHStr;
    static const std::string kSELECTStr;
    static const std::string kEXAMINEStr;
    static const std::string kCREATEStr;
    static const std::string kDELETEStr;
    static const std::string kRENAMEStr;
    static const std::string kLOGINStr;
    static const std::string kSUBSCRIBEStr;
    static const std::string kUNSUBSCRIBEStr;
    static const std::string kLISTStr;
    static const std::string kLSUBStr;
    static const std::string kSTATUSStr;
    static const std::string kAPPENDStr;
    static const std::string kCHECKStr;
    static const std::string kCLOSEStr;
    static const std::string kEXPUNGEStr;
    static const std::string kFETCHStr;
    static const std::string kSTOREStr;
    static const std::string kCOPYStr;
    static const std::string kNOOPStr;
    static const std::string kLOGOUTStr;
    static const std::string kIDLEStr;
    static const std::string kCAPABILITYStr;

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
    // Command response structure shared pointer wrapper.
    //
    
    typedef  std::shared_ptr<BaseResponse> BASERESPONSE; 
    typedef  std::shared_ptr<SearchResponse> SEARCHRESPONSE;
    typedef  std::shared_ptr<SelectResponse> SELECTRESPONSE;
    typedef  std::shared_ptr<ExamineResponse> EXAMINERESPONSE;
    typedef  std::shared_ptr<ListResponse> LISTRESPONSE;
    typedef  std::shared_ptr<LSubResponse> LSUBRESPONSE;
    typedef  std::shared_ptr<StatusResponse> STATUSRESPONSE;
    typedef  std::shared_ptr<ExpungeResponse> EXPUNGERESPONSE;
    typedef  std::shared_ptr<StoreResponse> STORERESPONSE;
    typedef  std::shared_ptr<CapabilityResponse> CAPABILITYRESPONSE;
    typedef  std::shared_ptr<FetchResponse> FETCHRESPONSE;
    typedef  std::shared_ptr<NoOpResponse> NOOPRESPONSE;
    typedef  std::shared_ptr<LogOutResponse> LOGOUTRESPONSE;
    typedef  std::shared_ptr<IdleResponse> IDLERESPONSE;

    // ============
    // CONSTRUCTORS
    // ============

    //
    // Main constructor
    //

    CMailIMAP();

    // ==========
    // DESTRUCTOR
    // ==========

    virtual ~CMailIMAP();

    // ==============
    // PUBLIC METHODS
    // ==============

    //
    // Set email server account details
    //

    void setServer(const std::string& serverURLStr);
    void setUserAndPassword(const std::string& userNameStr, const std::string& userPasswordStr);

    //
    // IMAP connect, send command and disconnect
    //

    void connect(void);
    BASERESPONSE sendCommand(const std::string& commandLineStr);
    void disconnect(void);

    // IMAP initialization and closedown processing

    static void init();
    static void closedown();
    
    // Get command string representation from internal code.
    
    static std::string commandCodeString (CMailIMAP::Commands commandCode);


    // ================
    // PUBLIC VARIABLES
    // ================

private:

    // ===========================
    // PRIVATE TYPES AND CONSTANTS
    // ===========================

    static const std::string kEOLStr;       // End of line
    
    //
    // Response strings
    //
    
    static const std::string kUntaggedStr;
    static const std::string kOKStr;
    static const std::string kBADStr;
    static const std::string kNOStr;
    static const std::string kFLAGSStr;
    static const std::string kPERMANENTFLAGSStr;
    static const std::string kUIDVALIDITYStr;
    static const std::string kUIDNEXTStr;
    static const std::string kHIGHESTMODSEQStr;
    static const std::string kUNSEENStr;
    static const std::string kEXISTSStr;
    static const std::string kRECENTStr;
    static const std::string kDONEStr;
    static const std::string kContinuationStr;
    static const std::string kENVELOPEStr;
    static const std::string kBODYSTRUCTUREStr;
    static const std::string kBODYStr;
    static const std::string kRFC822Str;
    static const std::string kINTERNALDATEStr;
    static const std::string kRFC822HEADERStr;
    static const std::string kRFC822SIZEStr;
    static const std::string kRFC822TEXTStr;
    static const std::string kUIDStr;
    static const std::string kBYEStr;
      
    //
    // Decode function pointer
    //
    
    typedef std::function<BASERESPONSE  (CommandData& commandData)> DecodeFunction;
    
    // =====================
    // DISABLED CONSTRUCTORS
    // =====================

    // ===============
    // PRIVATE METHODS
    // ===============

    //
    // Send IDLE/APPEND command (requires a special handler).
    //
    
    void sendCommandIDLE(const std::string& commandLineStr);
    void sendCommandAPPEND(const std::string& commandLineStr);
   
    //
    // Talks to server using curl_easy_send/recv()
    //

    void sendIMAPCommand(const std::string& command);
    void waitForIMAPCommandResponse(const std::string& commandTag, std::string& commandResponseStr);

    //
    // Generate next command tag
    //
    
    void generateTag(void);
    
    //
    // Command response decode utility methods
    //
    
    static std::string extractBetween(const std::string& lineStr, const char first, const char last);
    static std::string extractBetweenDelimeter(const std::string& lineStr, const char delimeter);
    static std::string extractTag(const std::string& lineStr);
    static std::string extractCommand(const std::string& lineStr);
    static std::string extractList(const std::string& lineStr);
    static std::string extractUntaggedNumber(const std::string& lineStr);
    
    static void decodeStatus(const std::string& tagStr, const std::string& lineStr, BASERESPONSE resp);
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
    static BASERESPONSE decodeResponse(const std::string& commandLineStr, const std::string& commandResponseStr);

    // =================
    // PRIVATE VARIABLES
    // =================

    bool bConnected=false;      // == true then connected to server
    
    std::string userNameStr = ""; // Email account user name
    std::string userPasswordStr = ""; // Email account user name password
    std::string serverURLStr = ""; // IMAP server URL

    CURL *curl = nullptr; // curl handle
    CURLcode res = CURLE_OK; // curl status

    std::string commandResponseStr; // IMAP command response

    char rxBuffer[CURL_MAX_WRITE_SIZE]; // IMAP rx buffer
    char errMsgBuffer[CURL_ERROR_SIZE]; // IMAP error string buffer
    
    uint64_t tagCount=1;        // Current command tag count
    std::string currentTagStr;     // Current command tag
    
    //
    // IMAP command to decode response function mapping table
    //
    
    static std::unordered_map<std::string, DecodeFunction> decodeCommmandMap;
    
    //
    // IMAP command string to code mapping table
    //
    
    static std::unordered_map<std::string, Commands> stringToCodeMap; 

};
#endif /* CMAILIMAP_HPP */

