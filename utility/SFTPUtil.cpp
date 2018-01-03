#include "HOST.hpp"
/*
 * File:   SFTPUtil.cpp(Work In Progress)
 * 
 * Author: Robert Tizzard
 *
 * Created on October 10, 2017, 2:34 PM
 * 
 * Copyright 2017.
 * 
 */

//
// Module: SFTPUtil
//
// Description: SFTP utility functions for the Antik class CSFTP.
// Perform selective and  more powerful operations not available directly through
// single raw SFTP commands. Any generated exceptions are not handled but passed back
// up the call stack.
// 
// Dependencies: 
// 
// C11++              : Use of C11++ features.
// Boost              : File system, string, iterators.
//

// =============
// INCLUDE FILES
// =============

//
// C++ STL
//

#include <iostream>
#include <system_error>

//
// FTP utility definitions
//

#include "SFTPUtil.hpp"

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace SSH {

        // =======
        // IMPORTS
        // =======

        using namespace std;

        // ===============
        // LOCAL FUNCTIONS
        // ===============


        // ================
        // PUBLIC FUNCTIONS
        // ================

        // Good chunk size
#define MAX_XFER_BUF_SIZE 16384

        void sftpGetFile(CSFTP &sftp, const string &srcFile, const string &dstFile) {

            CSFTP::File remoteFile;
            ofstream localFile;
            char buffer[MAX_XFER_BUF_SIZE];
            int bytesRead{0}, bytesWritten {0}, returnCode{0};

            try {

                remoteFile = sftp.openFile(srcFile, O_RDONLY, 0);

                localFile.open(dstFile, ios_base::out | ios_base::binary | ios_base::trunc);
                if (!localFile) {
                    sftp.closeFile(remoteFile);
                    throw system_error(errno,std::system_category());
                }

                for (;;) {
                    bytesRead = sftp.readFile(remoteFile, buffer, sizeof (buffer));
                    if (bytesRead == 0) {
                        break; // EOF
                    }
                    localFile.write(buffer, bytesRead);
                    bytesWritten += bytesRead;
                    if (bytesWritten != localFile.tellp()) {
                        sftp.closeFile(remoteFile);
                        throw system_error(errno, std::system_category());
                    }
                }

                sftp.closeFile(remoteFile);
                
                localFile.close();

            } catch (const CSFTP::Exception &e) {
               if (localFile.is_open()) {
                    localFile.close();
                }
                throw;
            } catch (const system_error &e) {
               if (localFile.is_open()) {
                    localFile.close();
                }
                throw;
            }

        }

        void sftpPutFile(CSFTP &sftp, const string &srcFile, const string &dstFile) {

            CSFTP::File remoteFile;
            ifstream localFile;
            char buffer[MAX_XFER_BUF_SIZE];
            int bytesWritten{0}, returnCode{0};

            try {

                localFile.open(srcFile, ios_base::in | ios_base::binary);
                if (!localFile) {
                    throw system_error(errno, std::system_category());
                }

                remoteFile = sftp.openFile(dstFile, O_CREAT | O_WRONLY | O_TRUNC, 0);

                for (;;) {

                    localFile.read(buffer, sizeof (buffer));

                    if (localFile.gcount()) {
                        bytesWritten = sftp.writeFile(remoteFile, buffer, localFile.gcount());
                        if (bytesWritten < 0) {
                            sftp.closeFile(remoteFile);
                            throw system_error(errno, std::system_category());
                        }
                    }

                    if (bytesWritten != localFile.gcount()) {
                        sftp.closeFile(remoteFile);
                        throw system_error(errno, std::system_category());
                    }

                    if (!localFile) break;

                }

                sftp.closeFile(remoteFile);
                
                localFile.close();

            } catch (const CSFTP::Exception &e) {
                if (localFile.is_open()) {
                    localFile.close();
                }
                throw;
            } catch (const system_error &e) {
                if (localFile.is_open()) {
                    localFile.close();
                }
                throw;
            }


        }

        int sftpGetDirectoryContents(CSFTP &sftp, const string &directoryPath, vector<CSFTP::FileAttributes> &directoryContents, bool recursive) {

            CSFTP::Directory directoryHandle;
            CSFTP::FileAttributes fileAttributes;
            int returnCode;

            try {

                directoryHandle = sftp.openDirectory(directoryPath);

                while (sftp.readDirectory(directoryHandle, fileAttributes)) {
                    if ((fileAttributes.name != ".") && (fileAttributes.name != "..")) {
                        fileAttributes.name = directoryPath + "/" + fileAttributes.name;
                        if (sftp.isADirectory(fileAttributes) && recursive) {
                            sftpGetDirectoryContents(sftp, fileAttributes.name, directoryContents, recursive);
                        }
                        directoryContents.push_back(fileAttributes);
                    }
                }

                if (!sftp.endOfDirectory(directoryHandle)) {
                    sftp.closeDirectory(directoryHandle);
                    throw CSFTP::Exception(sftp, __func__);
                }

                sftp.closeDirectory(directoryHandle);

            } catch (const CSFTP::Exception &e) {
                throw;
            }


        }

    } // namespace SSH

} // namespace Antik

