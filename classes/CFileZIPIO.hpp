/*
 * File:   CFileZIPIOIO.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on January 24, 2017, 2:33 PM
 *
 * Copyright 2016.
 *
 */

#ifndef CFILEZIPIO_HPP
#define CFILEZIPIO_HPP

//
// C++ STL definitions
//

#include <string>
#include <vector>
#include <stdexcept>
#include <fstream>

// =========
// NAMESPACE
// =========

namespace Antik {

    // ================
    // CLASS DEFINITION
    // ================

    class CFileZIPIO {
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
            : std::runtime_error("CFileZIPIO Failure: " + messageStr) {
            }

        };

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
            const std::uint32_t size = 56;
            const std::uint32_t signature = 0x06064b50;
            std::uint64_t  totalRecordSize = 0;
            std::uint16_t creatorVersion = 0x0314;
            std::uint16_t extractorVersion = 0x0014;
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

        // ============
        // CONSTRUCTORS
        // ============

        CFileZIPIO();

        // ==========
        // DESTRUCTOR
        // ==========

        virtual ~CFileZIPIO();

        // ==============
        // PUBLIC METHODS
        // ==============
 
        bool fieldOverflow(const std::uint64_t& field);
        bool fieldOverflow(const std::uint32_t& field);
        bool fieldOverflow(const std::uint16_t& field);
  
        void putField(const std::uint64_t& field, std::vector<std::uint8_t>& buffer);
        void putField(const std::uint32_t& field, std::vector<std::uint8_t>& buffer);
        void putField(const std::uint16_t& field, std::vector<std::uint8_t>& buffer);
        void putDataDescriptor(CFileZIPIO::DataDescriptor& entry);
        void putCentralDirectoryFileHeader(CFileZIPIO::CentralDirectoryFileHeader& entry);
        void putFileHeader(CFileZIPIO::LocalFileHeader& entry);
        void putEOCentralDirectoryRecord(CFileZIPIO::EOCentralDirectoryRecord& entry);

        void putZip64ExtendedInformationExtraField(Zip64ExtendedInformationExtraField& extendedInfo, std::vector<std::uint8_t>& info);
        void putZip64EOCentralDirectoryRecord(Zip64EOCentralDirectoryRecord& entry);
        void putZip64EOCentDirRecordLocator(Zip64EOCentDirRecordLocator& entry);
      
        void getField(std::uint64_t& field, std::uint8_t *buffer);
        void getField(std::uint32_t& field, std::uint8_t *buffer);
        void getField(std::uint16_t& field, std::uint8_t *buffer);
        void getDataDescriptor(CFileZIPIO::DataDescriptor& entry);
        void getCentralDirectoryFileHeader(CFileZIPIO::CentralDirectoryFileHeader& entry);
        void getLocalFileHeader(CFileZIPIO::LocalFileHeader& entry);
        void getEOCentralDirectoryRecord(CFileZIPIO::EOCentralDirectoryRecord& entry);
  
        void getZip64EOCentralDirectoryRecord(CFileZIPIO::Zip64EOCentralDirectoryRecord& entry);
        void getZip64EOCentDirRecordLocator(CFileZIPIO::Zip64EOCentDirRecordLocator& entry);
        void getZip64ExtendedInformationExtraField(Zip64ExtendedInformationExtraField& extendedInfo, std::vector<std::uint8_t> & info);

        void openZIPFile(const std::string fileNameStr, std::ios_base::openmode mode);
        void closeZIPFile(void);
        void positionInZIPFile(std::uint64_t offset);
        std::uint64_t currentPositionZIPFile(void);
        void writeZIPFile(std::vector<std::uint8_t>& buffer, std::uint64_t count);
        void readZIPFile(std::vector<std::uint8_t>& buffer, std::uint64_t count);
        std::uint64_t readCountZIPFile(void);
        bool errorInZIPFile(void);
        
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

        CFileZIPIO(const CFileZIPIO & orig) = delete;
        CFileZIPIO(const CFileZIPIO && orig) = delete;
        CFileZIPIO& operator=(CFileZIPIO other) = delete;

        // ===============
        // PRIVATE METHODS
        // ===============

        // =================
        // PRIVATE VARIABLES
        // =================
          
        //
        // ZIP archive I/O stream
        //
        
        std::fstream zipFileStream;
 
    };

} // namespace Antik

#endif /* CFILEZIP_HPP */

