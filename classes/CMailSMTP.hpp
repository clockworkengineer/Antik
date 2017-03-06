/*
 * File:   CMailSMTP.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2016, 2:33 PM
 *
 * Copyright 2016.
 *
 */

#ifndef CMAILSMTP_HPP
#define CMAILSMTP_HPP

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

// ================
// CLASS DEFINITION
// ================

class CMailSMTP {
    
public:
    
    // ==========================
    // PUBLIC TYPES AND CONSTANTS
    // ==========================
    
    //
    // Class exception
    //
    
    struct Exception : public std::runtime_error {

        Exception(std::string const& message)
        : std::runtime_error("CMailSMTP Failure: "+ message) { }
        
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
    
    CMailSMTP();
    
    // ==========
    // DESTRUCTOR
    // ==========
    
    virtual ~CMailSMTP();

    // ==============
    // PUBLIC METHODS
    // ==============
    
    // Set/Get email server account details. Note : No password get.

    void setServer(const std::string& serverURL);
    void setUserAndPassword(const std::string& userName, const std::string& userPassword);  
    
    std::string getServer(void) const;
    std::string getUser(void) const;
 
    // Set/Get email message header details
    
    void setFromAddress(const std::string& addressFrom);
    void setToAddress(const std::string& addressTo);
    void setCCAddress(const std::string& addressCC);
    
    std::string  getFromAddress(void) const;
    std::string  getToAddress(void) const;
    std::string  getCCAddress(void) const;
    
    // Set email content details
    
    void setMailSubject(const std::string& mailSubject);
    void setMailMessage(const std::vector<std::string>& mailMessage);
    void addFileAttachment(const std::string& fileName, const std::string& contentType, const std::string& contentTransferEncoding);
 
    std::string getMailSubject(void) const;
    std::string getMailMessage(void) const;
    
    // Send email
   
    void postMail(void);
       
    // Initialization and closedown processing
    
    static void init(bool bCurlVerbosity=false);
    static void closedown();
    
    // Get whole of email message
    
    std::string getMailFull(void);
    
    // Encode/decode bytes to base64 string
    
    static void encodeToBase64(const std::string& decodedString, std::string& encodedString, uint32_t numberOfBytes);
    static void decodeFromBase64 (const std::string& encodedString, std::string& decodedString, uint32_t numberOfBytes);

    
    // ================
    // PUBLIC VARIABLES
    // ================
    
private:

    // ===========================
    // PRIVATE TYPES AND CONSTANTS
    // ===========================
       
    // Attachments
    
    struct emailAttachment {
        std::string fileName;                       // Attached file name
        std::string contentTypes;                   // Attached file MIME content type
        std::string contentTransferEncoding;        // Attached file content encoding 
        std::vector<std::string> encodedContents;   // Attached file encoded contents
    };
    
    static const char *kMimeBoundaryStr;               // Text string used for MIME boundary
    
    static const int kBase64EncodeBufferSize=54;    // Optimum encode buffer size (since encoded max 76 bytes)
  
    static const char *kEOLStr;                        // End of line
    
    static const char kCB64[];                      // Valid characters for base64 encode/decode.
    
    // ===========================================
    // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
    // ===========================================
    
    CMailSMTP(const CMailSMTP & orig) = delete;
    CMailSMTP(const CMailSMTP && orig) = delete;
    CMailSMTP& operator=(CMailSMTP other) = delete;

    // ===============
    // PRIVATE METHODS
    // ===============
        
    // Encode email attachment
    
    void encodeAttachment(CMailSMTP::emailAttachment& attachment);
    
    // Add attachments to payload
    
    void buildAttachments(void);
    
    // Construct email payload
    
    void buildMailPayload(void);
    
    // libcurl read callback for payload
    
    static size_t payloadSource(void *ptr, size_t size, size_t nmemb,  std::deque<std::string> *mailPayload);
    
    // Date and time for email
    
    static const std::string currentDateAndTime(void);
    
    // Load file extension to MIME type mapping table
    
    static void loadMIMETypes (void);

    // Decode character to base64 index.
    
    static int decodeChar(char ch);

    // =================
    // PRIVATE VARIABLES
    // =================
    
    std::string userName="";                  // Email account user name
    std::string userPassword="";              // Email account user name password
    std::string serverURL="";                 // SMTP server URL
    
    std::string addressFrom="";               // Email Sender
    std::string addressTo="";                 // Main recipients addresses
    std::string addressCC="";                 // CC recipients addresses
    
    std::string mailSubject="";               // Email subject
    std::vector<std::string> mailMessage;     // Email body
    
    std::string  mailCABundle="";             // Path to CA bundle (Untested at present)
    
    CURL     *curl=nullptr;                   // curl handle
    struct   curl_slist *recipients=nullptr;  // curl email recipients list
    CURLcode res = CURLE_OK;                  // curl status
    char errMsgBuffer[CURL_ERROR_SIZE];       // curl error string buffer  
    static bool bCurlVerbosity;               // curl verbosity setting
    
    std::deque<std::string> mailPayload;      // Email payload
    
    std::vector<CMailSMTP::emailAttachment> attachedFiles;  // Attached files
    
};

#endif /* CMAILSMTP_HPP */

