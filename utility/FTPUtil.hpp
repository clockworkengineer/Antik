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
    
      void listFilesRecursive(CFTP &ftpServer, std::string directoryPath, std::vector<std::string> &allFiles);
      std::vector<std::string> getFiles(CFTP &ftpServer, const std::string &destinationPath, const std::vector<std::string> &fileList, bool safe = true, char postFix = '~');
      std::vector<std::string> putFiles(CFTP &ftpServer, const std::string &baseFolder);
      
    } // namespace FTP
} // namespace Antik


#endif /* FTPUTIL_HPP */

