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

namespace Antik::FTP
{

void makeRemotePath(CFTP &ftpServer, const std::string &remotePath, bool saveCWD = true);
void listRemoteRecursive(CFTP &ftpServer, const std::string &remoteDirecory, FileList &fileList, FileFeedBackFn remoteFileFeedbackFn = nullptr);
FileList getFiles(CFTP &ftpServer, const std::string &localDirectory, const FileList &fileList, FileCompletionFn completionFn = nullptr, bool safe = false, char postFix = '~');
FileList putFiles(CFTP &ftpServer, const std::string &localDirectory, const FileList &fileList, FileCompletionFn completionFn = nullptr, bool safe = false, char postFix = '~');

} // namespace Antik::FTP

#endif /* FTPUTIL_HPP */
