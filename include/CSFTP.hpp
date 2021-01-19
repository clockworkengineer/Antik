#ifndef CSFTP_HPP
#define CSFTP_HPP
//
// C++ STL
//
#include <stdexcept>
#include <vector>
#include <string>
#include <cstring>
#include <memory>
#include <cassert>
//
// Antik classes
//
#include "CommonAntik.hpp"
#include "CSSHSession.hpp"
//
// Linux
//
#include <fcntl.h>
//
// Libssh
//
#include <libssh/sftp.h>
// =========
// NAMESPACE
// =========
namespace Antik::SSH
{
    class CSSHSession;
    // ==========================
    // PUBLIC TYPES AND CONSTANTS
    // ==========================
    // ================
    // CLASS DEFINITION
    // ================
    class CSFTP
    {
    public:
        // ==========================
        // PUBLIC TYPES AND CONSTANTS
        // ==========================
        //
        // Class exception
        //
        struct Exception
        {
            Exception(CSFTP &sftp, const std::string &functionName) : m_errorCode{sftp.getSession().getErrorCode()},
                                                                      m_errorMessage{sftp.getSession().getError()}, m_sftpErrorCode{sftp.getErrorCode()},
                                                                      m_functionName{functionName}
            {
            }
            Exception(const std::string &errorMessage, const std::string &functionName) : m_errorMessage{errorMessage},
                                                                                          m_functionName{functionName}
            {
            }
            int getCode() const
            {
                return m_errorCode;
            }
            std::string getMessage() const
            {
                return static_cast<std::string>("CSFTP Failure: (") + m_functionName + ") [" + m_errorMessage + "]";
            }
            int sftpGetCode() const
            {
                return m_sftpErrorCode;
            }

