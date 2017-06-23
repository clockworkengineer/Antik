/*
 * File:   CIMAP.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on January 24, 2017, 2:33 PM
 *
 * Copyright 2016.
 *
 */

#ifndef CIMAP_HPP
#define CIMAP_HPP

//
// C++ STL
//

#include <vector>
#include <string>
#include <stdexcept>

//
// libcurl
//

#include <curl/curl.h>

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace IMAP {

        // ==========================
        // PUBLIC TYPES AND CONSTANTS
        // ==========================

        //
        // End Of Line terminator
        //

        constexpr const char *kEOLStr{ "\r\n"};

        //
        // IMAP Command strings
        //

        constexpr const char *kSTARTTLSStr{ "STARTTLS"};
        constexpr const char *kAUTHENTICATEStr{ "AUTHENTICATE"};
        constexpr const char *kSEARCHStr{ "SEARCH"};
        constexpr const char *kSELECTStr{ "SELECT"};
        constexpr const char *kEXAMINEStr{ "EXAMINE"};
        constexpr const char *kCREATEStr{ "CREATE"};
        constexpr const char *kDELETEStr{ "DELETE"};
        constexpr const char *kRENAMEStr{ "RENAME"};
        constexpr const char *kLOGINStr{ "LOGIN"};
        constexpr const char *kSUBSCRIBEStr{ "SUBSCRIBE"};
        constexpr const char *kUNSUBSCRIBEStr{ "UNSUBSCRIBE"};
        constexpr const char *kLISTStr{ "LIST"};
        constexpr const char *kLSUBStr{ "LSUB"};
        constexpr const char *kSTATUSStr{ "STATUS"};
        constexpr const char *kAPPENDStr{ "APPEND"};
        constexpr const char *kCHECKStr{ "CHECK"};
        constexpr const char *kCLOSEStr{ "CLOSE"};
        constexpr const char *kEXPUNGEStr{ "EXPUNGE"};
        constexpr const char *kFETCHStr{ "FETCH"};
        constexpr const char *kSTOREStr{ "STORE"};
        constexpr const char *kCOPYStr{ "COPY"};
        constexpr const char *kNOOPStr{ "NOOP"};
        constexpr const char *kLOGOUTStr{ "LOGOUT"};
        constexpr const char *kIDLEStr{ "IDLE"};
        constexpr const char *kCAPABILITYStr{ "CAPABILITY"};
        constexpr const char *kUIDStr{ "UID"};

        //
        // IMAP Response strings
        //

        constexpr const char *kUntaggedStr{ "*"};
        constexpr const char *kOKStr{ "OK"};
        constexpr const char *kBADStr{ "BAD"};
        constexpr const char *kNOStr{ "NO"};
        constexpr const char *kFLAGSStr{ "FLAGS"};
        constexpr const char *kPERMANENTFLAGSStr{ "PERMANENTFLAGS"};
        constexpr const char *kUIDVALIDITYStr{ "UIDVALIDITY"};
        constexpr const char *kUIDNEXTStr{ "UIDNEXT"};
        constexpr const char *kHIGHESTMODSEQStr{ "HIGHESTMODSEQ"};
        constexpr const char *kUNSEENStr{ "UNSEEN"};
        constexpr const char *kEXISTSStr{ "EXISTS"};
        constexpr const char *kRECENTStr{ "RECENT"};
        constexpr const char *kDONEStr{ "DONE"};
        constexpr const char *kContinuationStr{ "+"};
        constexpr const char *kENVELOPEStr{ "ENVELOPE"};
        constexpr const char *kBODYSTRUCTUREStr{ "BODYSTRUCTURE"};
        constexpr const char *kBODYStr{ "BODY"};
        constexpr const char *kRFC822Str{ "RFC822"};
        constexpr const char *kINTERNALDATEStr{ "INTERNALDATE"};
        constexpr const char *kRFC822HEADERStr{ "RFC822.HEADER"};
        constexpr const char *kRFC822SIZEStr{ "RFC822.SIZE"};
        constexpr const char *kRFC822TEXTStr{ "RFC822.TEXT"};
        constexpr const char *kBYEStr{ "BYE"};

        //
        // Response MAP generated entries.
        //

        constexpr const char *kMAILBOXNAMEStr{ "MAILBOX-NAME"};
        constexpr const char *kMAILBOXACCESSStr{ "MAILBOX-ACCESS"};
        
        //
        // Default command tag prefix.
        //
        
        constexpr const char *kDefaultTagPrefixStr { "A" };

        // ================
        // CLASS DEFINITION
        // ================

        class CIMAP {
        public:

            // ==========================
            // PUBLIC TYPES AND CONSTANTS
            // ==========================

            //
            // Class exception
            //

            struct Exception : public std::runtime_error {

                Exception(std::string const& messageStr)
                : std::runtime_error("CIMAP Failure: " + messageStr) {
                }

            };

            // ============
            // CONSTRUCTORS
            // ============

            //
            // Main constructor
            //

            CIMAP();

            // ==========
            // DESTRUCTOR
            // ==========

            virtual ~CIMAP();

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

            static const long kWaitOnSocketTimeOut{ 60000};

            // ===========================================
            // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
            // ===========================================

            CIMAP(const CIMAP & orig) = delete;
            CIMAP(const CIMAP && orig) = delete;
            CIMAP& operator=(CIMAP other) = delete;

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

            bool bConnected{ false}; // == true then connected to server
            std::string userNameStr; // Email account user name
            std::string userPasswordStr; // Email account user name password
            std::string serverURLStr; // IMAP server URL

            CURL *curlHandle{ nullptr}; // curl handle
            CURLcode curlResult{ CURLE_OK}; // curl status
            curl_socket_t curlSocketFD; // curl socket
            static bool bCurlVerbosity; // curl verbosity setting 
            char curlRxBuffer[CURL_MAX_WRITE_SIZE]; // curl rx buffer
            char curlErrMsgBuffer[CURL_ERROR_SIZE]; // curl error string buffer

            std::string commandResponseStr; // IMAP command response

            uint64_t tagCount{ 1}; // Current command tag count
            std::string currentTagStr; // Current command tag
            std::string tagPrefixStr{ kDefaultTagPrefixStr}; // Current command tag prefixes

        };

    } // namespace IMAP
} // namespace Antik

#endif /* CIMAP_HPP */

