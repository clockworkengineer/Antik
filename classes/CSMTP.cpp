#include "HOST.hpp"
/*
 * File:   CSMTP.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2016, 2:33 PM
 *
 * Copyright 2016.
 *
 */

//
// Class: CSMTP
// 
// Description: Class that enables an email to be setup and sent
// to a specified address using the libcurl library. SSL is supported
// and attached files in either 7bit or base64 encoded format.
//
// Dependencies:   C11++     - Language standard features used.
//                 libcurl   - Used to talk to SMTP server.
//

// =================
// CLASS DEFINITIONS
// =================

#include "CSMTP.hpp"

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
#include <sstream>

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace Mail {

    // ===========================
    // PRIVATE TYPES AND CONSTANTS
    // ===========================


    // MIME multi-part text boundary string 

    const char *CSMTP::kMimeBoundaryStr = "xxxxCSMTPBoundaryText";

    // Line terminator

    const char *CSMTP::kEOLStr = "\r\n";

    // Valid characters for base64 encode/decode.

    const char CSMTP::kCB64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    // ==========================
    // PUBLIC TYPES AND CONSTANTS
    // ==========================

    // Supported encoding methods

    const char *CSMTP::kEncoding7BitStr = "7Bit";
    const char *CSMTP::kEncodingBase64Str = "base64";

    // ========================
    // PRIVATE STATIC VARIABLES
    // ========================

    // curl verbosity setting

    bool CSMTP::bCurlVerbosity = false;

    // =======================
    // PUBLIC STATIC VARIABLES
    // =======================

    // ===============
    // PRIVATE METHODS
    // ===============

    //
    // Get string for current date time. Note: Resizing buffer effectively removes 
    // the null character added to the end of the string by strftime().
    //

    const std::string CSMTP::currentDateAndTime(void) {

        std::time_t rawtime;
        struct std::tm *info;
        std::string buffer(80, ' ');

        std::time(&rawtime);
        info = std::localtime(&rawtime);
        buffer.resize(std::strftime(&buffer[0], buffer.length(), "%a, %d %b %Y %H:%M:%S %z", info));
        return (buffer);

    }

    //
    // Fill libcurl read request buffer.
    //

    size_t CSMTP::payloadSource(void *ptr, size_t size, size_t nmemb,
            std::deque<std::string> *mailPayload) {

        size_t bytesCopied = 0;

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
    // Encode a specified file in either 7bit or base64.
    //

    void CSMTP::encodeAttachment(CSMTP::EmailAttachment& attachment) {

        std::string line;

        // 7bit just copy

        if ((attachment.contentTransferEncoding.compare(kEncodingBase64Str) != 0)) {

            std::ifstream attachmentFile(attachment.fileName);

            // As sending text file via email strip any host specific end of line and replace with <cr><lf>

            while (std::getline(attachmentFile, line)) {
                if (line.back() == '\n') line.pop_back();
                if (line.back() == '\r') line.pop_back();
                attachment.encodedContents.push_back(line + kEOLStr);
            }

            // Base64

        } else {

            std::ifstream ifs(attachment.fileName, std::ios::binary);
            std::string buffer(kBase64EncodeBufferSize, ' ');

            ifs.seekg(0, std::ios::beg);
            while (ifs.good()) {
                ifs.read(&buffer[0], kBase64EncodeBufferSize);
                this->encodeToBase64(buffer, line, ifs.gcount());
                attachment.encodedContents.push_back(line + kEOLStr);
                line.clear();
            }

        }

    }

    //
    // Place attachments into email payload
    //

    void CSMTP::buildAttachments(void) {

        for (auto attachment : this->attachedFiles) {

            std::string baseFileName = attachment.fileName.substr(attachment.fileName.find_last_of("/\\") + 1);

            this->encodeAttachment(attachment);

            this->mailPayload.push_back(std::string("--") + kMimeBoundaryStr + kEOLStr);
            this->mailPayload.push_back("Content-Type: " + attachment.contentTypes + ";" + kEOLStr);
            this->mailPayload.push_back("Content-transfer-encoding: " + attachment.contentTransferEncoding + kEOLStr);
            this->mailPayload.push_back(std::string("Content-Disposition: attachment;") + kEOLStr);
            this->mailPayload.push_back("     filename=\"" + baseFileName + "\"" + kEOLStr);
            this->mailPayload.push_back(kEOLStr);

            // Encoded file

            for (auto str : attachment.encodedContents) {
                this->mailPayload.push_back(str);
            }

            this->mailPayload.push_back(kEOLStr); // EMPTY LINE 

        }


    }

    //
    // Build email message in a dqeue of std::strings to be sent.
    //

    void CSMTP::buildMailPayload(void) {

        bool bAttachments = !this->attachedFiles.empty();

        // Email header.

        this->mailPayload.push_back("Date: " + currentDateAndTime() + kEOLStr);
        this->mailPayload.push_back("To: " + this->addressTo + kEOLStr);
        this->mailPayload.push_back("From: " + this->addressFrom + kEOLStr);

        if (!this->addressCC.empty()) {
            this->mailPayload.push_back("cc: " + this->addressCC + kEOLStr);
        }

        this->mailPayload.push_back("Subject: " + this->mailSubject + kEOLStr);
        this->mailPayload.push_back(std::string("MIME-Version: 1.0") + kEOLStr);

        if (!bAttachments) {
            this->mailPayload.push_back(std::string("Content-Type: text/plain; charset=UTF-8") + kEOLStr);
            this->mailPayload.push_back(std::string("Content-Transfer-Encoding: 7bit") + kEOLStr);
        } else {
            this->mailPayload.push_back(std::string("Content-Type: multipart/mixed;") + kEOLStr);
            this->mailPayload.push_back(std::string("     boundary=\"") + kMimeBoundaryStr + "\"" + kEOLStr);
        }

        this->mailPayload.push_back(kEOLStr); // EMPTY LINE 

        if (bAttachments) {
            this->mailPayload.push_back(std::string("--") + kMimeBoundaryStr + kEOLStr);
            this->mailPayload.push_back(std::string("Content-Type: text/plain") + kEOLStr);
            this->mailPayload.push_back(std::string("Content-Transfer-Encoding: 7bit") + kEOLStr);
            this->mailPayload.push_back(kEOLStr); // EMPTY LINE 
        }

        // Message body

        for (auto str : this->mailMessage) {
            this->mailPayload.push_back(str + kEOLStr);
        }


        if (bAttachments) {
            this->mailPayload.push_back(kEOLStr); // EMPTY LINE 
            this->buildAttachments();
            this->mailPayload.push_back(std::string("--") + kMimeBoundaryStr + "--" + kEOLStr);
        }

    }

    //
    // Decode character to base64 index.
    //

    inline int CSMTP::decodeChar(char ch) {

        int index = 0;
        do {
            if (ch == kCB64[index]) return (index);
        } while (kCB64[index++]);

        return (0);

    }

    // ==============
    // PUBLIC METHODS
    // ==============

    //
    // Set STMP server URL
    // 

    void CSMTP::setServer(const std::string& serverURL) {

        this->serverURL = serverURL;

    }

    //
    // Get STMP server URL
    // 

    std::string CSMTP::getServer(void) const {

        return (this->serverURL);

    }


    //
    // Set email account details
    //

    void CSMTP::setUserAndPassword(const std::string& userName,
            const std::string& userPassword) {

        this->userName = userName;
        this->userPassword = userPassword;

    }

    //
    // Get email account user
    //

    std::string CSMTP::getUser(void) const {

        return (this->userName);

    }

    //
    // Set From address
    //

    void CSMTP::setFromAddress(const std::string& addressFrom) {

        this->addressFrom = addressFrom;
    }

    //
    // Get From address
    //

    std::string CSMTP::getFromAddress(void) const {

        return (this->addressFrom);

    }

    //
    // Set To address
    //

    void CSMTP::setToAddress(const std::string& addressTo) {

        this->addressTo = addressTo;

    }

    //
    // Get To address
    //

    std::string CSMTP::getToAddress(void) const {

        return (this->addressTo);

    }

    //
    // Set CC recipient address
    //

    void CSMTP::setCCAddress(const std::string& addressCC) {

        this->addressCC = addressCC;
    }

    //
    // Get CC recipient address
    //

    std::string CSMTP::getCCAddress(void) const {

        return (this->addressCC);

    }

    //
    // Set email subject
    //

    void CSMTP::setMailSubject(const std::string& mailSubject) {

        this->mailSubject = mailSubject;

    }

    //
    // Get email subject
    //

    std::string CSMTP::getMailSubject(void) const {

        return (this->mailSubject);

    }

    //
    // Set body of email message
    //

    void CSMTP::setMailMessage(const std::vector<std::string>& mailMessage) {
        this->mailMessage = mailMessage;
    }

    //
    // Get body of email message
    //

    std::string CSMTP::getMailMessage(void) const {

        std::string mailMessage;

        for (auto line : this->mailMessage) {
            mailMessage.append(line);
        }

        return (mailMessage);

    }

    //
    // Add file attachment.
    // 

    void CSMTP::addFileAttachment(const std::string& fileName,
            const std::string& contentType,
            const std::string& contentTransferEncoding) {

        this->attachedFiles.push_back({fileName, contentType, contentTransferEncoding});

    }

    //
    // Post email
    //

    void CSMTP::postMail(void) {

        this->curlHandle = curl_easy_init();

        if (this->curlHandle) {

            curl_easy_setopt(curlHandle, CURLOPT_PROTOCOLS, CURLPROTO_SMTP | CURLPROTO_SMTPS);

            curl_easy_setopt(this->curlHandle, CURLOPT_USERNAME, this->userName.c_str());
            curl_easy_setopt(this->curlHandle, CURLOPT_PASSWORD, this->userPassword.c_str());
            curl_easy_setopt(this->curlHandle, CURLOPT_URL, this->serverURL.c_str());

            curl_easy_setopt(this->curlHandle, CURLOPT_USE_SSL, (long) CURLUSESSL_ALL);

            curl_easy_setopt(this->curlHandle, CURLOPT_ERRORBUFFER, this->curlErrMsgBuffer);

            if (!this->mailCABundle.empty()) {
                curl_easy_setopt(this->curlHandle, CURLOPT_CAINFO, this->mailCABundle.c_str());
            }

            curl_easy_setopt(this->curlHandle, CURLOPT_MAIL_FROM, this->addressFrom.c_str());

            this->curlRecipients = curl_slist_append(this->curlRecipients, this->addressTo.c_str());

            if (!this->addressCC.empty()) {
                this->curlRecipients = curl_slist_append(this->curlRecipients, this->addressCC.c_str());
            }

            curl_easy_setopt(this->curlHandle, CURLOPT_MAIL_RCPT, this->curlRecipients);

            this->buildMailPayload();

            curl_easy_setopt(this->curlHandle, CURLOPT_READFUNCTION, payloadSource);
            curl_easy_setopt(this->curlHandle, CURLOPT_READDATA, &this->mailPayload);
            curl_easy_setopt(this->curlHandle, CURLOPT_UPLOAD, 1L);

            curl_easy_setopt(this->curlHandle, CURLOPT_VERBOSE, bCurlVerbosity);

            curlErrMsgBuffer[0] = 0;
            this->curlResult = curl_easy_perform(this->curlHandle);

            // Check for errors

            if (this->curlResult != CURLE_OK) {
                std::string errMsg;
                if (std::strlen(this->curlErrMsgBuffer) != 0) {
                    errMsg = this->curlErrMsgBuffer;
                } else {
                    errMsg = curl_easy_strerror(curlResult);
                }
                throw Exception("curl_easy_perform() failed: " + errMsg);
            }

            // Clear sent email

            this->mailPayload.clear();

            // Free the list of this->recipients

            curl_slist_free_all(this->curlRecipients);

            // Always cleanup

            curl_easy_cleanup(curlHandle);

        }

    }

    //
    // Encode string to base64 string.
    //

    void CSMTP::encodeToBase64(const std::string& decodedString,
            std::string& encodedString, uint32_t numberOfBytes) {

        int trailing, byteIndex = 0;
        register uint8_t byte1, byte2, byte3;

        if (numberOfBytes == 0) {
            return;
        }

        encodedString.clear();

        trailing = (numberOfBytes % 3); // Trailing bytes
        numberOfBytes /= 3; // No of 3 byte values to encode

        while (numberOfBytes--) {

            byte1 = decodedString[byteIndex++];
            byte2 = decodedString[byteIndex++];
            byte3 = decodedString[byteIndex++];

            encodedString += kCB64[(byte1 & 0xfc) >> 2];
            encodedString += kCB64[((byte1 & 0x03) << 4) + ((byte2 & 0xf0) >> 4)];
            encodedString += kCB64[((byte2 & 0x0f) << 2) + ((byte3 & 0xc0) >> 6)];
            encodedString += kCB64[byte3 & 0x3f];

        }

        // One trailing byte

        if (trailing == 1) {
            byte1 = decodedString[byteIndex++];
            encodedString += kCB64[(byte1 & 0xfc) >> 2];
            encodedString += kCB64[((byte1 & 0x03) << 4)];
            encodedString += '=';
            encodedString += '=';

            // Two trailing bytes

        } else if (trailing == 2) {
            byte1 = decodedString[byteIndex++];
            byte2 = decodedString[byteIndex++];
            encodedString += kCB64[(byte1 & 0xfc) >> 2];
            encodedString += kCB64[((byte1 & 0x03) << 4) + ((byte2 & 0xf0) >> 4)];
            encodedString += kCB64[((byte2 & 0x0f) << 2)];
            encodedString += '=';
        }

    }

    //
    // Decode string from base64 encoded string.
    //

    void CSMTP::decodeFromBase64(const std::string& encodedString,
            std::string& decodedString, uint32_t numberOfBytes) {

        int byteIndex = 0;
        register uint8_t byte1, byte2, byte3, byte4;

        if ((numberOfBytes == 0) || (numberOfBytes % 4)) {
            return;
        }

        decodedString.clear();

        numberOfBytes = (numberOfBytes / 4);
        while (numberOfBytes--) {

            byte1 = encodedString[byteIndex++];
            byte2 = encodedString[byteIndex++];
            byte3 = encodedString[byteIndex++];
            byte4 = encodedString[byteIndex++];

            byte1 = decodeChar(byte1);
            byte2 = decodeChar(byte2);

            if (byte3 == '=') {
                byte3 = 0;
                byte4 = 0;
            } else if (byte4 == '=') {
                byte3 = decodeChar(byte3);
                byte4 = 0;
            } else {
                byte3 = decodeChar(byte3);
                byte4 = decodeChar(byte4);
            }

            decodedString += ((byte1 << 2) + ((byte2 & 0x30) >> 4));
            decodedString += (((byte2 & 0xf) << 4) + ((byte3 & 0x3c) >> 2));
            decodedString += (((byte3 & 0x3) << 6) + byte4);

        }

    }

    //
    // Get whole of email message (including headers and encoded attachments).
    //

    std::string CSMTP::getMailFull(void) {

        std::string mailMessage;

        this->buildMailPayload();

        for (auto line : this->mailPayload) {
            mailMessage.append(line);
        }

        this->mailPayload.clear();

        return (mailMessage);

    }

    //
    // Main CMailSend object constructor. 
    //

    CSMTP::CSMTP() {

    }

    //
    // CMailSend Destructor
    //

    CSMTP::~CSMTP() {

    }

    //
    // CMailSend initialization. Globally init curl.
    //

    void CSMTP::init(bool bCurlVerbosity) {

        if (curl_global_init(CURL_GLOBAL_ALL)) {
            throw Exception("curl_global_init() : failure to initialize libcurl.");
        }

        bCurlVerbosity = bCurlVerbosity;

    }

    //
    // CMailSend closedown
    //

    void CSMTP::closedown(void) {

        curl_global_cleanup();

    }

   } // namespace Mail
} // namespace Antik
