/*
 * File:   CMailSMTP.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2016, 2:33 PM
 *
 * Copyright 2016.
 
 */

#ifndef CMAILSMTP_HPP
#define CMAILSMTP_HPP

//
// C++ STL definitions
//

#include <string>
#include <vector>
#include <unordered_map>
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
    
    static const std::string kEncoding7Bit;
    static const std::string kEncodingBase64;
 
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
    
    // Set email server account details

    void setServer(const std::string& serverURL);
    void setUserAndPassword(const std::string& userName, const std::string& userPassword);
    
    // Set email message header details
    
    void setFromAddress(const std::string& addressFrom);
    void setToAddress(const std::string& addressTo);
    void setCCAddress(const std::string& addressCC);
    
    // Set email content details
    
    void setMailSubject(const std::string& mailSubject);
    void setMailMessage(const std::vector<std::string>& mailMessage);
    void addFileAttachment(const std::string& fileName, const std::string& contentType, const std::string& contentTransferEncoding);
    
    // Send email
   
    void postMail(void);
       
    // Initialization and closedown processing
    
    static void init();
    static void closedown();
    
    // Get email message body
    
    std::string getMailMessage();
    
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
    
    static const std::string kMimeBoundary;         // Text string used for MIME boundary
    
    static const int kBase64EncodeBufferSize=54;    // Optimum encode buffer size (since encoded max 76 bytes)
  
    static const std::string kEOL;                  // End of line
    
    // =====================
    // DISABLED CONSTRUCTORS
    // =====================
 
    // ===============
    // PRIVATE METHODS
    // ===============
    
    // Encode bytes to base64 string
    
    void encodeToBase64(std::string bytesToEncode, uint32_t numberOfBytes, std::string& encodedString);
    
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

    // =================
    // PRIVATE VARIABLES
    // =================
    
    std::string userName="";                  // Email account user name
    std::string userPassword="";              // Email account user name password
    std::string serverURL="";                 // SMTp server URL
    
    std::string addressFrom="";               // Email Sender
    std::string addressTo="";                 // Main recipients addresses
    std::string addressCC="";                 // CC recipients addresses
    
    std::string mailSubject="";               // Email subject
    std::vector<std::string> mailMessage;     // Email body
    
    std::string  mailCABundle="";             // Path to CA bundle (Untested at present)
    
    CURL     *curl=nullptr;                   // curl handle
    struct   curl_slist *recipients=nullptr;  // curl email recipients list
    CURLcode res = CURLE_OK;                  // curl status
    
    std::deque<std::string> mailPayload;      // Email payload
    
    std::vector<CMailSMTP::emailAttachment> attachedFiles;  // Attached files
    
    static std::unordered_map<std::string, std::string> extToMimeType;    // File extension to MIME type
    
};

#endif /* CMAILSMTP_HPP */

