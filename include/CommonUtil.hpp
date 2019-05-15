/*
 * File:   CommonUtil.hpp
 * 
 * Author: Robert Tizzard
 *
 * Created on April 10, 2017, 2:34 PM
 * 
 * Copyright 2017.
 * 
 */

#ifndef COMMONUTIL_HPP
#define COMMONUTIL_HPP

// =============
// INCLUDE FILES
// =============

//
// C++ STL
//

//
// Antik classes
//

#include "CommonAntik.hpp"

//
// Boost file system, string
//

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

namespace Antik
{

//
// File transfer complete function
//

typedef std::function<void(const std::string &)> FileCompletionFn;

//
// Remote file recursive list feedback function
//

typedef std::function<void(const std::string &)> FileFeedBackFn;

//
// Map files from to/from local/remote directories
//

class FileMapper
{
public:
    explicit FileMapper(const std::string &localDirectory, const std::string &remoteDirectory) : m_localDirectory{localDirectory}, m_remoteDirectory{remoteDirectory}
    {
        if (m_localDirectory.back() == kServerPathSep)
            m_localDirectory.pop_back();
        if (m_remoteDirectory.back() == kServerPathSep)
            m_remoteDirectory.pop_back();
    }

    std::string toLocal(const std::string &filePath)
    {
        boost::filesystem::path localPath{m_localDirectory + kServerPathSep + filePath.substr(m_remoteDirectory.size())};
        localPath.normalize();
        return (localPath.string());
    }

    std::string toRemote(const std::string &filePath)
    {
        boost::filesystem::path remotePath{m_remoteDirectory + kServerPathSep + filePath.substr(m_localDirectory.size())};
        remotePath.normalize();
        return (remotePath.string());
    }

    std::string getRemoteDirectory() const
    {
        return m_remoteDirectory;
    }

    std::string getLocalDirectory() const
    {
        return m_localDirectory;
    }

private:
    std::string m_localDirectory;
    std::string m_remoteDirectory;
};

//
// Recursively parse a local directory and produce a list of files. (static for moment)
//

static inline void listLocalRecursive(const std::string &localDirectory, FileList &fileList, FileFeedBackFn localFileFeedbackFn = nullptr)
{

    for (auto directoryEntry : boost::filesystem::recursive_directory_iterator{localDirectory})
    {
        fileList.push_back(directoryEntry.path().string());
        if (localFileFeedbackFn)
        {
            localFileFeedbackFn(directoryEntry.path().string());
        }
    }
}

} // namespace Antik

#endif /* COMMONUTIL_HPP */
