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
        
        bool verifyKnownServer(CSSHSession &sshSession,  ServerVerificationContext &verificationContext) {

            vector<unsigned char> keyHash;
            CSSHSession::Key serverPublicKey;
            int returnCode;

            returnCode = sshSession.isServerKnown();

            serverPublicKey = sshSession.getPublicKey();

            sshSession.getPublicKeyHash(serverPublicKey, keyHash);

            switch (returnCode) {

                case SSH_SERVER_KNOWN_OK:
                    verificationContext.serverKnown(sshSession);
                    break;

                case SSH_SERVER_KNOWN_CHANGED:
                    return (verificationContext.serverKnownChanged(sshSession, keyHash));

                case SSH_SERVER_FOUND_OTHER:
                    return (verificationContext.serverFoundOther(sshSession));

                case SSH_SERVER_FILE_NOT_FOUND:
                    return(verificationContext.serverFileNotFound(sshSession, keyHash));

                case SSH_SERVER_NOT_KNOWN:
                    return(verificationContext.serverNotKnown(sshSession, keyHash));
                    break;

                case SSH_SERVER_ERROR:
                    return(verificationContext.serverError(sshSession));

            }

            return (true);

        }

    } // namespace SSH

} // namespace Antik

