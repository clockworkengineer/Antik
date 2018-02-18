#include "HOST.hpp"
/*
 * File:   CSCP.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on September 23, 2017, 2:33 PM
 *
 * Copyright 2017.
 *
 */

//
// Class: CSCP
// 
// Description: A class to open an SCP session with a server over SSH and issue SCP
// commands on remote files. It is very much a wrapper class for libssh scp functionality
// but it also wraps the main data structures in unique pointers with there own custom deleters.
// It also tries to hide as much of its implementation using libssh as possible and use/return 
// C11++ data structures/exceptions. It is not complete by any means but may be updated to 
// future to use more libssh features.
//
// Dependencies:   
// 
// C11++        - Language standard features used.
// libssh       - Used to talk to SSH server (https://www.libssh.org/) (0.7.5)
//


// =================
// CLASS DEFINITIONS
// =================

#include "CSCP.hpp"

// ====================
// CLASS IMPLEMENTATION
// ====================

// =======
// IMPORTS
// =======

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace SSH {

        // ===========================
        // PRIVATE TYPES AND CONSTANTS
        // ===========================

        // ==========================
        // PUBLIC TYPES AND CONSTANTS
        // ==========================

        // ========================
        // PRIVATE STATIC VARIABLES
        // ========================

        // =======================
        // PUBLIC STATIC VARIABLES
        // =======================

        // ===============
        // PRIVATE METHODS
        // ===============
        
        // ==============
        // PUBLIC METHODS
        // ==============

        //
        // Main CSCP object constructor. 
        //

        CSCP::CSCP(CSSHSession &session, int mode, const std::string &location) : m_session {session},
                m_mode {mode }, m_location {location }
        {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};
   
            if ((m_scp = ssh_scp_new(m_session.getSession(), mode, location.c_str()))== NULL) {
                throw Exception("Could not allocate new SCP session.", __func__);
            }
            
        }

        //
        // CSCP Destructor.
        //

        CSCP::~CSCP() {
            
        }

        //
        // Open SCP server connection.
        //
        
        void CSCP::open() {
   
            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};
            
            if (ssh_scp_init(m_scp)!=SSH_OK) {
                throw Exception(*this, __func__);
            }
             
        }
        
        //
        // Close SCP server connection and free its resources.
        //
        
        void CSCP::close() {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};
   
            if (m_scp) {
                ssh_scp_close(m_scp);
                ssh_scp_free(m_scp);
                m_scp == NULL;
            }  
             
             m_ioBuffer.reset();
             
        }
        
        //
        // Create a directory on remote server.
        //
        
        void CSCP::pushDirectory(const std::string &directoryName, FilePermissions permissions) {
   
            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};
            
            if (ssh_scp_push_directory(m_scp, directoryName.c_str(), permissions)!=SSH_OK) {
                 throw Exception(*this, __func__);              
            }
            
        }
        
        //
        // Create a file on remote server.
        //

        void CSCP::pushFile(const std::string &fileName, size_t fileSize, FilePermissions permissions) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};
   
            if (ssh_scp_push_file(m_scp, fileName.c_str(), fileSize, permissions) != SSH_OK) {
                throw Exception(*this, __func__);
            }

        }
        
        //
        // Create a file on remote server ( > 4GB).
        //
        
        void CSCP::pushFile64(const std::string &fileName, uint64_t fileSize, FilePermissions permissions) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};
   
            if (ssh_scp_push_file64(m_scp, fileName.c_str(), fileSize, permissions) != SSH_OK) {
                throw Exception(*this, __func__);
            }

        }

        //
        // Write data to newly created remote file.
        //
        
        void  CSCP::write(const void *buffer, size_t bufferSize) {
   
            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};
            
          if (ssh_scp_write(m_scp, buffer, bufferSize) != SSH_OK) {
                throw Exception(*this, __func__);
          }
          
        }
        
        //
        // Read data from recently requested remote file.
        //

        int CSCP::read(void * buffer, size_t bufferSize) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};
   
            int returnCode = ssh_scp_read(m_scp, buffer, bufferSize);

            if (returnCode == SSH_ERROR) {
                throw Exception(*this, __func__);
            }

            return (returnCode);

        }
        
	//
        // Request next remote file/directory (can be recursive).
        //
        
        int CSCP::pullRequest() {
   
            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};
            
            int returnCode = ssh_scp_pull_request(m_scp);
            
            if (returnCode==SSH_ERROR) {
               throw Exception(*this, __func__);             
            }
            
            return(returnCode);

        }
        
        //
        // Retrieve currently requested file size.
        //

        size_t CSCP::requestFileSize() {
   
            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};
            
            return(ssh_scp_request_get_size(m_scp));

        }
        
        //
        // Retrieve currently requested file size (>4GB).
        //

        uint64_t CSCP::requestFileSize64() {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};
   
            return (ssh_scp_request_get_size64(m_scp));

        }
        
        //
        // Retrieve currently requested file name.
        //

        std::string CSCP::requestFileName() {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};
   
            std::string fileName;
            const char *filename = ssh_scp_request_get_filename(m_scp);

            if (filename == NULL) {
                throw Exception(*this, __func__);
            }

            fileName.assign(&filename[0], &filename[std::strlen(filename)]);
         
            return(fileName);
            
        }

        //
        // Retrieve currently requested file permissions.
        //
        
        CSCP::FilePermissions CSCP::requestFilePermissions() {
   
            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};
            
             return(ssh_scp_request_get_permissions(m_scp));

        }

        //
        // Accept last requested file/directory.
        //
        
        void CSCP::acceptRequest() {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};
   
            if (ssh_scp_accept_request(m_scp) != SSH_OK) {
                throw Exception(*this, __func__);
            }

        }

        //
        // Deny last requested file/directory..
        //
        
        void CSCP::denyRequest(const std::string &reason) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};
   
            if (ssh_scp_deny_request(m_scp, reason.c_str()) != SSH_OK) {
                throw Exception(*this, __func__);
            }
                       
        }

        //
        // If last pull request resulted in a warning then retrieve the message.
        //
        
        std::string CSCP::getRequestWarning() {
   
            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};
            
            std::string requestWarning;
            const char *warning = ssh_scp_request_get_warning(m_scp);
            
            if (warning==NULL) {
                throw Exception(*this, __func__);
            }
            
            requestWarning.assign(&warning[0], &warning[std::strlen(warning)]);
            
            return (requestWarning);
            
        }

        //
        // Move up a in remote directory structure.
        //
        
        void CSCP::leaveDirectory() {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};
   
            if (ssh_scp_leave_directory(m_scp) != SSH_OK) {
                throw Exception(*this, __func__);
            }

        }
   
        //
        // Set/Get IO buffer parameters.
        //

        std::shared_ptr<char> CSCP::getIoBuffer() {
   
            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};
            
            if (!m_ioBuffer) {
                setIoBufferSize(m_ioBufferSize);
            }
            
            return m_ioBuffer;
            
        }

        void CSCP::setIoBufferSize(std::uint32_t ioBufferSize) {
   
            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};
            
            m_ioBufferSize = ioBufferSize;
            m_ioBuffer.reset(new char[m_ioBufferSize]);
            
        }

        std::uint32_t CSCP::getIoBufferSize() const {
            
            return m_ioBufferSize;
            
        }
        
        //
        // Return internal libssh session reference,
        //
        
        CSSHSession& CSCP::getSession() const {

            return m_session;

        }

        ssh_scp CSCP::getSCP() const {
            
            return m_scp;
            
        }
        
    } // namespace FTP
} // namespace Antik
