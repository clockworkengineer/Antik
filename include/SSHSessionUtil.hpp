/*
 * File:   SSHSessionUtil.hpp
 * 
 * Author: Robert Tizzard
 *
 * Created on April 10, 2017, 2:34 PM
 * 
 * Copyright 2017.
 * 
 */

#ifndef SSHSESSIONUTIL_HPP
#define SSHSESSIONUTIL_HPP

// =============
// INCLUDE FILES
// =============

//
// Antik utility
//

#include "CommonUtil.hpp"

//
// Antik Classes
//

#include "CSSHSession.hpp"

#include <iostream>

namespace Antik {
    namespace SSH {

        //
        // Context for server verification feedback
        //
        
        class ServerVerificationContext {
        public:
            explicit ServerVerificationContext(void *context=nullptr) : m_contextData{context}
            {
            }
            
            virtual void serverKnown(CSSHSession &sshSession) {

            }

            virtual bool serverKnownChanged(CSSHSession &sshSession, std::vector<unsigned char> &keyHash) {
                std::cerr << "Host key for server changed: it is now:\n" << sshSession.convertKeyHashToHex(keyHash) << std::endl;
                std::cerr << "For security reasons, connection will be stopped" << std::endl;
                return (false);
            }

            virtual bool serverFoundOther(CSSHSession &sshSession) {
                std::cerr << "The host key for this server was not found but an other type of key exists.\n";
                std::cerr << "An attacker might change the default server key to confuse your client into ";
                std::cerr << "thinking the key does not exist" << std::endl;
                return (false);
            }
            
            virtual bool serverFileNotFound(CSSHSession &sshSession,  std::vector<unsigned char> &keyHash) {
                std::cerr << "Could not find known host file.\n";
                std::cerr << "If you accept the host key here, the file will be automatically created." << std::endl;
                return(serverNotKnown(sshSession, keyHash));
            }
            
            virtual bool serverNotKnown(CSSHSession &sshSession,  std::vector<unsigned char> &keyHash) {
                std::string reply;
                std::cerr << "The server is unknown. Do you trust the host key?\n";
                std::cerr << "Public key hash: " << sshSession.convertKeyHashToHex(keyHash) << std::endl;
                std::cin >> reply;
                if (reply != "yes") {
                    return (false);
                }
                sshSession.writeKnownHost();
                return(true);
            }
            
            virtual bool serverError(CSSHSession &sshSession) {
                return(false);
            }
            
        protected:
            void *m_contextData;
            
        };
        
        bool userAuthorize(CSSHSession &session);
        bool verifyKnownServer(CSSHSession &sshSession, ServerVerificationContext &verificationContext);
        
    } // namespace SSH
} // namespace Antik

#endif /* SSHSESSIONUTIL_HPP */

