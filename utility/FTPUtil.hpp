/*
 * File:   FTPUtil.hpp
 * 
 * Author: Robert Tizzard
 *
 * Created on Apil 10, 2017, 2:34 PM
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
// Antik Classes
//

#include "CFTP.hpp"

namespace Antik {
    namespace FTP {
 
      void makeRemotePath (CFTP &ftpServer, const std::string &remotePath);
      void listLocalRecursive(const std::string &localDirectory, std::vector<std::string> &fileList);
      void listRemoteRecursive(CFTP &ftpServer, const std::string &remoteDirecory, std::vector<std::string> &fileList);
      std::vector<std::string> getFiles(CFTP &ftpServer, const std::string &localDirectory, const std::vector<std::string> &fileList, bool safe = false, char postFix = '~');
      std::vector<std::string> putFiles(CFTP &ftpServer, const std::string &remoteDirectory, const std::vector<std::string> &fileList, bool safe = false, char postFix = '~');
      
    } // namespace FTP
} // namespace Antik


#endif /* FTPUTIL_HPP */

