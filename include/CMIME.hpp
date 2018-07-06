/*
 * File:   CMIME.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2016, 2:33 PM
 *
 * Copyright 2016.
 *
 */

#ifndef CMIME_HPP
#define CMIME_HPP

//
// C++ STL
//

#include <string>
#include <stdexcept>
#include <unordered_map>
#include <sstream>
#include <vector>

//
// Antik classes
//

#include "CommonAntik.hpp"
#include <CSMTP.hpp>

//
// libcurl
//

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace File {

        // ================
        // CLASS DEFINITION
        // ================

        class CMIME {
        public:

            // ==========================
            // PUBLIC TYPES AND CONSTANTS
            // ==========================

            //
            // Class exception
            //

            struct Exception : public std::runtime_error {

                explicit Exception(std::string const& message)
                : std::runtime_error("CMIME Failure: " + message) {
                }

            };

            //
            // Parsed MIME string entry
            //

            struct ParsedMIMEString {
                unsigned char type  { ' ' }; // Type Q (Quoted Printable), B (base64), ' ' None.
                std::string encoding; // Encoding used
                std::string contents; // Contents
            };

            // ============
            // CONSTRUCTORS
            // ============

            // ==========
            // DESTRUCTOR
            // ==========

            // ==============
            // PUBLIC METHODS
            // ==============

            static std::string getFileMIMEType(const std::string& fileName);
            static std::string convertMIMEStringToASCII(const std::string& mime);

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

            static const char *kEncodedWordPrefix;
            static const char *kEncodedWordPostfix;
            static const char *kEncodedWordSeparator;
            static const char *kEncodedWordASCII;

            static const char kEncodedWordTypeBase64;
            static const char kEncodedWordTypeQuoted;
            static const char kEncodedWordTypeNone;
            static const char kQuotedPrintPrefix;


            // ===========================================
            // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
            // ===========================================

            CMIME() = delete;
            virtual ~CMIME() = delete;
            CMIME(const CMIME & orig) = delete;
            CMIME(const CMIME && orig) = delete;
            CMIME& operator=(CMIME other) = delete;
            
            // ===============
            // PRIVATE METHODS
            // ===============

            static std::vector<ParsedMIMEString> parseMIMEString(const std::string& mime);

            // =================
            // PRIVATE VARIABLES
            // =================

            static std::unordered_map<std::string, std::string> m_extToMimeType; // File extension to MIME type

        };

    } // namespace File
} // namespace Antik

#endif /* CMIME_HPP */

