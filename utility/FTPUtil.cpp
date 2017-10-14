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
// Description: FTP utility functions for the Antikythera class CFTP.
// Perform selective and  more powerful operations not available directly through
// single raw FTP commands.
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
        // Recursively parse a remote server path passed in and pass back a list of directories/files found.
        //
        
        void listFilesRecursive(CFTP &ftpServer, string directoryPath, vector<string> &fileList) {

            vector<string> serverFileList;
            uint16_t statusCode;

            statusCode = ftpServer.listFiles(directoryPath, serverFileList);
            if (statusCode == 226) {
                for (auto file : serverFileList) {
                    if (directoryPath != file) {
                        listFilesRecursive(ftpServer, file, fileList);
                        fileList.push_back(file);
                    }
                }
            }
        }
        
        //
        // Download all files passed in list from server to the destination path passed in; recreating any server directory
        // structure in situ. If safe == true then the file is downloaded to a filename with a postfix then the file is renamed
        // to its correct value on success. Returns a list of successfully downloaded files and directories created.
        //

        vector<string> getFiles(CFTP &ftpServer, const string &destinationPath, const vector<string> &fileList, bool safe, char postFix) {

            uint16_t statusCode;
            vector<string> successList;

            if (!safe) {
                postFix = '\0';
            }

            for (auto file : fileList) {
                fs::path destination{ destinationPath};
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
        // Take base folder and recursively traverse it uploading all files to server;  recreating 
        // any local directory structure in situ on the server.
        //
        
        vector<string> putFiles(CFTP &ftpServer, const string &baseFolder) {

            vector<string> successList;
            fs::path basePath{ baseFolder};

            for (auto dir : fs::recursive_directory_iterator{basePath}) {

                if (fs::exists(dir.path())) {

                    string directory;
                    bool putFile = false;

                    if (fs::is_directory(dir.path())) {
                        directory = dir.path().string().substr(basePath.parent_path().size());
                        putFile = false;
                    } else if (fs::is_regular_file(dir.path())) {
                        directory = dir.path().parent_path().string().substr(basePath.parent_path().size());
                        putFile = true;
                    }

                    if (!directory.empty()) {

                        if (!ftpServer.isDirectory(directory)) {
                            vector<string> directories;
                            boost::split(directories, directory, boost::is_any_of("/"));
                            ftpServer.changeWorkingDirectory("/");
                            for (auto files : directories) {
                                ftpServer.makeDirectory(files);
                                ftpServer.changeWorkingDirectory(files);
                            }
                            successList.push_back(directory);
                        } else {
                            ftpServer.changeWorkingDirectory(directory);
                        }
                        if (putFile) {
                            if (ftpServer.putFile(dir.path().filename().string(), dir.path().string()) == 226) {
                                successList.push_back(dir.path().string());
                            }
                        }
                    }

                }

            }

            return (successList);

        }
    } // namespace FTP

} // namespace Antik

