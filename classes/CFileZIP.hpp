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
#include <ctime>

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
        // Although these are meant to be the specified sizes add a double check.
        //
        
        static_assert(sizeof(std::uint16_t)==2, "Error : std::uint16_t needs to be 2 bytes.");
        static_assert(sizeof(std::uint32_t)==4, "Error : std::uint32_t needs to be 4 bytes.");
        static_assert(sizeof(std::uint64_t)==8, "Error : std::uint64_t needs to be 8 bytes.");
        
        //
        // Class exception
        //

        struct Exception : public std::runtime_error {

            Exception(std::string const& messageStr)
            : std::runtime_error("CFileZIP Failure: " + messageStr) {
            }

        };

        //
        // ZIP file archive file details entry
        //
        
        struct FileDetail {
            std::string fileNameStr;                // Name
            std::string fileCommentStr;             // Comment
            std::tm  modificationDateTime={ 0 };    // Last modified date/time
            std::uint32_t uncompressedSize=0;       // Uncompressed size
            std::uint32_t compressedSize=0;         // Compressed size
            std::uint16_t compression=0;            // Compression stored as
            std::uint16_t creatorVersion=0;         // Archive creator
            std::uint32_t externalFileAttrib=0;     // Attributes
            std::vector<std::uint8_t>extraField;    // Extra data
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

        //
        // Set ZIP archive name
        //
        
        void name(const std::string& zipFileNameStr);
        
        //
        // Create an empty archive file
        //
        void create(void);
        
        //
        // Open/close archive file
        //
        void open(void);
        void close(void);
        
        //
        // Add/extract files to archive
        //
        
        bool extract(const std::string& fileNameStr, const std::string& destFolderStr);
        bool add(const std::string& fileNameStr, const std::string& zippedFileNameStr );

        //
        // Get archives contents
        //
        
        std::vector<CFileZIP::FileDetail> contents(void);
        
        //
        // Return true if archive file entry is a directory
        //
        
        bool isDirectory(const CFileZIP::FileDetail& fileEntry);
  
        // ================
        // PUBLIC VARIABLES
        // ================

    private:

        // ===========================
        // PRIVATE TYPES AND CONSTANTS
        // ===========================

        //
        // Archive Local File Header record
        //
        
        struct LocalFileHeader {
            const std::uint32_t size = 30;
            const std::uint32_t signature = 0x04034b50;
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

        //
        // Archive Data Descriptor record.
        //

        struct DataDescriptor {
            const std::uint32_t size = 12;
            const std::uint32_t signature = 0x08074b50;
            std::uint32_t crc32 = 0;
            std::uint32_t compressedSize = 0;
            std::uint32_t uncompressedSize = 0;
        };

        //
        // Archive Central Directory File Header record.
        //

        struct CentralDirectoryFileHeader {
            const std::uint32_t size = 46;            
            const std::uint32_t signature = 0x02014b50;
            std::uint16_t creatorVersion = 0x0314;    // Unix / ZIP 2.0
            std::uint16_t extractorVersion = 0x0014;  // ZIP 2.0
            std::uint16_t bitFlag = 0;
            std::uint16_t compression = 8;            // Deflated
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

        //
        // Archive End Of Central Directory record.
        //

        struct EOCentralDirectoryRecord {
            const std::uint32_t size = 22;
            const std::uint32_t signature = 0x06054b50;
            std::uint16_t diskNumber = 0;
            std::uint16_t startDiskNumber = 0;
            std::uint16_t numberOfCentralDirRecords = 0;
            std::uint16_t totalCentralDirRecords = 0;
            std::uint32_t sizeOfCentralDirRecords = 0;
            std::uint32_t offsetCentralDirRecords = 0;
            std::uint16_t commentLength = 0;
            std::vector<std::uint8_t> comment;
        };
              
        //
        // ZIP64 Archive End Of Central Directory record.
        //

        struct Zip64EOCentralDirectoryRecord {
            const std::uint32_t size = 54;
            const std::uint32_t signature = 0x06064b50;
            std::uint64_t  totalRecordSize = 0;
            std::uint16_t creatorVersion = 0;
            std::uint16_t extractorVersion = 0;
            std::uint32_t diskNumber = 0;
            std::uint32_t startDiskNumber = 0;
            std::uint64_t numberOfCentralDirRecords = 0;
            std::uint64_t  totalCentralDirRecords = 0;
            std::uint64_t sizeOfCentralDirRecords = 0;
            std::uint64_t offsetCentralDirRecords = 0;
            std::vector<std::uint8_t> extensibleDataSector;
 
        };

        //
        // ZIP64 Archive End Of Central Directory record locator.
        //

        struct Zip64EOCentDirRecordLocator {
            const std::uint32_t size = 20;
            const std::uint32_t signature = 0x07064b50;
            std::uint32_t startDiskNumber = 0;
            std::uint64_t offset = 0;
            std::uint32_t numberOfDisks = 0;
        };

        //
        // ZIP64 Archive extended information field.
        //

        struct Zip64ExtendedInformationExtraField {
            const std::uint16_t signature = 0x0001;
            std::uint16_t size = 0;
            std::uint64_t originalSize = 0;
            std::uint64_t compressedSize = 0;
            std::uint64_t fileHeaderOffset = 0;
            std::uint32_t diskNo = 0;
        };
        
        //
        // ZIP inflate/deflate buffer size.
        //
        
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

        bool fieldOverflow(const std::uint64_t& field);
        bool fieldOverflow(const std::uint32_t& field);
        bool fieldOverflow(const std::uint16_t& field);
                     
        void putField(const std::uint64_t& field, std::vector<std::uint8_t>& buffer);
        void putField(const std::uint32_t& field, std::vector<std::uint8_t>& buffer);
        void putField(const std::uint16_t& field, std::vector<std::uint8_t>& buffer);
        void putDataDescriptor(CFileZIP::DataDescriptor& entry);
        void putCentralDirectoryFileHeader(CFileZIP::CentralDirectoryFileHeader& entry);
        void putFileHeader(CFileZIP::LocalFileHeader& entry);
        void putEOCentralDirectoryRecord(CFileZIP::EOCentralDirectoryRecord& entry);
 
        void getField(std::uint64_t& field, std::uint8_t *buffer);
        void getField(std::uint32_t& field, std::uint8_t *buffer);
        void getField(std::uint16_t& field, std::uint8_t *buffer);
        void getDataDescriptor(CFileZIP::DataDescriptor& entry);
        void getCentralDirectoryFileHeader(CFileZIP::CentralDirectoryFileHeader& entry);
        void getFileHeader(CFileZIP::LocalFileHeader& entry);
        void getEOCentralDirectoryRecord(CFileZIP::EOCentralDirectoryRecord& entry);
  
        void getZip64EOCentralDirectoryRecord(CFileZIP::Zip64EOCentralDirectoryRecord& entry);
        void zip64EOCentDirRecordLocator(CFileZIP::Zip64EOCentDirRecordLocator& entry);
        void getZip64ExtendedInformationExtraField(Zip64ExtendedInformationExtraField& extendedInfo, std::vector<std::uint8_t> & info);

        void convertModificationDateTime(std::tm& modificationDateTime, std::uint16_t dateWord, std::uint16_t timeWord);
        
        void inflateFile(const std::string& fileNameStr, std::uint64_t fileSize, std::uint32_t& crc);
        void extractFile(const std::string& fileNameStr, std::uint64_t fileSize, std::uint32_t& crc); 
        void deflateFile(const std::string& fileNameStr, std::uint32_t uncompressedSize, std::uint32_t& compressedSize, std::uint32_t& crc);
        void storeFile(const std::string& fileNameStr, std::uint32_t fileLength);
   
        bool fileExists(const std::string& fileNameStr);
        void getFileAttributes(const std::string& fileNameStr, std::uint32_t& attributes);
        void getFileSize(const std::string& fileNameStr, std::uint32_t& fileSize);     
        void getFileModificationDateTime(const std::string& fileNameStr, std::uint16_t& modificationDate, std::uint16_t& modificationTime);
   
        void addFileHeaderAndContents(const std::string& fileNameStr, const std::string& zippedFileNameStr);
        void UpdateCentralDirectory(void);
        
        // =================
        // PRIVATE VARIABLES
        // =================
        
        //
        // ZIP archive status
        //
        
        bool bOpen=false;
        bool bModified=false;
        
        //
        // ZIP archive filename and added contents list
        //

        std::string zipFileNameStr;
        
        //
        // ZIP archive I/O stream
        //
        
        std::fstream zipFileStream;
        
        //
        //  ZIP archive End Of Central Directory record  and  Central Directory
        // 
        
        EOCentralDirectoryRecord zipEOCentralDirectory;
        std::vector<CentralDirectoryFileHeader> zipCentralDirectory;
        
        Zip64EOCentDirRecordLocator zip64EOCentralDirLocator;
        Zip64EOCentralDirectoryRecord zip64EOCentralDirectory;
        
        //
        // Offset in ZIP archive to put next File Header added.
        //
        
        std::uint64_t offsetToNextFileHeader=0;
        
        //
        // Inflate/deflate buffers.
        //
        
        std::vector<uint8_t> zipInBuffer;
        std::vector<uint8_t> zipOutBuffer;
        

    };

} // namespace Antik

#endif /* CFILEZIP_HPP */

