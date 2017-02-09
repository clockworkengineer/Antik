/*
 * File:   CMailSMTP.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2016, 2:33 PM
 *
 * Copyright 2016.
 *
 */

//
// Class: CMailSMTP
// 
// Description: Class that enables an email to be setup and sent
// to a specified address using the libcurl library. SSL is supported
// and attached files in either 7bit or base64 encoded format. When
// adding an attachment it creates its MIME type by first
// checking an internal table created from /etc/mime.types. If no 
// mapping is present it uses the passed in value as default.
//
// Dependencies:   C11++     - Language standard features used.
//                 libcurl   - Used to talk to SMTP server.
//                 Linux     - /etc/mime.types used to create MIME file 
//                             extension mapping and function basename().
//

// =================
// CLASS DEFINITIONS
// =================

#include "CMailSMTP.hpp"

// ====================
// CLASS IMPLEMENTATION
// ====================

//
// C++ STL definitions
//

#include <cstring>
#include <memory>
#include <ctime>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <array>

// ===========================
// PRIVATE TYPES AND CONSTANTS
// ===========================


// MIME multi-part text boundary string 

const std::string CMailSMTP::kMimeBoundary("xxxxCMailSMTPBoundaryText");

// Line terminator

const std::string CMailSMTP::kEOL("\r\n");

// ==========================
// PUBLIC TYPES AND CONSTANTS
// ==========================

// Supported encoding methods

const std::string CMailSMTP::kEncoding7Bit("7Bit");
const std::string CMailSMTP::kEncodingBase64("base64");

// ========================
// PRIVATE STATIC VARIABLES
// ========================

// File extension to MIME type

std::unordered_map<std::string, std::string> CMailSMTP::extToMimeType;

// =======================
// PUBLIC STATIC VARIABLES
// =======================

// ===============
// PRIVATE METHODS
// ===============

//
// Build extension to MIME mapping table from /etc/mimes.types.
// This is Linux dependent but use until a better solution found.
//

void CMailSMTP::loadMIMETypes (void) {
    
    std::ifstream mimeFile("/etc/mime.types");
    std::string   extension, mimeType, line;

    while (std::getline(mimeFile, line)) {
        if (line[0] != '#') {
            std::istringstream iss(line);
            iss >> mimeType;
            while (iss.good()) {
                iss >> extension;
                if (!extension.empty()) {
                    CMailSMTP::extToMimeType.insert({extension, mimeType});
                }
            }
        }
    }
      
}

//
// Get string for current date time. Note: Resizing buffer effectively removes 
// the null character added to the end of the string by strftime().
//

const std::string CMailSMTP::currentDateAndTime(void) {
    
   std::time_t rawtime;
   struct std::tm *info;
   std::string buffer(80, ' ');

   std::time( &rawtime );
   info = std::localtime( &rawtime );
   buffer.resize(std::strftime(&buffer[0],buffer.length(),"%a, %d %b %Y %H:%M:%S %z", info));
   return(buffer);

}

//
// Fill libcurl read request buffer.
//

size_t CMailSMTP::payloadSource(void *ptr, size_t size, size_t nmemb, std::deque<std::string> *mailPayload) {

    size_t bytesCopied=0;

    if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1)) {
        return 0;
    }

    while (!mailPayload->empty()) {
        if ((mailPayload->front().length() + bytesCopied) > (size * nmemb)) break;
        mailPayload->front().copy(& static_cast<char *> (ptr)[bytesCopied], mailPayload->front().length(), 0);
        bytesCopied += mailPayload->front().length();
        mailPayload->pop_front();
    }
    
    return bytesCopied;

}

//
// Encode bytes to base64 string
//

void CMailSMTP::encodeToBase64(std::string bytesToEncode, uint32_t numberOfBytes, std::string& encodedString) {

    static const char cb64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    int trailing, byteIndex=0;
    register uint8_t byte1, byte2, byte3;

    trailing = (numberOfBytes % 3);  // Trailing bytes
    numberOfBytes /= 3;              // No of 3 byte values to encode

    while (numberOfBytes--) {

        byte1 = bytesToEncode[byteIndex++];
        byte2 = bytesToEncode[byteIndex++];
        byte3 = bytesToEncode[byteIndex++];

        encodedString += cb64[(byte1 & 0xfc) >> 2];
        encodedString += cb64[((byte1 & 0x03) << 4) + ((byte2 & 0xf0) >> 4)];
        encodedString += cb64[((byte2 & 0x0f) << 2) + ((byte3 & 0xc0) >> 6)];
        encodedString += cb64[byte3 & 0x3f];

    }

    // One trailing byte

    if (trailing == 1) {
        byte1 = bytesToEncode[byteIndex++];
        encodedString += cb64[(byte1 & 0xfc) >> 2];
        encodedString += cb64[((byte1 & 0x03) << 4)];
        encodedString += '=';
        encodedString += '=';

        // Two trailing bytes

    } else if (trailing == 2) {
        byte1 = bytesToEncode[byteIndex++];
        byte2 = bytesToEncode[byteIndex++];
        encodedString += cb64[(byte1 & 0xfc) >> 2];
        encodedString += cb64[((byte1 & 0x03) << 4) + ((byte2 & 0xf0) >> 4)];
        encodedString += cb64[((byte2 & 0x0f) << 2)];
        encodedString += '=';
    }

}

