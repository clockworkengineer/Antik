/*
 * File:   CSCP.hpp
 * 
 * Author: Robert Tizzard (Work in Progress)
 * 
 * Created on September 23, 2017, 2:33 PM
 *
 * Copyright 2017.
 *
 */

#ifndef CSCP_HPP
#define CSCP_HPP

//
// C++ STL
//

#include <stdexcept>

//
// Antik classes
//

#include "CommonAntik.hpp"
#include "CSSHSession.hpp"

//
// Libssh
//

#include <libssh/libssh.h>

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

        class CSCP {
        public:

            // ==========================
            // PUBLIC TYPES AND CONSTANTS
            // ==========================

            //
            // Class exception
            //

            struct Exception {
                
                Exception(CSCP &scp, const std::string functionName) : m_errorCode{scp.getSession().getErrorCode()},
                m_errorMessage{ scp.getSession().getError()}, m_functionName{functionName}
                {
                }

                Exception(const std::string &errorMessage, const std::string &functionName) : m_errorMessage{ errorMessage }, 
                m_functionName{functionName}
                {
                }
                
                int getCode() const {
                    return m_errorCode;
                }

                std::string getMessage() const {
                    return static_cast<std::string> ("CSCP Failure: (") + m_functionName + ") [" + m_errorMessage + "]";
                }

            private:
                std::string m_functionName; // Current function name
                int m_errorCode {SSH_OK };  // SSH error code
                std::string m_errorMessage; // SSH error message

            };

            
            //
            // Re-map some linux types used (possibly make these more abstract at a later date).
            //
            
            typedef mode_t FilePermissions;     // File permission (boost::filesystem status for portable way to get)
            
            // ============
            // CONSTRUCTORS
            // ============

            //
            // Main constructor
            //

            explicit CSCP(CSSHSession &session, int mode, const std::string &location);

            // ==========
            // DESTRUCTOR
            // ==========

            virtual ~CSCP();

            // ==============
            // PUBLIC METHODS
            // ==============
        
            void open();
            void close();
            
            void pushDirectory(const std::string &directoryName, FilePermissions permissions);
            
            void pushFile(const std::string &fileName, size_t fileSize, FilePermissions permissions);
            void pushFile64(const std::string &fileName, uint64_t fileSize, FilePermissions permissions);
            
            void write(const void *buffer, size_t bufferSize);
            int read(void * buffer, size_t bufferSize);
            
            void leaveDirectory();
            
            int pullRequest(); 
            void acceptRequest();
            void denyRequest(const std::string &reason);
            std::string getRequestWarning();
            size_t requestFileSize();
            uint64_t requestFileSize64();
            std::string requestFileName();
            FilePermissions requestFilePermissions();

            CSSHSession& getSession() const;
            ssh_scp getSCP() const;
  
            //
            // Set IO buffer parameters.
            //
            
            std::shared_ptr<char> getIoBuffer();
            void setIoBufferSize(std::uint32_t ioBufferSize);
            std::uint32_t getIoBufferSize() const;
            
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

            CSCP(const CSCP & orig) = delete;
            CSCP(const CSCP && orig) = delete;
            CSCP& operator=(CSCP other) = delete;

            // ===============
            // PRIVATE METHODS
            // ===============
                   
            // =================
            // PRIVATE VARIABLES
            // =================
            
            
            CSSHSession &m_session;         // SCP session

            ssh_scp m_scp;
            
            int m_mode;
            std::string m_location;
            
            std::shared_ptr<char> m_ioBuffer { nullptr };  // IO buffer
            std::uint32_t m_ioBufferSize     { 32*1024 };  // IO buffer size

            
        };

    } // namespace SSH
} // namespace Antik

#endif /* CSCP_HPP */

