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

        Exception(std::string const& message)
        : std::runtime_error("CMailIMAP Failure: "+ message) { }
        
    };

    //
    // End of line
    //
    
    static const std::string kEOLStr;
 
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
    static const std::string kUIDStr;
    
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
    static const std::string kBYEStr;

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
    std::string sendCommand(const std::string& commandLineStr);
    void disconnect(void);

    // IMAP initialization and closedown processing

    static void init(bool bCurlVerbosity=false);
    static void closedown();

    // ================
    // PUBLIC VARIABLES
    // ================

private:

    // ===========================
    // PRIVATE TYPES AND CONSTANTS
    // ===========================
   
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
    // Send IDLE/APPEND command (requires a special handler).
    //
    
    void sendCommandIDLE(const std::string& commandLineStr);
    void sendCommandAPPEND(const std::string& commandLineStr);
   
    //
    // Talks to server using curl_easy_send/recv()
    //

    void sendIMAPCommand(const std::string& commandLineStr);
    void waitForIMAPCommandResponse(const std::string& commandTag, std::string& commandResponseStr);

    //
    // Generate next command tag
    //
    
    void generateTag(void);

    // =================
    // PRIVATE VARIABLES
    // =================

    bool bConnected=false;              // == true then connected to server
    
    std::string userNameStr = "";       // Email account user name
    std::string userPasswordStr = "";   // Email account user name password
    std::string serverURLStr = "";      // IMAP server URL

    CURL *curl = nullptr;               // curl handle
    CURLcode res = CURLE_OK;            // curl status
    static bool bCurlVerbosity;         // curl verbosity setting 
 
    std::string commandResponseStr;     // IMAP command response
    char rxBuffer[CURL_MAX_WRITE_SIZE]; // IMAP rx buffer
    char errMsgBuffer[CURL_ERROR_SIZE]; // IMAP error string buffer
    
    uint64_t tagCount=1;                // Current command tag count
    std::string currentTagStr;          // Current command tag

};
#endif /* CMAILIMAP_HPP */

