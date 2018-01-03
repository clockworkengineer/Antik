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

            struct Exception : public std::runtime_error {

                Exception(std::string const& message)
                : std::runtime_error("CSFTP Failure: " + message) {
                }

            };

   
            struct SFTPFileAttributes {
                std::string name;
                uint32_t flags;
                uint8_t type;
                uint64_t size;
                uint32_t permissions;
            };
            
            typedef sftp_file SFTPFile;
            typedef sftp_dir STFPDirectory;
            typedef mode_t SFTPFilePermissions;
            typedef uid_t SFTPFileOwner;
            typedef gid_t SFTPFileGroup;

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
  
            SFTPFile openFile(const std::string &fileName, int accessType, int mode);
            size_t readFile(SFTPFile fileDesc, void *readBuffer, size_t bytesToRead);
            size_t writeFile(SFTPFile fileDesc, void *writeBuffer, size_t bytesToWrite);
            int closeFile(SFTPFile fileDesc);
            
            STFPDirectory openDirectory(const std::string &directoryPath);
            bool readDirectory(STFPDirectory &directoryHandle, SFTPFileAttributes &fileAttributes);
            int endOfDirectory(STFPDirectory &directoryHandle);	
            int closeDirectory(STFPDirectory &directoryHandle);
            
            int changePermissions(const SFTPFileAttributes &fileAttributes, const SFTPFilePermissions &filePermissions);
            int changeOwnerGroup(const SFTPFileAttributes &fileAttributes, const SFTPFileOwner &owner, const SFTPFileGroup &group);
            void getFileAttributes(const SFTPFile &fileDesc, SFTPFileAttributes &fileAttributes);
            void getLinkAttributes(const std::string &linkPath, SFTPFileAttributes &fileAttributes);
            
            int createDirectory(const std::string &directoryPath, const SFTPFilePermissions &filePermissions);
            int removeDirectory(const std::string &directoryPath);
            
            std::string readLink(const std::string &linkPath);
            int renameFile(const std::string &sourceFile, const std::string &destinationFile);

            void rewindFile(SFTPFile fileDesc);
            int seekFile(SFTPFile fileDesc, uint32_t offset);
            int seekFile64(SFTPFile fileDesc, uint64_t offset);
            
            int serverVsersion();
                
            bool isADirectory (const SFTPFileAttributes &fileAttributes);
            bool isARegularFile (const SFTPFileAttributes &fileAttributes);
            bool isASymbolicLink(const SFTPFileAttributes &fileAttributes);
            
            int getErrorCode();
    
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
                   
            void convertToFileAttributes(sftp_attributes file, SFTPFileAttributes &fileAttributes);
            
            // =================
            // PRIVATE VARIABLES
            // =================

            CSSHSession &m_session;
                        
            sftp_session m_sftp;


        };

    } // namespace FTP
} // namespace Antik

#endif /* CSFTP_HPP */

