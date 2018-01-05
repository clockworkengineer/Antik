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

        void sftpGetFile(CSFTP &sftp, const std::string &sourceFile, const std::string &destinationFile);
        void sftpPutFile(CSFTP &sftp, const std::string &sourceFile, const std::string &destinationFile);
        void sftpGetFileList(CSFTP &sftp, const std::string &directoryPath, std::vector<std::string> &fileList, bool recursive = false);

    } // namespace SSH
} // namespace Antik

#endif /* SFTPUTIL_HPP */

