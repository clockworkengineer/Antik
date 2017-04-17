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

    const char *CSMTP::kMimeBoundaryStr { "xxxxCSMTPBoundaryText" };

    // Line terminator

    const char *CSMTP::kEOLStr { "\r\n" };

    // Valid characters for base64 encode/decode.

    const char CSMTP::kCB64[] { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" };

    // ==========================
    // PUBLIC TYPES AND CONSTANTS
    // ==========================

    // Supported encoding methods

    const char *CSMTP::kEncoding7BitStr { "7Bit" };
    const char *CSMTP::kEncodingBase64Str { "base64" };

    // ========================
    // PRIVATE STATIC VARIABLES
    // ========================

    // curl verbosity setting

    bool CSMTP::bCurlVerbosity { false };

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

        std::time_t rawtime { 0 };
        struct std::tm *info { 0 };
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

        size_t bytesCopied { 0 };

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

        std::string lineStr;

        // 7bit just copy

        if ((attachment.contentTransferEncodingStr.compare(kEncodingBase64Str) != 0)) {

            std::ifstream attachmentFile(attachment.fileNameStr);

            // As sending text file via email strip any host specific end of line and replace with <cr><lf>

            while (std::getline(attachmentFile, lineStr)) {
                if (lineStr.back() == '\n') lineStr.pop_back();
                if (lineStr.back() == '\r') lineStr.pop_back();
                attachment.encodedContents.push_back(lineStr + kEOLStr);
            }

            // Base64

        } else {

            std::ifstream ifs { attachment.fileNameStr, std::ios::binary };
            std::string buffer(kBase64EncodeBufferSize, ' ');

            ifs.seekg(0, std::ios::beg);
            while (ifs.good()) {
                ifs.read(&buffer[0], kBase64EncodeBufferSize);
                this->encodeToBase64(buffer, lineStr, ifs.gcount());
                attachment.encodedContents.push_back(lineStr + kEOLStr);
                lineStr.clear();
            }

        }

    }

    //
    // Place attachments into email payload
    //

    void CSMTP::buildAttachments(void) {

        for (auto attachment : this->attachedFiles) {

            std::string baseFileNameStr { attachment.fileNameStr.substr(attachment.fileNameStr.find_last_of("/\\") + 1) };

            this->encodeAttachment(attachment);

            this->mailPayload.push_back(std::string("--") + kMimeBoundaryStr + kEOLStr);
            this->mailPayload.push_back("Content-Type: " + attachment.contentTypesStr + ";" + kEOLStr);
            this->mailPayload.push_back("Content-transfer-encoding: " + attachment.contentTransferEncodingStr + kEOLStr);
            this->mailPayload.push_back(std::string("Content-Disposition: attachment;") + kEOLStr);
            this->mailPayload.push_back("     filename=\"" + baseFileNameStr + "\"" + kEOLStr);
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

        bool bAttachments { !this->attachedFiles.empty() };

        // Email header.

        this->mailPayload.push_back("Date: " + currentDateAndTime() + kEOLStr);
        this->mailPayload.push_back("To: " + this->addressToStr + kEOLStr);
        this->mailPayload.push_back("From: " + this->addressFromStr + kEOLStr);

        if (!this->addressCCStr.empty()) {
            this->mailPayload.push_back("cc: " + this->addressCCStr + kEOLStr);
        }

        this->mailPayload.push_back("Subject: " + this->mailSubjectStr + kEOLStr);
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

    void CSMTP::setServer(const std::string& serverURLStr) {

        this->serverURLStr = serverURLStr;

    }

    //
    // Get STMP server URL
    // 

    std::string CSMTP::getServer(void) const {

        return (this->serverURLStr);

    }


    //
    // Set email account details
    //

    void CSMTP::setUserAndPassword(const std::string& userNameStr,
            const std::string& userPasswordStr) {

        this->userNameStr = userNameStr;
        this->userPasswordStr = userPasswordStr;

    }

    //
    // Get email account user
    //

    std::string CSMTP::getUser(void) const {

        return (this->userNameStr);

    }

    //
    // Set From address
    //

    void CSMTP::setFromAddress(const std::string& addressFromStr) {

        this->addressFromStr = addressFromStr;
    }

    //
    // Get From address
    //

    std::string CSMTP::getFromAddress(void) const {

        return (this->addressFromStr);

    }

    //
    // Set To address
    //

    void CSMTP::setToAddress(const std::string& addressToStr) {

        this->addressToStr = addressToStr;

    }

    //
    // Get To address
    //

    std::string CSMTP::getToAddress(void) const {

        return (this->addressToStr);

    }

    //
    // Set CC recipient address
    //

    void CSMTP::setCCAddress(const std::string& addressCCStr) {

        this->addressCCStr = addressCCStr;
    }

    //
    // Get CC recipient address
    //

    std::string CSMTP::getCCAddress(void) const {

        return (this->addressCCStr);

    }

    //
    // Set email subject
    //

    void CSMTP::setMailSubject(const std::string& mailSubjectStr) {

        this->mailSubjectStr = mailSubjectStr;

    }

    //
    // Get email subject
    //

    std::string CSMTP::getMailSubject(void) const {

        return (this->mailSubjectStr);

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

        std::string mailMessageStr;

        for (auto line : this->mailMessage) {
            mailMessageStr.append(line);
        }

        return (mailMessageStr);

    }

    //
    // Add file attachment.
    // 

    void CSMTP::addFileAttachment(const std::string& fileNameStr,
            const std::string& contentTypeStr,
            const std::string& contentTransferEncodingStr) {

        this->attachedFiles.push_back({fileNameStr, contentTypeStr, contentTransferEncodingStr});

    }

    //
    // Post email
    //

    void CSMTP::postMail(void) {

        this->curlHandle = curl_easy_init();

        if (this->curlHandle) {

            curl_easy_setopt(curlHandle, CURLOPT_PROTOCOLS, CURLPROTO_SMTP | CURLPROTO_SMTPS);

            curl_easy_setopt(this->curlHandle, CURLOPT_USERNAME, this->userNameStr.c_str());
            curl_easy_setopt(this->curlHandle, CURLOPT_PASSWORD, this->userPasswordStr.c_str());
            curl_easy_setopt(this->curlHandle, CURLOPT_URL, this->serverURLStr.c_str());

            curl_easy_setopt(this->curlHandle, CURLOPT_USE_SSL, (long) CURLUSESSL_ALL);

            curl_easy_setopt(this->curlHandle, CURLOPT_ERRORBUFFER, this->curlErrMsgBuffer);

            if (!this->mailCABundleStr.empty()) {
                curl_easy_setopt(this->curlHandle, CURLOPT_CAINFO, this->mailCABundleStr.c_str());
            }

            curl_easy_setopt(this->curlHandle, CURLOPT_MAIL_FROM, this->addressFromStr.c_str());

            this->curlRecipients = curl_slist_append(this->curlRecipients, this->addressToStr.c_str());

            if (!this->addressCCStr.empty()) {
                this->curlRecipients = curl_slist_append(this->curlRecipients, this->addressCCStr.c_str());
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
                std::string errMsgStr;
                if (std::strlen(this->curlErrMsgBuffer) != 0) {
                    errMsgStr = this->curlErrMsgBuffer;
                } else {
                    errMsgStr = curl_easy_strerror(curlResult);
                }
                throw Exception("curl_easy_perform() failed: " + errMsgStr);
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

    void CSMTP::encodeToBase64(const std::string& decodedStringStr,
            std::string& encodedStringStr, uint32_t numberOfBytes) {

        int trailing, byteIndex = 0;
        register uint8_t byte1, byte2, byte3;

        if (numberOfBytes == 0) {
            return;
        }

        encodedStringStr.clear();

        trailing = (numberOfBytes % 3); // Trailing bytes
        numberOfBytes /= 3; // No of 3 byte values to encode

        while (numberOfBytes--) {

            byte1 = decodedStringStr[byteIndex++];
            byte2 = decodedStringStr[byteIndex++];
            byte3 = decodedStringStr[byteIndex++];

            encodedStringStr += kCB64[(byte1 & 0xfc) >> 2];
            encodedStringStr += kCB64[((byte1 & 0x03) << 4) + ((byte2 & 0xf0) >> 4)];
            encodedStringStr += kCB64[((byte2 & 0x0f) << 2) + ((byte3 & 0xc0) >> 6)];
            encodedStringStr += kCB64[byte3 & 0x3f];

        }

        // One trailing byte

        if (trailing == 1) {
            byte1 = decodedStringStr[byteIndex++];
            encodedStringStr += kCB64[(byte1 & 0xfc) >> 2];
            encodedStringStr += kCB64[((byte1 & 0x03) << 4)];
            encodedStringStr += '=';
            encodedStringStr += '=';

            // Two trailing bytes

        } else if (trailing == 2) {
            byte1 = decodedStringStr[byteIndex++];
            byte2 = decodedStringStr[byteIndex++];
            encodedStringStr += kCB64[(byte1 & 0xfc) >> 2];
            encodedStringStr += kCB64[((byte1 & 0x03) << 4) + ((byte2 & 0xf0) >> 4)];
            encodedStringStr += kCB64[((byte2 & 0x0f) << 2)];
            encodedStringStr += '=';
        }

    }

    //
    // Decode string from base64 encoded string.
    //

    void CSMTP::decodeFromBase64(const std::string& encodedStr,
            std::string& decodedStr, uint32_t numberOfBytes) {

        int byteIndex { 0 };
        register uint8_t byte1, byte2, byte3, byte4;

        if ((numberOfBytes == 0) || (numberOfBytes % 4)) {
            return;
        }

        decodedStr.clear();

        numberOfBytes = (numberOfBytes / 4);
        while (numberOfBytes--) {

            byte1 = encodedStr[byteIndex++];
            byte2 = encodedStr[byteIndex++];
            byte3 = encodedStr[byteIndex++];
            byte4 = encodedStr[byteIndex++];

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

            decodedStr += ((byte1 << 2) + ((byte2 & 0x30) >> 4));
            decodedStr += (((byte2 & 0xf) << 4) + ((byte3 & 0x3c) >> 2));
            decodedStr += (((byte3 & 0x3) << 6) + byte4);

        }

    }

    //
    // Get whole of email message (including headers and encoded attachments).
    //

    std::string CSMTP::getMailFull(void) {

        std::string mailMessageStr;

        this->buildMailPayload();

        for (auto line : this->mailPayload) {
            mailMessageStr.append(line);
        }

        this->mailPayload.clear();

        return (mailMessageStr);

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
