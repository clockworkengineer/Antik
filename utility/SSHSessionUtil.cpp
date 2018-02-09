#include "HOST.hpp"
/*
 * File:   SSHSessionUtil.cpp(Work In Progress)
 * 
 * Author: Robert Tizzard
 *
 * Created on October 10, 2017, 2:34 PM
 * 
 * Copyright 2017.
 * 
 */

//
// Module: SSHSessionUtil
//
// Description: SSH Session utility functions for the Antik class CSSHSession.
// 
// Dependencies: 
// 
// C11++              : Use of C11++ features.
// Antik classes      : CSSHSession
//

// =============
// INCLUDE FILES
// =============

//
// C++ STL
//

#include <iostream>

//
// SSH Session utility definitions
//

#include "SSHSessionUtil.hpp"

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace SSH {

        // =======
        // IMPORTS
        // =======

        using namespace std;

        // ===============
        // LOCAL FUNCTIONS
        // ===============

        // ================
        // PUBLIC FUNCTIONS
        // ================

        //
        // Default implementation of ServerVerificationContext virtual methods.
        //
        
        void ServerVerificationContext::serverKnown() {
        }

        bool ServerVerificationContext::serverKnownChanged(std::vector<unsigned char> &keyHash) {
            if (m_contextData) {
                CSSHSession *sshSession = static_cast<CSSHSession *> (m_contextData);
                std::cerr << "Host key for server changed: it is now:\n" << sshSession->convertKeyHashToHex(keyHash) << std::endl;
                std::cerr << "For security reasons, connection will be stopped" << std::endl;
            }
            return (false);
        }

        bool ServerVerificationContext::serverFoundOther() {
            if (m_contextData) {
                CSSHSession * sshSession{ static_cast<CSSHSession *> (m_contextData)};
                std::cerr << "The host key for this server was not found but an other type of key exists.\n";
                std::cerr << "An attacker might change the default server key to confuse your client into ";
                std::cerr << "thinking the key does not exist" << std::endl;
            }
            return (false);
        }

        bool ServerVerificationContext::serverFileNotFound(std::vector<unsigned char> &keyHash) {
            if (m_contextData) {
                CSSHSession * sshSession{ static_cast<CSSHSession *> (m_contextData)};
                std::cerr << "Could not find known host file.\n";
                std::cerr << "If you accept the host key here, the file will be automatically created." << std::endl;
            }
            return (serverNotKnown(keyHash));
        }

        bool ServerVerificationContext::serverNotKnown(std::vector<unsigned char> &keyHash) {
            if (m_contextData) {
                CSSHSession * sshSession{ static_cast<CSSHSession *> (m_contextData)};
                std::string reply;
                std::cerr << "The server is unknown. Do you trust the host key?\n";
                std::cerr << "Public key hash: " << sshSession->convertKeyHashToHex(keyHash) << std::endl;
                std::cin >> reply;
                if (reply != "yes") {
                    return (false);
                }
                sshSession->writeKnownHost();
            }
            return (true);
        }

        bool ServerVerificationContext::serverError() {
            return (false);
        }

        //
        // Authorize a user(client) with SSH server. It gets a list of methods supported
        // by the server and tries each one until it passes (returning true).If none are
        // successful then return false. As a fall back alway try password last.
        //

        bool userAuthorize(CSSHSession &session) {

            int authorizationMethod;

            if (session.userAuthorizationNone() == SSH_AUTH_SUCCESS) {
                return (true);
            }

            authorizationMethod = session.userAuthorizationList();

            if (authorizationMethod & SSH_AUTH_METHOD_NONE) {
                if (session.userAuthorizationNone() == SSH_AUTH_SUCCESS) {
                    return (true);
                }
            }

            if (authorizationMethod & SSH_AUTH_METHOD_PUBLICKEY) {
                if (session.userAuthorizationWithPublicKeyAuto() == SSH_AUTH_SUCCESS) {
                    return (true);
                }
            }

            if (authorizationMethod & SSH_AUTH_METHOD_INTERACTIVE) {
                if (session.userAuthorizationWithKeyboardInteractive() == SSH_AUTH_SUCCESS) {
                    return (true);
                }
            }

            if (authorizationMethod & SSH_AUTH_METHOD_PASSWORD) {
                if (session.userAuthorizationWithPassword() == SSH_AUTH_SUCCESS) {
                    return (true);
                }
            }

            return (false);

        }

        //
        // Verify whether a server is known to client. If its public key is already known then return
        // true. If host entry not found or server not known ask user if they accept is public key hash
        // and write away if they do.
        //

        bool verifyKnownServer(CSSHSession &sshSession, ServerVerificationContext &verificationContext) {

            vector<unsigned char> keyHash;
            CSSHSession::Key serverPublicKey;
            int returnCode;

            returnCode = sshSession.isServerKnown();

            serverPublicKey = sshSession.getPublicKey();

            sshSession.getPublicKeyHash(serverPublicKey, keyHash);

            switch (returnCode) {

                case SSH_SERVER_KNOWN_OK:
                    verificationContext.serverKnown();
                    break;

                case SSH_SERVER_KNOWN_CHANGED:
                    return (verificationContext.serverKnownChanged(keyHash));

                case SSH_SERVER_FOUND_OTHER:
                    return (verificationContext.serverFoundOther());

                case SSH_SERVER_FILE_NOT_FOUND:
                    return (verificationContext.serverFileNotFound(keyHash));

                case SSH_SERVER_NOT_KNOWN:
                    return (verificationContext.serverNotKnown(keyHash));

                case SSH_SERVER_ERROR:
                    return (verificationContext.serverError());

            }

            return (true);

        }

    } // namespace SSH

} // namespace Antik

