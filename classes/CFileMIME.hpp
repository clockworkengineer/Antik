/*
 * File:   CFileMIME.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2016, 2:33 PM
 *
 * Copyright 2016.
 *
 */

#ifndef CFILEMIME_HPP
#define CFILEMIME_HPP

//
// C++ STL definitions
//

#include <string>
#include <stdexcept>
#include <unordered_map>
#include <sstream>
#include <vector>

//
// SMTP class
//

#include <CMailSMTP.hpp>

//
// libcurl definitions
//

// =========
// NAMESPACE
// =========

namespace Antik {

    // ================
    // CLASS DEFINITION
    // ================

    class CFileMIME {
    public:

        // ==========================
        // PUBLIC TYPES AND CONSTANTS
        // ==========================

        //
        // Class exception
        //

        struct Exception : public std::runtime_error {

            Exception(std::string const& message)
            : std::runtime_error("CFileMIME Failure: " + message) {
            }

        };

        //
        // Parsed MIME string entry
        //

        struct ParsedMIMEString {
            unsigned char type; // Type Q (Quoted Printable), B (base64), ' ' None.
            std::string encoding; // Encoding used
            std::string contents; // Contents
        };

        // ============
        // CONSTRUCTORS
        // ============

        CFileMIME();

        // ==========
        // DESTRUCTOR
        // ==========

        virtual ~CFileMIME();

        // ==============
        // PUBLIC METHODS
        // ==============

        static std::string getFileMIMEType(const std::string& fileName);
        static std::string convertMIMEStringToASCII(const std::string& mimeString);

        // ================
        // PUBLIC VARIABLES
        // ================

    private:

        // ===========================
        // PRIVATE TYPES AND CONSTANTS
        // ===========================

        //
        // MIME encoded word
        //

        static const char *kEncodedWordPrefixStr;
        static const char *kEncodedWordPostfixStr;
        static const char *kEncodedWordSeparatorStr;
        static const char *kEncodedWordASCIIStr;

        static const char kEncodedWordTypeBase64;
        static const char kEncodedWordTypeQuoted;
        static const char kEncodedWordTypeNone;
        static const char kQuotedPrintPrefix;


        // ===========================================
        // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
        // ===========================================

        CFileMIME(const CFileMIME & orig) = delete;
        CFileMIME(const CFileMIME && orig) = delete;
        CFileMIME& operator=(CFileMIME other) = delete;

        // ===============
        // PRIVATE METHODS
        // ===============

        static std::vector<ParsedMIMEString> parseMIMEString(const std::string& mimeStr);

        // =================
        // PRIVATE VARIABLES
        // =================

        static std::unordered_map<std::string, std::string> extToMimeType; // File extension to MIME type

    };

} // namespace Antik

#endif /* CMAILSMTP_HPP */

