#ifndef SCPUTIL_HPP
#define SCPUTIL_HPP
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
#include "CSCP.hpp"
namespace Antik::SSH
{
    void getFile(CSSHSession &sshSession, const std::string &sourceFile, const std::string &destinationFile);
    void putFile(CSSHSession &sshSession, const std::string &sourceFile, const std::string &destinationFile);
    FileList getFiles(CSSHSession &sshSession, FileMapper &fileMapper, FileCompletionFn completionFn = nullptr);
    FileList putFiles(CSSHSession &sshSession, FileMapper &fileMapper, FileCompletionFn completionFn = nullptr);
} // namespace Antik::SSH
#endif /* SCPUTIL_HPP */
