/*
 * File:   CSSHSession.hpp(Work In Progress)
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

            struct Exception {

                Exception(CSSHSession &session, const std::string functionName) : m_errorCode{session.getErrorCode()}, 
                m_errorMessage{ session.getError()}, m_functionName{functionName}
                {
                }

                int getCode() {
                    return m_errorCode;
                }

                std::string getMessage() {
                    return static_cast<std::string> ("CSSHSession Failure: (") +m_functionName + ") [" + m_errorMessage + "]";
                }

            private:
                std::string m_functionName;
                int m_errorCode;
                std::string m_errorMessage;

            };

            typedef ssh_key Key;
            
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
        
            void setServer(const std::string &server);
            void setPort(unsigned int port);
            void setUser(const std::string &user);
            void setUserPassword(const std::string &password);
            
            void connect();
            void disconnect(bool silent=false);
            
            int userAuthorizationList();
            int userAuthorizationNone();
            int userAuthorizationWithPublicKeyAuto();
            
            virtual int userAuthorizationWithPassword();
            virtual int userAuthorizationWithPublicKey();
            virtual int userAuthorizationWithKeyboardInteractive();
            
            int isServerKnown();
            void writeKnownHost();
            std::string dumpKnownHost();
            
            std::string getCipherIn();
            std::string getCipherOut();
            std::string getHMACIn();
            std::string getHMACOut();
    
             
            Key getPublicKey();
            void getPublicKeyHash(Key serverPublicKey, std::vector<unsigned char> &keyHash);
            std::string convertKeyHashToHex(std::vector<unsigned char> &keyHash);
            void freeKey(Key keyToFree);

            std::string getBanner() const;
            std::string getClientBanner() const;
            std::string getServerBanner() const;
            std::string getDisconnectMessage() const;
            
            int getSSHVersion() const;
            int getOpenSSHVersion() const;
            int getStatus() const;
            bool isConnected() const;
            
            std::string getError() const;
            int getErrorCode() const;
            
            ssh_session getSession() const;
            
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
            
            std::string m_server;
            unsigned int  m_port { 22 };
            std::string m_user;
            std::string m_password;

        };

    } // namespace SSH
} // namespace Antik

#endif /* CSSHSESSION_HPP */