//
// Encode a specified file in either 7bit or base64.
//

void CMailSMTP::encodeAttachment(CMailSMTP::emailAttachment& attachment) {

    std::string line;

    // 7bit just copy

    if ((attachment.contentTransferEncoding.compare(CMailSMTP::kEncodingBase64) != 0)) {

        std::ifstream attachmentFile(attachment.fileName);

        // As sending text file via email strip any host specific end of line and replace with <cr><lf>
        
        while (std::getline(attachmentFile, line)) {
            if (line.back()=='\n') line.pop_back();
            if (line.back()=='\r') line.pop_back();
            attachment.encodedContents.push_back(line + kEOL);
        }

    // Base64

    } else {

        std::ifstream ifs(attachment.fileName, std::ios::binary);
        std::string buffer(CMailSMTP::kBase64EncodeBufferSize, ' ');
  
        ifs.seekg(0, std::ios::beg);
        while (ifs.good()) {
            ifs.read(&buffer[0], CMailSMTP::kBase64EncodeBufferSize);
            this->encodeToBase64(buffer, ifs.gcount(), line);
            attachment.encodedContents.push_back(line + kEOL);
            line.clear();
        }

    }

}

//
// Place attachments into email payload
//

void CMailSMTP::buildAttachments(void) {

    for (auto attachment : this->attachedFiles) {

        std::string baseFileName = basename(attachment.fileName.c_str());

        this->encodeAttachment(attachment);

        this->mailPayload.push_back("--" + CMailSMTP::kMimeBoundary + kEOL);
        this->mailPayload.push_back("Content-Type: " + attachment.contentTypes + ";"+kEOL);
        this->mailPayload.push_back("Content-transfer-encoding: " + attachment.contentTransferEncoding + kEOL);
        this->mailPayload.push_back("Content-Disposition: attachment;"+kEOL);
        this->mailPayload.push_back("     filename=\"" + baseFileName + "\""+kEOL);
        this->mailPayload.push_back(kEOL);

        // Encoded file

        for (auto str : attachment.encodedContents) {
            this->mailPayload.push_back(str);
        }

        this->mailPayload.push_back(kEOL); // EMPTY LINE 

    }


}

//
// Build email message in a dqeue of std::strings to be sent.
//

void CMailSMTP::buildMailPayload(void) {

    bool bAttachments = !this->attachedFiles.empty();

    // Email header.

    this->mailPayload.push_back("Date: " + CMailSMTP::currentDateAndTime() + kEOL);
    this->mailPayload.push_back("To: " + this->addressTo + kEOL);
    this->mailPayload.push_back("From: " + this->addressFrom + kEOL);

    if (!this->addressCC.empty()) {
        this->mailPayload.push_back("cc: " + this->addressCC + kEOL);
    }

    this->mailPayload.push_back("Subject: " + this->mailSubject + kEOL);
    this->mailPayload.push_back("MIME-Version: 1.0"+kEOL);

    if (!bAttachments) {
        this->mailPayload.push_back("Content-Type: text/plain; charset=UTF-8"+kEOL);
        this->mailPayload.push_back("Content-Transfer-Encoding: 7bit"+kEOL);
    } else {
        this->mailPayload.push_back("Content-Type: multipart/mixed;"+kEOL);
        this->mailPayload.push_back("     boundary=\"" + CMailSMTP::kMimeBoundary + "\""+kEOL);
    }

    this->mailPayload.push_back(kEOL); // EMPTY LINE 

    if (bAttachments) {
        this->mailPayload.push_back("--" + CMailSMTP::kMimeBoundary + kEOL);
        this->mailPayload.push_back("Content-Type: text/plain"+kEOL);
        this->mailPayload.push_back("Content-Transfer-Encoding: 7bit"+kEOL);
        this->mailPayload.push_back(kEOL); // EMPTY LINE 
    }

    // Message body

    for (auto str : this->mailMessage) {
        this->mailPayload.push_back(str + kEOL);
    }

 
    if (bAttachments) {
        this->mailPayload.push_back(kEOL); // EMPTY LINE 
        this->buildAttachments();
        this->mailPayload.push_back("--" + CMailSMTP::kMimeBoundary + "--"+kEOL);
    }

}

// ==============
// PUBLIC METHODS
// ==============

//
// Set STMP server URL
// 

void CMailSMTP::setServer(const std::string& serverURL) {

    this->serverURL = serverURL;
    
}

//
// Set email account details
//

void CMailSMTP::setUserAndPassword(const std::string& userName, const std::string& userPassword) {

    this->userName = userName;
    this->userPassword = userPassword;

}

