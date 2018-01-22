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

namespace Antik {
    namespace SSH {

        bool userAuthorize(CSSHSession &session);
        bool verifyKnownServer(CSSHSession &sshSession);
        
    } // namespace SSH
} // namespace Antik

#endif /* SSHSESSIONUTIL_HPP */

