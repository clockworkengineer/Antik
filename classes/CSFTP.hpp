/*
 * File:   CSFTP.hpp(Work In Progress)
 * 
 * Author: Robert Tizzard
 * 
 * Created on September 23, 2017, 2:33 PM
 *
 * Copyright 2017.
 *
 */

#ifndef CSFTP_HPP
#define CSFTP_HPP

//
// C++ STL
//

#include <stdexcept>
#include <vector>
#include <string>
#include <cstring>
#include <memory>
//#include <functional>

//
// Libssh
//

#include <fcntl.h>
#include <libssh/sftp.h>

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

        class CSFTP {
        public:

            // ==========================
            // PUBLIC TYPES AND CONSTANTS
            // ==========================

            //
            // Class exception
            //

            struct Exception {
                
                Exception(CSFTP &sftp, const std::string &functionName) : m_errorCode{sftp.getSession().getErrorCode()},
                m_errorMessage{ sftp.getSession().getError()}, m_sftpErrorCode{ sftp.getErrorCode()},
                m_functionName{functionName}
                {
                }

                int getCode() const {
                    return m_errorCode;
                }

                std::string getMessage() const {
                    return static_cast<std::string> ("CSFTP Failure: (") + m_functionName + ") [" + m_errorMessage + "]";
                }

                int sftpGetCode() const {
                    return m_sftpErrorCode;
                }

            private:
                std::string m_functionName;
                int m_errorCode{ SSH_OK};
                std::string m_errorMessage;
                int m_sftpErrorCode{SSH_FX_OK};

            };

            struct FileAttributesDeleter {

                void operator()(sftp_attributes fileAttributes) const {
                    sftp_attributes_free(fileAttributes);
                }
            };

            struct FileDeleter {

                void operator()(sftp_file file) const {
                    sftp_close(file);
                }
            };

            struct DirectoryDeleter {

                void operator()(sftp_dir directory) const {
                    sftp_closedir(directory);

                }
            };

            struct FileSystemInfoDeleter {

                void operator()(sftp_statvfs_t statvfs) const {
                     sftp_statvfs_free(statvfs);
                }
            };

            typedef std::unique_ptr<sftp_attributes_struct, FileAttributesDeleter> FileAttributes;
            typedef std::unique_ptr<sftp_file_struct, FileDeleter> File;
            typedef std::unique_ptr<sftp_dir_struct, DirectoryDeleter> Directory;
            typedef std::unique_ptr<sftp_statvfs_struct, FileSystemInfoDeleter> FileSystemInfo;
            
            typedef mode_t FilePermissions;
            typedef uid_t FileOwner;
            typedef gid_t FileGroup;
            typedef timeval Time;

            // ============
            // CONSTRUCTORS
            // ============

            //
            // Main constructor
            //

            explicit CSFTP(CSSHSession &session);

            // ==========
            // DESTRUCTOR
            // ==========

            virtual ~CSFTP();


            // ==============
            // PUBLIC METHODS
            // ==============

            void open();
            void close();

            File openFile(const std::string &fileName, int accessType, int mode);
            size_t readFile(const File &fileHandle, void *readBuffer, size_t bytesToRead);
            size_t writeFile(const File &fileHandle, void *writeBuffer, size_t bytesToWrite);
            void closeFile(File &fileHandle);

            Directory openDirectory(const std::string &directoryPath);
            bool readDirectory(const Directory &directoryHandle, FileAttributes &fileAttributes);
            bool endOfDirectory(const Directory &directoryHandle);
            void closeDirectory(Directory &directoryHandle);

            void changePermissions(const FileAttributes &fileAttributes, const FilePermissions &filePermissions);
            void changeOwnerGroup(const FileAttributes &fileAttributes, const FileOwner &owner, const FileGroup &group);
            void getFileAttributes(const File &fileHandle, FileAttributes &fileAttributes);
            void getFileAttributes(const std::string &filePath, FileAttributes &fileAttributes);
            void setFileAttributes(const std::string &filePath, const FileAttributes &fileAttributes);
            void getLinkAttributes(const std::string &linkPath, FileAttributes &fileAttributes);

            void createDirectory(const std::string &directoryPath, const FilePermissions &filePermissions);
            void removeDirectory(const std::string &directoryPath);

            void createLink(const std::string &targetPath, const std::string &linkPath);
            void removeLink(const std::string &filePath);

            std::string readLink(const std::string &linkPath);
            void renameFile(const std::string &sourceFile, const std::string &destinationFile);

            void rewindFile(const File &fileHandle);
            void seekFile(const File &fileHandle, uint32_t offset);
            void seekFile64(const File &fileHandle, uint64_t offset);
            uint32_t currentFilePostion(const File &fileHandle);
            uint64_t currentFilePostion64(const File &fileHandle);

            std::string canonicalizePath(const std::string &pathName);
            
            void getFileSystemInfo(const File &fileHandle, FileSystemInfo &fileSystem);
            void getFileSystemInfo(const std::string &fileSystemName, FileSystemInfo &fileSystem);
            
            void changeFileModificationAccessTimes(const std::string &filePath, const Time *newTimeValues);

            int getServerVersion()const;

            bool isADirectory(const FileAttributes &fileAttributes);
            bool isARegularFile(const FileAttributes &fileAttributes);
            bool isASymbolicLink(const FileAttributes &fileAttributes);
            
            int getExtensionCount();
            std::string getExtensionName(int index);
            std::string getExtensionData(int index);
            bool extensionSupported(const std::string &name, const std::string &data);

            int getErrorCode() const;

            sftp_session getSFTP() const;
            CSSHSession& getSession() const;
            std::shared_ptr<char> getIoBuffer() const;
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

            CSFTP(const CSFTP & orig) = delete;
            CSFTP(const CSFTP && orig) = delete;
            CSFTP& operator=(CSFTP other) = delete;

            // ===============
            // PRIVATE METHODS
            // ===============

            // =================
            // PRIVATE VARIABLES
            // =================

            CSSHSession &m_session;

            sftp_session m_sftp;
            
            std::shared_ptr<char> m_ioBuffer { nullptr };  // IO buffer
            std::uint32_t m_ioBufferSize     { 64*1024 };  // IO buffer size

        };

    } // namespace FTP
} // namespace Antik

#endif /* CSFTP_HPP */

