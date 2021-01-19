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
namespace Antik::SSH
{
    void listRemoteRecursive(CSFTP &sftpServer, const std::string &directoryPath, FileList &fileList, FileFeedBackFn remoteFileFeedbackFn = nullptr);
    void getFile(CSFTP &sftpServer, const std::string &sourceFile, const std::string &destinationFile, FileCompletionFn completionFn = nullptr);
    void putFile(CSFTP &sftpServer, const std::string &sourceFile, const std::string &destinationFile, FileCompletionFn completionFn = nullptr);
    FileList getFiles(CSFTP &sftpServer, FileMapper &fileMapper, const FileList &fileList, FileCompletionFn completionFn = nullptr, bool safe = false, char postFix = '~');
    FileList putFiles(CSFTP &sftpServer, FileMapper &fileMapper, const FileList &fileList, FileCompletionFn completionFn = nullptr, bool safe = false, char postFix = '~');
} // namespace Antik::SSH
#endif /* SFTPUTIL_HPP */
