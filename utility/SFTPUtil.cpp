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
// SFTP utility definitions
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

            std::vector<std::string> pathComponents;
            fs::path currentPath{ std::string(1, kServerPathSep)};

            boost::split(pathComponents, remotePath, boost::is_any_of(std::string(1, kServerPathSep)));

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

        void getFile(CSFTP &sftpServer, const std::string &sourceFile, const std::string &destinationFile, FileCompletionFn completionFn) {

            CSFTP::File remoteFile;
            std::ofstream localFile;

            try {

                CSFTP::FileAttributes fileAttributes;
                int bytesRead{0}, bytesWritten{0};

                remoteFile = sftpServer.openFile(sourceFile, O_RDONLY, 0);
                sftpServer.getFileAttributes(remoteFile, fileAttributes);

                if (sftpServer.isARegularFile(fileAttributes)) {

                    if (!fs::exists(fs::path(destinationFile).parent_path())) {
                        fs::create_directories(fs::path(destinationFile).parent_path());
                    }

                    localFile.open(destinationFile, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
                    if (!localFile) {
                        throw std::system_error(errno, std::system_category());
                    }

                    for (;;) {
                        bytesRead = sftpServer.readFile(remoteFile, sftpServer.getIoBuffer().get(), sftpServer.getIoBufferSize());
                        if (bytesRead == 0) {
                            break; // EOF
                        }
                        localFile.write(sftpServer.getIoBuffer().get(), bytesRead);
                        bytesWritten += bytesRead;
                        if (bytesWritten != localFile.tellp()) {
                            throw CSFTP::Exception(sftpServer, __func__);
                        }
                    }


                    localFile.close();

                    fs::permissions(destinationFile, static_cast<fs::perms> (fileAttributes->permissions));

                    if (completionFn) {
                        completionFn(destinationFile);
                    }

                } else if (sftpServer.isADirectory(fileAttributes)) {
                    if (!fs::exists(fs::path(destinationFile))) {
                        fs::create_directories(fs::path(destinationFile));
                    }
                    if (completionFn) {
                        completionFn(destinationFile);
                    }
                }


                sftpServer.closeFile(remoteFile);

            } catch (const CSFTP::Exception &e) {
                throw;
            } catch (const std::system_error &e) {
                throw;
            }

        }

        //
        // Download a file to remote SFTP server assigning it the same permissions as the local file. 
        // It will be created with the owner and group of the currently logged in SSH account.
        // SFTP does not directly support file upload/download so this function is not part of the
        // CSFTP class.
        //

        void putFile(CSFTP &sftpServer, const std::string &sourceFile, const std::string &destinationFile, FileCompletionFn completionFn) {

            CSFTP::File remoteFile;
            std::ifstream localFile;

            try {

                std::string remoteFilePath;
                fs::file_status fileStatus;
                int bytesWritten{0};
                bool transferFile{ false};

                if (fs::is_directory(sourceFile)) {
                    remoteFilePath = destinationFile;
                    fileStatus = fs::status(sourceFile);
                 } else if (fs::is_regular_file(sourceFile)) {
                    remoteFilePath = fs::path(destinationFile).parent_path().string();
                    fileStatus = fs::status(fs::path(sourceFile).parent_path());
                    transferFile = true;
                 } else {
                    return; // Not valid for transfer NEXT FILE!
                }

                if (!fileExists(sftpServer, remoteFilePath)) {
                    makeRemotePath(sftpServer, remoteFilePath, static_cast<CSFTP::FilePermissions>(fileStatus.permissions()));
                    if (!transferFile && completionFn) {
                        completionFn(remoteFilePath);
                    }
                }

                if (transferFile) {

                    localFile.open(sourceFile, std::ios_base::in | std::ios_base::binary);
                    if (!localFile) {
                        throw std::system_error(errno, std::system_category());
                    }

                    fileStatus = fs::status(sourceFile);

                    remoteFile = sftpServer.openFile(destinationFile, O_CREAT | O_WRONLY | O_TRUNC, fileStatus.permissions());

                    for (;;) {

                        localFile.read(sftpServer.getIoBuffer().get(), sftpServer.getIoBufferSize());

                        if (localFile.gcount()) {
                            bytesWritten = sftpServer.writeFile(remoteFile, sftpServer.getIoBuffer().get(), localFile.gcount());
                            if ((bytesWritten < 0) || (bytesWritten != localFile.gcount())) {
                                throw CSFTP::Exception(sftpServer, __func__);
                            }
                        }

                        if (!localFile) break;

                    }

                    sftpServer.closeFile(remoteFile);

                    localFile.close();

                    if (completionFn) {
                        completionFn(destinationFile);
                    }

                }

            } catch (const CSFTP::Exception &e) {
                throw;
            } catch (const std::system_error &e) {
                throw;
            }


        }

        //
        // Recursively parse a remote server path passed in and pass back a list of directories/files found.
        // If a feedback function has been passed in then it is called for each file found.
        //

        void listRemoteRecursive(CSFTP &sftpServer, const std::string &directoryPath, FileList &remoteFileList, FileFeedBackFn remoteFileFeedbackFn) {

            try {

                CSFTP::Directory directoryHandle;
                CSFTP::FileAttributes fileAttributes;
                std::string filePath;

                directoryHandle = sftpServer.openDirectory(directoryPath);

                while (sftpServer.readDirectory(directoryHandle, fileAttributes)) {
                    if ((static_cast<std::string> (fileAttributes->name) != ".") && (static_cast<std::string> (fileAttributes->name) != "..")) {
                        std::string filePath{ directoryPath};
                        if (filePath.back() == kServerPathSep) filePath.pop_back();
                        filePath += std::string(1, kServerPathSep) + fileAttributes->name;
                        if (sftpServer.isADirectory(fileAttributes)) {
                            listRemoteRecursive(sftpServer, filePath, remoteFileList, remoteFileFeedbackFn);
                        }
                        remoteFileList.push_back(filePath);
                        if (remoteFileFeedbackFn) {
                            remoteFileFeedbackFn(remoteFileList.back());
                        }
                    }
                }

                if (!sftpServer.endOfDirectory(directoryHandle)) {
                    sftpServer.closeDirectory(directoryHandle);
                    throw CSFTP::Exception(sftpServer, __func__);
                }

                sftpServer.closeDirectory(directoryHandle);

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

                        std::string destinationFileName{ localFilePath + postFix};

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
                std::cerr << e.what() << std::endl;
            } catch (const std::exception &e) {
                std::cerr << e.what() << std::endl;
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

                        std::string remoteFilePath;
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
                            if (!transferFile && completionFn) {
                                completionFn(successList.back());
                            }
                        }

                        remoteFilePath = fileMapper.toRemote(localFile);

                        // Transfer file

                        if (transferFile) {
                            std::string destinationFilePath{ remoteFilePath + postFix};
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
                std::cerr << e.what() << std::endl;
            } catch (const std::exception &e) {
                std::cerr << e.what() << std::endl;
            }

            return (successList);

        }

    } // namespace SSH

} // namespace Antik

