/*
 * File:   CMailIMAP.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on Januray 24, 2017, 2:33 PM
 *
 * Copyright 2016.
 
 */

#ifndef CMAILIMAP_HPP
#define CMAILIMAP_HPP

//
// C++ STL definitions
//

#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <stdexcept>

//
// libcurl definitions
//

#include <curl/curl.h>

// ================
// CLASS DEFINITION
// ================

class CMailIMAP {
    
public:
    
    // ==========================
    // PUBLIC TYPES AND CONSTANTS
    // ==========================
    
    // ============
    // CONSTRUCTORS
    // ============
    
    //
    // Main constructor
    //
    
    CMailIMAP();
    
    // ==========
    // DESTRUCTOR
    // ==========
    
    virtual ~CMailIMAP();

    // ==============
    // PUBLIC METHODS
    // ==============
    
    //
    // Set email server account details
    //
    
    void setServer(const std::string& serverURL);
    void setUserAndPassword(const std::string& userName, const std::string& userPassword);
    
    //
    // Wait on IDLE for mailbox
    //
    
    void waitOnIdle(const std::string& imapMailBox);
    
    //
    // IMAP connect, send command and disconnect
    //
    
    void connect(void);
    std::string  sendIMAPCommand(const std::string& command);
    void disconnect(void);
       
    // IMAP initialization and closedown processing
    
    static void init();
    static void closedown();
    
    // ================
    // PUBLIC VARIABLES
    // ================
    
private:

    // ===========================
    // PRIVATE TYPES AND CONSTANTS
    // ===========================
  
    static const std::string kEOL;                  // End of line
    
    // =====================
    // DISABLED CONSTRUCTORS
    // =====================
 
    // ===============
    // PRIVATE METHODS
    // ===============
    
    //
    // IDLE command functions. Talks to server using curl_easy_send/recv() as this is the only
    // way to get IDLE to work.
    //
    
    void sendCommandDirect(const std::string& command);
    void waitForCommandResponse(const std::string& commandTag, std::string& commandResponse);
    
    //
    // Add libcurl receive data to reply string.
    //
    
    static size_t writeFunction(void *ptr, size_t size, size_t nmemb, std::string* data);
    
    // =================
    // PRIVATE VARIABLES
    // =================
    
    std::string userName="";                  // Email account user name
    std::string userPassword="";              // Email account user name password
    std::string serverURL="";                 // IMAP server URL
    
    CURL     *curl=nullptr;                   // curl handle
    CURLcode res = CURLE_OK;                  // curl status
  
    std::string commandResponse;              // IMAP command response
    std::string commandHeader;                // IMAP reply header
 
    char rxBuffer[CURL_MAX_WRITE_SIZE];       // IMAP rx buffer
    char errMsgBuffer[CURL_ERROR_SIZE];       // IMAP error string buffer
    
};

#endif /* CMAILIMAP_HPP */