        private:
            int m_errorCode{SSH_OK};        // SSH error code
            std::string m_errorMessage;     // SSH error message
            int m_sftpErrorCode{SSH_FX_OK}; // SFTP error code
            std::string m_functionName;     // Current function name
        };
        //
        // Custom deleter for re-mapped libssh sftp data structures.
        //
        struct Deleter
        {
            void operator()(sftp_attributes fileAttributes) const
            {
                sftp_attributes_free(fileAttributes);
            }
            void operator()(sftp_file file) const
            {
                sftp_close(file);
            }
            void operator()(sftp_dir directory) const
            {
                sftp_closedir(directory);
            }
            void operator()(sftp_statvfs_t statvfs) const
            {
                sftp_statvfs_free(statvfs);
            }
        };
        //
        // Encapsulate libssh sftp data in unique pointers.
        //
        using FileAttributes = std::unique_ptr<std::pointer_traits<sftp_attributes>::element_type, Deleter>;
        using File = std::unique_ptr<std::pointer_traits<sftp_file>::element_type, Deleter>;
        using Directory = std::unique_ptr<std::pointer_traits<sftp_dir>::element_type, Deleter>;
        using FileSystemInfo = std::unique_ptr<std::pointer_traits<sftp_statvfs_t>::element_type, Deleter>;
        //
        // Re-map some linux types used (possibly make these more abstract at a later date).
        //
        using FilePermissions = mode_t; // File permission (boost::filesystem status for portable way to get)
        using FileOwner = uid_t;        // File owner (Linux specific)
        using FileGroup = gid_t;        // File group (Linux specific)
        using Time = timeval;           // Time (Needs some work).
        // ============
        // CONSTRUCTORS
        // ============
        //
        // Main constructor
        //
        explicit CSFTP(CSSHSession &session);
        // ==========
        // DESTRUCTOR
        // ==========
        virtual ~CSFTP();
        // ==============
        // PUBLIC METHODS
        // ==============
        //
        // Open/Close SFTP session.
        //
        void open();
        void close();
        //
        // File IO
        //
        File openFile(const std::string &fileName, int accessType, int mode);
        size_t readFile(const File &fileHandle, void *readBuffer, size_t bytesToRead);
        size_t writeFile(const File &fileHandle, void *writeBuffer, size_t bytesToWrite);
        void closeFile(File &fileHandle);
        void rewindFile(const File &fileHandle);
        void seekFile(const File &fileHandle, uint32_t offset);
        void seekFile64(const File &fileHandle, uint64_t offset);
        uint32_t currentFilePostion(const File &fileHandle);
        uint64_t currentFilePostion64(const File &fileHandle);
        //
        // Directory IO
        //
        Directory openDirectory(const std::string &directoryPath);
        bool readDirectory(const Directory &directoryHandle, FileAttributes &fileAttributes);
        bool endOfDirectory(const Directory &directoryHandle);
        void closeDirectory(Directory &directoryHandle);
        //
        // Set/Get file attributes
        //
        void changePermissions(const std::string &filePath, const FilePermissions &filePermissions);
        void changeOwnerGroup(const std::string &filePath, const FileOwner &owner, const FileGroup &group);
        void getFileAttributes(const File &fileHandle, FileAttributes &fileAttributes);
        void getFileAttributes(const std::string &filePath, FileAttributes &fileAttributes);
        void setFileAttributes(const std::string &filePath, const FileAttributes &fileAttributes);
        void getLinkAttributes(const std::string &linkPath, FileAttributes &fileAttributes);
        bool isADirectory(const FileAttributes &fileAttributes);
        bool isARegularFile(const FileAttributes &fileAttributes);
        bool isASymbolicLink(const FileAttributes &fileAttributes);
        void changeFileModificationAccessTimes(const std::string &filePath, const Time *newTimeValues);
        //
        // Create/Remove directories.
        //
        void createDirectory(const std::string &directoryPath, const FilePermissions &filePermissions);
        void removeDirectory(const std::string &directoryPath);
        //
        // Create/Remove(delete) symbolic link). Get target file of link.
        //
        void createLink(const std::string &targetPath, const std::string &linkPath);
        void removeLink(const std::string &filePath);
        std::string readLink(const std::string &linkPath);
        //
        // Rename file
        //
        void renameFile(const std::string &sourceFile, const std::string &destinationFile);
        std::string canonicalizePath(const std::string &pathName);
        //
        // Get mounted volume information
        //
        void getFileSystemInfo(const File &fileHandle, FileSystemInfo &fileSystem);
        void getFileSystemInfo(const std::string &fileSystemName, FileSystemInfo &fileSystem);
        //
        // Get SFTP server version.
        //
        int getServerVersion() const;
        //
        // Get server extension information.
        //
        int getExtensionCount();
        std::string getExtensionName(int index);
        std::string getExtensionData(int index);
        bool extensionSupported(const std::string &name, const std::string &data);
        //
        // get last SFTP command error code.
        //
        int getErrorCode() const;
        //
        // Set IO buffer parameters.
        //
        std::shared_ptr<char[]> getIoBuffer();
        void setIoBufferSize(std::uint32_t ioBufferSize);
        std::uint32_t getIoBufferSize() const;
        //
        // Get internal libssh ssh/sftp session data structure pointers.
        //
        sftp_session getSFTP() const;
        CSSHSession &getSession() const;
        // ================
        // PUBLIC VARIABLES
        // ================
    private:
        // ===========================
        // PRIVATE TYPES AND CONSTANTS
        // ===========================
        // ===========================================
        // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
        // ===========================================
        CSFTP() = delete;
        CSFTP(const CSFTP &orig) = delete;
        CSFTP(const CSFTP &&orig) = delete;
        CSFTP &operator=(CSFTP other) = delete;
        // ===============
        // PRIVATE METHODS
        // ===============
        // =================
        // PRIVATE VARIABLES
        // =================
        CSSHSession &m_session;                      // Channel session
        sftp_session m_sftp;                         // libssh sftp structure.
        std::shared_ptr<char[]> m_ioBuffer{nullptr}; // IO buffer
        std::uint32_t m_ioBufferSize{32 * 1024};     // IO buffer size
    };
} // namespace Antik::SSH
#endif /* CSFTP_HPP */
