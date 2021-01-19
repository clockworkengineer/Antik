//
// Module: SCPUtil
//
// Description: SCP utility functions for the Antik class CSCP. Perform single/recursive
// file transfers to/from a remote server. Any file paths are created in situ on either
// the remote server or local host.
//
// Dependencies:
//
// C20++              : Use of C20++ features.
// Antik classes      : CSCP
//
// =============
// INCLUDE FILES
// =============
//
// C++ STL
//
#include <iostream>
#include <system_error>
#include <filesystem>
//
// SCP utility definitions
//
#include "SCPUtil.hpp"
// =========
// NAMESPACE
// =========
namespace Antik ::SSH
{
    // =======
    // IMPORTS
    // =======
    // ===============
    // LOCAL FUNCTIONS
    // ===============
    //
    // Break path into its component directories and create path structure on
    // remote SCP server.
    //
    static void makeRemotePath(CSCP &scpServer, const std::string &remotePath, const CSCP::FilePermissions permissions)
    {
        std::vector<std::string> pathComponents;
        std::filesystem::path currentPath{std::string(1, kServerPathSep)};
        boost::split(pathComponents, remotePath, boost::is_any_of(std::string(1, kServerPathSep)));
        for (auto directory : pathComponents)
        {
            if (!directory.empty())
            {
                scpServer.pushDirectory(directory, permissions);
            }
        }
    }
    //
    // Download the currently requested file from SCP server and write to local directory.
    //
    static void downloadFile(CSCP &scpServer, const std::string &destinationFile)
    {
        std::ofstream localFile;
        CSCP::FilePermissions filePermissions;
        char *ioBuffer = scpServer.getIoBuffer().get();
        size_t ioBufferSize = scpServer.getIoBufferSize();
        int bytesRead{0};
        int fileSize{0};
        filePermissions = scpServer.requestFilePermissions();
        fileSize = scpServer.requestFileSize();
        scpServer.acceptRequest();
        if (!std::filesystem::exists(std::filesystem::path(destinationFile).parent_path()))
        {
            std::filesystem::create_directories(std::filesystem::path(destinationFile).parent_path());
        }
        localFile.open(destinationFile, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
        if (!localFile)
        {
            throw std::system_error(errno, std::system_category());
        }
        for (;;)
        {
            bytesRead = scpServer.read(ioBuffer, ioBufferSize);
            if (bytesRead)
            {
                localFile.write(ioBuffer, bytesRead);
                if (!localFile)
                {
                    throw std::system_error(errno, std::system_category());
                }
                fileSize -= bytesRead;
            }
            if ((fileSize <= 0) || (bytesRead == 0))
            {
                break;
            }
        }
        localFile.close();
        std::filesystem::permissions(destinationFile, static_cast<std::filesystem::perms>(filePermissions));
    }
    // ================
    // PUBLIC FUNCTIONS
    // ================
    //
    // Download a file from remote SCP server assigning it the same permissions as the remote file.
    // SCP does not directly support file upload/download so this function is not part of the
    // CSCP class.
    //
    void getFile(CSSHSession &sshSession, const std::string &sourceFile, const std::string &destinationFile)
    {
        CSCP scpServer{sshSession, SSH_SCP_READ, sourceFile};
        int pullStatus{0};
        scpServer.open();
        if ((pullStatus = scpServer.pullRequest()) != SSH_SCP_REQUEST_NEWFILE)
        {
            if (pullStatus == SSH_SCP_REQUEST_WARNING)
            {
                throw CSCP::Exception(scpServer, __func__);
            }
        }
        downloadFile(scpServer, destinationFile);
        scpServer.close();
    }
    //
    // Upload a file to remote SCP server assigning it the same permissions as the local file.
    // It will be created with the owner and group of the currently logged in SSH account.
    // SCP does not directly support file upload/download so this function is not part of the
    // CSCP class.
    //
    void putFile(CSSHSession &sshSession, const std::string &sourceFile, const std::string &destinationFile)
    {
        std::ifstream localFile;
        std::filesystem::file_status fileStatus;
        localFile.open(sourceFile, std::ios_base::in | std::ios_base::binary);
        if (!localFile)
        {
            throw std::system_error(errno, std::system_category());
        }
        fileStatus = std::filesystem::status(std::filesystem::path(sourceFile).parent_path().string());
        CSCP scpServer{sshSession, SSH_SCP_WRITE | SSH_SCP_RECURSIVE, std::string(1, kServerPathSep)};
        scpServer.open();
        if (std::filesystem::is_directory(sourceFile))
        {
            makeRemotePath(scpServer, std::filesystem::path(destinationFile).string(), (CSCP::FilePermissions)fileStatus.permissions());
        }
        else if (std::filesystem::is_regular_file(sourceFile))
        {
            makeRemotePath(scpServer, std::filesystem::path(destinationFile).parent_path().string(), (CSCP::FilePermissions)fileStatus.permissions());
            fileStatus = std::filesystem::status(std::filesystem::path(sourceFile));
            scpServer.pushFile(std::filesystem::path(destinationFile).filename().string(), std::filesystem::file_size(sourceFile), (CSCP::FilePermissions)fileStatus.permissions());
            for (;;)
            {
                localFile.read(scpServer.getIoBuffer().get(), scpServer.getIoBufferSize());
                if (localFile.gcount())
                {
                    scpServer.write(scpServer.getIoBuffer().get(), localFile.gcount());
                }
                if (!localFile)
                    break;
            }
            localFile.close();
        }
        scpServer.close();
    }
    //
    // Download all files in a remote directory structure (recursive); recreating
    // any server directory structure in situ. Returns a list of successfully downloaded files and
    // directories created in the local directory.
    //
    FileList getFiles(CSSHSession &sshSession, FileMapper &fileMapper, FileCompletionFn completionFn)
    {
        FileList successList;
        try
        {
            CSCP scpServer{sshSession, SSH_SCP_READ | SSH_SCP_RECURSIVE, fileMapper.getRemoteDirectory()};
            int pullStatus;
            std::filesystem::path currentPath{fileMapper.getRemoteDirectory()};
            std::filesystem::path localFilePath;
            scpServer.open();
            scpServer.pullRequest();
            scpServer.acceptRequest();
            while ((pullStatus = scpServer.pullRequest()) != SSH_SCP_REQUEST_EOF)
            {
                switch (pullStatus)
                {
                case SSH_SCP_REQUEST_NEWFILE:
                    localFilePath = currentPath.string();
                    localFilePath /= scpServer.requestFileName();
                    localFilePath = fileMapper.toLocal(localFilePath.string());
                    downloadFile(scpServer, localFilePath.string());
                    successList.push_back(localFilePath.string());
                    if (completionFn)
                    {
                        completionFn(successList.back());
                    }
                    break;
                case SSH_SCP_REQUEST_NEWDIR:
                    currentPath /= scpServer.requestFileName();
                    if (!std::filesystem::exists(fileMapper.toLocal(currentPath.string())))
                    {
                        std::filesystem::create_directories(fileMapper.toLocal(currentPath.string()));
                        std::filesystem::permissions(fileMapper.toLocal(currentPath.string()),
                                                     static_cast<std::filesystem::perms>(scpServer.requestFilePermissions()));
                        successList.push_back(fileMapper.toLocal(currentPath.string()));
                        if (completionFn)
                        {
                            completionFn(successList.back());
                        }
                    }
                    scpServer.acceptRequest();
                    break;
                case SSH_SCP_REQUEST_ENDDIR:
                    currentPath = currentPath.parent_path();
                    break;
                case SSH_SCP_REQUEST_WARNING:
                    throw CSCP::Exception(scpServer, __func__);
                    break;
                }
            }
            scpServer.close();
            // On exception report and return with files that where successfully downloaded.
        }
        catch (const CSCP::Exception &e)
        {
            std::cerr << e.getMessage() << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << std::endl;
        }
        return (successList);
    }
    //
    // Take local directory and upload all its files to server;  recreating
    // any local directory structure in situ on the server. Returns a list of successfully
    // uploaded files and directories created.
    //
    FileList putFiles(CSSHSession &sshSession, FileMapper &fileMapper, FileCompletionFn completionFn)
    {
        FileList successList;
        try
        {
            FileList localFileList;
            listLocalRecursive(fileMapper.getLocalDirectory(), localFileList);
            for (auto localFile : localFileList)
            {
                putFile(sshSession, localFile, fileMapper.toRemote(localFile));
                successList.push_back(fileMapper.toRemote(localFile));
                if (completionFn)
                {
                    completionFn(successList.back());
                }
            }
            // On exception report and return with files that where successfully uploaded.
        }
        catch (const CSCP::Exception &e)
        {
            std::cerr << e.getMessage() << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << std::endl;
        }
        return (successList);
    }
} // namespace Antik::SSH
