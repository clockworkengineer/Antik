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
// C++ STL
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
    namespace SMTP {

    // ===========================
    // PRIVATE TYPES AND CONSTANTS
    // ===========================


    // MIME multi-part text boundary string 

    const char *CSMTP::kMimeBoundary { "xxxxCSMTPBoundaryText" };

    // Line terminator

    const char *CSMTP::kEOL { "\r\n" };

    // Valid characters for base64 encode/decode.

    const char CSMTP::kCB64[] { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" };

    // ==========================
    // PUBLIC TYPES AND CONSTANTS
    // ==========================

    // Supported encoding methods

    const char *CSMTP::kEncoding7Bit { "7Bit" };
    const char *CSMTP::kEncodingBase64 { "base64" };

    // ========================
    // PRIVATE STATIC VARIABLES
    // ========================

    // curl verbosity setting

    bool CSMTP::m_curlVerbosity { false };

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

        std::string line;

        // 7bit just copy

        if ((attachment.contentTransferEncoding.compare(kEncodingBase64) != 0)) {

            std::ifstream attachmentFile(attachment.fileName);

            // As sending text file via email strip any host specific end of line and replace with <cr><lf>

            while (std::getline(attachmentFile, line)) {
                if (line.back() == '\n') line.pop_back();
                if (line.back() == '\r') line.pop_back();
                attachment.encodedContents.push_back(line + kEOL);
            }

            // Base64

        } else {

            std::ifstream ifs { attachment.fileName, std::ios::binary };
            std::string buffer(kBase64EncodeBufferSize, ' ');

            ifs.seekg(0, std::ios::beg);
            while (ifs.good()) {
                ifs.read(&buffer[0], kBase64EncodeBufferSize);
                encodeToBase64(buffer, line, ifs.gcount());
                attachment.encodedContents.push_back(line + kEOL);
                line.clear();
            }

        }

    }

    //
    // Place attachments into email payload
    //

    void CSMTP::buildAttachments(void) {

        for (auto attachment : m_attachedFiles) {

            std::string baseFileName { attachment.fileName.substr(attachment.fileName.find_last_of(R"(/\)") + 1) };

            encodeAttachment(attachment);

            m_mailPayload.emplace_back(std::string("--") + kMimeBoundary + kEOL);
            m_mailPayload.emplace_back("Content-Type: " + attachment.contentTypes + ";" + kEOL);
            m_mailPayload.emplace_back("Content-transfer-encoding: " + attachment.contentTransferEncoding + kEOL);
            m_mailPayload.emplace_back(std::string("Content-Disposition: attachment;") + kEOL);
            m_mailPayload.emplace_back(R"(     filename=")" + baseFileName + R"(")" + kEOL);
            m_mailPayload.emplace_back(kEOL);

            // Encoded file

            for (auto str : attachment.encodedContents) {
                m_mailPayload.emplace_back(str);
            }

            m_mailPayload.emplace_back(kEOL); // EMPTY LINE 

        }


    }

    //
    // Build email message in a dqeue of std::strings to be sent.
    //

    void CSMTP::buildMailPayload(void) {

        bool bAttachments { !m_attachedFiles.empty() };

        // Email header.

        m_mailPayload.emplace_back("Date: " + currentDateAndTime() + kEOL);
        m_mailPayload.emplace_back("To: " + m_addressTo + kEOL);
        m_mailPayload.emplace_back("From: " + m_addressFrom + kEOL);

        if (!m_addressCC.empty()) {
            m_mailPayload.emplace_back("cc: " + m_addressCC + kEOL);
        }

        m_mailPayload.emplace_back("Subject: " + m_mailSubject + kEOL);
        m_mailPayload.emplace_back(std::string("MIME-Version: 1.0") + kEOL);

        if (!bAttachments) {
            m_mailPayload.emplace_back(std::string("Content-Type: text/plain; charset=UTF-8") + kEOL);
            m_mailPayload.emplace_back(std::string("Content-Transfer-Encoding: 7bit") + kEOL);
        } else {
            m_mailPayload.emplace_back(std::string("Content-Type: multipart/mixed;") + kEOL);
            m_mailPayload.emplace_back(std::string(R"(     boundary=")") + kMimeBoundary + R"(")" + kEOL);
        }

        m_mailPayload.emplace_back(kEOL); // EMPTY LINE 

        if (bAttachments) {
            m_mailPayload.emplace_back(std::string("--") + kMimeBoundary + kEOL);
            m_mailPayload.emplace_back(std::string("Content-Type: text/plain") + kEOL);
            m_mailPayload.emplace_back(std::string("Content-Transfer-Encoding: 7bit") + kEOL);
            m_mailPayload.emplace_back(kEOL); // EMPTY LINE 
        }

        // Message body

        for (auto &str : m_mailMessage) {
            m_mailPayload.emplace_back(str + kEOL);
        }


        if (bAttachments) {
            m_mailPayload.emplace_back(kEOL); // EMPTY LINE 
            buildAttachments();
            m_mailPayload.emplace_back(std::string("--") + kMimeBoundary + "--" + kEOL);
        }

    }

    //
    // Decode character to base64 index.
    //

    int CSMTP::decodeChar(char ch) {

        auto basePtr = kCB64;
        while (*basePtr){
            if (ch == *basePtr) return (basePtr-kCB64);
            basePtr++;
        }

        return (0);

    }

    // ==============
    // PUBLIC METHODS
    // ==============

    //
    // Set STMP server URL
    // 

    void CSMTP::setServer(const std::string& serverURL) {

        m_serverURL = serverURL;

    }

    //
    // Get STMP server URL
    // 

    std::string CSMTP::getServer(void) const {

        return (m_serverURL);

    }


    //
    // Set email account details
    //

    void CSMTP::setUserAndPassword(const std::string& userName,
            const std::string& userPassword) {

        m_userName = userName;
        m_userPassword = userPassword;

    }

    //
    // Get email account user
    //

    std::string CSMTP::getUser(void) const {

        return (m_userName);

    }

    //
    // Set From address
    //

    void CSMTP::setFromAddress(const std::string& addressFrom) {

        m_addressFrom = addressFrom;
    }

    //
    // Get From address
    //

    std::string CSMTP::getFromAddress(void) const {

        return (m_addressFrom);

    }

    //
    // Set To address
    //

    void CSMTP::setToAddress(const std::string& addressTo) {

        m_addressTo = addressTo;

    }

    //
    // Get To address
    //

    std::string CSMTP::getToAddress(void) const {

        return (m_addressTo);

    }

    //
    // Set CC recipient address
    //

    void CSMTP::setCCAddress(const std::string& addressCC) {

        m_addressCC = addressCC;
    }

    //
    // Get CC recipient address
    //

    std::string CSMTP::getCCAddress(void) const {

        return (m_addressCC);

    }

    //
    // Set email subject
    //

    void CSMTP::setMailSubject(const std::string& mailSubject) {

        m_mailSubject = mailSubject;

    }

    //
    // Get email subject
    //

    std::string CSMTP::getMailSubject(void) const {

        return (m_mailSubject);

    }

    //
    // Set body of email message
    //

    void CSMTP::setMailMessage(const std::vector<std::string>& mailMessage) {
        m_mailMessage = mailMessage;
    }

    //
    // Get body of email message
    //

    std::string CSMTP::getMailMessage(void) const {

        std::string mailMessage;

        for (auto &line : m_mailMessage) {
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

        m_attachedFiles.push_back({fileName, contentType, contentTransferEncoding});

    }

    //
    // Post email
    //

    void CSMTP::postMail(void) {

        m_curlHandle = curl_easy_init();

        if (m_curlHandle) {

            curl_easy_setopt(m_curlHandle, CURLOPT_PROTOCOLS, CURLPROTO_SMTP | CURLPROTO_SMTPS);

            curl_easy_setopt(m_curlHandle, CURLOPT_USERNAME, m_userName.c_str());
            curl_easy_setopt(m_curlHandle, CURLOPT_PASSWORD, m_userPassword.c_str());
            curl_easy_setopt(m_curlHandle, CURLOPT_URL, m_serverURL.c_str());

            curl_easy_setopt(m_curlHandle, CURLOPT_USE_SSL, (long) CURLUSESSL_ALL);

            curl_easy_setopt(m_curlHandle, CURLOPT_ERRORBUFFER, m_curlErrMsgBuffer);

            if (!m_mailCABundle.empty()) {
                curl_easy_setopt(m_curlHandle, CURLOPT_CAINFO, m_mailCABundle.c_str());
            }

            curl_easy_setopt(m_curlHandle, CURLOPT_MAIL_FROM, m_addressFrom.c_str());

            m_curlRecipients = curl_slist_append(m_curlRecipients, m_addressTo.c_str());

            if (!m_addressCC.empty()) {
                m_curlRecipients = curl_slist_append(m_curlRecipients, m_addressCC.c_str());
            }

            curl_easy_setopt(m_curlHandle, CURLOPT_MAIL_RCPT, m_curlRecipients);

            buildMailPayload();

            curl_easy_setopt(m_curlHandle, CURLOPT_READFUNCTION, payloadSource);
            curl_easy_setopt(m_curlHandle, CURLOPT_READDATA, &m_mailPayload);
            curl_easy_setopt(m_curlHandle, CURLOPT_UPLOAD, 1L);

            curl_easy_setopt(m_curlHandle, CURLOPT_VERBOSE, m_curlVerbosity);

            m_curlErrMsgBuffer[0] = 0;
            m_curlResult = curl_easy_perform(m_curlHandle);

            // Check for errors

            if (m_curlResult != CURLE_OK) {
                std::string errMsg;
                if (std::strlen(m_curlErrMsgBuffer) != 0) {
                    errMsg = m_curlErrMsgBuffer;
                } else {
                    errMsg = curl_easy_strerror(m_curlResult);
                }
                throw Exception("curl_easy_perform() failed: " + errMsg);
            }

            // Clear sent email

            m_mailPayload.clear();

            // Free the list of recipients

            curl_slist_free_all(m_curlRecipients);

            // Always cleanup

            curl_easy_cleanup(m_curlHandle);

        }

    }

    //
    // Encode string to base64 string.
    //

    void CSMTP::encodeToBase64(const std::string& decoding,
            std::string& encoding, std::uint32_t numberOfBytes) {

        int trailing, byteIndex = 0;
        std::uint8_t byte1, byte2, byte3;

        if (numberOfBytes == 0) {
            return;
        }

        encoding.clear();

        trailing = (numberOfBytes % 3); // Trailing bytes
        numberOfBytes /= 3; // No of 3 byte values to encode

        while (numberOfBytes--) {

            byte1 = decoding[byteIndex++];
            byte2 = decoding[byteIndex++];
            byte3 = decoding[byteIndex++];

            encoding.append(1, kCB64[(byte1 & 0xfc) >> 2]);
            encoding.append(1, kCB64[((byte1 & 0x03) << 4) + ((byte2 & 0xf0) >> 4)]);
            encoding.append(1, kCB64[((byte2 & 0x0f) << 2) + ((byte3 & 0xc0) >> 6)]);
            encoding.append(1, kCB64[byte3 & 0x3f]);

        }

        // One trailing byte

        if (trailing == 1) {
            byte1 = decoding[byteIndex++];
            encoding.append(1, kCB64[(byte1 & 0xfc) >> 2]);
            encoding.append(1, kCB64[((byte1 & 0x03) << 4)]);
            encoding.append(1,'=');
            encoding.append(1, '=');

            // Two trailing bytes

        } else if (trailing == 2) {
            byte1 = decoding[byteIndex++];
            byte2 = decoding[byteIndex++];
            encoding.append(1, kCB64[(byte1 & 0xfc) >> 2]);
            encoding.append(1, kCB64[((byte1 & 0x03) << 4) + ((byte2 & 0xf0) >> 4)]);
            encoding.append(1, kCB64[((byte2 & 0x0f) << 2)]);
            encoding.append(1, '=');
        }

    }

    //
    // Decode string from base64 encoded string.
    //

    void CSMTP::decodeFromBase64(const std::string& encoding,
            std::string& decoding, std::uint32_t numberOfBytes) {

        int byteIndex { 0 };
        std::uint8_t byte1, byte2, byte3, byte4;

        if ((numberOfBytes == 0) || (numberOfBytes % 4)) {
            return;
        }

        decoding.clear();

        numberOfBytes = (numberOfBytes / 4);
        while (numberOfBytes--) {

            byte1 = encoding[byteIndex++];
            byte2 = encoding[byteIndex++];
            byte3 = encoding[byteIndex++];
            byte4 = encoding[byteIndex++];

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

            decoding.append(1, ((byte1 << 2) + ((byte2 & 0x30) >> 4)));
            decoding.append(1, (((byte2 & 0xf) << 4) + ((byte3 & 0x3c) >> 2)));
            decoding.append(1, (((byte3 & 0x3) << 6) + byte4));

        }

    }

    //
    // Get whole of email message (including headers and encoded attachments).
    //

    std::string CSMTP::getMailFull(void) {

        std::string mailMessage;

        buildMailPayload();

        for (auto &line : m_mailPayload) {
            mailMessage.append(line);
        }

        m_mailPayload.clear();

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

   } // namespace SMTP
} // namespace Antik
