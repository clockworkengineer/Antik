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

// =========
// NAMESPACE
// =========

namespace Antik {

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
            : std::runtime_error("CMailIMAP Failure: " + messageStr) {
            }

        };

        //
        // End of line
        //

        static const char *kEOLStr;

        //
        // Command string constants
        //

        static const char *kSTARTTLSStr;
        static const char *kAUTHENTICATEStr;
        static const char *kSEARCHStr;
        static const char *kSELECTStr;
        static const char *kEXAMINEStr;
        static const char *kCREATEStr;
        static const char *kDELETEStr;
        static const char *kRENAMEStr;
        static const char *kLOGINStr;
        static const char *kSUBSCRIBEStr;
        static const char *kUNSUBSCRIBEStr;
        static const char *kLISTStr;
        static const char *kLSUBStr;
        static const char *kSTATUSStr;
        static const char *kAPPENDStr;
        static const char *kCHECKStr;
        static const char *kCLOSEStr;
        static const char *kEXPUNGEStr;
        static const char *kFETCHStr;
        static const char *kSTOREStr;
        static const char *kCOPYStr;
        static const char *kNOOPStr;
        static const char *kLOGOUTStr;
        static const char *kIDLEStr;
        static const char *kCAPABILITYStr;
        static const char *kUIDStr;

        //
        // Response strings
        //

        static const char *kUntaggedStr;
        static const char *kOKStr;
        static const char *kBADStr;
        static const char *kNOStr;
        static const char *kFLAGSStr;
        static const char *kPERMANENTFLAGSStr;
        static const char *kUIDVALIDITYStr;
        static const char *kUIDNEXTStr;
        static const char *kHIGHESTMODSEQStr;
        static const char *kUNSEENStr;
        static const char *kEXISTSStr;
        static const char *kRECENTStr;
        static const char *kDONEStr;
        static const char *kContinuationStr;
        static const char *kENVELOPEStr;
        static const char *kBODYSTRUCTUREStr;
        static const char *kBODYStr;
        static const char *kRFC822Str;
        static const char *kINTERNALDATEStr;
        static const char *kRFC822HEADERStr;
        static const char *kRFC822SIZEStr;
        static const char *kRFC822TEXTStr;
        static const char *kBYEStr;

        //
        // Response MAP generated entries.
        //

        static const char *kMAILBOXNAMEStr;
        static const char *kMAILBOXACCESSStr;

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
        std::string getServer(void) const;
        std::string getUser(void) const;

        //
        // IMAP connect, send command and disconnect
        //

        void connect(void);
        std::string sendCommand(const std::string& commandLineStr);
        void disconnect(void);
        bool getConnectedStatus(void) const;
        
        //
        // Set IMAP command tag prefix
        //
        
        void setTagPrefix(const std::string& tagPrefixStr);

        //
        // IMAP initialization and closedown processing
        //
        
        static void init(bool bCurlVerbosity = false);
        static void closedown(void);

        // ================
        // PUBLIC VARIABLES
        // ================

    private:

        // ===========================
        // PRIVATE TYPES AND CONSTANTS
        // ===========================

        //
        // Wait on socket timeout in milliseconds
        //
        
        static const long kWaitOnSocketTimeOut = 60000;
        
        //
        // Default command prefix tag
        //

        static const char *kDefaultTagPrefixStr;
        
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

        bool bConnected = false; // == true then connected to server
        std::string userNameStr = ""; // Email account user name
        std::string userPasswordStr = ""; // Email account user name password
        std::string serverURLStr = ""; // IMAP server URL

        CURL *curlHandle = nullptr; // curl handle
        CURLcode curlResult = CURLE_OK; // curl status
        curl_socket_t curlSocketFD; // curl socket
        static bool bCurlVerbosity; // curl verbosity setting 
        char curlRxBuffer[CURL_MAX_WRITE_SIZE]; // curl rx buffer
        char curlErrMsgBuffer[CURL_ERROR_SIZE]; // curl error string buffer

        std::string commandResponseStr; // IMAP command response

        uint64_t tagCount = 1; // Current command tag count
        std::string currentTagStr; // Current command tag
        std::string tagPrefixStr{ kDefaultTagPrefixStr }; // Current command tag prefixes

    };

} // namespace Antik

#endif /* CMAILIMAP_HPP */

