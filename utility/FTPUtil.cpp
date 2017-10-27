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

        // ================
        // PUBLIC FUNCTIONS
        // ================

        //
        // Recursively parse a local directory and produce a list of files.
        //

        void listLocalRecursive(const string &localDirectory, vector<string> &fileList) {

            for (auto directoryEntry : fs::recursive_directory_iterator{localDirectory}) {
                fileList.push_back(directoryEntry.path().string());
            }

        }
        
        //
        // Recursively parse a remote server path passed in and pass back a list of directories/files found.
        //
        
        void listRemoteRecursive(CFTP &ftpServer, const string &remoteDirectory, vector<string> &fileList) {

            vector<string> serverFileList;
            uint16_t statusCode;

            statusCode = ftpServer.listFiles(remoteDirectory, serverFileList);
            if (statusCode == 226) {
                for (auto file : serverFileList) {
                    if (remoteDirectory != file) {
                        listRemoteRecursive(ftpServer, file, fileList);
                        fileList.push_back(file);
                    }
                }
            }
        }
        
        //
        // Download all files passed in file list from server to the local path passed in; recreating any server directory
        // structure in situ. If safe == true then the file is downloaded to a filename with a postfix then the file is renamed
        // to its correct value on success. Returns a list of successfully downloaded files and directories created.
        //

        vector<string> getFiles(CFTP &ftpServer, const string &localDirectory, const vector<string> &fileList, bool safe, char postFix) {

            uint16_t statusCode;
            vector<string> successList;

            if (!safe) {
                postFix = '\0';
            }

            for (auto file : fileList) {
                fs::path destination{ localDirectory};
                destination /= file;
                if (!fs::exists(destination.parent_path())) {
                    fs::create_directories(destination.parent_path());
                }
                if (!ftpServer.isDirectory(file)) {
                    statusCode = ftpServer.getFile(file, destination.string() + postFix);
                    if (statusCode == 226) {
                        if (safe) fs::rename(destination.string() + postFix, destination.string());
                        successList.push_back(file); // File get ok so add to success
                    }
                } else {
                    successList.push_back(file); // File directory created so add to success
                }

            }

            return (successList);

        }
        
        //
        // Take local folder, file list and upload all files to server;  recreating 
        // any local directory structure in situ on the server. Returns a list of successfully 
        // uploaded files and directories created.If safe == true then the file is uploaded to a 
        // filename with a postfix then the file is renamed to its correct value on success. 
        // Returns a list of successfully downloaded files and directories created.
        //
        
        vector<string> putFiles(CFTP &ftpServer, const string &localFolder,  const vector<string> &fileList, bool safe, char postFix) {

            vector<string> successList;
            fs::path localPath{ localFolder};

            if (!safe) {
                postFix = '\0';
            }
            
            for (auto file : fileList) {
                
                fs::path filePath { file };

                if (fs::exists(filePath)) {

                    string remoteDirectory;
                    bool putFile = false;

                    if (fs::is_directory(filePath)) {
                        remoteDirectory = filePath.string().substr(localPath.parent_path().size());
                        putFile = false;
                    } else if (fs::is_regular_file(filePath)) {
                        remoteDirectory = filePath.parent_path().string().substr(localPath.parent_path().size());
                        putFile = true;
                    }

                    if (!remoteDirectory.empty()) {

                        if (!ftpServer.isDirectory(remoteDirectory)) {
                            vector<string> directories;
                            boost::split(directories, remoteDirectory, boost::is_any_of("/"));
                            ftpServer.changeWorkingDirectory("/");
                            for (auto files : directories) {
                                ftpServer.makeDirectory(files);
                                ftpServer.changeWorkingDirectory(files);
                            }
                            successList.push_back(remoteDirectory);
                        } else {
                            ftpServer.changeWorkingDirectory(remoteDirectory);
                        }
                        if (putFile) {
                            if (ftpServer.putFile(filePath.filename().string()+postFix, filePath.string()) == 226) {
                                if (safe) ftpServer.renameFile(filePath.filename().string()+postFix, filePath.filename().string());
                                successList.push_back(filePath.string());
                            }
                        }
                    }

                }

            }

            return (successList);

        }
        
    } // namespace FTP

} // namespace Antik

