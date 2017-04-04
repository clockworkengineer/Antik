#include "HOST.hpp"
/*
 * File:   CFileZIP.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on January 24, 2017, 2:33 PM
 *
 * Copyright 2016.
 *
 */

//
// Class: CFileZIP
// 
// Description:  Class to create and manipulate ZIP file archives. At present it
// supports archive creation and addition/extraction of files from an existing 
// archives; ZIP64 extensions are also supported. Files are either saved 
// using store (file copy) or deflate compression. Use is made of the stat64 API
// instead of stat for 64 bit files. The current class compiles and works on 
// Linux/CYGWIN and it marks the archives as created on Unix.
//
// Dependencies:   C11++     - Language standard features used.
//                 ziplib    - File compression/decompression
//                 Linux     - stat64 call for file information.
//

// =================
// CLASS DEFINITIONS
// =================

// ====================
// CLASS IMPLEMENTATION
// ====================

#include "CFileZIP.hpp"

//
// C++ STL definitions
//

#include <iostream>
#include <cstring>

//
// Ziplib and Linux stat file interface
//

#include <zlib.h>
#include <sys/stat.h>

// =========
// NAMESPACE
// =========

namespace Antik {

    // ===========================
    // PRIVATE TYPES AND CONSTANTS
    // ===========================

    //
    // ZIP deflate/inflate default buffer size
    //

    const std::uint64_t CFileZIP::kZIPDefaultBufferSize;

    // ==========================
    // PUBLIC TYPES AND CONSTANTS
    // ==========================

    // ========================
    // PRIVATE STATIC VARIABLES
    // ========================

    // =======================
    // PUBLIC STATIC VARIABLES
    // =======================

    // ===============
    // PRIVATE METHODS
    // ===============

    //
    // Convert  ZIP format (MSDOS) based modified date/time to Linux tm format.
    //

    std::tm CFileZIP::convertModificationDateTime(std::uint16_t dateWord, std::uint16_t timeWord) {

        std::time_t rawtime = 0;
        std::tm modificationDateTime;
        std::time(&rawtime);
        std::memcpy(&modificationDateTime, std::localtime(&rawtime), sizeof (std::tm));

        modificationDateTime.tm_sec = (timeWord & 0b11111) >> 2;
        modificationDateTime.tm_min = (timeWord & 0b11111100000) >> 5;
        modificationDateTime.tm_hour = (timeWord & 0b1111100000000000) >> 11;
        modificationDateTime.tm_mday = (dateWord & 0b11111);
        modificationDateTime.tm_mon = ((dateWord & 0b111100000) >> 5) - 1;
        modificationDateTime.tm_year = ((dateWord & 0b1111111000000000) >> 9) + 80;

        mktime(&modificationDateTime);

        return (modificationDateTime);

    }

    //
    // Uncompress ZIP file entry data to file. Note: The files crc32 is calculated 
    // while the data is being inflated and returned.
    //

