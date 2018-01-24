#include "HOST.hpp"
/*
 * File:   SCPUtil.cpp(Work In Progress)
 * 
 * Author: Robert Tizzard
 *
 * Created on October 10, 2017, 2:34 PM
 * 
 * Copyright 2017.
 * 
 */

//
// Module: SCPUtil
//
// Description: SCP utility functions for the Antik class CSCP. Perform single/recursive
// file transfers to/from a remote server. Any file paths are converted in situ on either
// the remote server or local host.
// 
// Dependencies: 
// 
// C11++              : Use of C11++ features.
// Antik classes      : CSCP
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
// SCP utility definitions
//

#include "SCPUtil.hpp"

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
        // Break path into its component directories and create path structure on
        // remote SCP server.
        //

        static void makeRemotePath(CSCP &scpServer, const string &remotePath, const CSCP::FilePermissions permissions) {

            vector<string> pathComponents;
            fs::path currentPath{ string(1, kServerPathSep)};

            boost::split(pathComponents, remotePath, boost::is_any_of(string(1, kServerPathSep)));

            for (auto directory : pathComponents) {
                if (!directory.empty()) {
                    scpServer.pushDirectory(directory, permissions);
                }
            }

        }

        //
        // Download the currently requested file from SCP server and write to local directory.
        //

        static void downloadFile(CSCP &scpServer, const std::string &destinationFile) {

            ofstream localFile;
            CSCP::FilePermissions filePermissions;
            char *ioBuffer = scpServer.getIoBuffer().get();
            size_t ioBufferSize = scpServer.getIoBufferSize();
            int bytesRead{0};
            size_t fileSize{0};

            filePermissions = scpServer.requestFilePermissions();
            fileSize = scpServer.requestFileSize();

            scpServer.acceptRequest();

            if (!fs::exists(fs::path(destinationFile).parent_path())) {
                fs::create_directories(fs::path(destinationFile).parent_path());
            }

            localFile.open(destinationFile, ios_base::out | ios_base::binary | ios_base::trunc);
            if (!localFile) {
                throw system_error(errno, system_category());
            }

            for (;;) {
                bytesRead = scpServer.read(ioBuffer, ioBufferSize);
                localFile.write(ioBuffer, ioBufferSize);
                fileSize -= bytesRead;
                if (fileSize == 0) {
                    break;
                }
            }

            localFile.close();

            fs::permissions(destinationFile, static_cast<fs::perms> (filePermissions));

        }

        // ================
        // PUBLIC FUNCTIONS
        // ================

        //
        // Upload a file from remote SCP server assigning it the same permissions as the remote file.
        // SCP does not directly support file upload/download so this function is not part of the
        // CSCP class.
        //

        void getFile(CSSHSession &sshSession, const string &sourceFile, const string &destinationFile) {


            CSCP scpServer{ sshSession, SSH_SCP_READ, sourceFile};
            int pullStatus{0};

            scpServer.open();

            if ((pullStatus = scpServer.pullRequest()) != SSH_SCP_REQUEST_NEWFILE) {
                if (pullStatus == SSH_SCP_REQUEST_WARNING) {
                    throw CSCP::Exception(scpServer, __func__);
                }

            }

            downloadFile(scpServer, destinationFile);

            scpServer.close();

        }

        //
        // Download a file to remote SCP server assigning it the same permissions as the local file. 
        // It will be created with the owner and group of the currently logged in SSH account.
        // SCP does not directly support file upload/download so this function is not part of the
        // CSCP class.
        //

        void putFile(CSSHSession &sshSession, const string &sourceFile, const string &destinationFile) {


            ifstream localFile;
            fs::file_status fileStatus;

            localFile.open(sourceFile, ios_base::in | ios_base::binary);
            if (!localFile) {
                throw system_error(errno, system_category());
            }

            fileStatus = fs::status(fs::path(sourceFile).parent_path().string());

            CSCP scpServer{ sshSession, SSH_SCP_WRITE | SSH_SCP_RECURSIVE, std::string(1, kServerPathSep)};

            scpServer.open();

            if (fs::is_directory(sourceFile)) {

                makeRemotePath(scpServer, fs::path(destinationFile).string(), fileStatus.permissions());

            } else if (fs::is_regular_file(sourceFile)) {

                makeRemotePath(scpServer, fs::path(destinationFile).parent_path().string(), fileStatus.permissions());

                fileStatus = fs::status(fs::path(sourceFile));

                scpServer.pushFile(fs::path(destinationFile).filename().string(), fs::file_size(sourceFile), fileStatus.permissions());

                for (;;) {

                    localFile.read(scpServer.getIoBuffer().get(), scpServer.getIoBufferSize());

                    if (localFile.gcount()) {
                        scpServer.write(scpServer.getIoBuffer().get(), localFile.gcount());
                    }

                    if (!localFile) break;

                }

                localFile.close();

            }

            scpServer.close();


        }

        //
        // Download all files in a remote directory structure (recursive); recreating 
        // any server directory structure in situ. Returns a list of successfully downloaded files and 
        // directories created in the local directory.
        //

        FileList getFiles(CSSHSession &sshSession, FileMapper &fileMapper, FileCompletionFn completionFn) {

            FileList successList;

            try {

                CSCP scpServer{ sshSession, SSH_SCP_READ | SSH_SCP_RECURSIVE, fileMapper.getRemoteDirectory()};

                int pullStatus;
                fs::path currentPath{ fileMapper.getRemoteDirectory()};
                fs::path localFilePath;

                scpServer.open();

                scpServer.pullRequest();
                scpServer.acceptRequest();

                while ((pullStatus = scpServer.pullRequest()) != SSH_SCP_REQUEST_EOF) {

                    switch (pullStatus) {

                        case SSH_SCP_REQUEST_NEWFILE:

                            localFilePath = currentPath.string();
                            localFilePath /= scpServer.requestFileName();
                            localFilePath = fileMapper.toLocal(localFilePath.string());

                            downloadFile(scpServer, localFilePath.string());

                            successList.push_back(localFilePath.string());

                            if (completionFn) {
                                completionFn(successList.back());
                            }

                            break;

                        case SSH_SCP_REQUEST_NEWDIR:
                            currentPath /= scpServer.requestFileName();
                            if (!fs::exists(fileMapper.toLocal(currentPath.string()))) {
                                fs::create_directories(fileMapper.toLocal(currentPath.string()));
                                fs::permissions(fileMapper.toLocal(currentPath.string()),
                                        static_cast<fs::perms> (scpServer.requestFilePermissions()));
                                successList.push_back(fileMapper.toLocal(currentPath.string()));
                                if (completionFn) {
                                    completionFn(successList.back());
                                }
                            }
                            scpServer.acceptRequest();
                            break;

                        case SSH_SCP_REQUEST_ENDDIR:
                            currentPath = currentPath.parent_path();
                            break;

                        case SSH_SCP_REQUEST_WARNING:
                            throw CSCP::Exception(scpServer, __func__);
                            break;

                    }

                }

                scpServer.close();

                // On exception report and return with files that where successfully downloaded.

            } catch (const CSCP::Exception &e) {
                cerr << e.getMessage() << endl;
            } catch (const boost::filesystem::filesystem_error & e) {
                cerr << string("BOOST file system exception occured: [") + e.what() + "]" << endl;
            } catch (const exception &e) {
                cerr << string("Standard exception occured: [") + e.what() + "]" << endl;
            }

            return (successList);

        }

        //
        // Take local directory and upload all its files to server;  recreating 
        // any local directory structure in situ on the server. Returns a list of successfully 
        // uploaded files and directories created.
        // 

        FileList putFiles(CSSHSession &sshSession, FileMapper &fileMapper, FileCompletionFn completionFn) {

            FileList successList;

            try {

                FileList localFileList;

                listLocalRecursive(fileMapper.getLocalDirectory(), localFileList);

                for (auto localFile : localFileList) {
                    putFile(sshSession, localFile, fileMapper.toRemote(localFile));
                    successList.push_back(fileMapper.toRemote(localFile));
                    if (completionFn) {
                        completionFn(successList.back());
                    }
                }

                // On exception report and return with files that where successfully uploaded.

            } catch (const CSCP::Exception &e) {
                cerr << e.getMessage() << endl;
            } catch (const boost::filesystem::filesystem_error & e) {
                cerr << string("BOOST file system exception occured: [") + e.what() + "]" << endl;
            } catch (const exception &e) {
                cerr << string("Standard exception occured: [") + e.what() + "]" << endl;
            }

            return (successList);

        }

    } // namespace SSH

} // namespace Antik

