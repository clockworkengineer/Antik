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
// Antik classes
//

#include "CommonAntik.hpp"
#include "CSocket.hpp"

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

        constexpr const char *kEOL{ "\r\n"};

        //
        // IMAP Command strings
        //

        constexpr const char *kSTARTTLS{ "STARTTLS"};
        constexpr const char *kAUTHENTICATE{ "AUTHENTICATE"};
        constexpr const char *kSEARCH{ "SEARCH"};
        constexpr const char *kSELECT{ "SELECT"};
        constexpr const char *kEXAMINE{ "EXAMINE"};
        constexpr const char *kCREATE{ "CREATE"};
        constexpr const char *kDELETE{ "DELETE"};
        constexpr const char *kRENAME{ "RENAME"};
        constexpr const char *kLOGIN{ "LOGIN"};
        constexpr const char *kSUBSCRIBE{ "SUBSCRIBE"};
        constexpr const char *kUNSUBSCRIBE{ "UNSUBSCRIBE"};
        constexpr const char *kLIST{ "LIST"};
        constexpr const char *kLSUB{ "LSUB"};
        constexpr const char *kSTATUS{ "STATUS"};
        constexpr const char *kAPPEND{ "APPEND"};
        constexpr const char *kCHECK{ "CHECK"};
        constexpr const char *kCLOSE{ "CLOSE"};
        constexpr const char *kEXPUNGE{ "EXPUNGE"};
        constexpr const char *kFETCH{ "FETCH"};
        constexpr const char *kSTORE{ "STORE"};
        constexpr const char *kCOPY{ "COPY"};
        constexpr const char *kNOOP{ "NOOP"};
        constexpr const char *kLOGOUT{ "LOGOUT"};
        constexpr const char *kIDLE{ "IDLE"};
        constexpr const char *kCAPABILITY{ "CAPABILITY"};
        constexpr const char *kUID{ "UID"};

        //
        // IMAP Response strings
        //

        constexpr const char *kUntagged{ "*"};
        constexpr const char *kOK{ "OK"};
        constexpr const char *kBAD{ "BAD"};
        constexpr const char *kNO{ "NO"};
        constexpr const char *kFLAGS{ "FLAGS"};
        constexpr const char *kPERMANENTFLAGS{ "PERMANENTFLAGS"};
        constexpr const char *kUIDVALIDITY{ "UIDVALIDITY"};
        constexpr const char *kUIDNEXT{ "UIDNEXT"};
        constexpr const char *kHIGHESTMODSEQ{ "HIGHESTMODSEQ"};
        constexpr const char *kUNSEEN{ "UNSEEN"};
        constexpr const char *kEXISTS{ "EXISTS"};
        constexpr const char *kRECENT{ "RECENT"};
        constexpr const char *kDONE{ "DONE"};
        constexpr const char *kContinuation{ "+"};
        constexpr const char *kENVELOPE{ "ENVELOPE"};
        constexpr const char *kBODYSTRUCTURE{ "BODYSTRUCTURE"};
        constexpr const char *kBODY{ "BODY"};
        constexpr const char *kRFC822{ "RFC822"};
        constexpr const char *kINTERNALDATE{ "INTERNALDATE"};
        constexpr const char *kRFC822HEADER{ "RFC822.HEADER"};
        constexpr const char *kRFC822SIZE{ "RFC822.SIZE"};
        constexpr const char *kRFC822TEXT{ "RFC822.TEXT"};
        constexpr const char *kBYE{ "BYE"};

        //
        // Response MAP generated entries.
        //

        constexpr const char *kMAILBOXNAME{ "MAILBOX-NAME"};
        constexpr const char *kMAILBOXACCESS{ "MAILBOX-ACCESS"};
        
        //
        // Default command tag prefix.
        //
        
        constexpr const char *kDefaultTagPrefix { "A" };

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

                Exception(std::string const& message)
                : std::runtime_error("CIMAP Failure: " + message) {
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

            void setServer(const std::string& serverURL);
            void setUserAndPassword(const std::string& userName, const std::string& userPassword);
            std::string getServer(void) const;
            std::string getUser(void) const;

            //
            // IMAP connect, send command and disconnect
            //

            void connect(void);
            std::string sendCommand(const std::string& commandLine);
            void disconnect(void);
            bool getConnectedStatus(void) const;

            //
            // Set IMAP command tag prefix
            //

            void setTagPrefix(const std::string& tagPrefix);
            
            //
            // Set IO Buffer Size
            //

            void setIOBufferSize(std::uint32_t bufferSize);
            
            //
            // IMAP initialization and closedown processing
            //

            static void init(void);
            static void closedown(void);

            // ================
            // PUBLIC VARIABLES
            // ================

        private:

            // ===========================
            // PRIVATE TYPES AND CONSTANTS
            // ===========================
            
            //
            // IO buffer default size
            //

            static const std::uint32_t kIODefaultBufferSize { 1024*32 };
                      
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
            // Send IDLE/APPEND command (requires a special handler).
            //

            void sendCommandIDLE(const std::string& commandLine);
            void sendCommandAPPEND(const std::string& commandLine);

            //
            // Talks to server using CSocket
            //

            void sendIMAPCommand(const std::string& commandLine);
            void waitForIMAPCommandResponse(const std::string& commandTag, std::string& commandResponse);

            //
            // Generate next command tag
            //

            void generateTag(void);

            // =================
            // PRIVATE VARIABLES
            // =================

            bool m_connected{ false}; // == true then connected to server
            
            std::string m_userName;     // Email account user name
            std::string m_userPassword; // Email account user name password
            std::string m_serverURL;    // IMAP server URL
            
            Antik::Network::CSocket m_imapSocket;   // IMAP CSocket
            
            std::unique_ptr<char> m_ioBuffer;                       // I/O Buffer
            std::uint32_t m_ioBufferSize { kIODefaultBufferSize };  // I/O Buffer Size

            std::string m_commandResponse; // IMAP command response

            std::uint64_t m_tagCount{ 1};                // Current command tag count
            std::string m_currentTag;                    // Current command tag
            std::string m_tagPrefix{ kDefaultTagPrefix}; // Current command tag prefixes

        };

    } // namespace IMAP
} // namespace Antik

#endif /* CIMAP_HPP */