    std::uint32_t CFileZIP::inflateFile(const std::string& fileNameStr, std::uint64_t fileSize) {

        int inflateResult = Z_OK;
        std::uint64_t inflatedBytes = 0;
        z_stream inlateZIPStream{};
        std::ofstream fileStream(fileNameStr, std::ios::binary | std::ios::trunc);
        std::uint32_t crc;

        if (fileStream.fail()) {
            throw Exception("Could not open destination file for inflate.");
        }

        crc = crc32(0L, Z_NULL, 0);

        if (fileSize == 0) {
            return (crc);
        }

        inflateResult = inflateInit2(&inlateZIPStream, -MAX_WBITS);
        if (inflateResult != Z_OK) {
            throw Exception("inflateInit2() Error = " + std::to_string(inflateResult));
        }

        do {

            this->readZIPFile(this->zipInBuffer, std::min(fileSize, this->zipIOBufferSize));

            if (this->errorInZIPFile()) {
                inflateEnd(&inlateZIPStream);
                throw Exception("Error reading ZIP archive file during inflate.");
            }

            inlateZIPStream.avail_in = this->readCountZIPFile();
            if (inlateZIPStream.avail_in == 0) {
                break;
            }

            inlateZIPStream.next_in = (Bytef *) & this->zipInBuffer[0];

            do {

                inlateZIPStream.avail_out = this->zipIOBufferSize;
                inlateZIPStream.next_out = (Bytef *) & this->zipOutBuffer[0];

                inflateResult = inflate(&inlateZIPStream, Z_NO_FLUSH);
                switch (inflateResult) {
                    case Z_NEED_DICT:
                        inflateResult = Z_DATA_ERROR;
                    case Z_DATA_ERROR:
                    case Z_MEM_ERROR:
                        inflateEnd(&inlateZIPStream);
                        throw Exception("Error inflating ZIP archive. = " + std::to_string(inflateResult));
                }

                inflatedBytes = this->zipIOBufferSize - inlateZIPStream.avail_out;
                fileStream.write((char *) & this->zipOutBuffer[0], inflatedBytes);
                if (fileStream.fail()) {
                    inflateEnd(&inlateZIPStream);
                    throw Exception("Error writing to file during inflate.");
                }

                crc = crc32(crc, &this->zipOutBuffer[0], inflatedBytes);

            } while (inlateZIPStream.avail_out == 0);

            fileSize -= this->zipIOBufferSize;

        } while (inflateResult != Z_STREAM_END);

        inflateEnd(&inlateZIPStream);

        return (crc);

    }

    //
    // Compress source file and write as part of ZIP file header record. The files 
    // crc32 is calculated  while the data is being deflated and returned. 
    // The files compressed size if also calculated and returned though a reference 
    // parameter.
    //

