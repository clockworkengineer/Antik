//
// Module: FTPUtil
//
// Description: FTP utility functions for the Antik class CFTP.
// Perform selective and  more powerful operations not available directly through
// single raw FTP commands.
//
// Dependencies:
//
// C20++              : Use of C20++ features.
// Antik Classes      : CFTP, CFile, CPath
// Boost              : String, iterators.
//
// =============
// INCLUDE FILES
// =============
//
// C++ STL
//
#include <iostream>
//
// FTP utility definitions
//
#include "FTPUtil.hpp"
//
// Antik Classes
//
#include "CFile.hpp"
#include "CPath.hpp"
//
// Boost string and iterators
//
#include <boost/algorithm/string.hpp>
#include <boost/range/iterator_range.hpp>
// =========
// NAMESPACE
// =========
namespace Antik ::FTP
{
    // =======
    // IMPORTS
    // =======
    using namespace Antik::File;
    // ===============
    // LOCAL FUNCTIONS
    // ===============
    //
    // Construct a remote path name from three passed in parameters and just for simplicity replace all
    // double path separators with just one if any have been generated.
    //
    static std::string constructRemotePathName(const std::string &currentWorkingDirectory, const std::string &remotePath, const std::string &remoteFileName)
    {
        std::string remoteDirectoryPath{currentWorkingDirectory + kServerPathSep + remotePath + kServerPathSep + remoteFileName};
        std::string::size_type i{0};
        while ((i = remoteDirectoryPath.find(std::string(2, kServerPathSep))) != std::string::npos)
        {
            remoteDirectoryPath.erase(remoteDirectoryPath.begin() + i);
        }
        if (remoteDirectoryPath.back() == kServerPathSep)
            remoteDirectoryPath.pop_back();
        return (remoteDirectoryPath);
    }
    //
    // Construct a remote path name from value returned from server list command. This may or may no have
    // the absolute path name on front. If it does return it otherwise construct one that does.
    //
    static std::string constructRemotePathName(const std::string &remotePath, const std::string &remoteFileName)
    {
        if (remoteFileName.find(remotePath) == 0)
        {
            return (remoteFileName);
        }
        else
        {
            return (constructRemotePathName("", remotePath, remoteFileName));
        }
    }
    // ================
    // PUBLIC FUNCTIONS
    // ================
    //
    // Recursively parse a remote server path passed in and pass back a list of directories/files found.
    // For servers that do not return a fully qualified path name create one. If a feedback function has
    // been passed in then it is called for each file found.
    //
    void listRemoteRecursive(CFTP &ftpServer, const std::string &remoteDirectory, FileList &remoteFileList, FileFeedBackFn remoteFileFeedbackFn)
    {
        FileList serverFileList;
        std::string currentWorkingDirectory;
        // Save current working directory
        ftpServer.getCurrentWoringDirectory(currentWorkingDirectory);
        ftpServer.changeWorkingDirectory(remoteDirectory);
        if (ftpServer.listFiles("", serverFileList) == 226)
        {
            for (auto file : serverFileList)
            {
                std::string fullFilePath{constructRemotePathName(remoteDirectory, file)};
                remoteFileList.push_back(fullFilePath);
                if (remoteFileFeedbackFn)
                {
                    remoteFileFeedbackFn(remoteFileList.back());
                }
                if (ftpServer.isDirectory(fullFilePath))
                {
                    listRemoteRecursive(ftpServer, fullFilePath, remoteFileList, remoteFileFeedbackFn);
                }
            }
        }
        // Restore saved current working directory
        ftpServer.changeWorkingDirectory(currentWorkingDirectory);
    }
    //
    // Break path into its component directories and create path structure on
    // remote FTP server. Note: This done relative to the server currently set
    // working directory and no errors are reported. To test for success/failure
    // use CFTP::fileExists() after call to see if it has been created.
    //
    void makeRemotePath(CFTP &ftpServer, const std::string &remotePath, bool saveCWD)
    {
        std::vector<std::string> pathComponents;
        std::string currentWorkingDirectory;
        // Save current working directory
        if (saveCWD)
        {
            ftpServer.getCurrentWoringDirectory(currentWorkingDirectory);
        }
        boost::split(pathComponents, remotePath, boost::is_any_of(std::string(1, kServerPathSep)));
        ftpServer.getCurrentWoringDirectory(currentWorkingDirectory);
        for (auto directory : pathComponents)
        {
            if (!directory.empty())
            {
                ftpServer.makeDirectory(directory);
                ftpServer.changeWorkingDirectory(directory);
            }
        }
        // Restore saved current working directory
        if (saveCWD)
        {
            ftpServer.changeWorkingDirectory(currentWorkingDirectory);
        }
    }
    //
    // Download all files passed in file list from server to the local directory passed in; recreating any server directory
    // structure in situ. If safe == true then the file is downloaded to a filename with a postfix then the file is renamed
    // to its correct value on success. Returns a list of successfully downloaded files and directories created in the local
    // directory. The local file name is calculated by removing the current working directory from each file in the list and
    // appending it to the passed in local directory to get the full local file path.
    //
    FileList getFiles(CFTP &ftpServer, const std::string &localDirectory, const FileList &fileList, FileCompletionFn completionFn, bool safe, char postFix)
    {
        FileList successList;
        std::string currentWorkingDirectory;
        // Save current working directory
        ftpServer.getCurrentWoringDirectory(currentWorkingDirectory);
        try
        {
            for (auto file : fileList)
            {
                CPath destination{localDirectory};
                destination.join(file.substr(currentWorkingDirectory.size()));
                destination.normalize();
                if (!CFile::exists(destination.parentPath()))
                {
                    CFile::createDirectory(destination.parentPath());
                }
                if (!ftpServer.isDirectory(file))
                {
                    std::string destinationFileName{destination.toString() + postFix};
                    if (!safe)
                    {
                        destinationFileName.pop_back();
                    }
                    if (ftpServer.getFile(file, destinationFileName) == 226)
                    {
                        if (safe)
                        {
                            CFile::rename(destinationFileName, destination);
                        }
                        successList.push_back(destination.toString());
                        if (completionFn)
                        {
                            completionFn(successList.back());
                        }
                    }
                }
                else
                {
                    if (!CFile::exists(destination))
                    {
                        CFile::createDirectory(destination);
                    }
                    successList.push_back(destination.toString());
                    if (completionFn)
                    {
                        completionFn(successList.back());
                    }
                }
            }
            // Restore saved current working directory
            ftpServer.changeWorkingDirectory(currentWorkingDirectory);
            // On exception report and return with files that where successfully downloaded.
        }
        catch (const CFTP::Exception &e)
        {
            std::cerr << e.what() << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << std::endl;
        }
        return (successList);
    }
    //
    // Take local directory, file list and upload all files to server;  recreating
    // any local directory structure in situ on the server. Returns a list of successfully
    // uploaded files and directories created.If safe == true then the file is uploaded to a
    // filename with a postfix then the file is renamed to its correct value on success.
    // All files/directories are placed/created relative to the server current working directory.
    //
    FileList putFiles(CFTP &ftpServer, const std::string &localDirectory, const FileList &fileList, FileCompletionFn completionFn, bool safe, char postFix)
    {
        FileList successList;
        size_t localPathLength{0};
        std::string currentWorkingDirectory;
        // Determine local path length for creating remote paths.
        localPathLength = localDirectory.size();
        if (localDirectory.back() != kServerPathSep)
            localPathLength++;
        // Save current working directory
        ftpServer.getCurrentWoringDirectory(currentWorkingDirectory);
        try
        {
            // Process file/directory list
            for (auto file : fileList)
            {
                CPath filePath{file};
                if (CFile::exists(filePath))
                {
                    std::string remoteDirectory;
                    bool transferFile{false};
                    // Create remote path and set file to be transfered flag
                    if (CFile::isDirectory(filePath))
                    {
                        remoteDirectory = filePath.toString();
                    }
                    else if (CFile::isFile(filePath))
                    {
                        remoteDirectory = filePath.parentPath().toString() + kServerPathSep;
                        transferFile = true;
                    }
                    else
                    {
                        continue; // Not valid for transfer NEXT FILE!
                    }
                    remoteDirectory = remoteDirectory.substr(localPathLength);
                    // Set current working directory and create any remote path needed
                    ftpServer.changeWorkingDirectory(currentWorkingDirectory);
                    if (!remoteDirectory.empty())
                    {
                        if (!ftpServer.isDirectory(remoteDirectory))
                        {
                            makeRemotePath(ftpServer, remoteDirectory, false);
                            successList.push_back(constructRemotePathName(currentWorkingDirectory, remoteDirectory, ""));
                            if (!transferFile && completionFn)
                            {
                                completionFn(successList.back());
                            }
                        }
                        else
                        {
                            ftpServer.changeWorkingDirectory(remoteDirectory);
                        }
                    }
                    // Transfer file
                    if (transferFile)
                    {
                        std::string destinationFileName{filePath.fileName() + postFix};
                        if (!safe)
                        {
                            destinationFileName.pop_back();
                        }
                        if (ftpServer.putFile(destinationFileName, filePath.toString()) == 226)
                        {
                            if (safe)
                            {
                                ftpServer.renameFile(destinationFileName, filePath.fileName());
                            }
                            successList.push_back(constructRemotePathName(currentWorkingDirectory, remoteDirectory, filePath.fileName()));
                            if (completionFn)
                            {
                                completionFn(successList.back());
                            }
                        }
                    }
                }
            }
            // Restore saved current working directory
            ftpServer.changeWorkingDirectory(currentWorkingDirectory);
            // On exception report and return with files that where successfully uploaded.
        }
        catch (const CFTP::Exception &e)
        {
            std::cerr << e.what() << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << std::endl;
        }
        return (successList);
    }
} // namespace Antik::FTP
