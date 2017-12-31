/*
 * File:   CSSHSession.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on September 23, 2017, 2:33 PM
 *
 * Copyright 2017.
 *
 */

#ifndef CSSHSESSION_HPP
#define CSSHSESSION_HPP

//
// C++ STL
//

#include <stdexcept>
#include <vector>

//
// Libssh
//

#include <libssh/libssh.h>

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace SSH {

        // ==========================
        // PUBLIC TYPES AND CONSTANTS
        // ==========================
        
        // ================
        // CLASS DEFINITION
        // ================

        class CSSHSession {
        public:

            // ==========================
            // PUBLIC TYPES AND CONSTANTS
            // ==========================

            //
            // Class exception
            //

            struct Exception : public std::runtime_error {

                Exception(std::string const& message)
                : std::runtime_error("CSSHSession Failure: " + message) {
                }

            };

            // ============
            // CONSTRUCTORS
            // ============

            //
            // Main constructor
            //

            CSSHSession();

            // ==========
            // DESTRUCTOR
            // ==========

            virtual ~CSSHSession();

            // ==============
            // PUBLIC METHODS
            // ==============
        
            int setServer(const std::string &server);
            int setPort(unsigned int port);
            int setUser(const std::string &user);
            int setUserPassword(const std::string &password);
            
            int connect();
            int disconnect();
            
            int userAuthorizationList();
            int userAuthorizationNone();
            int userAuthorizationWithPublicKeyAuto();
            
            virtual int userAuthorizationWithPassword();
            virtual int userAuthorizationWithPublicKey();
            virtual int userAuthorizationWithKeyboardInteractive();
            
            int isServerKnown();
            int writeKnownHost();
             
            int getPublicKeyHash(std::vector<unsigned char> &keyHash);
            std::string convertKeyHashToHex(std::vector<unsigned char> &keyHash);

            std::string getBanner();
            
            int getSSHVersion();
            
            std::string getError();
            
            ssh_session getSession() const;
            int getLastReturnCode() const;
            
            // ================
            // PUBLIC VARIABLES
            // ================

        private:

            // ===========================
            // PRIVATE TYPES AND CONSTANTS
            // ===========================
          
                        
            // ===========================================
            // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
            // ===========================================

            CSSHSession(const CSSHSession & orig) = delete;
            CSSHSession(const CSSHSession && orig) = delete;
            CSSHSession& operator=(CSSHSession other) = delete;

            // ===============
            // PRIVATE METHODS
            // ===============
                   
           
            // =================
            // PRIVATE VARIABLES
            // =================

            ssh_session m_session;
            int m_lastReturnedCode;
            
            std::string m_server;
            unsigned int  m_port { 22 };
            std::string m_user;
            std::string m_password;

        };

    } // namespace SSH
} // namespace Antik

#endif /* CSSHSESSION_HPP */

