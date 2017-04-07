/*
 * File:   CZIPIO.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on January 24, 2017, 2:33 PM
 *
 * Copyright 2016.
 *
 */

#ifndef CZIPIO_HPP
#define CZIPIO_HPP

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
    namespace File {
        
    // ================
    // CLASS DEFINITION
    // ================

    class CZIPIO {
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
        // ZIP archive compression methods.
        //
 
        static const std::uint16_t kZIPCompressionStore = 0;
        static const std::uint16_t kZIPCompressionDeflate = 8;
        
        //
        // ZIP archive versions
        //
        
        static const std::uint8_t kZIPVersion10 = 0x0a;
        static const std::uint8_t kZIPVersion20 = 0x14;
        static const std::uint8_t kZIPVersion45 = 0x2d;
        
        //
        // ZIP archive creator
        //
   
        static const std::uint8_t kZIPCreatorUnix = 0x03;

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
            std::uint16_t creatorVersion = (kZIPCreatorUnix<<8)|kZIPVersion20;
            std::uint16_t extractorVersion = kZIPVersion20;
            std::uint16_t bitFlag = 0;
            std::uint16_t compression = kZIPCompressionDeflate;
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
            std::string commentStr;
        };
              
        //
        // ZIP64 Archive End Of Central Directory record.
        //

        struct Zip64EOCentralDirectoryRecord {
            const std::uint32_t size = 56;
            const std::uint32_t signature = 0x06064b50;
            std::uint64_t  totalRecordSize = 0;
            std::uint16_t creatorVersion = (kZIPCreatorUnix<<8)|kZIPVersion45;
            std::uint16_t extractorVersion = kZIPVersion45;
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

        struct Zip64ExtendedInfoExtraField {
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

        CZIPIO();

        // ==========
        // DESTRUCTOR
        // ==========

        virtual ~CZIPIO();

        // ==============
        // PUBLIC METHODS
        // ==============
 
        template <typename T> static bool fieldOverflow(const T& field);
        
        static bool fieldRequires64bits(std::uint64_t field) { return(field & 0xFFFFFFFF00000000); };
        static bool fieldRequires32bits(std::uint32_t field) { return(field & 0xFFFF0000); };
        
        void putDataDescriptor(CZIPIO::DataDescriptor& entry);
        void putCentralDirectoryFileHeader(CZIPIO::CentralDirectoryFileHeader& entry);
        void putFileHeader(CZIPIO::LocalFileHeader& entry);
        void putEOCentralDirectoryRecord(CZIPIO::EOCentralDirectoryRecord& entry);
        void putZip64EOCentralDirectoryRecord(Zip64EOCentralDirectoryRecord& entry);
        void putZip64EOCentDirRecordLocator(Zip64EOCentDirRecordLocator& entry);
        static void putZip64ExtendedInfoExtraField(Zip64ExtendedInfoExtraField& extendedInfo, std::vector<std::uint8_t>& info);
        
        void getDataDescriptor(CZIPIO::DataDescriptor& entry);
        void getCentralDirectoryFileHeader(CZIPIO::CentralDirectoryFileHeader& entry);
        void getLocalFileHeader(CZIPIO::LocalFileHeader& entry);
        void getEOCentralDirectoryRecord(CZIPIO::EOCentralDirectoryRecord& entry);
        void getZip64EOCentralDirectoryRecord(CZIPIO::Zip64EOCentralDirectoryRecord& entry);
        void getZip64EOCentDirRecordLocator(CZIPIO::Zip64EOCentDirRecordLocator& entry);
        static void getZip64ExtendedInfoExtraField(Zip64ExtendedInfoExtraField& extendedInfo, std::vector<std::uint8_t> & info);

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

        CZIPIO(const CZIPIO & orig) = delete;
        CZIPIO(const CZIPIO && orig) = delete;
        CZIPIO& operator=(CZIPIO other) = delete;

        // ===============
        // PRIVATE METHODS
        // ===============

        template <typename T> static void putField(T field, std::vector<std::uint8_t>& buffer);
        template <typename T> static std::uint8_t *getField(T& field, std::uint8_t* buffptr);
 
        static void readDataDescriptor(std::fstream &zipFileStream, DataDescriptor& entry);
        static void readCentralDirectoryFileHeader(std::fstream &zipFileStream, CentralDirectoryFileHeader& entry);
        static void readLocalFileHeader(std::fstream &zipFileStream, LocalFileHeader& entry);
        static void readEOCentralDirectoryRecord(std::fstream &zipFileStream, EOCentralDirectoryRecord& entry);
        static void readZip64EOCentralDirectoryRecord(std::fstream &zipFileStream, Zip64EOCentralDirectoryRecord& entry);
        static void readZip64EOCentDirRecordLocator(std::fstream &zipFileStream, Zip64EOCentDirRecordLocator& entry);

        static void writeDataDescriptor(std::fstream &zipFileStream, DataDescriptor& entry);
        static void writeCentralDirectoryFileHeader(std::fstream &zipFileStream, CentralDirectoryFileHeader& entry);
        static void writeFileHeader(std::fstream &zipFileStream, LocalFileHeader& entry);
        static void writeEOCentralDirectoryRecord(std::fstream &zipFileStream, EOCentralDirectoryRecord& entry);
        static void writeZip64EOCentralDirectoryRecord(std::fstream &zipFileStream, Zip64EOCentralDirectoryRecord& entry);
        static void writeZip64EOCentDirRecordLocator(std::fstream &zipFileStream, Zip64EOCentDirRecordLocator& entry);
        
        // =================
        // PRIVATE VARIABLES
        // =================
          
        //
        // ZIP archive I/O stream
        //
        
        std::fstream zipFileStream;
 
    };

    //
    // Return true if field contains all 1s.
    //
    
    template <typename T>
    bool CZIPIO::fieldOverflow(const T& field) {
        return (field == static_cast<T> (~0));
    }

    //
    // Place a word into buffer.
    //

    template <typename T>
    void CZIPIO::putField(T field, std::vector<std::uint8_t>& buffer) {
        uint16_t size = sizeof (T);
        while (size--) {
            buffer.push_back(static_cast<std::uint8_t> (field & 0xFF));
            field >>= 8;
        }
    }
    
    //
    // Get word from buffer. Incrementing buffptr by the word size after.
    //

    template <typename T>
    std::uint8_t * CZIPIO::getField(T& field, std::uint8_t *buffptr) {
        std::uint16_t size = sizeof (T) - 1;
        field = buffptr[size];
        do {
            field <<= 8;
            field |= buffptr[size - 1];
        } while (--size);
       return(buffptr + sizeof(T));
    }

    } // namespace File
} // namespace Antik

#endif /* CZIPIO_HPP */

