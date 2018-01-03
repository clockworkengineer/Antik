#include "HOST.hpp"
/*
 * File:   CSFTP.cpp(Work In Progress)
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
// Dependencies:   C11++        - Language standard features used.
//                 libssh       - Used to talk to SSH server (https://www.libssh.org/)
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
        
        void CSFTP::convertToFileAttributes(sftp_attributes file, SFTPFileAttributes &fileAttributes) {
            
            fileAttributes.name.assign(&file->name[0], &file->name[std::strlen(file->name)]);
            fileAttributes.flags = file->flags;
            fileAttributes.permissions = file->permissions;
            fileAttributes.size = file->size;
            fileAttributes.type = file->type;
            sftp_attributes_free(file);
            
        }
        
        // ==============
        // PUBLIC METHODS
        // ==============

        //
        // Main CSFTP object constructor. 
        //

        CSFTP::CSFTP(CSSHSession &session) : m_session{session}
        {
   
            m_sftp = sftp_new(m_session.getSession());
            if (m_sftp == NULL) {
                throw Exception ("Error allocating SFTP session: "+m_session.getError());
            }
            
        }

        //
        // CSFTP Destructor
        //

        CSFTP::~CSFTP() {

            close();
                
        }

        void CSFTP::open() {

            if (sftp_init(m_sftp) != SSH_OK) {
                sftp_free(m_sftp);
                m_sftp = NULL;
                throw Exception("Error initializing SFTP session: " + sftp_get_error(m_sftp));
            }

        }

        void CSFTP::close() {

            if (m_sftp) {
                sftp_free(m_sftp);
                m_sftp = NULL;
            }

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
                convertToFileAttributes(file, fileAttributes);
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
        
        int CSFTP::changePermissions(const SFTPFileAttributes &fileAttributes, const SFTPFilePermissions &filePermissions) {
            
            return(sftp_chmod(m_sftp, fileAttributes.name.c_str(), filePermissions));
            
        }

        int CSFTP::changeOwnerGroup(const SFTPFileAttributes &fileAttributes, const SFTPFileOwner &owner, const SFTPFileGroup &group) {
            return (sftp_chown(m_sftp, fileAttributes.name.c_str(), owner, group));
        }
        
        void CSFTP::getFileAttributes(const SFTPFile &fileDesc, SFTPFileAttributes &fileAttributes) {
            
            sftp_attributes file = sftp_fstat(fileDesc);
            
            if (file) {
                this->convertToFileAttributes(file, fileAttributes);
            }
            
        }
        
        void CSFTP::getLinkAttributes(const std::string &linkPath, SFTPFileAttributes &fileAttributes) {
            
            sftp_attributes file = sftp_lstat(m_sftp, linkPath.c_str());
            
            if (file) {
                this->convertToFileAttributes(file, fileAttributes);
            }
            
        }
        
        int CSFTP::createDirectory(const std::string &directoryPath, const SFTPFilePermissions &filePermissions) {
            
            return(sftp_mkdir(m_sftp, directoryPath.c_str(),filePermissions ));
            
        }

        int CSFTP::removeDirectory(const std::string &directoryPath) {

            return (sftp_rmdir(m_sftp, directoryPath.c_str()));
            
        }
        
        std::string CSFTP::readLink(const std::string &linkPath) {
            
            std::string finalPath;
            char *file;
            
            file = sftp_readlink(m_sftp, linkPath.c_str());
            
            if (file){
                finalPath.assign(&file[0],&file[std::strlen(file)]);
                free(file);
            }
            
            return(finalPath);
	
        }
        
        int CSFTP::renameFile(const std::string &sourceFile, const std::string &destinationFile) {
            
            return(sftp_rename(m_sftp,sourceFile.c_str(), destinationFile.c_str() ));
            
        }
        
        void CSFTP::rewindFile (SFTPFile fileDesc) {
            
            sftp_rewind(fileDesc);
            
        }
        
        int CSFTP::seekFile (SFTPFile fileDesc, uint32_t offset) {
            
            return(sftp_seek(fileDesc, offset));
            
        }
        
        int CSFTP::seekFile64 (SFTPFile fileDesc, uint64_t offset) {
            
            return(sftp_seek64(fileDesc, offset));
            
        }

        int CSFTP::serverVsersion() {

            return (sftp_server_version(m_sftp));

        }
        
        int CSFTP::getErrorCode() {
            
            int errorCode=sftp_get_error(m_sftp);
            
            return(errorCode);
            
        }
            
        sftp_session CSFTP::getSFTP() const {
            return m_sftp;
        }

        CSSHSession& CSFTP::getSession() const {
            return m_session;
        }

    } // namespace SSH
} // namespace Antik
