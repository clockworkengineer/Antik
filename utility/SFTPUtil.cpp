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
// single raw SFTP commands. These functions are different from the FTP variants
// in that they use a FileMapper to convert paths and also deal in absolute paths
// and not the current working directory ( does not exist in SFTP).
// 
// Dependencies: 
// 
// C11++              : Use of C11++ features.
// Antik classes      : CSFTP
// Boost              : File system.
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

//
// Boost file system
//

#include <boost/filesystem.hpp>

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace SSH {

        // =======
        // IMPORTS
        // =======

        using namespace std;

        namespace fs = boost::filesystem;

        // ===============
        // LOCAL FUNCTIONS
        // ===============

        //
        // Return true if a given remote files exists.
        //
        
        static bool fileExists(CSFTP &sftpServer, const std::string &remotePath) {

            try {

                CSFTP::FileAttributes fileAttributes;
                sftpServer.getFileAttributes(remotePath, fileAttributes);
                return (true);

            } catch (CSFTP::Exception &e) {

                if (e.sftpGetCode() != SSH_FX_NO_SUCH_FILE) {
                    throw;
                }

            }

            return (false);

        }

        //
        // Break path into its component directories and create path structure on
        // remote FTP server.
        //
        
        static void makeRemotePath(CSFTP &sftpServer, const std::string &remotePath, const CSFTP::FilePermissions &permissions) {

            vector<string> pathComponents;
            fs::path currentPath{ string(1, kServerPathSep) };

            boost::split(pathComponents, remotePath, boost::is_any_of(string(1, kServerPathSep)));

            for (auto directory : pathComponents) {
                currentPath /= directory;
                if (!directory.empty()) {
                    if (!fileExists(sftpServer, currentPath.string())) {
                        sftpServer.createDirectory(currentPath.string(), permissions);
                    }

                }
            }


        }

        //
        // Return true if a given remote path is a directory.
        //
        
        static bool isDirectory(CSFTP &sftpServer, const std::string &remotePath) {

            CSFTP::FileAttributes fileAttributes;
            sftpServer.getFileAttributes(remotePath, fileAttributes);
            return (sftpServer.isADirectory(fileAttributes));

        }

        //
        // Return true if a given remote path is a regular file.
        //
        
        static bool isRegularFile(CSFTP &sftpServer, const std::string &remotePath) {

            CSFTP::FileAttributes fileAttributes;
            sftpServer.getFileAttributes(remotePath, fileAttributes);
            return (sftpServer.isARegularFile(fileAttributes));

        }

        // ================
        // PUBLIC FUNCTIONS
        // ================

        //
        // Upload a file from remote SFTP server assigning it the same permissions as the remote file.
        // SFTP does not directly support file upload/download so this function is not part of the
        // CSFTP class.
        //
        
        void getFile(CSFTP &sftp, const string &sourceFile, const string &destinationFile) {

            CSFTP::File remoteFile;
            ofstream localFile;

            try {

                CSFTP::FileAttributes fileAttributes;
                int bytesRead{0}, bytesWritten{0};

                remoteFile = sftp.openFile(sourceFile, O_RDONLY, 0);
                sftp.getFileAttributes(remoteFile, fileAttributes);

                localFile.open(destinationFile, ios_base::out | ios_base::binary | ios_base::trunc);
                if (!localFile) {
                    throw system_error(errno, std::system_category());
                }

                for (;;) {
                    bytesRead = sftp.readFile(remoteFile, sftp.getIoBuffer().get(), sftp.getIoBufferSize());
                    if (bytesRead == 0) {
                        break; // EOF
                    }
                    localFile.write(sftp.getIoBuffer().get(), bytesRead);
                    bytesWritten += bytesRead;
                    if (bytesWritten != localFile.tellp()) {
                        throw CSFTP::Exception(sftp, __func__);
                    }
                }

                sftp.closeFile(remoteFile);

                localFile.close();

                fs::permissions(destinationFile, static_cast<fs::perms> (fileAttributes->permissions));

            } catch (const CSFTP::Exception &e) {
                throw;
            } catch (const system_error &e) {
                throw;
            }

        }
        
        //
        // Download a file to remote SFTP server assigning it the same permissions as the local file. 
        // It will be created with the owner and group of the currently logged in SSH account.
        // SFTP does not directly support file upload/download so this function is not part of the
        // CSFTP class.
        //

        void putFile(CSFTP &sftp, const string &sourceFile, const string &destinationFile) {

            CSFTP::File remoteFile;
            ifstream localFile;

            try {

                fs::file_status fileStatus;
                int bytesWritten{0};

                localFile.open(sourceFile, ios_base::in | ios_base::binary);
                if (!localFile) {
                    throw system_error(errno, std::system_category());
                }

                fileStatus = fs::status(sourceFile);

                remoteFile = sftp.openFile(destinationFile, O_CREAT | O_WRONLY | O_TRUNC, fileStatus.permissions());

                for (;;) {

                    localFile.read(sftp.getIoBuffer().get(), sftp.getIoBufferSize());

                    if (localFile.gcount()) {
                        bytesWritten = sftp.writeFile(remoteFile, sftp.getIoBuffer().get(), localFile.gcount());
                        if ((bytesWritten < 0) || (bytesWritten != localFile.gcount())) {
                            throw CSFTP::Exception(sftp, __func__);
                        }
                    }

                    if (!localFile) break;

                }

                sftp.closeFile(remoteFile);

                localFile.close();

            } catch (const CSFTP::Exception &e) {
                throw;
            } catch (const system_error &e) {
                throw;
            }


        }
        
        //
        // Recursively parse a remote server path passed in and pass back a list of directories/files found.
        //
        
        void listRemoteRecursive(CSFTP &sftp, const string &directoryPath, vector<std::string> &remoteFileList) {

            try {

                CSFTP::Directory directoryHandle;
                CSFTP::FileAttributes fileAttributes;
                std::string filePath;

                directoryHandle = sftp.openDirectory(directoryPath);

                while (sftp.readDirectory(directoryHandle, fileAttributes)) {
                    if ((static_cast<string> (fileAttributes->name) != ".") && (static_cast<string> (fileAttributes->name) != "..")) {
                        std::string filePath{ directoryPath};
                        if (filePath.back() == kServerPathSep) filePath.pop_back();
                        filePath += string(1, kServerPathSep) + fileAttributes->name;
                        if (sftp.isADirectory(fileAttributes)) {
                            listRemoteRecursive(sftp, filePath, remoteFileList);
                        }
                        remoteFileList.push_back(filePath);
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

        //
        // Download all files passed in file list from server to the local directory passed in; recreating any server directory
        // structure in situ. If safe == true then the file is downloaded to a filename with a postfix then the file is renamed
        // to its correct value on success. Returns a list of successfully downloaded files and directories created in the local
        // directory.
        //
        
        FileList getFiles(CSFTP &sftpServer, FileMapper &fileMapper, const FileList &remoteFileList, FileCompletionFn completionFn, bool safe, char postFix) {

            FileList successList;

            try {

                for (auto remoteFile : remoteFileList) {

                    std::string localFilePath{ fileMapper.toLocal(remoteFile)};

                    if (isRegularFile(sftpServer, remoteFile)) {

                        string destinationFileName{ localFilePath + postFix};

                        if (!fs::exists(fs::path(localFilePath).parent_path())) {
                            fs::create_directories(fs::path(localFilePath).parent_path());
                        }

                        if (!safe) {
                            destinationFileName.pop_back();
                        }
                        getFile(sftpServer, remoteFile, destinationFileName);
                        if (safe) {
                            fs::rename(destinationFileName, localFilePath);
                        }


                    } else if (isDirectory(sftpServer, remoteFile)) {

                        if (!fs::exists(localFilePath)) {
                            fs::create_directories(localFilePath);
                        }

                    } else {
                        continue;
                    }

                    successList.push_back(localFilePath);
                    if (completionFn) {
                        completionFn(successList.back());
                    }

                }

                // On exception report and return with files that where successfully downloaded.

            } catch (const CSFTP::Exception &e) {
                std::cerr << e.getMessage() << std::endl;
            } catch (const boost::filesystem::filesystem_error & e) {
                std::cerr << string("BOOST file system exception occured: [") + e.what() + "]" << std::endl;
            } catch (const exception &e) {
                std::cerr << string("Standard exception occured: [") + e.what() + "]" << std::endl;
            }

            return (successList);

        }
        
        //
        // Take local directory, file list and upload all files to server;  recreating 
        // any local directory structure in situ on the server. Returns a list of successfully 
        // uploaded files and directories created.If safe == true then the file is uploaded to a 
        // filename with a postfix then the file is renamed to its correct value on success.
        // 

        FileList putFiles(CSFTP &sftpServer, FileMapper &fileMapper, const FileList &localFileList, FileCompletionFn completionFn, bool safe, char postFix) {

            FileList successList;

            try {

                CSFTP::FileAttributes remoteDirectoryAttributes;

                // Create any directories using root path permissions

                sftpServer.getFileAttributes(fileMapper.getRemoteDirectory(), remoteDirectoryAttributes);

                // Process file/directory list

                for (auto localFile : localFileList) {

                    if (fs::exists(localFile)) {

                        string remoteFilePath;
                        bool transferFile{ false};

                        // Create remote full remote path and set file to be transfered flag

                        if (fs::is_directory(localFile)) {
                            remoteFilePath = fileMapper.toRemote(localFile);
                        } else if (fs::is_regular_file(localFile)) {
                            remoteFilePath = fileMapper.toRemote(fs::path(localFile).parent_path().string());
                            transferFile = true;
                        } else {
                            continue; // Not valid for transfer NEXT FILE!
                        }

                        if (!fileExists(sftpServer, remoteFilePath)) {
                            makeRemotePath(sftpServer, remoteFilePath, remoteDirectoryAttributes->permissions);
                            successList.push_back(remoteFilePath);
                            if (completionFn) {
                                completionFn(successList.back());
                            }
                        }

                        remoteFilePath = fileMapper.toRemote(localFile);

                        // Transfer file

                        if (transferFile) {
                            string destinationFilePath{ remoteFilePath + postFix};
                            if (!safe) {
                                destinationFilePath.pop_back();
                            }
                            putFile(sftpServer, localFile, destinationFilePath);
                            if (safe) {
                                if (fileExists(sftpServer, remoteFilePath)) {
                                    sftpServer.removeLink(remoteFilePath);
                                }
                                sftpServer.renameFile(destinationFilePath, remoteFilePath);
                            }
                            successList.push_back(remoteFilePath);
                            if (completionFn) {
                                completionFn(successList.back());
                            }

                        }

                    }

                }

           // On exception report and return with files that where successfully uploaded.

            } catch (const CSFTP::Exception &e) {
                std::cerr << e.getMessage() << std::endl;
            } catch (const boost::filesystem::filesystem_error & e) {
                std::cerr << string("BOOST file system exception occured: [") + e.what() + "]" << std::endl;
            } catch (const exception &e) {
                std::cerr << string("Standard exception occured: [") + e.what() + "]" << std::endl;
            }

            return (successList);

        }

    } // namespace SSH

} // namespace Antik