    std::pair<std::uint32_t, std::uint64_t> CFileZIP::deflateFile(const std::string& fileNameStr, std::uint64_t fileSize) {

        int deflateResult = 0, flushRemainder = 0;
        std::uint64_t bytesDeflated = 0;
        z_stream deflateZIPStream{};
        std::ifstream fileStream(fileNameStr, std::ios::binary);
        std::uint32_t crc;
        std::uint64_t compressedSize=0;

        if (fileStream.fail()) {
            throw Exception("Could not open source file for deflate.");
        }

        crc = crc32(0L, Z_NULL, 0);

        deflateResult = deflateInit2(&deflateZIPStream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
        if (deflateResult != Z_OK) {
            throw Exception("deflateInit2() Error = " + std::to_string(deflateResult));
        }

        do {

            fileStream.read((char *) & this->zipInBuffer[0], std::min(fileSize, this->zipIOBufferSize));
            if (fileStream.fail() && !fileStream.eof()) {
                deflateEnd(&deflateZIPStream);
                throw Exception("Error reading source file to deflate.");
            }

            deflateZIPStream.avail_in = fileStream.gcount();
            fileSize -= deflateZIPStream.avail_in;

            crc = crc32(crc, &this->zipInBuffer[0], deflateZIPStream.avail_in);

            flushRemainder = ((fileStream.eof() || fileSize == 0)) ? Z_FINISH : Z_NO_FLUSH;

            deflateZIPStream.next_in = &this->zipInBuffer[0];

            do {

                deflateZIPStream.avail_out = this->zipIOBufferSize;
                deflateZIPStream.next_out = &this->zipOutBuffer[0];
                deflateResult = deflate(&deflateZIPStream, flushRemainder); /* no bad return value */

                bytesDeflated = this->zipIOBufferSize - deflateZIPStream.avail_out;
                this->writeZIPFile(this->zipOutBuffer, bytesDeflated);
                if (this->errorInZIPFile()) {
                    deflateEnd(&deflateZIPStream);
                    throw Exception("Error writing deflated data to ZIP archive.");
                }

                compressedSize += bytesDeflated;

            } while (deflateZIPStream.avail_out == 0);


        } while (flushRemainder != Z_FINISH);

        deflateEnd(&deflateZIPStream);

        fileStream.close();

        return (std::make_pair(crc, compressedSize));

    }

    //
    // Extract uncompressed (stored) ZIP file entry data to file. Note: The files 
    // crc32 is calculated while the data being is copied and returned.
    //

    std::uint32_t CFileZIP::extractFile(const std::string& fileNameStr, std::uint64_t fileSize) {

        std::uint32_t crc;
        crc = crc32(0L, Z_NULL, 0);
        std::ofstream fileStream(fileNameStr, std::ios::binary | std::ios::trunc);

        if (fileStream.fail()) {
            throw Exception("Could not open destination file for extract.");
        }

        while (fileSize) {
            this->readZIPFile(this->zipInBuffer, std::min(fileSize, this->zipIOBufferSize));
            if (this->errorInZIPFile()) {
                throw Exception("Error in reading ZIP archive file.");
            }
            crc = crc32(crc, &this->zipInBuffer[0], this->readCountZIPFile());
            fileStream.write((char *) & this->zipInBuffer[0], this->readCountZIPFile());
            if (fileStream.fail()) {
                throw Exception("Error in writing extracted file.");
            }
            fileSize -= (std::min(fileSize, this->zipIOBufferSize));

        }

        return (crc);

    }

    //
    // Store file as part of ZIP file header.
    //

    void CFileZIP::storeFile(const std::string& fileNameStr, std::uint64_t fileSize) {

        std::ifstream fileStream(fileNameStr, std::ios::binary);

        if (fileStream.fail()) {
            throw Exception("Could not open source file for store.");
        }

        while (fileSize) {

            fileStream.read((char *) & this->zipInBuffer[0], std::min(fileSize, this->zipIOBufferSize));
            if (fileStream.fail()) {
                throw Exception("Error reading source file to store in ZIP archive.");
            }

            this->writeZIPFile(this->zipInBuffer, fileStream.gcount());
            if (this->errorInZIPFile()) {
                throw Exception("Error writing to ZIP archive.");
            }

            fileSize -= (std::min(fileSize, this->zipIOBufferSize));

        }

    }

    //
    // Get file Linux attributes. To convert to ZIP file  format just shift 16
    // bits left.
    //

    std::uint32_t CFileZIP::getFileAttributes(const std::string& fileNameStr) {

        struct stat64 fileStat {
        };
        std::uint32_t attributes = 0;

        int rc = lstat64(fileNameStr.c_str(), &fileStat);

        if (rc == 0) {
            attributes = fileStat.st_mode;
            attributes <<= 16;
        } else {
            throw Exception("stat() error getting file attributes. ERRNO = " + std::to_string(errno));
        }

        return (attributes);

    }

    //
    // Get a files size. Directories have size 0.
    //

    std::uint64_t CFileZIP::getFileSize(const std::string& fileNameStr) {

        struct stat64 fileStat {
        };
        std::uint64_t fileSize = 0;

        int rc = lstat64(fileNameStr.c_str(), &fileStat);
        if (rc == 0) {
            if (S_ISDIR(fileStat.st_mode)) {
                fileSize = 0;
            } else {
                fileSize = fileStat.st_size;
            }
        } else {
            throw Exception("stat() error getting file size. ERRNO = " + std::to_string(errno));
        }

        return (fileSize);

    }

    //
    //Return true if a files exists.
    //

    bool CFileZIP::fileExists(const std::string& fileNameStr) {

        struct stat64 fileStat {
        };

        int rc = lstat64(fileNameStr.c_str(), &fileStat);
        if (rc != 0) {
            throw Exception("stat() error getting file size. ERRNO = " + std::to_string(errno));
        }
        return (rc == 0);

    }

    //
    // Get files stat based modified date/time and convert to ZIP format. The values
    // are passed back through a pair.
    //

    std::pair<std::uint16_t, std::uint16_t> CFileZIP::getFileModificationDateTime(const std::string& fileNameStr) {

        struct stat64 fileStat {};
        std::uint16_t modificatioDate, modificationTime;

        int rc = lstat64(fileNameStr.c_str(), &fileStat);
        if (rc == 0) {
            struct std::tm * fileTimeInfo = std::localtime(&fileStat.st_mtime);
            modificationTime = (fileTimeInfo->tm_sec & 0b11111) |
                    ((fileTimeInfo->tm_min & 0b111111) << 5) |
                    ((fileTimeInfo->tm_hour & 0b11111) << 11);
            modificatioDate = (fileTimeInfo->tm_mday & 0b11111) |
                    ((((fileTimeInfo->tm_mon + 1) & 0b1111)) << 5) |
                    (((fileTimeInfo->tm_year - 80)& 0b1111111) << 9);
        } else {
            throw Exception("stat() error getting file modified time. ERRNO = " + std::to_string(errno));
        }

        return(std::make_pair(modificatioDate, modificationTime));
        
    }

    //
    // Add a Local File Header record and file contents to ZIP file. Note: Also add 
    // an entry to central directory for flushing out to the archive on close. Any files
    // that are > 4GB are stored in ZIP64 format.
    //

    void CFileZIP::addFileHeaderAndContents(const std::string& fileNameStr, const std::string& zippedFileNameStr) {

        LocalFileHeader fileHeader;
        CentralDirectoryFileHeader directoryEntry;
        Zip64ExtendedInformationExtraField info;
        bool bZIP64 = false;

        // Save filename details

        directoryEntry.fileNameStr = zippedFileNameStr;
        directoryEntry.fileNameLength = directoryEntry.fileNameStr.length();

        // If current offset > 32 bits use ZIP64

        if (this->fieldRequires64bits(this->offsetToEndOfLocalFileHeaders)) {
            directoryEntry.fileHeaderOffset = static_cast<std::uint32_t> (~0);
            info.fileHeaderOffset = this->offsetToEndOfLocalFileHeaders;
            bZIP64 = true;
        } else {
            directoryEntry.fileHeaderOffset = this->offsetToEndOfLocalFileHeaders;
        }

        // File size > 32 bits then use ZIP64

        info.originalSize = getFileSize(fileNameStr);
        if (this->fieldRequires64bits(info.originalSize)) {
            info.compressedSize = info.originalSize;
            directoryEntry.uncompressedSize = static_cast<std::uint32_t> (~0);
            directoryEntry.compressedSize = static_cast<std::uint32_t> (~0);
            bZIP64 = true;
        } else {
            directoryEntry.uncompressedSize = info.originalSize;
        }

        // Get file modified time and attributes.

        std::pair<std::uint16_t, std::uint16_t> modification = getFileModificationDateTime(fileNameStr);
       
        directoryEntry.modificationDate = modification.first;
        directoryEntry.modificationTime =  modification.second;
        
        directoryEntry.externalFileAttrib = getFileAttributes(fileNameStr);

        // File is a directory so add trailing delimeter, set no compression and extractor version  1.0

        if (S_ISDIR(directoryEntry.externalFileAttrib >> 16)) {
            if (directoryEntry.fileNameStr.back() != '/') {
                directoryEntry.fileNameStr.push_back('/');
                directoryEntry.fileNameLength++;
            }
            directoryEntry.extractorVersion = CFileZIP::kZIPVersion10;
            directoryEntry.creatorVersion = (CFileZIP::kZIPCreatorUnix<<8)|CFileZIP::kZIPVersion10;
            directoryEntry.compression = CFileZIP::kZIPCompressionStore;
        }

        // > 4 GB Files so ZIP64. Values not able to be stored in 32 bits have
        // there fields set to all ones and values placed in the extended 
        // information field where their format has more bits.

        if (bZIP64) {
            this->bZIP64 = true;
            directoryEntry.extractorVersion = CFileZIP::kZIPVersion45;
            directoryEntry.creatorVersion = (CFileZIP::kZIPCreatorUnix << 8) | (CFileZIP::kZIPVersion45);
            this->putZip64ExtendedInformationExtraField(info, directoryEntry.extraField);
            directoryEntry.extraFieldLength = directoryEntry.extraField.size();
        }

        // Copy information for file header and write to disk

        fileHeader.creatorVersion = directoryEntry.creatorVersion;
        fileHeader.bitFlag = directoryEntry.bitFlag;
        fileHeader.compression = directoryEntry.compression;
        fileHeader.modificationTime = directoryEntry.modificationTime;
        fileHeader.modificationDate = directoryEntry.modificationDate;
        fileHeader.uncompressedSize = directoryEntry.uncompressedSize;
        fileHeader.compressedSize = directoryEntry.compressedSize;
        fileHeader.fileNameLength = directoryEntry.fileNameLength;
        fileHeader.extraFieldLength = directoryEntry.extraFieldLength;
        fileHeader.fileNameStr = directoryEntry.fileNameStr;
        fileHeader.extraField = directoryEntry.extraField;

        this->positionInZIPFile(this->offsetToEndOfLocalFileHeaders);
        this->putFileHeader(fileHeader);

        // Write any file contents next

        if (directoryEntry.uncompressedSize) {

            std::uint64_t compressedSize = 0, uncompressedSize;

            if (bZIP64) {
                uncompressedSize = info.originalSize;
            } else {
                uncompressedSize = directoryEntry.uncompressedSize;
            }

            // Calculate files compressed size while deflating it and then either modify its
            // Local File Header record to have the correct compressed size and CRC or if its 
            // compressed size is greater then or equal to its original size then store file 
            // instead of compress.

            std::pair<std::uint32_t, std::int64_t> deflateValues = this->deflateFile(fileNameStr, uncompressedSize);
            directoryEntry.crc32 = deflateValues.first;
            compressedSize = deflateValues.second;

            fileHeader.crc32 = directoryEntry.crc32;

            // Save away current position for next file

            this->offsetToEndOfLocalFileHeaders = this->currentPositionZIPFile();

            // Back up to beginning of Local file header

            if (this->fieldOverflow(directoryEntry.fileHeaderOffset)) {
                this->positionInZIPFile(info.fileHeaderOffset);
            } else {
                this->positionInZIPFile(directoryEntry.fileHeaderOffset);
            }

            // Rewrite local file header with compressed size if compressed file
            // smaller or if ZIP64 format.

            if ((compressedSize < uncompressedSize) || bZIP64) {
                if (bZIP64) {
                    info.compressedSize = compressedSize;
                    this->putZip64ExtendedInformationExtraField(info, directoryEntry.extraField);
                    fileHeader.extraField = directoryEntry.extraField;
                } else {
                    fileHeader.compressedSize = directoryEntry.compressedSize = compressedSize;
                }
                this->putFileHeader(fileHeader);
            } else {
                // Store non-compressed file.
                directoryEntry.extractorVersion = CFileZIP::kZIPVersion10;
                fileHeader.creatorVersion = (CFileZIP::kZIPCreatorUnix<<8)|CFileZIP::kZIPVersion10;
                fileHeader.compression = directoryEntry.compression = CFileZIP::kZIPCompressionStore;
                fileHeader.compressedSize = directoryEntry.compressedSize = directoryEntry.uncompressedSize;
                this->putFileHeader(fileHeader);
                this->storeFile(fileNameStr, uncompressedSize);
                this->offsetToEndOfLocalFileHeaders = this->currentPositionZIPFile();
            }

        } else {
            this->offsetToEndOfLocalFileHeaders = this->currentPositionZIPFile();
        }

        // Save Central Directory File Entry

        this->zipCentralDirectory.push_back(directoryEntry);

        this->bModified = true;

    }

    //
    // Update a ZIP archives Central Directory.
    //

    void CFileZIP::UpdateCentralDirectory(void) {

        if (this->bModified) {

            EOCentralDirectoryRecord zipEOCentralDirectory;
            Zip64EOCentralDirectoryRecord zip64EOCentralDirectory;           
            bool bZIP64 = false;

            // Position to end of local file headers
            
            this->positionInZIPFile(this->offsetToEndOfLocalFileHeaders);

            // Initalise central directory offset and size
            
            zip64EOCentralDirectory.numberOfCentralDirRecords = this->zipCentralDirectory.size();
            zip64EOCentralDirectory.totalCentralDirRecords = this->zipCentralDirectory.size();
            zip64EOCentralDirectory.offsetCentralDirRecords = this->currentPositionZIPFile();

            // Write Central Directory to ZIP archive
            
            for (auto& directoryEntry : this->zipCentralDirectory) {
                this->putCentralDirectoryFileHeader(directoryEntry);
            }

            // Calculate Central Directory size in byes.
            
            zip64EOCentralDirectory.sizeOfCentralDirRecords = this->currentPositionZIPFile();
            zip64EOCentralDirectory.sizeOfCentralDirRecords -= zip64EOCentralDirectory.offsetCentralDirRecords;

            // Number of records 16 bit overflow so use ZIP64 ie. 32 bits
            
            if (this->fieldRequires32bits(zip64EOCentralDirectory.numberOfCentralDirRecords)) {
                zipEOCentralDirectory.numberOfCentralDirRecords = static_cast<std::uint16_t> (~0);
                bZIP64 = true;
            } else {
                zipEOCentralDirectory.numberOfCentralDirRecords = zip64EOCentralDirectory.numberOfCentralDirRecords;
            }

            // Total number of records 16 bit overflow so use ZIP64 ie. 32 bits
            
            if (this->fieldRequires32bits(zip64EOCentralDirectory.totalCentralDirRecords)) {
                zipEOCentralDirectory.totalCentralDirRecords = static_cast<std::uint16_t> (~0);
                bZIP64 = true;
            } else {
                zipEOCentralDirectory.totalCentralDirRecords = zip64EOCentralDirectory.totalCentralDirRecords;
            }

            // Offset 32 bit overflow so use ZIP64 ie. 64 bits
            
            if (this->fieldRequires64bits(zip64EOCentralDirectory.offsetCentralDirRecords)) {
                zipEOCentralDirectory.offsetCentralDirRecords = static_cast<std::uint32_t> (~0);
                bZIP64 = true;
            } else {
                zipEOCentralDirectory.offsetCentralDirRecords = zip64EOCentralDirectory.offsetCentralDirRecords;
            }

            // Central Directory size 32 bit overflow so use ZIP64 ie. 64 bits
            
            if (this->fieldRequires64bits(zip64EOCentralDirectory.sizeOfCentralDirRecords)) {
                zipEOCentralDirectory.sizeOfCentralDirRecords = static_cast<std::uint32_t> (~0);
                bZIP64 = true;
            } else {
                zipEOCentralDirectory.sizeOfCentralDirRecords = zip64EOCentralDirectory.sizeOfCentralDirRecords;
            }
            
            // ZIP64 so write extension records
            
            if (bZIP64) {
                Zip64EOCentDirRecordLocator locator;
                locator.offset = this->currentPositionZIPFile();
                this->putZip64EOCentralDirectoryRecord(zip64EOCentralDirectory);
                this->putZip64EOCentDirRecordLocator(locator);
            }

            // Write End Of Central Directory record
            
            this->putEOCentralDirectoryRecord(zipEOCentralDirectory);

        }

    }

    // ==============
    // PUBLIC METHODS
    // ==============

    //
    // Constructor
    //

    CFileZIP::CFileZIP(const std::string& zipFileNameStr) : zipFileNameStr{zipFileNameStr}
    {

        this->zipInBuffer.resize(this->zipIOBufferSize);
        this->zipOutBuffer.resize(this->zipIOBufferSize);

    }

    //
    // Destructor
    //

    CFileZIP::~CFileZIP() {

    }

    //
    // Set ZIP archive name
    //

    void CFileZIP::name(const std::string& zipFileNameStr) {

        this->zipFileNameStr = zipFileNameStr;

    }

    //
    // Open ZIP archive and read in Central Directory Header records.
    //

    void CFileZIP::open(void) {

        if (this->bOpen) {
            throw Exception("ZIP archive has already been opened.");
        }

        EOCentralDirectoryRecord zipEOCentralDirectory;
        Zip64EOCentralDirectoryRecord zip64EOCentralDirectory;

        this->openZIPFile(this->zipFileNameStr, std::ios::binary | std::ios_base::in | std::ios_base::out);

        std::int64_t noOfFileRecords = 0;

        this->getEOCentralDirectoryRecord(zipEOCentralDirectory);

        // If one of the central directory fields is to large to store so ZIP64

        if (this->fieldOverflow(zipEOCentralDirectory.totalCentralDirRecords) ||
                this->fieldOverflow(zipEOCentralDirectory.numberOfCentralDirRecords) ||
                this->fieldOverflow(zipEOCentralDirectory.sizeOfCentralDirRecords) ||
                this->fieldOverflow(zipEOCentralDirectory.totalCentralDirRecords) ||
                this->fieldOverflow(zipEOCentralDirectory.startDiskNumber) ||
                this->fieldOverflow(zipEOCentralDirectory.diskNumber) ||
                this->fieldOverflow(zipEOCentralDirectory.offsetCentralDirRecords)) {

            this->bZIP64 = true;
            this->getZip64EOCentralDirectoryRecord(zip64EOCentralDirectory);
            this->positionInZIPFile(zip64EOCentralDirectory.offsetCentralDirRecords);
            noOfFileRecords = zip64EOCentralDirectory.numberOfCentralDirRecords;
            this->offsetToEndOfLocalFileHeaders = zip64EOCentralDirectory.offsetCentralDirRecords;

        } else {
            this->positionInZIPFile(zipEOCentralDirectory.offsetCentralDirRecords);
            noOfFileRecords = zipEOCentralDirectory.numberOfCentralDirRecords;
            this->offsetToEndOfLocalFileHeaders = zipEOCentralDirectory.offsetCentralDirRecords;
        }

        // Read in Central File Directory

        for (auto cnt01 = 0; cnt01 < noOfFileRecords; cnt01++) {
            CFileZIP::CentralDirectoryFileHeader directoryEntry;
            this->getCentralDirectoryFileHeader(directoryEntry);
            this->zipCentralDirectory.push_back(directoryEntry);
            this->bZIP64 = this->fieldOverflow(directoryEntry.compressedSize) ||
                    this->fieldOverflow(directoryEntry.uncompressedSize) ||
                    this->fieldOverflow(directoryEntry.fileHeaderOffset);
        }

        this->bOpen = true;

    }

    //
    // Read Central Directory and return a list of ZIP archive contents.
    //

    std::vector<CFileZIP::FileDetail> CFileZIP::contents(void) {

        FileDetail fileEntry;
        std::vector<CFileZIP::FileDetail> fileDetailList;

        if (!this->bOpen) {
            throw Exception("ZIP archive has not been opened.");
        }

        for (auto directoryEntry : this->zipCentralDirectory) {

            fileEntry.fileNameStr = directoryEntry.fileNameStr;
            fileEntry.fileCommentStr = directoryEntry.fileCommentStr;
            fileEntry.uncompressedSize = directoryEntry.uncompressedSize;
            fileEntry.compressedSize = directoryEntry.compressedSize;
            fileEntry.compression = directoryEntry.compression;
            fileEntry.externalFileAttrib = directoryEntry.externalFileAttrib;
            fileEntry.creatorVersion = directoryEntry.creatorVersion;
            fileEntry.extraField = directoryEntry.extraField;
            fileEntry.modificationDateTime =
                    this->convertModificationDateTime(directoryEntry.modificationDate,
                    directoryEntry.modificationTime);

            // File size information stored in Extended information.

            if (this->fieldOverflow(directoryEntry.compressedSize) ||
                    this->fieldOverflow(directoryEntry.uncompressedSize) ||
                    this->fieldOverflow(directoryEntry.fileHeaderOffset)) {

                Zip64ExtendedInformationExtraField extra;
                extra.compressedSize = directoryEntry.compressedSize;
                extra.fileHeaderOffset = directoryEntry.fileHeaderOffset;
                extra.originalSize = directoryEntry.uncompressedSize;
                this->getZip64ExtendedInformationExtraField(extra, fileEntry.extraField);
                fileEntry.uncompressedSize = extra.originalSize;
                fileEntry.compressedSize = extra.compressedSize;
                fileEntry.bZIP64 = true;

            }

            fileDetailList.push_back(fileEntry);
        }

        return (fileDetailList);

    }

    //
    // Extract a ZIP archive file and create in a specified destination.
    //

    bool CFileZIP::extract(const std::string& fileNameStr, const std::string& destFileNameStr) {

        bool fileExtracted = false;

        if (!this->bOpen) {
            throw Exception("ZIP archive has not been opened.");
        }

        for (auto directoryEntry : this->zipCentralDirectory) {

            if (directoryEntry.fileNameStr.compare(fileNameStr) == 0) {

                Zip64ExtendedInformationExtraField extendedInfo;
                LocalFileHeader fileHeader;
                std::uint32_t crc32;

                // Set up 64 bit data values if needed

                extendedInfo.compressedSize = directoryEntry.compressedSize;
                extendedInfo.originalSize = directoryEntry.uncompressedSize;
                extendedInfo.fileHeaderOffset = directoryEntry.fileHeaderOffset;

                // If dealing with ZIP64 extract full 64 bit values from extended field

                if (this->fieldOverflow(directoryEntry.compressedSize) ||
                        this->fieldOverflow(directoryEntry.uncompressedSize) ||
                        this->fieldOverflow(directoryEntry.fileHeaderOffset)) {
                    getZip64ExtendedInformationExtraField(extendedInfo, directoryEntry.extraField);
                }

                // Move to and read file header

                this->positionInZIPFile(extendedInfo.fileHeaderOffset);
                this->getLocalFileHeader(fileHeader);

                // Now positioned at file contents so extract

                if (directoryEntry.compression == CFileZIP::kZIPCompressionDeflate) {
                    crc32 = this->inflateFile(destFileNameStr, extendedInfo.compressedSize);
                    fileExtracted = true;
                } else if (directoryEntry.compression == CFileZIP::kZIPCompressionStore) {
                    crc32 = this->extractFile(destFileNameStr, extendedInfo.originalSize);
                    fileExtracted = true;
                } else {
                    throw Exception("File uses unsupported compression = " + std::to_string(directoryEntry.compression));
                }

                // Check file CRC32

                if (crc32 != directoryEntry.crc32) {
                    throw Exception("File " + destFileNameStr + " has an invalid CRC.");
                }

                break;

            }

        }

        return (fileExtracted);

    }

    //
    // Create an empty ZIP archive.
    //

    void CFileZIP::create(void) {

        if (this->bOpen) {
            throw Exception("ZIP archive should not be open.");
        }

        EOCentralDirectoryRecord zipEOCentralDirectory;
    
        this->openZIPFile(this->zipFileNameStr, std::ios::binary | std::ios_base::in | std::ios_base::out | std::ios_base::trunc);

        this->putEOCentralDirectoryRecord(zipEOCentralDirectory);

        this->closeZIPFile();

    }

    //
    // Close ZIP archive
    //

    void CFileZIP::close(void) {

        if (!this->bOpen) {
            throw Exception("ZIP archive has not been opened.");
        }

        this->UpdateCentralDirectory();

        this->zipCentralDirectory.clear();

        this->offsetToEndOfLocalFileHeaders = 0;

        this->closeZIPFile();

        this->bOpen = false;
        this->bModified = false;
        this->bZIP64 = false;

    }

    //
    // Add file to ZIP archive.
    //

    bool CFileZIP::add(const std::string& fileNameStr, const std::string& zippedFileNameStr) {

        if (!this->bOpen) {
            throw Exception("ZIP archive has not been opened.");
        }

        // Check that an entry does not already exist

        for (auto& directoryEntry : this->zipCentralDirectory) {
            if (directoryEntry.fileNameStr.compare(zippedFileNameStr) == 0) {
                std::cerr << "File already present in archive [" << zippedFileNameStr << "]" << std::endl;
                return (false);
            }
        }

        // Add file if it exists

        if (this->fileExists(fileNameStr)) {
            this->addFileHeaderAndContents(fileNameStr, zippedFileNameStr);
            return (true);

        } else {
            std::cerr << "File does not exist [" << fileNameStr << "]" << std::endl;
        }

        return (false);

    }

    //
    // If a archive file entry is a directory return true
    //

    bool CFileZIP::isDirectory(const CFileZIP::FileDetail& fileEntry) {

        return ((fileEntry.externalFileAttrib & 0x10) ||
                (S_ISDIR(fileEntry.externalFileAttrib >> 16)));

    }

    //
    // If a ZIP64 archive return true. Note if any part of an archive contains
    // ZIP64 format entry then this will be true.
    //

    bool CFileZIP::isZIP64(void) {

        return (this->bZIP64);

    }

    //
    // Set ZIP I/O buffer size.
    //

    void CFileZIP::setZIPBufferSize(std::uint64_t newBufferSize) {

        this->zipIOBufferSize = newBufferSize;
        this->zipInBuffer.resize(this->zipIOBufferSize);
        this->zipOutBuffer.resize(this->zipIOBufferSize);

    }


} // namespace Antik