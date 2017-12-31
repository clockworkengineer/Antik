#include "HOST.hpp"
/*
 * File:   CSFTP.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on September 23, 2017, 2:33 PM
 *
 * Copyright 2017.
 *
 */

//
// Class: CSFTP
// 
// Description:
//
// Note: TLS/SSL connections are supported.
//
// Dependencies:   C11++        - Language standard features used.
//                 CSocket   -  - Used to talk to FTP server.
//

// =================
// CLASS DEFINITIONS
// =================

#include "CSFTP.hpp"
#include "CSSHSession.hpp"

// ====================
// CLASS IMPLEMENTATION
// ====================

//
// C++ STL
//

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
        // Main CSFTP object constructor. 
        //

        CSFTP::CSFTP(CSSHSession &session) : m_session{session}
        {
            int returnCode;
            m_sftp = sftp_new(m_session.getSession());
            if (m_sftp == NULL) {
                throw Exception ("Error allocating SFTP session: "+m_session.getError());
            }
            returnCode = sftp_init(m_sftp);
            if (returnCode != SSH_OK) {
                sftp_free(m_sftp);
                m_sftp=NULL;
                throw Exception ("Error initializing SFTP session: "+sftp_get_error(m_sftp));
            }
            
        }

        //
        // CSFTP Destructor
        //

        CSFTP::~CSFTP() {
            
            sftp_free(m_sftp);
            m_sftp=NULL;
                
        }

        Antik::SSH::CSFTP::SFTPFile CSFTP::openFile(const std::string &fileName, int accessType, int mode) {
            
            return(sftp_open(m_sftp, fileName.c_str(), accessType, mode));

        }

        size_t CSFTP::readFile(SFTPFile fileDesc, void *readBuffer, size_t bytesToRead) {

            return(sftp_read(fileDesc, readBuffer, bytesToRead));
            
        }
        
        size_t CSFTP::writeFile(SFTPFile fileDesc, void *writeBuffer, size_t bytesToWrite) {

            return(sftp_write(fileDesc, writeBuffer, bytesToWrite));
            
        }

        int CSFTP::closeFile(SFTPFile fileDesc) {
            
            return(sftp_close(fileDesc));
            
        }

        Antik::SSH::CSFTP::STFPDirectory CSFTP::openDirectory(const std::string &directoryPath) {
            
            return(sftp_opendir(m_sftp,directoryPath.c_str()));
            
        }

        bool CSFTP::readDirectory(STFPDirectory &directoryHandle, SFTPFileAttributes &fileAttributes) {
            
            sftp_attributes file;
            
            file = sftp_readdir(m_sftp, directoryHandle);
            
            if (file) {
                fileAttributes.name.assign(&file->name[0], &file->name[std::strlen(file->name)]);
                fileAttributes.flags = file->flags;
                fileAttributes.permissions = file->permissions;
                fileAttributes.size = file->size;
                fileAttributes.type = file->type;
                sftp_attributes_free(file);
                return(true);
            }
            
            return(false);
            
        }

        int CSFTP::endOfDirectory(STFPDirectory &directoryHandle) {
            
            return(sftp_dir_eof(directoryHandle));
            
        }

        int CSFTP::closeDirectory(STFPDirectory &directoryHandle) {
            
            return(sftp_closedir(directoryHandle));
            
        }

        bool CSFTP::isADirectory(const SFTPFileAttributes &fileAttributes) {
            
            return(fileAttributes.type==2);
            
        }
        
        bool CSFTP::isASymbolicLink(const SFTPFileAttributes &fileAttributes) {
            
            return(fileAttributes.type==3);
            
        }

        bool CSFTP::isARegularFile(const SFTPFileAttributes &fileAttributes) {
             return(fileAttributes.type==1);
        }
            
        sftp_session CSFTP::getSFTP() const {
            return m_sftp;
        }

        CSSHSession& CSFTP::getSession() const {
            return m_session;
        }

    } // namespace SSH
} // namespace Antik
