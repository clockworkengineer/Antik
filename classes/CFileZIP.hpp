/*
 * File:   CFileZIP.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on January 24, 2017, 2:33 PM
 *
 * Copyright 2016.
 *
 */

#ifndef CFILEZIP_HPP
#define CFILEZIP_HPP

//
// C++ STL definitions
//

#include <string>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <memory>
#include <ctime>
#include <set>

// =========
// NAMESPACE
// =========

namespace Antik {

    // ================
    // CLASS DEFINITION
    // ================

    class CFileZIP {
    public:

        // ==========================
        // PUBLIC TYPES AND CONSTANTS
        // ==========================

        //
        // Class exception
        //

        struct Exception : public std::runtime_error {

            Exception(std::string const& messageStr)
            : std::runtime_error("CFileZIP Failure: " + messageStr) {
            }

        };

        struct FileDetail {
            std::string fileNameStr;
            std::string fileCommentStr;
            std::tm  modificationDateTime={ 0 };
            std::uint32_t uncompressedSize=0;
            std::uint32_t compressedSize=0;
            std::uint16_t compression=0;
            std::uint16_t creatorVersion=0;
            std::uint32_t externalFileAttrib=0;
            std::vector<std::uint8_t>extraField;
        };

        // ============
        // CONSTRUCTORS
        // ============

        CFileZIP(const std::string& zipFileNameStr);

        // ==========
        // DESTRUCTOR
        // ==========

        virtual ~CFileZIP();

        // ==============
        // PUBLIC METHODS
        // ==============

        void name(const std::string& zipFileNameStr);
        void open(void);
        void close(void);
        std::vector<CFileZIP::FileDetail> contents(void);
        bool extract(const std::string& fileNameStr, const std::string& destFolderStr);
        void create(void);
        void add(const std::string& fileNameStr);
        void save(void);

        // ================
        // PUBLIC VARIABLES
        // ================

    private:

        // ===========================
        // PRIVATE TYPES AND CONSTANTS
        // ===========================

        struct FileHeader {
            const std::uint32_t signature = 0x04034b50;
            const std::uint32_t size = 30;
            std::uint16_t creatorVersion = 0;
            std::uint16_t bitFlag = 0;
            std::uint16_t compression = 0;
            std::uint16_t modificationTime = 0;
            std::uint16_t modificationDate = 0;
            std::uint32_t crc32 = 0;
            std::uint32_t compressedSize = 0;
            std::uint32_t uncompressedSize = 0;
            std::uint16_t fileNameLength = 0;
            std::uint16_t extraFieldLength = 0;
            std::string fileNameStr;
            std::vector<std::uint8_t> extraField;
        };

//        static const std::uint32_t kFileHeaderSize = 30;

        struct DataDescriptor {
            const std::uint32_t signature = 0x08074b50;
            const std::uint32_t size = 12;
            std::uint32_t crc32 = 0;
            std::uint32_t compressedSize = 0;
            std::uint32_t uncompressedSize = 0;
        };

 //       static const std::uint32_t kDataDescriptorSize = 12;

        struct CentralDirectoryFileHeader {
            const std::uint32_t signature = 0x02014b50;
            const std::uint32_t size = 46;
            std::uint16_t creatorVersion = 0;
            std::uint16_t extractorVersion = 0;
            std::uint16_t bitFlag = 0;
            std::uint16_t compression = 0;
            std::uint16_t modificationTime = 0;
            std::uint16_t modificationDate = 0;
            std::uint32_t crc32 = 0;
            std::uint32_t compressedSize = 0;
            std::uint32_t uncompressedSize = 0;
            std::uint16_t fileNameLength = 0;
            std::uint16_t extraFieldLength = 0;
            std::uint16_t fileCommentLength = 0;
            std::uint16_t diskNoStart = 0;
            std::uint16_t internalFileAttrib = 0;
            std::uint32_t externalFileAttrib = 0;
            std::uint32_t fileHeaderOffset = 0;
            std::string fileNameStr;
            std::vector<std::uint8_t>extraField;
            std::string fileCommentStr;
        };

