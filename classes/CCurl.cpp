/*
 * File:   CCurl.cpp
 *  
 * Author: Robert Tizzard
 * 
 * Created on October 04, 2018, 2:33 PM
 *
 * Copyright 2016.
 *
 */

//
// Class: CCurl
// 
// Description:    Simple C++ wrapper class for the libcurl easy interface.
//
// Dependencies:   C11++     - Language standard features used.
//                 libcurl   - Used to talk to servers.
//

// =================
// CLASS DEFINITIONS
// =================

#include "CCurl.hpp"
#include <iostream>


// ====================
// CLASS IMPLEMENTATION
// ====================

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace Network {

        // ===========================
        // PRIVATE TYPES AND CONSTANTS
        // ===========================

        // ==========================
        // PUBLIC TYPES AND CONSTANTS
        // ==========================

        // ========================
        // PRIVATE STATIC VARIABLES
        // ========================

        bool CCurl::m_globalInitialised{false};

        // =======================
        // PUBLIC STATIC VARIABLES
        // =======================

        // ===============
        // PRIVATE METHODS
        // ===============

        // ==============
        // PUBLIC METHODS
        // ==============

        //
        // Main Curl object constructor. 
        //

        CCurl::CCurl() {

            if (!m_globalInitialised) {
                curl_global_init(CURL_GLOBAL_DEFAULT);
                m_globalInitialised = true;
            }

            m_curlConnection = curl_easy_init();

            if (m_curlConnection == NULL) {
                throw Exception("Failed to create CURL connection.");
            }

            m_errorBuffer.resize(CURL_ERROR_SIZE);
            m_errorBuffer[0] = '\0';

        }

        //
        // Curl Destructor.
        //

        CCurl::~CCurl() {

            if (m_curlConnection) {
                curl_easy_cleanup(m_curlConnection);
            }

        }

        //
        // Set extended error message buffer.
        //
        
        void CCurl::setErrorBuffer(size_t errorBufferSize) {
            
            this->m_errorBuffer.resize(errorBufferSize);
            auto code = curl_easy_setopt(m_curlConnection, CURLOPT_ERRORBUFFER, m_errorBuffer.c_str());
            if (code != CURLE_OK) {
                throw Exception("Failed to set error buffer [" + std::to_string(code) + "]");
            }

        }

        //
        // Perform setup connection transfer.
        //
        
        void CCurl::transfer() {

            auto code = curl_easy_perform(m_curlConnection);

            if (code != CURLE_OK) {
                if (m_errorBuffer[0]) {
                    throw Exception("Connection transfer failed." + m_errorBuffer);
                } else {
                    throw Exception(std::string("Connection transfer failed.") + curl_easy_strerror(code) + ".");
                }
            }

        }
        
        //
        // Perform connection reset.
        //
        
        void CCurl::reset() {

            curl_easy_reset(m_curlConnection);

        }

    } //namespace Network
} // namespace Antik

