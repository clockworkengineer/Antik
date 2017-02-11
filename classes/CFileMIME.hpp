/*
 * File:   CFileMIME.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2016, 2:33 PM
 *
 * Copyright 2016.
 
 */

#ifndef CFILEMIME_HPP
#define CFILEMIME_HPP

//
// C++ STL definitions
//

#include <string>
#include <stdexcept>
#include <unordered_map>

//
// libcurl definitions
//

#include <curl/curl.h>

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
        : std::runtime_error("CFileMIME Failure: "+ message) { }
        
    };
  
    // ============
    // CONSTRUCTORS
    // ============
    
    //
    // Main constructor
    //
    
    CFileMIME();
    
    // ==========
    // DESTRUCTOR
    // ==========
    
    virtual ~CFileMIME();

    // ==============
    // PUBLIC METHODS
    // ==============
    
    static std::string getFileMIMEType(const std::string& fileName);
    
    // ================
    // PUBLIC VARIABLES
    // ================
    
private:

    // ===========================
    // PRIVATE TYPES AND CONSTANTS
    // ===========================
          
    // =====================
    // DISABLED CONSTRUCTORS
    // =====================
 
    // ===============
    // PRIVATE METHODS
    // ===============
        
    // Load file extension to MIME type mapping table
    
    static void loadMIMETypes (void);

    // =================
    // PRIVATE VARIABLES
    // =================
        
    static std::unordered_map<std::string, std::string> extToMimeType;    // File extension to MIME type
    
};

#endif /* CMAILSMTP_HPP */

