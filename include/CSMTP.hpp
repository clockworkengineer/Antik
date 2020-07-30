#ifndef CSMTP_HPP
#define CSMTP_HPP

//
// C++ STL
//

#include <string>
#include <vector>
#include <stdexcept>
#include <deque>

//
// Antik classes
//

#include "CommonAntik.hpp"
#include "CCurl.hpp"

// =========
// NAMESPACE
// =========

namespace Antik::SMTP
{

// ================
// CLASS DEFINITION
// ================

class CSMTP
{
public:
    // ==========================
    // PUBLIC TYPES AND CONSTANTS
    // ==========================

    //
    // Class exception
    //

    struct Exception : public std::runtime_error
    {

        explicit Exception(std::string const &message)
            : std::runtime_error("CSMTP Failure: " + message)
        {
        }
    };

    // Supported contents encodings

    static const char *kEncoding7Bit;
    static const char *kEncodingBase64;

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

    void setServer(const std::string &serverURL);
    void setUserAndPassword(const std::string &userName, const std::string &userPassword);

    std::string getServer(void) const;
    std::string getUser(void) const;

    // Set/Get email message header details

    void setFromAddress(const std::string &addressFrom);
    void setToAddress(const std::string &addressTo);
    void setCCAddress(const std::string &addressCC);

    std::string getFromAddress(void) const;
    std::string getToAddress(void) const;
    std::string getCCAddress(void) const;

    // Set email content details

    void setMailSubject(const std::string &mailSubject);
    void setMailMessage(const std::vector<std::string> &mailMessage);
    void addFileAttachment(const std::string &fileName, const std::string &contentType, const std::string &contentTransferEncoding);

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

    static void encodeToBase64(const std::string &decoding, std::string &encoding, std::uint32_t numberOfBytes);
    static void decodeFromBase64(const std::string &encoding, std::string &decoding, std::uint32_t numberOfBytes);

    // ================
    // PUBLIC VARIABLES
    // ================

private:
    // ===========================
    // PRIVATE TYPES AND CONSTANTS
    // ===========================

    // Attachments

    struct EmailAttachment
    {
        std::string fileName;                        // Attached file name
        std::string contentTypes;                    // Attached file MIME content type
        std::string contentTransferEncoding;         // Attached file content encoding
        std::vector<std::string> encodedContents {}; // Attached file encoded contents
    };

    static const char *kMimeBoundary; // Text string used for MIME boundary

    static const int kBase64EncodeBufferSize{54}; // Optimum encode buffer size (since encoded max 76 bytes)

    static const char *kEOL; // End of line

    static const char kCB64[]; // Valid characters for base64 encode/decode.

    // ===========================================
    // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
    // ===========================================

    CSMTP(const CSMTP &orig) = delete;
    CSMTP(const CSMTP &&orig) = delete;
    CSMTP &operator=(CSMTP other) = delete;

    // ===============
    // PRIVATE METHODS
    // ===============

    // Encode email attachment

    void encodeAttachment(CSMTP::EmailAttachment &attachment);

    // Add attachments to payload

    void buildAttachments(void);

    // Construct email payload

    void buildMailPayload(void);

    // libcurl read callback for payload

    static size_t payloadSource(char *ptr, size_t size, size_t nmemb, void *userData);

    // Date and time for email

    static const std::string currentDateAndTime(void);

    // Load file extension to MIME type mapping table

    static void loadMIMETypes(void);

    // Decode character to base64 index.

    static int decodeChar(char ch);

    // =================
    // PRIVATE VARIABLES
    // =================

    std::string m_userName;     // Email account user name
    std::string m_userPassword; // Email account user name password
    std::string m_serverURL;    // SMTP server URL

    std::string m_addressFrom; // Email Sender
    std::string m_addressTo;   // Main recipients addresses
    std::string m_addressCC;   // CC recipients addresses

    std::string m_mailSubject;              // Email subject
    std::vector<std::string> m_mailMessage; // Email body

    std::string m_mailCABundle; // Path to CA bundle (Untested at present)

    Antik::Network::CCurl m_connection;                       // Connection handle
    Antik::Network::CCurl::StringList m_recipientsList{NULL}; // Email recipients list
    static bool m_curlVerbosity;                              // curl verbosity setting        // Curl verbosity flag.

    std::deque<std::string> m_mailPayload; // Email payload

    std::vector<CSMTP::EmailAttachment> m_attachedFiles; // Attached files
};

} // namespace Antik::SMTP

#endif /* CSMTP_HPP */