//
// Set From address
//

void CMailSMTP::setFromAddress(const std::string& addressFrom) {

    this->addressFrom = addressFrom;
}

//
// Set To address
//

void CMailSMTP::setToAddress(const std::string& addressTo) {

    this->addressTo = addressTo;
}

//
// Set CC recipient address
//

void CMailSMTP::setCCAddress(const std::string& addressCC) {

    this->addressCC = addressCC;
}


//
// Set email subject
//

void CMailSMTP::setMailSubject(const std::string& mailSubject) {

    this->mailSubject = mailSubject;

}

//
// Set body of email message
//

void CMailSMTP::setMailMessage(const std::vector<std::string>& mailMessage) {
    this->mailMessage = mailMessage;
}

//
// Add file attachment. Try to find MIME mapping for file extension from internal table 
// but if not found then use passed in value as a fallback.
// 

void CMailSMTP::addFileAttachment(const std::string& fileName, const std::string& contentType, const std::string& contentTransferEncoding) {

    std::string mimeMapping(contentType);
    std::string baseFileName = basename(fileName.c_str());
    std::size_t fullStop = baseFileName.find_last_of('.');
    
    if (fullStop != std::string::npos) {
        baseFileName = baseFileName.substr(fullStop+1);
        auto foundMapping= CMailSMTP::extToMimeType.find(baseFileName);
        if (foundMapping != CMailSMTP::extToMimeType.end()) {
            mimeMapping = foundMapping->second;
        }
    }

    this->attachedFiles.push_back({fileName, mimeMapping, contentTransferEncoding});

}

//
// Post email
//

void CMailSMTP::postMail(void) {

    char errMsgBuffer[CURL_ERROR_SIZE];
    
    this->curl = curl_easy_init();

    if (this->curl) {

        curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_SMTP | CURLPROTO_SMTPS);
        
        curl_easy_setopt(this->curl, CURLOPT_USERNAME, this->userName.c_str());
        curl_easy_setopt(this->curl, CURLOPT_PASSWORD, this->userPassword.c_str());
        curl_easy_setopt(this->curl, CURLOPT_URL, this->serverURL.c_str());

        curl_easy_setopt(this->curl, CURLOPT_USE_SSL, (long) CURLUSESSL_ALL);

        curl_easy_setopt(this->curl, CURLOPT_ERRORBUFFER, errMsgBuffer);
        
        if (!this->mailCABundle.empty()) {
            curl_easy_setopt(this->curl, CURLOPT_CAINFO, this->mailCABundle.c_str());
        }

        curl_easy_setopt(this->curl, CURLOPT_MAIL_FROM, this->addressFrom.c_str());

        this->recipients = curl_slist_append(this->recipients, this->addressTo.c_str());

        if (!this->addressCC.empty()) {
            this->recipients = curl_slist_append(this->recipients, this->addressCC.c_str());
        }

        curl_easy_setopt(this->curl, CURLOPT_MAIL_RCPT, this->recipients);

        this->buildMailPayload();

        curl_easy_setopt(this->curl, CURLOPT_READFUNCTION, CMailSMTP::payloadSource);
        curl_easy_setopt(this->curl, CURLOPT_READDATA, &this->mailPayload);
        curl_easy_setopt(this->curl, CURLOPT_UPLOAD, 1L);

        curl_easy_setopt(this->curl, CURLOPT_VERBOSE, 0L);
        
        errMsgBuffer[0] = 0;
        this->res = curl_easy_perform(this->curl);

        // Check for errors

        if (this->res != CURLE_OK) {
            std::string errMsg;
            if (std::strlen(errMsgBuffer)!=0) {
                errMsg = errMsgBuffer;
            } else {
                errMsg=curl_easy_strerror(res);
            }
            throw CMailSMTP::Exception("curl_easy_perform() failed: "+errMsg);
        }
        
        // Clear sent email
        
        this->mailPayload.clear();

        // Free the list of this->recipients

        curl_slist_free_all(this->recipients);

        // Always cleanup

        curl_easy_cleanup(curl);

    }

}

//
// Get email message body
//

std::string CMailSMTP::getMailMessage() {
 
    std::string mailMessage;
    
    this->buildMailPayload();
    
    for (auto line : this->mailPayload) {
        mailMessage.append(line);
    }
    
    this->mailPayload.clear();
    
    return(mailMessage);
    
}

//
// Main CMailSend object constructor. 
//

CMailSMTP::CMailSMTP() {

}

//
// CMailSend Destructor
//

CMailSMTP::~CMailSMTP() {

}

//
// CMailSend initialisation. Glocally init curl and load MIME types.
//

void CMailSMTP::init(void) {

    if (curl_global_init(CURL_GLOBAL_ALL)) {
        throw CMailSMTP::Exception("curl_global_init() : failure to initialize libcurl.");
    }
    
    loadMIMETypes();
        
}

//
// CMailSend closedown
//

void CMailSMTP::closedown(void) {
    
    curl_global_cleanup();

}

