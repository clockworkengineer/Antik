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
// Antik Classes
//

#include "CSSHSession.hpp"
#include "CSFTP.hpp"

namespace Antik {
    namespace SSH {

        //
        // Container for list of file paths
        //
        
        typedef std::vector<std::string> FileList;
        
        //
        // Server path separator
        //
        
        const char kServerPathSep { '/' };
        
        //
        // File transfer complete function
        //
        
        typedef std::function<void(std::string)> FileCompletionFn;
        
        //
        // Map files from to/from local/remote directories
        //
        
        class FileMapper {
        public:
            explicit FileMapper(const std::string &localDirectory, const std::string &remoteDirectory) :
            m_localDirectory{localDirectory}, m_remoteDirectory{ remoteDirectory}{ }
            
            std::string toLocal(const std::string &filePath)    
            {
                boost::filesystem::path localPath { m_localDirectory + kServerPathSep + filePath.substr(m_remoteDirectory.size()) };
                localPath.normalize();
                return(localPath.string());     
            }
            std::string toRemote(const std::string &filePath) 
            {
                boost::filesystem::path remotePath { m_remoteDirectory + kServerPathSep + filePath.substr(m_localDirectory.size()) };
                remotePath.normalize();
                return(remotePath.string());     
            }

            std::string getRemoteDirectory() const {
                return m_remoteDirectory;
            }

            std::string getLocalDirectory() const {
                return m_localDirectory;
            }

            
        private:
            std::string m_localDirectory;
            std::string m_remoteDirectory;
            
        };
        
        //
        // Get/put files to SFTP server
        //
        
        FileList getFiles(CSFTP &ftpServer,FileMapper &fileMapper, const FileList &fileList, FileCompletionFn completionFn=nullptr, bool safe = false, char postFix = '~');
        FileList putFiles(CSFTP &ftpServer, FileMapper &fileMapper, const FileList &fileList, FileCompletionFn completionFn=nullptr, bool safe = false, char postFix = '~');
  
        //
        // Get/put a single file to SFTP server
        //
        
        void getFile(CSFTP &sftp, const std::string &sourceFile, const std::string &destinationFile);
        void putFile(CSFTP &sftp, const std::string &sourceFile, const std::string &destinationFile);
        
        void listRemoteRecursive(CSFTP &sftp, const std::string &directoryPath, FileList &fileList);

    } // namespace SSH
} // namespace Antik

#endif /* SFTPUTIL_HPP */

