/*
 * File:   CMailSend.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2016, 2:33 PM
 *
 * Copyright 2016.
 *
 */

//
// Class: CMailSend
// 
// Description: Class that enables an email to be setup and sent
// to a specified address using the libcurl library. SSL is supported
// (but untested) and attached files in either 7bit or base64 encoded
// format. When adding an atatchment it creates its MIME type by first
// checking an internal table created from /etc/mime.types. If no 
// mapping is present it uses the pass in value as default.
//
// Dependencies: C11++, libcurl, Linux.
//

// =================
// CLASS DEFINITIONS
// =================

#include "CMailSend.hpp"

// ====================
// CLASS IMPLEMENTATION
// ====================

// ===========================
// PRIVATE TYPES AND CONSTANTS
// ===========================


// MIME multi-part text boundary string 

const std::string CMailSend::kMimeBoundary("xxxxCMailSendBoundaryText");

// Line terminator

const std::string CMailSend::kEOL("\r\n");

// ==========================
// PUBLIC TYPES AND CONSTANTS
// ==========================

// Supported encoding methods

const std::string CMailSend::kEncoding7Bit("7Bit");
const std::string CMailSend::kEncodingBase64("base64");

// ========================
// PRIVATE STATIC VARIABLES
// ========================

// File extension to MIME type

std::unordered_map<std::string, std::string> CMailSend::extToMimeType;

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

void CMailSend::loadMIMETypes (void) {
    
    std::ifstream mimeFile("/etc/mime.types");
    std::string   extension, mimeType, line;

    while (std::getline(mimeFile, line)) {
        if (line[0] != '#') {
            std::istringstream iss(line);
            iss >> mimeType;
            while (iss.good()) {
                iss >> extension;
                if (!extension.empty()) {
                    CMailSend::extToMimeType.insert({extension, mimeType});
                }
            }
        }
    }
      
}

//
// Get string for current date time.
//

const std::string CMailSend::currentDateAndTime(void) {
    
   time_t rawtime;
   struct tm *info;
   char buffer[80];

   time( &rawtime );
   info = localtime( &rawtime );
   strftime(buffer,80,"%a, %d %b %Y %H:%M:%S %z", info);
   return(std::string(buffer));

}

//
// Next line of email required by libcurl so copy
//

size_t CMailSend::payloadSource(void *ptr, size_t size, size_t nmemb, void *userData) {

    CMailSend::UploadStatus *uploadContext = (CMailSend::UploadStatus *) userData;

    if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1)) {
        return 0;
    }
    
    return uploadContext->mailPayload[uploadContext->linesRead++].copy(static_cast<char *> (ptr), size*nmemb, 0);
    
}

//
// Encode bytes to base64 string
//

void CMailSend::encodeToBase64(uint8_t const* bytesToEncode, uint32_t numberOfBytes, std::string& encodedString) {

    static const char cb64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    int trailing;
    register uint8_t byte1, byte2, byte3;

    trailing = (numberOfBytes % 3); // Trailing bytes
    numberOfBytes /= 3; // No of 3 byte values to encode

    while (numberOfBytes--) {

        byte1 = *(bytesToEncode++);
        byte2 = *(bytesToEncode++);
        byte3 = *(bytesToEncode++);

        encodedString += cb64[(byte1 & 0xfc) >> 2];
        encodedString += cb64[((byte1 & 0x03) << 4) + ((byte2 & 0xf0) >> 4)];
        encodedString += cb64[((byte2 & 0x0f) << 2) + ((byte3 & 0xc0) >> 6)];
        encodedString += cb64[byte3 & 0x3f];

    }

    // One trailing byte

    if (trailing == 1) {
        byte1 = *(bytesToEncode++);
        encodedString += cb64[(byte1 & 0xfc) >> 2];
        encodedString += cb64[((byte1 & 0x03) << 4)];
        encodedString += '=';
        encodedString += '=';

        // Two trailing bytes

    } else if (trailing == 2) {
        byte1 = *(bytesToEncode++);
        byte2 = *(bytesToEncode++);
        encodedString += cb64[(byte1 & 0xfc) >> 2];
        encodedString += cb64[((byte1 & 0x03) << 4) + ((byte2 & 0xf0) >> 4)];
        encodedString += cb64[((byte2 & 0x0f) << 2)];
        encodedString += '=';
    }

}

