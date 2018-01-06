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
// C++ STL
//

//
// Antik Classes
//

#include "CSSHSession.hpp"

namespace Antik {
    namespace SSH {

        bool sshUserAuthorize(CSSHSession &session);
        bool sshVerifyKnownServer(CSSHSession &sshSession);
        
    } // namespace SSH
} // namespace Antik

#endif /* SSHSESSIONUTIL_HPP */

