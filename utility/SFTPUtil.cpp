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
// FTP utility definitions
//

#include "SFTPUtil.hpp"
#include <iostream>

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

        int sftpGetFile(CSFTP &sftp, const string &srcFile, const string &dstFile) {

            CSFTP::SFTPFile remoteFile;
            ofstream localFile;
            char buffer[MAX_XFER_BUF_SIZE];
            int bytesRead, bytesWritten, returnCode;

            remoteFile = sftp.openFile(srcFile, O_RDONLY, 0);
            if (remoteFile == NULL) {
                cerr << "Can't open file for reading: " + sftp.getSession().getError() << endl;
                return SSH_ERROR;
            }

            localFile.open(dstFile, ios_base::out | ios_base::binary | ios_base::trunc);
            if (!localFile) {
                cerr << "Can't open file for writing: " << strerror(errno) << endl;
                sftp.closeFile(remoteFile);
                return SSH_ERROR;
            }

            for (;;) {
                bytesRead = sftp.readFile(remoteFile, buffer, sizeof (buffer));
                if (bytesRead == 0) {
                    break; // EOF
                } else if (bytesRead < 0) {
                    cerr << "Error while reading file: " + sftp.getSession().getError() << endl;
                    sftp.closeFile(remoteFile);
                    return SSH_ERROR;
                }
                localFile.write(buffer, bytesRead);
                bytesWritten += bytesRead;
                if (bytesWritten != localFile.tellp()) {
                    cerr << "Error writing: " << strerror(errno) << endl;
                    sftp.closeFile(remoteFile);
                    return SSH_ERROR;
                }
            }

            returnCode = sftp.closeFile(remoteFile);
            if (returnCode != SSH_OK) {
                cerr << "Can't close the read file: " + sftp.getSession().getError() << endl;
                return returnCode;
            }

            return SSH_OK;

        }

        int sftpPutFile(CSFTP &sftp, const string &srcFile, const string &dstFile) {

            CSFTP::SFTPFile remoteFile;
            ifstream localFile;
            char buffer[MAX_XFER_BUF_SIZE];
            int bytesWritten{0}, returnCode{0};

            localFile.open(srcFile, ios_base::in | ios_base::binary);
            if (!localFile) {
                cerr << "Can't open file for writing: " << strerror(errno) << endl;
                return SSH_ERROR;
            }

            remoteFile = sftp.openFile(dstFile, O_CREAT | O_WRONLY | O_TRUNC, 0);
            if (remoteFile == NULL) {
                localFile.close();
                cerr << "Can't open file for reading: " + sftp.getSession().getError() << endl;
                return SSH_ERROR;
            }

            for (;;) {

                localFile.read(buffer, sizeof (buffer));

                if (localFile.gcount()) {
                    bytesWritten = sftp.writeFile(remoteFile, buffer, localFile.gcount());
                    if (bytesWritten < 0) {
                        cerr << "Error while writing file: " + sftp.getSession().getError() << endl;
                        sftp.closeFile(remoteFile);
                        return SSH_ERROR;
                    }
                }

                if (bytesWritten != localFile.gcount()) {
                    cerr << "Error writing: " << strerror(errno) << endl;
                    sftp.closeFile(remoteFile);
                    return SSH_ERROR;
                }

                if (!localFile) break;

            }

            returnCode = sftp.closeFile(remoteFile);
            if (returnCode != SSH_OK) {
                cerr << "Can't close the read file: " + sftp.getSession().getError() << endl;
                return returnCode;
            }

            return SSH_OK;

        }

        int sftpGetDirectoryContents(CSFTP &sftp, const string &directoryPath, vector<CSFTP::SFTPFileAttributes> &directoryContents, bool recursive) {

            CSFTP::STFPDirectory directoryHandle;
            CSFTP::SFTPFileAttributes fileAttributes;
            int returnCode;

            directoryHandle = sftp.openDirectory(directoryPath);

            if (!directoryHandle) {
                cerr << "Directory not opened: " + sftp.getSession().getError() << endl;
                return SSH_ERROR;
            }

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
                cerr << "Can't list directory: " + sftp.getSession().getError() << endl;
                sftp.closeDirectory(directoryHandle);
                return SSH_ERROR;
            }

            returnCode = sftp.closeDirectory(directoryHandle);
            if (returnCode != SSH_OK) {
                cerr << "Can't close directory: " + sftp.getSession().getError() << endl;
                return returnCode;
            }

        }

    } // namespace SSH

} // namespace Antik

