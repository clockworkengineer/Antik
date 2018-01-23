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
// Boost file system, string
//

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

//
// Antik utility
//

#include "CommonUtil.hpp"

//
// Antik Classes
//

#include "CSSHSession.hpp"
#include "CSFTP.hpp"

namespace Antik {
    namespace SSH {

        void listRemoteRecursive(CSFTP &sftpServer, const std::string &directoryPath, FileList &fileList);
        void getFile(CSFTP &sftpServer, const std::string &sourceFile, const std::string &destinationFile);
        void putFile(CSFTP &sftpServer, const std::string &sourceFile, const std::string &destinationFile);
        FileList getFiles(CSFTP &sftpServer, FileMapper &fileMapper, const FileList &fileList, FileCompletionFn completionFn = nullptr, bool safe = false, char postFix = '~');
        FileList putFiles(CSFTP &sftpServer, FileMapper &fileMapper, const FileList &fileList, FileCompletionFn completionFn=nullptr, bool safe = false, char postFix = '~');       
   
    } // namespace SSH
} // namespace Antik

#endif /* SFTPUTIL_HPP */

