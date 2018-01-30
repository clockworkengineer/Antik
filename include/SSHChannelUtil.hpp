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
        // IO context for channel feedback
        //
        
        class IOContext {
        public:
            explicit IOContext(void *context) : m_contextData{context}
            {
            }
            virtual void writeOutFn(void *data, uint32_t size) {
                std::cout.write((char *) data, size);
            }
            virtual void writeErrFn(void *data, uint32_t size) {
                std::cerr.write((char *) data, size);
            }
            bool useInternalInput() const {
                return m_internalInput;
            }
        protected:
            void *m_contextData  {nullptr};
            bool m_internalInput {true};
        };
           
        void interactiveShell(CSSHChannel &channel, int columns, int rows, IOContext &ioContext);
        void executeCommand(CSSHChannel &channel, const std::string &command, IOContext &ioContext);
        std::thread directForwarding(CSSHChannel &forwardingChannel, const std::string &remoteHost, int remotePort, const std::string &localHost, int localPort, IOContext &ioContext);
       
    } // namespace SSH
} // namespace Antik

#endif /* SSHCHANNELUTIL_HPP */

