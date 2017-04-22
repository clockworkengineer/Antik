/*
 * File:   CSMTP.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2016, 2:33 PM
 *
 * Copyright 2016.
 *
 */

#ifndef CSMTP_HPP
#define CSMTP_HPP

//
// C++ STL definitions
//

#include <string>
#include <vector>
#include <stdexcept>
#include <deque>

//
// libcurl definitions
//

#include <curl/curl.h>

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace SMTP {

        // ================
        // CLASS DEFINITION
        // ================

        class CSMTP {
        public:

            // ==========================
            // PUBLIC TYPES AND CONSTANTS
            // ==========================

            //
            // Class exception
            //

            struct Exception : public std::runtime_error {

                Exception(std::string const& messageStr)
                : std::runtime_error("CSMTP Failure: " + messageStr) {
                }

            };

            // Supported contents encodings

            static const char *kEncoding7BitStr;
            static const char *kEncodingBase64Str;

            // ============
            // CONSTRUCTORS
            // ============

            //
            // Main constructor
            //

            CSMTP();

            // ==========
            // DESTRUCTOR
            // ==========

            virtual ~CSMTP();

            // ==============
            // PUBLIC METHODS
            // ==============

            // Set/Get email server account details. Note : No password get.

            void setServer(const std::string& serverURLStr);
            void setUserAndPassword(const std::string& userName, const std::string& userPasswordStr);

            std::string getServer(void) const;
            std::string getUser(void) const;

            // Set/Get email message header details

            void setFromAddress(const std::string& addressFromStr);
            void setToAddress(const std::string& addressToStr);
            void setCCAddress(const std::string& addressCCStr);

            std::string getFromAddress(void) const;
            std::string getToAddress(void) const;
            std::string getCCAddress(void) const;

            // Set email content details

            void setMailSubject(const std::string& mailSubjectStr);
            void setMailMessage(const std::vector<std::string>& mailMessageStr);
            void addFileAttachment(const std::string& fileNameStr, const std::string& contentTypeStr, const std::string& contentTransferEncodingStr);

            std::string getMailSubject(void) const;
            std::string getMailMessage(void) const;

            // Send email

            void postMail(void);

            // Initialization and closedown processing

            static void init(bool bCurlVerbosity = false);
            static void closedown();

            // Get whole of email message

            std::string getMailFull(void);

            // Encode/decode bytes to base64 string

            static void encodeToBase64(const std::string& decodedStringStr, std::string& encodedStringStr, uint32_t numberOfBytes);
            static void decodeFromBase64(const std::string& encodedStringStr, std::string& decodedStringStr, uint32_t numberOfBytes);


            // ================
            // PUBLIC VARIABLES
            // ================

        private:

            // ===========================
            // PRIVATE TYPES AND CONSTANTS
            // ===========================

            // Attachments

            struct EmailAttachment {
                std::string fileNameStr; // Attached file name
                std::string contentTypesStr; // Attached file MIME content type
                std::string contentTransferEncodingStr; // Attached file content encoding 
                std::vector<std::string> encodedContents; // Attached file encoded contents
            };

            static const char *kMimeBoundaryStr; // Text string used for MIME boundary

            static const int kBase64EncodeBufferSize { 54 }; // Optimum encode buffer size (since encoded max 76 bytes)

            static const char *kEOLStr; // End of line

            static const char kCB64[]; // Valid characters for base64 encode/decode.

            // ===========================================
            // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
            // ===========================================

            CSMTP(const CSMTP & orig) = delete;
            CSMTP(const CSMTP && orig) = delete;
            CSMTP& operator=(CSMTP other) = delete;

            // ===============
            // PRIVATE METHODS
            // ===============

            // Encode email attachment

            void encodeAttachment(CSMTP::EmailAttachment& attachment);

            // Add attachments to payload

            void buildAttachments(void);

            // Construct email payload

            void buildMailPayload(void);

            // libcurl read callback for payload

            static size_t payloadSource(void *ptr, size_t size, size_t nmemb, std::deque<std::string> *mailPayloadStr);

            // Date and time for email

            static const std::string currentDateAndTime(void);

            // Load file extension to MIME type mapping table

            static void loadMIMETypes(void);

            // Decode character to base64 index.

            static int decodeChar(char ch);

            // =================
            // PRIVATE VARIABLES
            // =================

            std::string userNameStr;       // Email account user name
            std::string userPasswordStr;   // Email account user name password
            std::string serverURLStr;      // SMTP server URL

            std::string addressFromStr;    // Email Sender
            std::string addressToStr;      // Main recipients addresses
            std::string addressCCStr;      // CC recipients addresses

            std::string mailSubjectStr; // Email subject
            std::vector<std::string> mailMessage; // Email body

            std::string mailCABundleStr; // Path to CA bundle (Untested at present)

            CURL *curlHandle { nullptr }; // curl handle
            struct curl_slist *curlRecipients { nullptr }; // curl email recipients list
            CURLcode curlResult { CURLE_OK }; // curl status
            char curlErrMsgBuffer[CURL_ERROR_SIZE]; // curl error string buffer  
            static bool bCurlVerbosity; // curl verbosity setting

            std::deque<std::string> mailPayload; // Email payload

            std::vector<CSMTP::EmailAttachment> attachedFiles; // Attached files

        };

    } // namespace SMTP
} // namespace Antik

#endif /* CSMTP_HPP */

