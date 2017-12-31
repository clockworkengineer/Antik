#include "HOST.hpp"
/*
 * File:   CSSHSession.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on September 23, 2017, 2:33 PM
 *
 * Copyright 2017.
 *
 */

//
// Class: CSSHSession
// 
// Description:
//
// Dependencies:   C11++        - Language standard features used.
//                 libssh       - Used to talk to SSH server.
//

// =================
// CLASS DEFINITIONS
// =================

#include "CSSHSession.hpp"

// ====================
// CLASS IMPLEMENTATION
// ====================

//
// C++ STL
//

#include <cstring>

// =======
// IMPORTS
// =======

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace SSH {

        // ===========================
        // PRIVATE TYPES AND CONSTANTS
        // ===========================

        // ==========================
        // PUBLIC TYPES AND CONSTANTS
        // ==========================

        // ========================
        // PRIVATE STATIC VARIABLES
        // ========================

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
        // Main CSSHSession object constructor. 
        //

        CSSHSession::CSSHSession() {
            m_session = ssh_new();
        }

        //
        // CSSHSession Destructor
        //

        CSSHSession::~CSSHSession() {
            if (m_session) { 
                ssh_free(m_session);
                m_session=nullptr;
            }
        }

        int CSSHSession::setServer(const std::string &server) {
            
            m_server = server;
            m_lastReturnedCode = ssh_options_set(m_session, SSH_OPTIONS_HOST, &m_server[0]);
            return(m_lastReturnedCode);
            
        }

        int CSSHSession::setPort(unsigned int port) {
            
            m_port = port;
            m_lastReturnedCode = ssh_options_set(m_session, SSH_OPTIONS_PORT, &m_port);
            return(m_lastReturnedCode);
            
        }

        int CSSHSession::setUser(const std::string &user) {
            
            m_user = user;
            m_lastReturnedCode = ssh_options_set(m_session, SSH_OPTIONS_USER, &m_user[0]);
            return(m_lastReturnedCode);
            
        }

        int CSSHSession::setUserPassword(const std::string &password) {
            
            m_password = password;        
            return(m_lastReturnedCode=SSH_OK);

        }

        int CSSHSession::connect() {
            
            m_lastReturnedCode = ssh_connect(m_session);
            return(m_lastReturnedCode);
            
        }

        int CSSHSession::disconnect() {
            
            m_lastReturnedCode = ssh_connect(m_session);
            return(m_lastReturnedCode);
            
        }

        int CSSHSession::userAuthorizationList() {
            
            return(ssh_userauth_list(m_session, NULL));
            
        }
        
        int CSSHSession::userAuthorizationNone() {
            
            m_lastReturnedCode = ssh_userauth_none(m_session, NULL);
            return(m_lastReturnedCode);
                        
        }

        int CSSHSession::userAuthorizationWithPassword() {
            
            m_lastReturnedCode = ssh_userauth_password(m_session, NULL, &m_password[0]);
            return(m_lastReturnedCode);
            
        }

        int CSSHSession::userAuthorizationWithPublicKeyAuto() {
            
            m_lastReturnedCode = ssh_userauth_publickey_auto(m_session, NULL, NULL);
            return(m_lastReturnedCode);
            
        }
        
        int CSSHSession::userAuthorizationWithPublicKey() {
            m_lastReturnedCode = SSH_AUTH_DENIED;
            return(m_lastReturnedCode);
        }

        int CSSHSession::userAuthorizationWithKeyboardInteractive() {
            m_lastReturnedCode = SSH_AUTH_DENIED;
            return (m_lastReturnedCode);
        }
        
        int CSSHSession::isServerKnown() {
            
            m_lastReturnedCode = ssh_is_server_known(m_session);
            return(m_lastReturnedCode);
            
        }
        
         int CSSHSession::getPublicKeyHash(std::vector<unsigned char> &keyHash) {
            unsigned char *hash = NULL;
            int hlen;
            hlen = ssh_get_pubkey_hash(m_session, &hash);
            if (hlen >= 0) {
                keyHash.assign(&hash[0], &hash[hlen]);
                free(hash);
            }
            return (hlen);
        }
         
        std::string CSSHSession::convertKeyHashToHex(std::vector<unsigned char> &keyHash) {
            char *hexa = NULL;
            std::string convertedHash;
            hexa = ssh_get_hexa(&keyHash[0], keyHash.size());
            if (hexa >= 0) {
                convertedHash.assign(&hexa[0], &hexa[std::strlen(hexa)]);
                free(hexa);
            }
            return (convertedHash);
        }
         
         int CSSHSession::writeKnownHost() {
             
            m_lastReturnedCode = ssh_write_knownhost(m_session);
            return(m_lastReturnedCode);
            
         }
         
         std::string CSSHSession::getBanner() {

            std::string sessionBanner;
            
            char *banner = ssh_get_issue_banner(m_session);
            if (banner) {
                sessionBanner.assign(&banner[0], &banner[std::strlen(banner)]);
                free(banner);
            }
            
            return(sessionBanner);
     
         }
         
        int CSSHSession::getSSHVersion() {

            return(ssh_get_version(m_session));
            
        }
         
         std::string CSSHSession::getError() {
             
             return(ssh_get_error(m_session));
             
         }

        ssh_session CSSHSession::getSession() const {
            
            return m_session;
            
        }

        int CSSHSession::getLastReturnCode() const {
            
            return m_lastReturnedCode;
            
        }
              
    } // namespace CSSH
} // namespace Antik
