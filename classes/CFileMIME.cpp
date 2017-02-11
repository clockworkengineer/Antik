/*
 * File:   CFileMIME.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2016, 2:33 PM
 *
 * Copyright 2016.
 *
 */

//
// Class: CFileMIME
// 
// Description: Class to provide file extension to MIME type mapping.
//
// Dependencies:   C11++     - Language standard features used.
//                 Linux     - /etc/mime.types used to create MIME file.
//

// =================
// CLASS DEFINITIONS
// =================

#include "CFileMIME.hpp"

// ====================
// CLASS IMPLEMENTATION
// ====================

//
// C++ STL definitions
//

#include <fstream>
#include <sstream>

// ===========================
// PRIVATE TYPES AND CONSTANTS
// ===========================

// ==========================
// PUBLIC TYPES AND CONSTANTS
// ==========================

// ========================
// PRIVATE STATIC VARIABLES
// ========================

// File extension to MIME type mapping table

std::unordered_map<std::string, std::string> CFileMIME::extToMimeType;

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

void CFileMIME::loadMIMETypes (void) {
    
    std::ifstream mimeFile("/etc/mime.types");
    std::string   extension, mimeType, line;

    while (std::getline(mimeFile, line)) {
        if (line[0] != '#') {
            std::istringstream iss(line);
            iss >> mimeType;
            while (iss.good()) {
                iss >> extension;
                if (!extension.empty()) {
                    CFileMIME::extToMimeType.insert({extension, mimeType});
                }
            }
        }
    }
      
}

// ==============
// PUBLIC METHODS
// ==============


std::string CFileMIME::getFileMIMEType(const std::string& fileName) {

    std::string mimeMapping;
    std::string baseFileName = fileName.substr(fileName.find_last_of("/\\") + 1);
    std::size_t fullStop = baseFileName.find_last_of('.');

    if (CFileMIME::extToMimeType.empty()) {
       CFileMIME::loadMIMETypes(); 
    }
    
    if (fullStop != std::string::npos) {
        baseFileName = baseFileName.substr(fullStop+1);
        auto foundMapping= CFileMIME::extToMimeType.find(baseFileName);
        if (foundMapping != CFileMIME::extToMimeType.end()) {
            mimeMapping = foundMapping->second;
        }
    }

    return(mimeMapping);

}

//
// Main CFileMIME object constructor. 
//

CFileMIME::CFileMIME() {

}

//
// CFileMIME Destructor
//

CFileMIME::~CFileMIME() {

}


