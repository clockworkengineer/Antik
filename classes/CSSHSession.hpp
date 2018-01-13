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
#include <memory>

//
// Libssh
//

#include <libssh/libssh.h>
#include <libssh/callbacks.h> // Threading intialisation

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

                int getCode() const {
                    return m_errorCode;
                }

                std::string getMessage() const {
                    return static_cast<std::string> ("CSSHSession Failure: (") + m_functionName + ") [" + m_errorMessage + "]";
                }

            private:
                std::string m_functionName; // Current function name
                int m_errorCode;            // SSH error code
                std::string m_errorMessage; // SSH error message

            };

            //
            // Custom deleter for re-mapped libssh session data structures.
            //

            struct KeyDeleter {

                void operator()(ssh_key key) const {
                    ssh_key_free(key);
                }

            };

            //
            // Encapsulate libssh session data in unique pointers.
            //

            typedef std::unique_ptr<ssh_key_struct, KeyDeleter> Key;

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

            //
            // Initialisation
            //
            
            static void initialise();
            
            //
            // Set session details
            //

            void setServer(const std::string &server);
            void setPort(unsigned int port);
            void setUser(const std::string &user);
            void setUserPassword(const std::string &password);

            //
            // Connect/disconnect sessions
            //

            void connect();
            void disconnect(bool silent = false);

            //
            // User authorization functions
            //

            int userAuthorizationList();
            int userAuthorizationNone();
            int userAuthorizationWithPublicKeyAuto();

            //
            // Overridable user authorization functions
            //

            virtual int userAuthorizationWithPassword();
            virtual int userAuthorizationWithPublicKey();
            virtual int userAuthorizationWithKeyboardInteractive();

            //
            // Verify server
            //

            int isServerKnown();

            //
            // Write server details away to local config
            //

            void writeKnownHost();

            //
            // Get names of session cipher in/out methods
            //

            std::string getCipherIn();
            std::string getCipherOut();

            //
            // Public key methods
            //

            Key getPublicKey();
            void getPublicKeyHash(Key &serverPublicKey, std::vector<unsigned char> &keyHash);
            std::string convertKeyHashToHex(std::vector<unsigned char> &keyHash);
            void freeKey(Key &keyToFree);

            //
            // Get various banners and disconnect message
            //

            std::string getBanner() const;
            std::string getClientBanner() const;
            std::string getServerBanner() const;
            std::string getDisconnectMessage() const;

            //
            // Get SSH/OpenSSH versions
            //
            int getSSHVersion() const;
            int getOpenSSHVersion() const;

            //
            // Session status methods
            //

            int getStatus() const;
            bool isConnected() const;

            //
            // Get SSH error code and message
            //

            std::string getError() const;
            int getErrorCode() const;

            ssh_session getSession() const;
            
            //
            // Set logging verbosity
            //
            
            void setLogging(int logging);

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

            ssh_session m_session; // Session

            std::string m_server;             // SSH server name
            unsigned int m_port{ 22};         // SSH server port
            std::string m_user;               // SSH server login account name
            std::string m_password;           // SSH server login account password
            int m_logging {SSH_LOG_NOLOG };   // libssh logging

        };

    } // namespace SSH
} // namespace Antik

#endif /* CSSHSESSION_HPP */

