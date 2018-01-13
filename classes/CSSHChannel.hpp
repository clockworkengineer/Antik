/*
 * File:   CSSHChannel.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on September 23, 2017, 2:33 PM
 *
 * Copyright 2017.
 *
 */

#ifndef CSSHCHANNEL_HPP
#define CSSHCHANNEL_HPP

//
// C++ STL
//

#include <stdexcept>

//
// Antik Classes
//

#include "CSSHSession.hpp"

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace SSH {
        
        class CSSHSession;

        // ==========================
        // PUBLIC TYPES AND CONSTANTS
        // ==========================
        
        // ================
        // CLASS DEFINITION
        // ================

        class CSSHChannel {
            
        public:

            // ==========================
            // PUBLIC TYPES AND CONSTANTS
            // ==========================

            //
            // Class exception
            //

            struct Exception {
                Exception(CSSHChannel &channel, const std::string functionName) : m_errorCode{channel.getSession().getErrorCode()},
                m_errorMessage{ channel.getSession().getError()}, m_functionName{functionName}
                {
                }

                int getCode() const {
                    return m_errorCode;
                }

                std::string getMessage() const {
                    return static_cast<std::string> ("CSSHChannel Failure: (") + m_functionName + ") [" + m_errorMessage + "]";
                }

            private:
                std::string m_functionName; // Current function name
                int m_errorCode; // SSH error code
                std::string m_errorMessage; // SSH error message

            };

            // ============
            // CONSTRUCTORS
            // ============

            //
            // Main constructor
            //

            explicit CSSHChannel(CSSHSession &session);
            
            // ==========
            // DESTRUCTOR
            // ==========

            virtual ~CSSHChannel();

            // ==============
            // PUBLIC METHODS
            // ==============
            
            void open();
            
            void close();
            
            void sendEndOfFile();
            
            void execute(const std::string &commandToRun);
            
            int read(void *buffer, uint32_t bytesToRead, bool isStdErr=false);
            
            int readNonBlocking(void *buffer, uint32_t bytesToRead, bool isStdErr=false);
            
            int write(void *buffer, uint32_t bytesToWrite);
            
            void requestTerminal();
            
            void requestTerminalSize(int columns, int rows);
            
            void requestShell();
            
            bool isOpen();
            bool isClosed();
            
            bool isEndOfFile();
            
            void setEnvironmentVariable(const std::string &variable, const std::string &value);
            
            int getExitStatus();
            
            //
            // Set IO buffer parameters.
            //
            
            std::shared_ptr<char> getIoBuffer() const;
            void setIoBufferSize(std::uint32_t ioBufferSize);
            std::uint32_t getIoBufferSize() const;
            
            CSSHSession& getSession() const;
            ssh_channel getChannel() const;
        
            // ================
            // PUBLIC VARIABLES
            // ================

        private:

            // ===========================
            // PRIVATE TYPES AND CONSTANTS
            // ===========================
          
                        
            // ===========================================
            // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
            // ===========================================

            CSSHChannel(const CSSHChannel & orig) = delete;
            CSSHChannel(const CSSHChannel && orig) = delete;
            CSSHChannel& operator=(CSSHChannel other) = delete;

            // ===============
            // PRIVATE METHODS
            // ===============
                   
           
            // =================
            // PRIVATE VARIABLES
            // =================
  
            CSSHSession &m_session;
                     
            ssh_channel m_channel;
            
            std::shared_ptr<char> m_ioBuffer { nullptr };  // IO buffer
            std::uint32_t m_ioBufferSize     { 32*1024 };  // IO buffer size


        };

    } // namespace SSH
} // namespace Antik

#endif /* CSSHCHANNEL_HPP */