//
// Encode a specified file in either 7bit or base64.
//

void CMailSend::encodeAttachment(CMailSend::emailAttachment& attachment) {

    std::string line;

    // 7bit just copy

    if ((attachment.contentTransferEncoding.compare(CMailSend::kEncodingBase64) != 0)) {

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
        std::unique_ptr<std::uint8_t> buffer(new uint8_t [CMailSend::kBase64EncodeBufferSize]);

        ifs.seekg(0, std::ios::beg);
        while (ifs.good()) {
            ifs.read((char *)buffer.get(), CMailSend::kBase64EncodeBufferSize);
            this->encodeToBase64(buffer.get(), ifs.gcount(), line);
            attachment.encodedContents.push_back(line + kEOL);
            line.clear();
        }

    }

}

//
// Place attachments into email payload
//

void CMailSend::buildAttachments(void) {

    for (auto attachment : this->attachedFiles) {

        std::string baseFileName = basename(attachment.fileName.c_str());

        this->encodeAttachment(attachment);

        this->uploadContext.mailPayload.push_back("--" + CMailSend::kMimeBoundary + kEOL);
        this->uploadContext.mailPayload.push_back("Content-Type: " + attachment.contentTypes + ";"+kEOL);
        this->uploadContext.mailPayload.push_back("Content-transfer-encoding: " + attachment.contentTransferEncoding + kEOL);
        this->uploadContext.mailPayload.push_back("Content-Disposition: attachment;"+kEOL);
        this->uploadContext.mailPayload.push_back("     filename=\"" + baseFileName + "\""+kEOL);
        this->uploadContext.mailPayload.push_back(kEOL);

        // Encoded file

        for (auto str : attachment.encodedContents) {
            this->uploadContext.mailPayload.push_back(str);
        }

        this->uploadContext.mailPayload.push_back(kEOL); // EMPTY LINE 

    }


}

//
// Build email message in a vector of std::strings to be sent.
//

void CMailSend::buildMailPayload(void) {

    bool bAttachments = !this->attachedFiles.empty();

    // Email header.

    this->uploadContext.mailPayload.push_back("Date: " + CMailSend::currentDateAndTime() + kEOL);
    this->uploadContext.mailPayload.push_back("To: " + this->addressTo + kEOL);
    this->uploadContext.mailPayload.push_back("From: " + this->addressFrom + kEOL);

    if (!this->addressCC.empty()) {
        this->uploadContext.mailPayload.push_back("cc: " + this->addressCC + kEOL);
    }

    this->uploadContext.mailPayload.push_back("Subject: " + this->mailSubject + kEOL);
    this->uploadContext.mailPayload.push_back("MIME-Version: 1.0"+kEOL);

    if (!bAttachments) {
        this->uploadContext.mailPayload.push_back("Content-Type: text/plain; charset=UTF-8"+kEOL);
        this->uploadContext.mailPayload.push_back("Content-Transfer-Encoding: 7bit"+kEOL);
    } else {
        this->uploadContext.mailPayload.push_back("Content-Type: multipart/mixed;"+kEOL);
        this->uploadContext.mailPayload.push_back("     boundary=\"" + CMailSend::kMimeBoundary + "\""+kEOL);
    }

    this->uploadContext.mailPayload.push_back(kEOL); // EMPTY LINE 

    if (bAttachments) {
        this->uploadContext.mailPayload.push_back("--" + CMailSend::kMimeBoundary + kEOL);
        this->uploadContext.mailPayload.push_back("Content-Type: text/plain"+kEOL);
        this->uploadContext.mailPayload.push_back("Content-Transfer-Encoding: 7bit"+kEOL);
        this->uploadContext.mailPayload.push_back(kEOL); // EMPTY LINE 
    }

    // Message body

    for (auto str : this->mailMessage) {
        this->uploadContext.mailPayload.push_back(str + kEOL);
    }

    this->uploadContext.mailPayload.push_back(kEOL); // EMPTY LINE 

    if (bAttachments) {
        this->buildAttachments();
        this->uploadContext.mailPayload.push_back("--" + CMailSend::kMimeBoundary + "--"+kEOL);
    }

    // End of message

    this->uploadContext.mailPayload.push_back("");
    

}

