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
            explicit ServerVerificationContext(void *context=nullptr) : m_contextData{context}{ }
            virtual void serverKnown();
            virtual bool serverKnownChanged(std::vector<unsigned char> &keyHash);
            virtual bool serverFoundOther();
            virtual bool serverFileNotFound(std::vector<unsigned char> &keyHash);
            virtual bool serverNotKnown(std::vector<unsigned char> &keyHash);
            virtual bool serverError();
        protected:
            void *m_contextData{nullptr};
        };
        
        bool userAuthorize(CSSHSession &session);
        bool verifyKnownServer(CSSHSession &sshSession, ServerVerificationContext &verificationContext);
        
    } // namespace SSH
} // namespace Antik

#endif /* SSHSESSIONUTIL_HPP */

