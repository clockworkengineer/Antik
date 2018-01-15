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
// C++ STL
//

#include <iostream>
#include <thread>
#include <functional>

//
// Antik Classes
//

#include "CSSHChannel.hpp"

namespace Antik {
    namespace SSH {

        typedef std::function<void (void *, uint32_t)>  ChannelWriteCallBack;
        
        void interactiveShell(CSSHChannel &channel, int columns, int rows);
        void executeCommand(CSSHChannel &channel, const std::string &command);
        std::thread directForwarding(CSSHChannel &forwardingChannel, const std::string &remoteHost, int remotePort, const std::string &localHost, int localPort, ChannelWriteCallBack writeFn);
       
    } // namespace SSH
} // namespace Antik

#endif /* SSHCHANNELUTIL_HPP */

