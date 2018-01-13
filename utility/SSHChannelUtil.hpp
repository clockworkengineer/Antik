/*
 * File:   SSHChannelUtil.hpp
 * 
 * Author: Robert Tizzard
 *
 * Created on April 10, 2017, 2:34 PM
 * 
 * Copyright 2017.
 * 
 */

#ifndef SSHCHANNELUTIL_HPP
#define SSHCHANNELUTIL_HPP

// =============
// INCLUDE FILES
// =============

//
// Antik Classes
//

#include "CSSHChannel.hpp"

namespace Antik {
    namespace SSH {

        int interactiveShell(CSSHChannel &channel, int columns, int rows);
        void executeCommand(CSSHChannel &channel, const std::string &command);
        
    } // namespace SSH
} // namespace Antik

#endif /* SSHCHANNELUTIL_HPP */

