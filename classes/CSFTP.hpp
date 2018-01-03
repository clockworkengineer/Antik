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

            struct FileAttributes {
                std::string name;
                uint32_t flags;
                uint8_t type;
                uint64_t size;
                uint32_t permissions;
            };

            typedef sftp_file File;
            typedef sftp_dir Directory;
            typedef mode_t FilePermissions;
            typedef uid_t FileOwner;
            typedef gid_t FileGroup;

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
            size_t readFile(File fileDesc, void *readBuffer, size_t bytesToRead);
            size_t writeFile(File fileDesc, void *writeBuffer, size_t bytesToWrite);
            void closeFile(File fileDesc);

            Directory openDirectory(const std::string &directoryPath);
            bool readDirectory(Directory &directoryHandle, FileAttributes &fileAttributes);
            bool endOfDirectory(Directory &directoryHandle);
            void closeDirectory(Directory &directoryHandle);

            void changePermissions(const FileAttributes &fileAttributes, const FilePermissions &filePermissions);
            void changeOwnerGroup(const FileAttributes &fileAttributes, const FileOwner &owner, const FileGroup &group);
            void getFileAttributes(const File &fileDesc, FileAttributes &fileAttributes);
            void setFileAttributes(const File &fileDesc, const FileAttributes &fileAttributes);
            void getLinkAttributes(const std::string &linkPath, FileAttributes &fileAttributes);

            void createDirectory(const std::string &directoryPath, const FilePermissions &filePermissions);
            void removeDirectory(const std::string &directoryPath);

            void createLink(const std::string &targetPath, const std::string &linkPath);
            void removeLink(const std::string &filePath);

            std::string readLink(const std::string &linkPath);
            void renameFile(const std::string &sourceFile, const std::string &destinationFile);

            void rewindFile(File fileDesc);
            void seekFile(File fileDesc, uint32_t offset);
            void seekFile64(File fileDesc, uint64_t offset);
            uint32_t currentFilePostion(File fileDesc);
            uint64_t currentFilePostion64(File fileDesc);

            std::string canonicalizePath(const std::string &pathName);

            int getServerVersion()const;

            bool isADirectory(const FileAttributes &fileAttributes);
            bool isARegularFile(const FileAttributes &fileAttributes);
            bool isASymbolicLink(const FileAttributes &fileAttributes);

            int getErrorCode() const;

            sftp_session getSFTP() const;
            CSSHSession& getSession() const;

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

            void convertSshFileToFileAttributes(sftp_attributes file, FileAttributes &fileAttributes);
            void convertFileAttributesToSshFile(sftp_attributes file, const FileAttributes &fileAttributes);

            // =================
            // PRIVATE VARIABLES
            // =================

            CSSHSession &m_session;

            sftp_session m_sftp;


        };

    } // namespace FTP
} // namespace Antik

#endif /* CSFTP_HPP */

