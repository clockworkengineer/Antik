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
//

// =============
// INCLUDE FILES
// =============

//
// C++ STL
//

#include <iostream>

//
// FTP utility definitions
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

        int sshUserAuthorize(CSSHSession &session) {

            int authorizationMethod;
            int returnCode;

            returnCode = session.userAuthorizationNone();
            if (returnCode == SSH_AUTH_SUCCESS || returnCode == SSH_AUTH_ERROR) {
                return returnCode;
            }

            authorizationMethod = session.userAuthorizationList();

            if (authorizationMethod & SSH_AUTH_METHOD_NONE) {
                returnCode = session.userAuthorizationNone();
                if (returnCode == SSH_AUTH_SUCCESS) {
                    return returnCode;
                }
            }

            if (authorizationMethod & SSH_AUTH_METHOD_PUBLICKEY) {
                returnCode = session.userAuthorizationWithPublicKeyAuto();
                if (returnCode == SSH_AUTH_SUCCESS) {
                    return returnCode;
                }
            }

            if (authorizationMethod & SSH_AUTH_METHOD_INTERACTIVE) {
                returnCode = session.userAuthorizationWithKeyboardInteractive();
                if (returnCode == SSH_AUTH_SUCCESS) {
                    return returnCode;
                }
            }

            if (authorizationMethod & SSH_AUTH_METHOD_PASSWORD) {
                returnCode = session.userAuthorizationWithPassword();
                if (returnCode == SSH_AUTH_SUCCESS) {
                    return returnCode;
                }
            }

            return SSH_AUTH_ERROR;

        }

        bool sshVerifyKnownServer(CSSHSession &sshSession) {

            std::vector<unsigned char> keyHash;
            CSSHSession::Key serverPublicKey;
            std::string reply;
            int returnCode;

            returnCode = sshSession.isServerKnown();
            
            serverPublicKey = sshSession.getPublicKey();

            sshSession.getPublicKeyHash(serverPublicKey, keyHash);
            
            sshSession.freeKey(serverPublicKey);

            switch (returnCode) {

                case SSH_SERVER_KNOWN_OK:
                    break; /* ok */

                case SSH_SERVER_KNOWN_CHANGED:
                    std::cerr << "Host key for server changed: it is now:\n" << sshSession.convertKeyHashToHex(keyHash) << std::endl;
                    std::cerr << "For security reasons, connection will be stopped" << std::endl;
                    return (false);

                case SSH_SERVER_FOUND_OTHER:
                    std::cerr << "The host key for this server was not found but an other type of key exists.\n";
                    std::cerr << "An attacker might change the default server key to confuse your client into ";
                    std::cerr << "thinking the key does not exist" << std::endl;
                    return (false);

                case SSH_SERVER_FILE_NOT_FOUND:
                    std::cerr << "Could not find known host file.\n";
                    std::cerr << "If you accept the host key here, the file will be automatically created." << std::endl;
                    /* fallback to SSH_SERVER_NOT_KNOWN behavior */

                case SSH_SERVER_NOT_KNOWN:
                    std::cerr << "The server is unknown. Do you trust the host key?\n";
                    std::cerr << "Public key hash: " << sshSession.convertKeyHashToHex(keyHash) << std::endl;
                    std::cin >> reply;
                    if (reply != "yes") {
                        return (false);
                    }
                    sshSession.writeKnownHost();
                    break;

                case SSH_SERVER_ERROR:
                    std::cerr << "Error: " << sshSession.getError() << std::endl;
                    return (false);

            }

            return (true);

        }

    } // namespace SSH

} // namespace Antik