        static const std::uint32_t kCentralDirectoryFileHeaderSize = 46;

        struct EOCentralDirectoryRecord {
            const std::uint32_t signature = 0x06054b50;
            const std::uint32_t size = 22;
            std::uint16_t diskNnumber = 0;
            std::uint16_t centralDirectoryStartDisk = 0;
            std::uint16_t numberOfCentralDirRecords = 0;
            std::uint16_t totalCentralDirRecords = 0;
            std::uint32_t sizeOfCentralDirRecords = 0;
            std::uint32_t offsetCentralDirRecords = 0;
            std::uint16_t commentLength = 0;
            std::vector<std::uint8_t> comment;
        };

 //       static const std::uint32_t kEOCentralDirectoryRecordSize = 22;
        
        static const std::uint32_t kZIPBufferSize = 16384;

        // ===========================================
        // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
        // ===========================================

        CFileZIP() = delete;
        CFileZIP(const CFileZIP & orig) = delete;
        CFileZIP(const CFileZIP && orig) = delete;
        CFileZIP& operator=(CFileZIP other) = delete;

        // ===============
        // PRIVATE METHODS
        // ===============

        void putField(const std::uint32_t& field, std::vector<std::uint8_t>& buffer);
        void putField(const std::uint16_t& field, std::vector<std::uint8_t>& buffer);
        void putDataDescriptor(CFileZIP::DataDescriptor& entry);
        void putCentralDirectoryFileHeader(CFileZIP::CentralDirectoryFileHeader& entry);
        void putFileHeader(CFileZIP::FileHeader& entry);
        void putEOCentralDirectoryRecord(CFileZIP::EOCentralDirectoryRecord& entry);
 
        void getField(std::uint32_t& field, std::uint8_t *buffer);
        void getField(std::uint16_t& field, std::uint8_t *buffer);
        void getDataDescriptor(CFileZIP::DataDescriptor& entry);
        void getCentralDirectoryFileHeader(CFileZIP::CentralDirectoryFileHeader& entry);
        void getFileHeader(CFileZIP::FileHeader& entry);
        void getEOCentralDirectoryRecord(CFileZIP::EOCentralDirectoryRecord& entry);

        std::uint32_t calculateCRC32(std::ifstream& sourceFileStream, std::uint32_t fileSize);
        void convertModificationDateTime(std::tm& modificationDateTime, std::uint16_t dateWord, std::uint16_t timeWord);
        bool inflateFile(std::ofstream& destFileStream, std::uint32_t fileSize);
        bool copyFile(std::ofstream& destFileStream, std::uint32_t fileSize); 
        bool deflateFile(std::ifstream& sourceFileStream, std::uint32_t uncompressedSize, std::uint32_t& compressedSize);
  
        bool fileExists(const std::string& fileNameStr);
        void getFileAttributes(const std::string& fileNameStr, std::uint32_t& attributes);
        void getFileSize(const std::string& fileNameStr, std::uint32_t& fileSize);     
        void getFileCRC32(const std::string& fileNameStr, std::uint32_t fileSize, std::uint32_t& crc32);
        void getFileModificationDateTime(const std::string& fileNameStr, std::uint16_t& modificationDate, std::uint16_t& modificationTime);
        void getFileData(const std::string& fileNameStr, std::uint32_t fileLength);
        void getFileDataCompressed(const std::string& fileNameStr, std::uint32_t uncompressedSize, std::uint32_t& compressedSize);
         
        void writeFileHeaderAndData(const std::string& fileNameStr);
        
        // =================
        // PRIVATE VARIABLES
        // =================

        std::string zipFileNameStr;
        std::fstream zipFileStream;
        EOCentralDirectoryRecord zipEOCentralDirectory;
        std::vector<CentralDirectoryFileHeader> zipCentralDirectory;
        std::vector<std::string> zipFileContentsList;
        
        std::vector<uint8_t> zipInBuffer;
        std::vector<uint8_t> zipOutBuffer;

    };

} // namespace Antik

#endif /* CFILEZIP_HPP */

