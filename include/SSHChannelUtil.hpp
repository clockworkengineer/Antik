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
// Antik utility
//

#include "CommonUtil.hpp"

//
// Antik Classes
//

#include "CSSHChannel.hpp"

namespace Antik {
    namespace SSH {
        
        //
        // Write call back function for channel feedback
        //
        
        typedef std::function<void (void *, uint32_t, void *)>  WriteCallBackFn;
        
        //
        // Write context for channel feedback
        //
        
        struct WriteOutputContext {
            WriteCallBackFn writeOutFn {nullptr};   // stdout
            WriteCallBackFn writeErrFn {nullptr};   // stderr
            void *contextData {nullptr};            // context data
        };
           
        void interactiveShell(CSSHChannel &channel, int columns, int rows, WriteOutputContext &writeContext);
        void executeCommand(CSSHChannel &channel, const std::string &command, WriteOutputContext &writeContext);
        std::thread directForwarding(CSSHChannel &forwardingChannel, const std::string &remoteHost, int remotePort, const std::string &localHost, int localPort, WriteOutputContext &writeContext);
       
    } // namespace SSH
} // namespace Antik

#endif /* SSHCHANNELUTIL_HPP */

