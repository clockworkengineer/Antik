/*
 * File:   FTPUtil.hpp
 * 
 * Author: Robert Tizzard
 *
 * Created on April 10, 2017, 2:34 PM
 * 
 * Copyright 2017.
 * 
 */

#ifndef FTPUTIL_HPP
#define FTPUTIL_HPP

// =============
// INCLUDE FILES
// =============

//
// C++ STL
//

#include <string>
#include <vector>

//
// Antik utility
//

#include "CommonUtil.hpp"

//
// Antik Classes
//

#include "CFTP.hpp"

namespace Antik {
    namespace FTP {
 
      void makeRemotePath (CFTP &ftpServer, const std::string &remotePath, bool saveCWD=true);
      void listRemoteRecursive(CFTP &ftpServer, const std::string &remoteDirecory, FileList&fileList, RemoteFileListFn remoteFileFeedbackFn=nullptr);
      FileList getFiles(CFTP &ftpServer, const std::string &localDirectory, const FileList &fileList, FileCompletionFn completionFn=nullptr, bool safe = false, char postFix = '~');
      FileList putFiles(CFTP &ftpServer, const std::string &localDirectory, const FileList &fileList, FileCompletionFn completionFn=nullptr, bool safe = false, char postFix = '~');
      
    } // namespace FTP
} // namespace Antik


#endif /* FTPUTIL_HPP */

