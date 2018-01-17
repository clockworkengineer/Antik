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

        typedef std::function<void (void *, uint32_t)>  WriteCallBackFn;

        static WriteCallBackFn defaultCOUT = [] (void *ioBuffer, uint32_t ioBufferSize) {
            std::cout.write(static_cast<char *> (ioBuffer), ioBufferSize);
            std::cout.flush();
        };

        static WriteCallBackFn defaultCERR = [] (void *ioBuffer, uint32_t ioBufferSize) {
            std::cerr.write(static_cast<char *> (ioBuffer), ioBufferSize);
        };
        
        void interactiveShell(CSSHChannel &channel, int columns, int rows, WriteCallBackFn writeFn=defaultCOUT);
        void executeCommand(CSSHChannel &channel, const std::string &command);
        std::thread directForwarding(CSSHChannel &forwardingChannel, const std::string &remoteHost, int remotePort, const std::string &localHost, int localPort, WriteCallBackFn writeFnwriteFn=defaultCOUT);
       
    } // namespace SSH
} // namespace Antik

#endif /* SSHCHANNELUTIL_HPP */

