/*
 * File:   SFTPUtil.hpp
 * 
 * Author: Robert Tizzard
 *
 * Created on April 10, 2017, 2:34 PM
 * 
 * Copyright 2017.
 * 
 */

#ifndef SFTPUTIL_HPP
#define SFTPUTIL_HPP

// =============
// INCLUDE FILES
// =============

//
// C++ STL
//

#include <string>
#include <vector>
#include <fstream>
#include <vector>

//
// Antik Classes
//

#include "CSSHSession.hpp"
#include "CSFTP.hpp"

namespace Antik {
    namespace SSH {

        // Container for list of file paths
        
        typedef std::vector<std::string> FileList;
        
        // Server path separator
        
        const char kServerPathSep { '/' };
        typedef std::function<void(std::string)> FileCompletionFn;
        
        void sftpMakeRemotePath (CSFTP &sftpServer, const std::string &remotePath);
        FileList getFiles(CSFTP &ftpServer, const std::string &localDirectory, const FileList &fileList, FileCompletionFn completionFn=nullptr, bool safe = false, char postFix = '~');
        FileList putFiles(CSFTP &ftpServer, const std::string &localDirectory, const std::string &remoteDirectory, const FileList &fileList, FileCompletionFn completionFn=nullptr, bool safe = false, char postFix = '~');
  
        void getFile(CSFTP &sftp, const std::string &sourceFile, const std::string &destinationFile);
        void putFile(CSFTP &sftp, const std::string &sourceFile, const std::string &destinationFile);
        void listRemote(CSFTP &sftp, const std::string &directoryPath, FileList &fileList, bool recursive = false);

    } // namespace SSH
} // namespace Antik

#endif /* SFTPUTIL_HPP */

