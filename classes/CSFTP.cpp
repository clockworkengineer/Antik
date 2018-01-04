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

        void CSFTP::convertSshFileToFileAttributes(sftp_attributes file, FileAttributes &fileAttributes) {

            if (file->name) {
                fileAttributes.name.assign(&file->name[0], &file->name[std::strlen(file->name)]);
            }
            fileAttributes.flags = file->flags;
            fileAttributes.type = file->type;
            if (fileAttributes.flags & 0x1) {
                fileAttributes.size = file->size;
            }
            if (fileAttributes.flags & 0x4) {
                fileAttributes.permissions = file->permissions;
            }
            
            sftp_attributes_free(file);

        }

        void CSFTP::convertFileAttributesToSshFile(sftp_attributes file, const FileAttributes &fileAttributes) {

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
                throw Exception(*this, __func__);
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
                throw Exception(*this, __func__);
            }
            
            // Allocate IO Buffer
            
            m_ioBuffer.reset(new char[m_ioBufferSize]);

        }

        void CSFTP::close() {

            if (m_sftp) {
                sftp_free(m_sftp);
                m_sftp = NULL;
            }
            
            // Free IO Buffer
            
            m_ioBuffer.reset();

        }

        Antik::SSH::CSFTP::File CSFTP::openFile(const std::string &fileName, int accessType, int mode) {

            File fileDesc = sftp_open(m_sftp, fileName.c_str(), accessType, mode);

            if (fileDesc == NULL) {
                throw Exception(*this, __func__);
            }

            return (fileDesc);

        }

        size_t CSFTP::readFile(File fileDesc, void *readBuffer, size_t bytesToRead) {

            size_t bytesRead = sftp_read(fileDesc, readBuffer, bytesToRead);

            if (bytesRead < 0) {
                throw Exception(*this, __func__);
            }

            return (bytesRead);

        }

        size_t CSFTP::writeFile(File fileDesc, void *writeBuffer, size_t bytesToWrite) {

            size_t bytesWritten = sftp_write(fileDesc, writeBuffer, bytesToWrite);

            if (bytesWritten < 0) {
                throw Exception(*this, __func__);
            }

            return (bytesWritten);

        }

        void CSFTP::closeFile(File fileDesc) {

            if (sftp_close(fileDesc) == SSH_ERROR) {
                throw Exception(*this, __func__);
            }

        }

        Antik::SSH::CSFTP::Directory CSFTP::openDirectory(const std::string &directoryPath) {

            Directory directory = sftp_opendir(m_sftp, directoryPath.c_str());

            if (directory == NULL) {
                throw Exception(*this, __func__);
            }

            return (directory);

        }

        bool CSFTP::readDirectory(Directory &directoryHandle, FileAttributes &fileAttributes) {

            sftp_attributes file;

            file = sftp_readdir(m_sftp, directoryHandle);

            if (file) {
                convertSshFileToFileAttributes(file, fileAttributes);
                return (true);
            }

            return (false);

        }

        bool CSFTP::endOfDirectory(Directory &directoryHandle) {

            return (sftp_dir_eof(directoryHandle));

        }

        void CSFTP::closeDirectory(Directory &directoryHandle) {

            if (sftp_closedir(directoryHandle) == SSH_ERROR) {
                throw Exception(*this, __func__);
            }

        }

        bool CSFTP::isADirectory(const FileAttributes &fileAttributes) {

            return (fileAttributes.type == 2);

        }

        bool CSFTP::isASymbolicLink(const FileAttributes &fileAttributes) {

            return (fileAttributes.type == 3);

        }

        bool CSFTP::isARegularFile(const FileAttributes &fileAttributes) {
            return (fileAttributes.type == 1);
        }

        void CSFTP::changePermissions(const FileAttributes &fileAttributes, const FilePermissions &filePermissions) {

            if (sftp_chmod(m_sftp, fileAttributes.name.c_str(), filePermissions) < 0) {
                throw Exception(*this, __func__);
            }

        }

        void CSFTP::changeOwnerGroup(const FileAttributes &fileAttributes, const FileOwner &owner, const FileGroup &group) {

            if (sftp_chown(m_sftp, fileAttributes.name.c_str(), owner, group) < 0) {
                throw Exception(*this, __func__);
            }

        }

        void CSFTP::getFileAttributes(const File &fileDesc, FileAttributes &fileAttributes) {

            sftp_attributes file = sftp_fstat(fileDesc);

            if (file == NULL) {
                throw Exception(*this, __func__);
            }

            this->convertSshFileToFileAttributes(file, fileAttributes);

        }

        void CSFTP::setFileAttributes(const File &fileDesc, const FileAttributes &fileAttributes) {

        }

        void CSFTP::getLinkAttributes(const std::string &linkPath, FileAttributes &fileAttributes) {

            sftp_attributes file = sftp_lstat(m_sftp, linkPath.c_str());

            if (file == NULL) {
                throw Exception(*this, __func__);
            }

            this->convertSshFileToFileAttributes(file, fileAttributes);

        }

        void CSFTP::createDirectory(const std::string &directoryPath, const FilePermissions &filePermissions) {

            if (sftp_mkdir(m_sftp, directoryPath.c_str(), filePermissions)) {
                throw Exception(*this, __func__);
            }

        }

        void CSFTP::removeDirectory(const std::string &directoryPath) {

            if (sftp_rmdir(m_sftp, directoryPath.c_str()) < 0) {
                throw Exception(*this, __func__);
            }

        }

        std::string CSFTP::readLink(const std::string &linkPath) {

            std::string finalPath;
            char *file;

            file = sftp_readlink(m_sftp, linkPath.c_str());

            if (file == NULL) {
                throw Exception(*this, __func__);
            }

            finalPath.assign(&file[0], &file[std::strlen(file)]);
            free(file);

            return (finalPath);

        }

        void CSFTP::createLink(const std::string &targetPath, const std::string &linkPath) {

            if (sftp_symlink(m_sftp, targetPath.c_str(), linkPath.c_str()) < 0) {
                throw Exception(*this, __func__);
            }

        }

        void CSFTP::removeLink(const std::string &filePath) {

            if (sftp_unlink(m_sftp, filePath.c_str()) < 0) {
                throw Exception(*this, __func__);
            }

        }

        void CSFTP::renameFile(const std::string &sourceFile, const std::string &destinationFile) {

            if (sftp_rename(m_sftp, sourceFile.c_str(), destinationFile.c_str()) < 0) {
                throw Exception(*this, __func__);
            }

        }

        void CSFTP::rewindFile(File fileDesc) {

            sftp_rewind(fileDesc);

        }

        void CSFTP::seekFile(File fileDesc, uint32_t offset) {

            if (sftp_seek(fileDesc, offset) < 0) {
                throw Exception(*this, __func__);
            }

        }

        void CSFTP::seekFile64(File fileDesc, uint64_t offset) {

            if (sftp_seek64(fileDesc, offset) < 0) {
                throw Exception(*this, __func__);
            }

        }

        uint32_t CSFTP::currentFilePostion(File fileDesc) {

            int32_t position = sftp_tell(fileDesc);

            if (position < 0) {
                throw Exception(*this, __func__);
            }

            return (static_cast<uint32_t> (position));

        }

        uint64_t CSFTP::currentFilePostion64(File fileDesc) {

            int64_t position = sftp_tell64(fileDesc);

            if (position < 0) {
                throw Exception(*this, __func__);
            }

            return (static_cast<uint64_t> (position));


        }

        std::string CSFTP::canonicalizePath(const std::string &pathName) {

            std::string finalPath;
            char *path;

            path = sftp_canonicalize_path(m_sftp, pathName.c_str());

            if (path == NULL) {
                throw Exception(*this, __func__);
            }

            finalPath.assign(&path[0], &path[std::strlen(path)]);
            free(path);

            return (finalPath);


        }

        int CSFTP::getServerVersion() const {

            return (sftp_server_version(m_sftp));

        }

        int CSFTP::getErrorCode() const {

            int errorCode = sftp_get_error(m_sftp);

            return (errorCode);

        }

        sftp_session CSFTP::getSFTP() const {
            return m_sftp;
        }

        CSSHSession& CSFTP::getSession() const {
            return m_session;
        }

        std::shared_ptr<char> CSFTP::getIoBuffer() const {
            return m_ioBuffer;
        }

        void CSFTP::setIoBufferSize(std::uint32_t ioBufferSize) {
            m_ioBufferSize = ioBufferSize;
            m_ioBuffer.reset(new char[m_ioBufferSize]);
        }

        std::uint32_t CSFTP::getIoBufferSize() const {
            return m_ioBufferSize;
        }

    } // namespace SSH
} // namespace Antik