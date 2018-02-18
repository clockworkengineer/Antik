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
// Description A class to open an SFTP session with a server over SSH and issue SFTP
// commands on remote files. It is very much a wrapper class for libssh sftp functionality
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
//

// =================
// CLASS DEFINITIONS
// =================

#include "CSFTP.hpp"

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
        // Main CSFTP object constructor. The passed in session has to be
        // connected and authorized for a SFTP session to be created.
        //

        CSFTP::CSFTP(CSSHSession &session) : m_session{session}
        {

            assert(session.isConnected() && session.isAuthorized());

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};
                   
            if ((m_sftp = sftp_new(m_session.getSession()))==NULL) {
                throw Exception("Could not allocate new SFTP session.", __func__);
            }

        }

        //
        // CSFTP Destructor
        //

        CSFTP::~CSFTP() {
          
        }

        //
        // Open up connection to SFTP server.
        //

        void CSFTP::open() {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            if (sftp_init(m_sftp) != SSH_OK) {
                sftp_free(m_sftp);
                m_sftp = NULL;
                throw Exception(*this, __func__);
            }

        }

        //
        // Close connection with SFTP server and free its resources.
        //

        void CSFTP::close() {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            if (m_sftp) {
                sftp_free(m_sftp);
                m_sftp = NULL;
            }
                    
            // Free IO Buffer

            m_ioBuffer.reset();

        }

        //
        // Open a remote file for IO.
        //

        CSFTP::File CSFTP::openFile(const std::string &fileName, int accessType, int mode) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            File fileHandle{ sftp_open(m_sftp, fileName.c_str(), accessType, mode)};

            if (fileHandle.get() == NULL) {
                throw Exception(*this, __func__);
            }

            return (fileHandle);

        }

        //
        // Read from a remote file.
        //

        size_t CSFTP::readFile(const File &fileHandle, void *readBuffer, size_t bytesToRead) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            size_t bytesRead = sftp_read(fileHandle.get(), readBuffer, bytesToRead);

            if (static_cast<int>(bytesRead) < 0) {
                throw Exception(*this, __func__);
            }

            return (bytesRead);

        }

        //
        // Write to a remote file.
        //

        size_t CSFTP::writeFile(const File &fileHandle, void *writeBuffer, size_t bytesToWrite) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            size_t bytesWritten = sftp_write(fileHandle.get(), writeBuffer, bytesToWrite);

            if (static_cast<int>(bytesWritten) < 0) {
                throw Exception(*this, __func__);
            }

            return (bytesWritten);

        }

        //
        // Close a remote file.
        //

        void CSFTP::closeFile(File &fileHandle) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            if (sftp_close(fileHandle.release()) == SSH_ERROR) {
                throw Exception(*this, __func__);
            }

        }

        //
        // Open a remote directory for reading.
        //

        CSFTP::Directory CSFTP::openDirectory(const std::string &directoryPath) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            Directory directory{ sftp_opendir(m_sftp, directoryPath.c_str())};

            if (directory.get() == NULL) {
                throw Exception(*this, __func__);
            }

            return (directory);

        }

        //
        // Read a files directory entry (return true on success).
        //

        bool CSFTP::readDirectory(const Directory &directoryHandle, FileAttributes &fileAttributes) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            sftp_attributes file{ sftp_readdir(m_sftp, directoryHandle.get())};

            if (file) {
                fileAttributes.reset(file);
                return (true);
            }

            return (false);

        }

        //
        // Return  true if the end of a directory file has been reached,
        //

        bool CSFTP::endOfDirectory(const Directory &directoryHandle) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            return (sftp_dir_eof(directoryHandle.get()));

        }

        //
        // Close remote directory file.
        //

        void CSFTP::closeDirectory(Directory &directoryHandle) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            if (sftp_closedir(directoryHandle.release()) == SSH_ERROR) {
                throw Exception(*this, __func__);
            }

        }

        //
        // Return true if remote file attributes belongs to a directory.
        //

        bool CSFTP::isADirectory(const FileAttributes &fileAttributes) {

            return (fileAttributes->type == SSH_FILEXFER_TYPE_DIRECTORY);

        }

        //
        // Return true if remote file attributes belongs to a symbolic link.
        //

        bool CSFTP::isASymbolicLink(const FileAttributes &fileAttributes) {

            return (fileAttributes->type == SSH_FILEXFER_TYPE_SYMLINK);

        }

        //
        // Return true if remote file attributes belongs to a regular file.
        //

        bool CSFTP::isARegularFile(const FileAttributes &fileAttributes) {
            
            return (fileAttributes->type == SSH_FILEXFER_TYPE_REGULAR);
            
        }

        //
        // Change the permissions on a remote file.
        //

        void CSFTP::changePermissions(const std::string &filePath, const FilePermissions &filePermissions) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            if (sftp_chmod(m_sftp, filePath.c_str(), filePermissions) < 0) {
                throw Exception(*this, __func__);
            }

        }

        //
        // Change to owner/group of a remote file.
        //

        void CSFTP::changeOwnerGroup(const std::string &filePath, const FileOwner &owner, const FileGroup &group) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            if (sftp_chown(m_sftp, filePath.c_str(), owner, group) < 0) {
                throw Exception(*this, __func__);
            }

        }

        //
        // Get the attributes of a file from file handle passed in.
        //

        void CSFTP::getFileAttributes(const File &fileHandle, FileAttributes &fileAttributes) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            sftp_attributes file{ sftp_fstat(fileHandle.get())};

            if (file == NULL) {
                throw Exception(*this, __func__);
            }

            fileAttributes.reset(file);

        }

        //
        // Get the attributes of a file from file name passed in.
        //

        void CSFTP::getFileAttributes(const std::string &filePath, FileAttributes &fileAttributes) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            sftp_attributes file{ sftp_stat(m_sftp, filePath.c_str())};

            if (file == NULL) {
                throw Exception(*this, __func__);
            }

            fileAttributes.reset(file);

        }

        //
        // Set the attributes of a file.
        //

        void CSFTP::setFileAttributes(const std::string &filePath, const FileAttributes &fileAttributes) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            if (sftp_setstat(m_sftp, filePath.c_str(), fileAttributes.get()) < 0) {
                throw Exception(*this, __func__);
            }

        }

        //
        // Get the attributes of a file that is the target of a symbolic link.
        //

        void CSFTP::getLinkAttributes(const std::string &linkPath, FileAttributes &fileAttributes) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            sftp_attributes file = sftp_lstat(m_sftp, linkPath.c_str());

            if (file == NULL) {
                throw Exception(*this, __func__);
            }

            fileAttributes.reset(file);

        }

        //
        // Create a remote directory.
        //

        void CSFTP::createDirectory(const std::string &directoryPath, const FilePermissions &filePermissions) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            if (sftp_mkdir(m_sftp, directoryPath.c_str(), filePermissions)) {
                throw Exception(*this, __func__);
            }

        }

        //
        // Remove a remote directory.
        //

        void CSFTP::removeDirectory(const std::string &directoryPath) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            if (sftp_rmdir(m_sftp, directoryPath.c_str()) < 0) {
                throw Exception(*this, __func__);
            }

        }

        //
        // Return the file name that is the target of a link.
        //

        std::string CSFTP::readLink(const std::string &linkPath) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

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

        //
        // Create a remote symbolic link.
        //

        void CSFTP::createLink(const std::string &targetPath, const std::string &linkPath) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            if (sftp_symlink(m_sftp, targetPath.c_str(), linkPath.c_str()) < 0) {
                throw Exception(*this, __func__);
            }

        }

        //
        // Remove a remote file.
        //

        void CSFTP::removeLink(const std::string &filePath) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            if (sftp_unlink(m_sftp, filePath.c_str()) < 0) {
                throw Exception(*this, __func__);
            }

        }

        //
        // Rename a remote file.
        //

        void CSFTP::renameFile(const std::string &sourceFile, const std::string &destinationFile) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            if (sftp_rename(m_sftp, sourceFile.c_str(), destinationFile.c_str()) < 0) {
                throw Exception(*this, __func__);
            }

        }

        //
        // Rewind a file to its start position.
        //

        void CSFTP::rewindFile(const File &fileHandle) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            sftp_rewind(fileHandle.get());

        }

        //
        // Move to a specified offset within a file.
        //

        void CSFTP::seekFile(const File &fileHandle, uint32_t offset) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            if (sftp_seek(fileHandle.get(), offset) < 0) {
                throw Exception(*this, __func__);
            }

        }

        //
        // Move to a specified offset within a file (64 bit). 
        //

        void CSFTP::seekFile64(const File &fileHandle, uint64_t offset) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            if (sftp_seek64(fileHandle.get(), offset) < 0) {
                throw Exception(*this, __func__);
            }

        }

        //
        // Get the current offset within a file.
        //

        uint32_t CSFTP::currentFilePostion(const File &fileHandle) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            int32_t position = sftp_tell(fileHandle.get());

            if (position < 0) {
                throw Exception(*this, __func__);
            }

            return (static_cast<uint32_t> (position));

        }

        //
        // Get the current offset within a file (64 bit).
        //

        uint64_t CSFTP::currentFilePostion64(const File &fileHandle) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            int64_t position = sftp_tell64(fileHandle.get());

            if (position < 0) {
                throw Exception(*this, __func__);
            }

            return (static_cast<uint64_t> (position));


        }

        //
        // Return a canonicalized path for a remote file.
        //
        
        std::string CSFTP::canonicalizePath(const std::string &pathName) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

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
        
        //
        // Return system information about mounted file system.
        // 

        void CSFTP::getFileSystemInfo(const File &fileHandle, FileSystemInfo &fileSystem) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            sftp_statvfs_t file{ sftp_fstatvfs(fileHandle.get())};

            if (file == NULL) {
                throw Exception(*this, __func__);
            }

            fileSystem.reset(file);

        }

        //
        // Return system information about mounted file system.
        // 
        
        void CSFTP::getFileSystemInfo(const std::string &fileSystemName, FileSystemInfo &fileSystem) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            sftp_statvfs_t file{ sftp_statvfs(m_sftp, fileSystemName.c_str())};

            if (file == NULL) {
                throw Exception(*this, __func__);
            }

            fileSystem.reset(file);

        }
        
        //
        // Change last access/modified time for a file.
        // 

        void CSFTP::changeFileModificationAccessTimes(const std::string &filePath, const Time *newTimeValues) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            if (sftp_utimes(m_sftp, filePath.c_str(), newTimeValues) < 0) {
                throw Exception(*this, __func__);
            }

        }

        //
        // Get server SFTP extension count
        //
        
        int CSFTP::getExtensionCount() {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            return (sftp_extensions_get_count(m_sftp));

        }

        //
        // Get a given server SFTP extension name
        //
        
        std::string CSFTP::getExtensionName(int index) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            std::string extentionName;
            const char *name;

            name = sftp_extensions_get_name(m_sftp, index);

            if (name == NULL) {
                throw Exception(*this, __func__);
            }

            extentionName.assign(&name[0], &name[std::strlen(name)]);

            return (extentionName);

        }

        //
        // Get a given server SFTP extension data
        //
        
        std::string CSFTP::getExtensionData(int index) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            std::string extensionData;
            const char *data;

            data = sftp_extensions_get_data(m_sftp, index);

            if (data == NULL) {
                throw Exception(*this, __func__);
            }

            extensionData.assign(&data[0], &data[std::strlen(data)]);

            return (extensionData);


        }

        //
        // Return true if a specified extension is supported.
        //
        
        bool CSFTP::extensionSupported(const std::string &name, const std::string &data) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            return (sftp_extension_supported(m_sftp, name.c_str(), data.c_str()));

        }

        //
        // Get SFTP server version.
        //
        
        int CSFTP::getServerVersion() const {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};

            return (sftp_server_version(m_sftp));

        }

        //
        // Get SFTP error code for last command.
        //
        
        int CSFTP::getErrorCode() const {

            int errorCode = sftp_get_error(m_sftp);

            return (errorCode);

        }

        //
        // Set/Get IO buffer parameters.
        //

        std::shared_ptr<char> CSFTP::getIoBuffer() {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};
            
            if (!m_ioBuffer) {
                setIoBufferSize(m_ioBufferSize);
            }
            
            return m_ioBuffer;
            
        }

        void CSFTP::setIoBufferSize(std::uint32_t ioBufferSize) {

            std::lock_guard<std::mutex>{ m_session.getSessionMutex()};
            
            m_ioBufferSize = ioBufferSize;
            m_ioBuffer.reset(new char[m_ioBufferSize]);
            
        }

        std::uint32_t CSFTP::getIoBufferSize() const {
            return m_ioBufferSize;
        }
        
        //
        // Get internal libssh ssh/sftp session data structure pointers.
        //
        
        sftp_session CSFTP::getSFTP() const {
            
            return m_sftp;
            
        }

        CSSHSession& CSFTP::getSession() const {
            
            return m_session;
            
        }

    } // namespace SSH
} // namespace Antik
