/*
 * File:   CMailIMAP.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on January 24, 2017, 2:33 PM
 *
 * Copyright 2016.
 *
 */

#ifndef CMAILIMAP_HPP
#define CMAILIMAP_HPP

//
// C++ STL definitions
//

#include <vector>
#include <string>
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

        Exception(std::string const& messageStr)
        : std::runtime_error("CMailIMAP Failure: "+ messageStr) { }
        
    };

    //
    // End of line
    //
    
    const static  std::string kEOLStr;
 
    //
    // Command string constants
    //
    
    const static  std::string kSTARTTLSStr;
    const static  std::string kAUTHENTICATEStr;
    const static  std::string kSEARCHStr;
    const static  std::string kSELECTStr;
    const static  std::string kEXAMINEStr;
    const static  std::string kCREATEStr;
    const static  std::string kDELETEStr;
    const static  std::string kRENAMEStr;
    const static  std::string kLOGINStr;
    const static  std::string kSUBSCRIBEStr;
    const static  std::string kUNSUBSCRIBEStr;
    const static  std::string kLISTStr;
    const static  std::string kLSUBStr;
    const static  std::string kSTATUSStr;
    const static  std::string kAPPENDStr;
    const static  std::string kCHECKStr;
    const static  std::string kCLOSEStr;
    const static  std::string kEXPUNGEStr;
    const static  std::string kFETCHStr;
    const static  std::string kSTOREStr;
    const static  std::string kCOPYStr;
    const static  std::string kNOOPStr;
    const static  std::string kLOGOUTStr;
    const static  std::string kIDLEStr;
    const static  std::string kCAPABILITYStr;
    const static  std::string kUIDStr;
    
    //
    // Response strings
    //
    
    const static  std::string kUntaggedStr;
    const static  std::string kOKStr;
    const static  std::string kBADStr;
    const static  std::string kNOStr;
    const static  std::string kFLAGSStr;
    const static  std::string kPERMANENTFLAGSStr;
    const static  std::string kUIDVALIDITYStr;
    const static  std::string kUIDNEXTStr;
    const static  std::string kHIGHESTMODSEQStr;
    const static  std::string kUNSEENStr;
    const static  std::string kEXISTSStr;
    const static  std::string kRECENTStr;
    const static  std::string kDONEStr;
    const static  std::string kContinuationStr;
    const static  std::string kENVELOPEStr;
    const static  std::string kBODYSTRUCTUREStr;
    const static  std::string kBODYStr;
    const static  std::string kRFC822Str;
    const static  std::string kINTERNALDATEStr;
    const static  std::string kRFC822HEADERStr;
    const static  std::string kRFC822SIZEStr;
    const static  std::string kRFC822TEXTStr;
    const static  std::string kBYEStr;

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
    // Set/Get email server account details
    //

    void setServer(const std::string& serverURLStr);
    void setUserAndPassword(const std::string& userNameStr, const std::string& userPasswordStr);
    std::string getServer(void);
    std::string getUser(void);
    
    //
    // IMAP connect, send command and disconnect
    //

    void connect(void);
    std::string sendCommand(const std::string& commandLineStr);
    void disconnect(void);
    bool getConnectedStatus(void);
    
    // IMAP initialization and closedown processing

    static void init(bool bCurlVerbosity=false);
    static void closedown(void);

    // ================
    // PUBLIC VARIABLES
    // ================

private:

    // ===========================
    // PRIVATE TYPES AND CONSTANTS
    // ===========================
    
    // Wait on socket timeout in milliseconds
    
    static const long kWaitOnSocketTimeOut = 60000;
    
    // ===========================================
    // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
    // ===========================================

    CMailIMAP(const CMailIMAP & orig) = delete;
    CMailIMAP(const CMailIMAP && orig) = delete;
    CMailIMAP& operator=(CMailIMAP other) = delete;

    // ===============
    // PRIVATE METHODS
    // ===============

    //
    // Generate curl error message and throw exception
    //
    void throwCurlError(std::string baseMessageStr);
    
    //
    // Send IDLE/APPEND command (requires a special handler).
    //
    
    void sendCommandIDLE(const std::string& commandLineStr);
    void sendCommandAPPEND(const std::string& commandLineStr);
   
    //
    // Talks to server using curl_easy_send/recv()
    //

    void sendIMAPCommand(const std::string& commandLineStr);
    void waitForIMAPCommandResponse(const std::string& commandTag, std::string& commandResponseStr);
    int waitOnSocket(bool bRecv, long timeoutMS);
    
    //
    // Generate next command tag
    //
    
    void generateTag(void);

    // =================
    // PRIVATE VARIABLES
    // =================

    bool bConnected=false;                   // == true then connected to server
    std::string userNameStr = "";            // Email account user name
    std::string userPasswordStr = "";        // Email account user name password
    std::string serverURLStr = "";           // IMAP server URL

    CURL *curlHandle = nullptr;              // curl handle
    CURLcode curlResult = CURLE_OK;          // curl status
    curl_socket_t curlSockettFD;             // curl socket
    static bool bCurlVerbosity;              // curl verbosity setting 
    char curlRxBuffer[CURL_MAX_WRITE_SIZE];  // curl rx buffer
    char curlErrMsgBuffer[CURL_ERROR_SIZE];  // curl error string buffer
  
    std::string commandResponseStr;     // IMAP command response
   
    uint64_t tagCount=1;                // Current command tag count
    std::string currentTagStr;          // Current command tag

};
#endif /* CMAILIMAP_HPP */

