#include "HOST.hpp"
/*
 * File:   FTPUtil.cpp
 * 
 * Author: Robert Tizzard
 *
 * Created on October 10, 2017, 2:34 PM
 * 
 * Copyright 2017.
 * 
 */

//
// Module: FTPUtil
//
// Description: FTP utility functions for the Antik class CFTP.
// Perform selective and  more powerful operations not available directly through
// single raw FTP commands. Any generated exceptions are not handled but passed back
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

//
// FTP utility definitions
//

#include "FTPUtil.hpp"

//
// Boost file system, string and iterators
//

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/range/iterator_range.hpp>

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace FTP {

        // =======
        // IMPORTS
        // =======

        using namespace std;

        namespace fs = boost::filesystem;

        // ===============
        // LOCAL FUNCTIONS
        // ===============

        //
        // Construct a remote path name from three passed in parameters and just for simplicity replace all
        // double path separators with just one if any have been generated.
        //

        static inline string constructRemotePathName(const string &currentWorkingDirectory, const string &remotePath, const string &remoteFileName) {

            string remoteDirectoryPath{ currentWorkingDirectory + kServerPathSep + remotePath + kServerPathSep + remoteFileName};
            string::size_type i{0};

            while ((i = remoteDirectoryPath.find(string(2, kServerPathSep))) != string::npos) {
                remoteDirectoryPath.erase(remoteDirectoryPath.begin() + i);
            }
            if (remoteDirectoryPath.back() == kServerPathSep) remoteDirectoryPath.pop_back();

            return (remoteDirectoryPath);

        }

        //
        // Construct a remote path name from value returned from server list command. This may or may no have
        // the absolute path name on front. If it does return it otherwise construct one that does.
        //

        static inline string constructRemotePathName(const string &remotePath, const string &remoteFileName) {

            if (remoteFileName.find(remotePath) == 0) {
                return (remoteFileName);
            } else {
                return (constructRemotePathName("", remotePath, remoteFileName));
            }

        }

        // ================
        // PUBLIC FUNCTIONS
        // ================

        //
        // Recursively parse a local directory and produce a list of files.
        //

        void listLocalRecursive(const string &localDirectory, FileList &fileList) {

            for (auto directoryEntry : fs::recursive_directory_iterator{localDirectory}) {
                fileList.push_back(directoryEntry.path().string());
            }

        }

        //
        // Recursively parse a remote server path passed in and pass back a list of directories/files found.
        // For servers that do not return a fully qualified path name create one.
        //

        void listRemoteRecursive(CFTP &ftpServer, const string &remoteDirectory, FileList &fileList) {

            FileList serverFileList;
            string currentWorkingDirectory;

            // Save current working directory

            ftpServer.getCurrentWoringDirectory(currentWorkingDirectory);

            ftpServer.changeWorkingDirectory(remoteDirectory);
            if (ftpServer.listFiles("", serverFileList) == 226) {
                for (auto file : serverFileList) {
                    string fullFilePath{ constructRemotePathName(remoteDirectory, file)};
                    fileList.push_back(fullFilePath);
                    if (ftpServer.isDirectory(fullFilePath)) {
                        listRemoteRecursive(ftpServer, fullFilePath, fileList);
                    }

                }
            }

            // Restore saved current working directory

            ftpServer.changeWorkingDirectory(currentWorkingDirectory);

        }

        //
        // Break path into its component directories and create path structure on
        // remote FTP server. Note: This done relative to the server currently set
        // working directory and no errors are reported. To test for success/failure
        // use CFTP::fileExists() after call to see if it has been created.
        //

        void makeRemotePath(CFTP &ftpServer, const string &remotePath, bool saveCWD) {

            vector<string> pathComponents;
            string currentWorkingDirectory;

            // Save current working directory

            if (saveCWD) {
                ftpServer.getCurrentWoringDirectory(currentWorkingDirectory);
            }

            boost::split(pathComponents, remotePath, boost::is_any_of(string(1, kServerPathSep)));

            ftpServer.getCurrentWoringDirectory(currentWorkingDirectory);

            for (auto directory : pathComponents) {
                if (!directory.empty()) {
                    ftpServer.makeDirectory(directory);
                    ftpServer.changeWorkingDirectory(directory);
                }
            }

            // Restore saved current working directory

            if (saveCWD) {
                ftpServer.changeWorkingDirectory(currentWorkingDirectory);
            }

        }

        //
        // Download all files passed in file list from server to the local directory passed in; recreating any server directory
        // structure in situ. If safe == true then the file is downloaded to a filename with a postfix then the file is renamed
        // to its correct value on success. Returns a list of successfully downloaded files and directories created in the local
        // directory. The local file name is calculated by removing the current working directory from each file in the list and
        // appending it to the passed in local directory to get the full local file path.
        //

        FileList getFiles(CFTP &ftpServer, const string &localDirectory, const FileList &fileList, FileCompletionFn completionFn, bool safe, char postFix) {

            FileList successList;
            string currentWorkingDirectory;

            // Save current working directory

            ftpServer.getCurrentWoringDirectory(currentWorkingDirectory);

            try {

                for (auto file : fileList) {

                    fs::path destination{ localDirectory};

                    destination /= file.substr(currentWorkingDirectory.size());
                    destination = destination.normalize();
                    if (!fs::exists(destination.parent_path())) {
                        fs::create_directories(destination.parent_path());
                    }

                    if (!ftpServer.isDirectory(file)) {

                        string destinationFileName{ destination.string() + postFix};
                        if (!safe) {
                            destinationFileName.pop_back();
                        }
                        if (ftpServer.getFile(file, destinationFileName) == 226) {
                            if (safe) {
                                fs::rename(destinationFileName, destination.string());
                            }
                            successList.push_back(destination.string());
                            if (completionFn) {
                                completionFn(successList.back());
                            }
                        }

                    } else {

                        if (!fs::exists(destination)) {
                            fs::create_directories(destination);
                        }
                        successList.push_back(destination.string());
                        if (completionFn) {
                            completionFn(successList.back());
                        }

                    }

                }

                // Restore saved current working directory

                ftpServer.changeWorkingDirectory(currentWorkingDirectory);

            // On exception report and return with files that where successfully downloaded.

            } catch (const CFTP::Exception &e) {
                std::cerr << e.what();
            } catch (const boost::filesystem::filesystem_error & e) {
                std::cerr << string("BOOST file system exception occured: [") + e.what() + "]";
            } catch (const exception &e) {
                std::cerr << string("Standard exception occured: [") + e.what() + "]";
            }

            return (successList);

        }

        //
        // Take local directory, file list and upload all files to server;  recreating 
        // any local directory structure in situ on the server. Returns a list of successfully 
        // uploaded files and directories created.If safe == true then the file is uploaded to a 
        // filename with a postfix then the file is renamed to its correct value on success.
        // All files/directories are placed/created relative to the server current working directory.
        // 

        FileList putFiles(CFTP &ftpServer, const string &localDirectory, const FileList &fileList, FileCompletionFn completionFn, bool safe, char postFix) {

            FileList successList;
            size_t localPathLength{ 0};
            string currentWorkingDirectory;

            // Determine local path length for creating remote paths.

            localPathLength = localDirectory.size();
            if (localDirectory.back() != kServerPathSep) localPathLength++;

            // Save current working directory

            ftpServer.getCurrentWoringDirectory(currentWorkingDirectory);

            try {

                // Process file/directory list

                for (auto file : fileList) {

                    fs::path filePath{ file};

                    if (fs::exists(filePath)) {

                        string remoteDirectory;
                        bool transferFile{ false};

                        // Create remote path and set file to be transfered flag

                        if (fs::is_directory(filePath)) {
                            remoteDirectory = filePath.string();
                        } else if (fs::is_regular_file(filePath)) {
                            remoteDirectory = filePath.parent_path().string() + kServerPathSep;
                            transferFile = true;
                        } else {
                            continue; // Not valid for transfer NEXT FILE!
                        }

                        remoteDirectory = remoteDirectory.substr(localPathLength);

                        // Set current working directory and create any remote path needed

                        ftpServer.changeWorkingDirectory(currentWorkingDirectory);

                        if (!remoteDirectory.empty()) {
                            if (!ftpServer.isDirectory(remoteDirectory)) {
                                makeRemotePath(ftpServer, remoteDirectory, false);
                                successList.push_back(constructRemotePathName(currentWorkingDirectory, remoteDirectory, ""));
                                if (completionFn) {
                                    completionFn(successList.back());
                                }
                            } else {
                                ftpServer.changeWorkingDirectory(remoteDirectory);
                            }
                        }

                        // Transfer file

                        if (transferFile) {
                            string destinationFileName{ filePath.filename().string() + postFix};
                            if (!safe) {
                                destinationFileName.pop_back();
                            }
                            if (ftpServer.putFile(destinationFileName, filePath.string()) == 226) {
                                if (safe) {
                                    ftpServer.renameFile(destinationFileName, filePath.filename().string());
                                }
                                successList.push_back(constructRemotePathName(currentWorkingDirectory, remoteDirectory, filePath.filename().string()));
                                if (completionFn) {
                                    completionFn(successList.back());
                                }
                            }
                        }

                    }

                }

                // Restore saved current working directory

                ftpServer.changeWorkingDirectory(currentWorkingDirectory);


            // On exception report and return with files that where successfully uploaded.

            } catch (const CFTP::Exception &e) {
                std::cerr << e.what();
            } catch (const boost::filesystem::filesystem_error & e) {
                std::cerr << string("BOOST file system exception occured: [") + e.what() + "]";
            } catch (const exception &e) {
                std::cerr << string("Standard exception occured: [") + e.what() + "]";
            }

            return (successList);

        }

    } // namespace FTP

} // namespace Antik