// ==============
// PUBLIC METHODS
// ==============

//
// Set STMP server URL
// 

void CMailSend::setServer(const std::string& serverURL) {

    this->serverURL = serverURL;
}

//
// Set email account details
//

void CMailSend::setUserAndPassword(const std::string& userName, const std::string& userPassword) {

    this->userName = userName;
    this->userPassword = userPassword;

}

//
// Set From address
//

void CMailSend::setFromAddress(const std::string& addressFrom) {

    this->addressFrom = addressFrom;
}

//
// Set To address
//

void CMailSend::setToAddress(const std::string& addressTo) {

    this->addressTo = addressTo;
}

//
// Set CC recipient address
//

void CMailSend::setCCAddress(const std::string& addressCC) {

    this->addressCC = addressCC;
}


//
// Set email subject
//

void CMailSend::setMailSubject(const std::string& mailSubject) {

    this->mailSubject = mailSubject;

}

//
// Set body of email message
//

void CMailSend::setMailMessage(const std::vector<std::string>& mailMessage) {
    this->mailMessage = mailMessage;
}

//
// Add file attachment. Try to find MIME mapping for file extension from internal table 
// but if not found then use passed in value as a fallback.
// 

void CMailSend::addFileAttachment(const std::string& fileName, const std::string& contentType, const std::string& contentTransferEncoding) {

    std::string mimeMapping(contentType);
    std::string baseFileName = basename(fileName.c_str());
    std::size_t fullStop = baseFileName.find_last_of('.');
    
    if (fullStop != std::string::npos) {
        baseFileName = baseFileName.substr(fullStop+1);
        auto foundMapping= CMailSend::extToMimeType.find(baseFileName);
        if (foundMapping != CMailSend::extToMimeType.end()) {
            mimeMapping = foundMapping->second;
        }
    }

    this->attachedFiles.push_back({fileName, mimeMapping, contentTransferEncoding});

}

//
// Post email
//

void CMailSend::postMail(void) {

    char errMsgBuffer[CURL_ERROR_SIZE];
    
    this->uploadContext.linesRead = 0;

    this->curl = curl_easy_init();

    if (this->curl) {

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

        curl_easy_setopt(this->curl, CURLOPT_READFUNCTION, CMailSend::payloadSource);
        curl_easy_setopt(this->curl, CURLOPT_READDATA, &this->uploadContext);
        curl_easy_setopt(this->curl, CURLOPT_UPLOAD, 1L);

        curl_easy_setopt(this->curl, CURLOPT_VERBOSE, 0L);
        
        errMsgBuffer[0] = 0;
        this->res = curl_easy_perform(this->curl);

        /* Check for errors */

        if (this->res != CURLE_OK) {
            std::string errMsg;
            if (std::strlen(errMsgBuffer)!=0) {
                errMsg = errMsgBuffer;
            } else {
                errMsg=curl_easy_strerror(res);
            }
            throw std::runtime_error(std::string("curl_easy_perform() failed: ")+errMsg);
        } 

        /* Free the list of this->recipients */

        curl_slist_free_all(this->recipients);

        /* Always cleanup */

        curl_easy_cleanup(curl);

    }

}


//
// Main CMailSend object constructor. 
//

CMailSend::CMailSend() {

}

//
// CMailSend Destructor
//

CMailSend::~CMailSend() {

}

//
// CMailSend initialisation. Glocally init curl and load MIME types.
//

void CMailSend::init(void) {
    
    curl_global_init(CURL_GLOBAL_ALL);
    
    loadMIMETypes();
        
}

//
// CMailSend closedown
//

void CMailSend::closedown(void) {
    
    curl_global_cleanup();

}

