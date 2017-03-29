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
// archives. For ZIP64 format archives only extraction is supported currently. Files
// are either saved using store (file copy) or deflate compression. The current
// class compiles and works on Linux/CYGWIN and it marks the archives as created on
// Unix.
//
// Dependencies:   C11++     - Language standard features used.
//                 ziplib    - File compression/decompression
//                 Linux     - stat call for file information.
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
    // ZIP deflate/inflate buffer size
    //

    const std::uint32_t CFileZIP::kZIPBufferSize;

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
    // Convert  ZIP format based modified date/time to tm format.
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
        
        return(modificationDateTime);

    }

    //
    // Uncompress ZIP file entry data to file. Note: The files crc32 is calculated 
    // while the data is being inflated and returned.
    //

    std::uint32_t CFileZIP::inflateFile(const std::string& fileNameStr, std::uint64_t fileSize) {

        int inflateResult = Z_OK;
        std::uint64_t inflatedBytes = 0;
        z_stream inlateZIPStream{0};
        std::ofstream fileStream(fileNameStr, std::ios::binary | std::ios::trunc);              
        std::uint32_t crc;
        
        if (fileStream.fail()) {
            throw Exception("Could not open destination file for inflate.");
        }

        crc = crc32(0L, Z_NULL, 0);

        if (fileSize == 0) {
            return(crc);
        }

        inflateResult = inflateInit2(&inlateZIPStream, -MAX_WBITS);
        if (inflateResult != Z_OK) {
            throw Exception("inflateInit2() Error = " + std::to_string(inflateResult));
        }

        do {

            this->readZIPFile(this->zipInBuffer, ((CFileZIP::kZIPBufferSize > fileSize) ? fileSize : CFileZIP::kZIPBufferSize));

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

                inlateZIPStream.avail_out = CFileZIP::kZIPBufferSize;
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

                inflatedBytes = CFileZIP::kZIPBufferSize - inlateZIPStream.avail_out;
                fileStream.write((char *) & this->zipOutBuffer[0], inflatedBytes);
                if (fileStream.fail()) {
                    inflateEnd(&inlateZIPStream);
                    throw Exception("Error writing to file during inflate.");
                }

                crc = crc32(crc, &this->zipOutBuffer[0], inflatedBytes);

            } while (inlateZIPStream.avail_out == 0);

            fileSize -= CFileZIP::kZIPBufferSize;

        } while (inflateResult != Z_STREAM_END);

        inflateEnd(&inlateZIPStream);
        
        return(crc);

    }

    //
    // Compress source file and write as part of ZIP file header record. The files 
    // crc32 is calculated  while the data is being deflated and returned. 
    // The files compressed size if also calculated and returned though a reference 
    // parameter.
    //

     std::uint32_t CFileZIP::deflateFile(const std::string& fileNameStr, std::uint32_t uncompressedSize, std::uint32_t& compressedSize) {

        int deflateResult = 0, flushRemainder = 0;
        std::uint32_t bytesDeflated = 0;
        z_stream deflateZIPStream{0};
        std::ifstream fileStream(fileNameStr, std::ios::binary);
        std::uint32_t crc;

        if (fileStream.fail()) {
            throw Exception("Could not open source file for deflate.");
        }

        crc = crc32(0L, Z_NULL, 0);

        deflateResult = deflateInit2(&deflateZIPStream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
        if (deflateResult != Z_OK) {
            throw Exception("deflateInit2() Error = " + std::to_string(deflateResult));
        }

        do {

            fileStream.read((char *) & this->zipInBuffer[0], std::min(uncompressedSize, CFileZIP::kZIPBufferSize));
            if (fileStream.fail() && !fileStream.eof()) {
                deflateEnd(&deflateZIPStream);
                throw Exception("Error reading source file to deflate.");
            }

            deflateZIPStream.avail_in = fileStream.gcount();
            uncompressedSize -= deflateZIPStream.avail_in;

            crc = crc32(crc, &this->zipInBuffer[0], deflateZIPStream.avail_in);

            flushRemainder = ((fileStream.eof() || uncompressedSize == 0)) ? Z_FINISH : Z_NO_FLUSH;

            deflateZIPStream.next_in = &this->zipInBuffer[0];

            do {

                deflateZIPStream.avail_out = CFileZIP::kZIPBufferSize;
                deflateZIPStream.next_out = &this->zipOutBuffer[0];
                deflateResult = deflate(&deflateZIPStream, flushRemainder); /* no bad return value */

                bytesDeflated = CFileZIP::kZIPBufferSize - deflateZIPStream.avail_out;
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
        
        return(crc);

    }

    //
    // Extract uncompressed (stored) ZIP file entry data to file. Note: The files 
    // crc32 is calculated while the data being is copied and returned.
    //

    std::uint32_t  CFileZIP::extractFile(const std::string& fileNameStr, std::uint64_t fileSize) {

        std::uint32_t crc;
        crc = crc32(0L, Z_NULL, 0);
        std::ofstream fileStream(fileNameStr, std::ios::binary | std::ios::trunc);

        if (fileStream.fail()) {
            throw Exception("Could not open destination file for extract.");
        }

        while (fileSize) {
            this->readZIPFile(this->zipInBuffer, std::min(fileSize, static_cast<std::uint64_t> (CFileZIP::kZIPBufferSize)));
            if (this->errorInZIPFile()) {
                throw Exception("Error in reading ZIP archive file.");
            }
            crc = crc32(crc, &this->zipInBuffer[0], this->readCountZIPFile());
            fileStream.write((char *) & this->zipInBuffer[0], this->readCountZIPFile());
            if (fileStream.fail()) {
                throw Exception("Error in writing extracted file.");
            }
            fileSize -= (std::min(fileSize, static_cast<std::uint64_t> (CFileZIP::kZIPBufferSize)));

        }
        
        return(crc);

    }

    //
    // Store file as part of ZIP file header.
    //

    void CFileZIP::storeFile(const std::string& fileNameStr, std::uint32_t fileSize) {

        std::ifstream fileStream(fileNameStr, std::ios::binary);

        if (fileStream.fail()) {
            throw Exception("Could not open source file for store.");
        }

        while (fileSize) {

            fileStream.read((char *) & this->zipInBuffer[0], std::min(fileSize, CFileZIP::kZIPBufferSize));
            if (fileStream.fail()) {
                throw Exception("Error reading source file to store in ZIP archive.");
            }

            this->writeZIPFile(this->zipInBuffer, fileStream.gcount());
            if (this->errorInZIPFile()) {
                throw Exception("Error writing to ZIP archive.");
            }

            fileSize -= (std::min(fileSize, CFileZIP::kZIPBufferSize));

        }

    }

    //
    // Get file attributes.
    //

    std::uint32_t CFileZIP::getFileAttributes(const std::string& fileNameStr) {

        struct stat fileStat {0};
        std::uint32_t attributes=0;

        int rc = stat(fileNameStr.c_str(), &fileStat);

        if (rc == 0) {
            attributes = fileStat.st_mode;
            attributes <<= 16;
        } else {
            throw Exception("stat() error getting file attributes. ERRNO = "+std::to_string(errno));
        }
        
        return(attributes);

    }

    //
    // Get a files size.
    //

    std::uint32_t  CFileZIP::getFileSize(const std::string& fileNameStr) {
        
        struct stat fileStat {0};
        std::uint32_t fileSize=0;
        
        int rc = stat(fileNameStr.c_str(), &fileStat);
        if (rc == 0) {
            if (S_ISDIR(fileStat.st_mode)) {
                fileSize = 0;
            } else {
                fileSize = fileStat.st_size;
            }
        } else {
            throw Exception("stat() error getting file size. ERRNO = "+std::to_string(errno));
        }
        
        return(fileSize);
        
    }

    //
    // Check whether a file exists.
    //

    bool CFileZIP::fileExists(const std::string& fileNameStr) {

        struct stat fileStat {0};

        int rc = stat(fileNameStr.c_str(), &fileStat);
        return (rc == 0);

    }

    //
    // Get files stat based modified date/time and convert to ZIP format. The values
    // are passed back though two reference parameters.
    //

    void CFileZIP::getFileModificationDateTime(const std::string& fileNameStr, std::uint16_t& modificatioDate, std::uint16_t& modificationTime) {

        struct stat fileStat {0};

        int rc = stat(fileNameStr.c_str(), &fileStat);
        if (rc == 0) {
            struct std::tm * fileTimeInfo = std::localtime(&fileStat.st_mtime);
            modificationTime = (fileTimeInfo->tm_sec & 0b11111) |
                    ((fileTimeInfo->tm_min & 0b111111) << 5) |
                    ((fileTimeInfo->tm_hour & 0b11111) << 11);
            modificatioDate = (fileTimeInfo->tm_mday & 0b11111) |
                    ((((fileTimeInfo->tm_mon + 1) & 0b1111)) << 5) |
                    (((fileTimeInfo->tm_year - 80)& 0b1111111) << 9);
        } else {
            throw Exception("stat() error getting file modified time. ERRNO = "+std::to_string(errno));
        }
        
    }

    //
    // Add a Local File Header record and file contents to ZIP file. Note: Also add 
    // an entry to central directory for flushing out to the archive on close.
    //

    void CFileZIP::addFileHeaderAndContents(const std::string& fileNameStr, const std::string& zippedFileNameStr) {

        LocalFileHeader fileHeader;
        CentralDirectoryFileHeader directoryEntry;

        directoryEntry.fileNameStr = zippedFileNameStr;
        directoryEntry.fileNameLength = directoryEntry.fileNameStr.length();
        directoryEntry.fileHeaderOffset = this->offsetToNextFileHeader;

        getFileModificationDateTime(fileNameStr, directoryEntry.modificationDate, directoryEntry.modificationTime);
        directoryEntry.uncompressedSize = getFileSize(fileNameStr);
        directoryEntry.externalFileAttrib = getFileAttributes(fileNameStr);

        // File is a directory so add trailing delimeter, set no compression and extractor version  1.0

        if (S_ISDIR(directoryEntry.externalFileAttrib >> 16)) {
            if (directoryEntry.fileNameStr.back() != '/') {
                directoryEntry.fileNameStr.push_back('/');
                directoryEntry.fileNameLength++;
            }
            directoryEntry.extractorVersion = 0x000a;
            directoryEntry.compression = 0;
        }

        fileHeader.creatorVersion = directoryEntry.creatorVersion;
        fileHeader.bitFlag = directoryEntry.bitFlag;
        fileHeader.compression = directoryEntry.compression;
        fileHeader.modificationTime = directoryEntry.modificationTime;
        fileHeader.modificationDate = directoryEntry.modificationDate;
        fileHeader.uncompressedSize = directoryEntry.uncompressedSize;
        fileHeader.fileNameLength = directoryEntry.fileNameLength;
        fileHeader.extraFieldLength = directoryEntry.extraFieldLength;
        fileHeader.fileNameStr = directoryEntry.fileNameStr;
        fileHeader.extraField = directoryEntry.extraField;

        this->positionInZIPFile(this->offsetToNextFileHeader);
        this->putFileHeader(fileHeader);

        if (directoryEntry.uncompressedSize) {

            // Calculate files compressed size while deflating it and then either modify its
            // Local File Header record to have the correct compressed size and CRC or if its 
            // compressed size is greater then or equal to its original size then store file 
            // instead of compress.

            directoryEntry.crc32 = this->deflateFile(fileNameStr, directoryEntry.uncompressedSize, directoryEntry.compressedSize);

            fileHeader.crc32 = directoryEntry.crc32;

            this->offsetToNextFileHeader = this->currentPositionZIPFile();

            this->positionInZIPFile(directoryEntry.fileHeaderOffset);
            if (directoryEntry.compressedSize < directoryEntry.uncompressedSize) {
                fileHeader.compressedSize = directoryEntry.compressedSize;
                this->putFileHeader(fileHeader);
            } else {
                directoryEntry.extractorVersion = 0x000a;
                fileHeader.compression = directoryEntry.compression = 0;
                fileHeader.compressedSize = directoryEntry.compressedSize = directoryEntry.uncompressedSize;
                this->putFileHeader(fileHeader);
                this->storeFile(fileNameStr, directoryEntry.uncompressedSize);
                this->offsetToNextFileHeader = this->currentPositionZIPFile();
            }

        } else {
            this->offsetToNextFileHeader = this->currentPositionZIPFile();
        }

        this->zipCentralDirectory.push_back(directoryEntry);

        this->bModified = true;

    }

    //
    // Update a ZIP archives Central Directory.
    //

    void CFileZIP::UpdateCentralDirectory(void) {

        if (this->bModified) {

            this->positionInZIPFile(this->offsetToNextFileHeader);

            this->zipEOCentralDirectory.numberOfCentralDirRecords = this->zipCentralDirectory.size();
            this->zipEOCentralDirectory.totalCentralDirRecords = this->zipCentralDirectory.size();
            this->zipEOCentralDirectory.offsetCentralDirRecords = this->currentPositionZIPFile();

            for (auto& directoryEntry : this->zipCentralDirectory) {
                this->putCentralDirectoryFileHeader(directoryEntry);
            }

            this->zipEOCentralDirectory.sizeOfCentralDirRecords = this->currentPositionZIPFile();
            this->zipEOCentralDirectory.sizeOfCentralDirRecords -= this->zipEOCentralDirectory.offsetCentralDirRecords;

            this->putEOCentralDirectoryRecord(this->zipEOCentralDirectory);

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
        this->zipInBuffer.resize(CFileZIP::kZIPBufferSize);
        this->zipOutBuffer.resize(CFileZIP::kZIPBufferSize);
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

        this->openZIPFile(this->zipFileNameStr, std::ios::binary | std::ios_base::in | std::ios_base::out);

        std::int64_t noOfFileRecords = 0;

        this->getEOCentralDirectoryRecord(this->zipEOCentralDirectory);

        // If one of the central directory fields is to large to store so ZIP64

        if (fieldOverflow(this->zipEOCentralDirectory.totalCentralDirRecords) ||
                fieldOverflow(this->zipEOCentralDirectory.numberOfCentralDirRecords) ||
                fieldOverflow(this->zipEOCentralDirectory.sizeOfCentralDirRecords) ||
                fieldOverflow(this->zipEOCentralDirectory.totalCentralDirRecords) ||
                fieldOverflow(this->zipEOCentralDirectory.startDiskNumber) ||
                fieldOverflow(this->zipEOCentralDirectory.diskNumber) ||
                fieldOverflow(this->zipEOCentralDirectory.offsetCentralDirRecords)) {

            this->bZIP64 = true;
            this->getZip64EOCentralDirectoryRecord(this->zip64EOCentralDirectory);
            this->positionInZIPFile(this->zip64EOCentralDirectory.offsetCentralDirRecords);
            noOfFileRecords = this->zip64EOCentralDirectory.numberOfCentralDirRecords;
            this->offsetToNextFileHeader = this->zip64EOCentralDirectory.offsetCentralDirRecords;

        } else {
            this->positionInZIPFile(this->zipEOCentralDirectory.offsetCentralDirRecords);
            noOfFileRecords = this->zipEOCentralDirectory.numberOfCentralDirRecords;
            this->offsetToNextFileHeader = this->zipEOCentralDirectory.offsetCentralDirRecords;
        }

        for (auto cnt01 = 0; cnt01 < noOfFileRecords; cnt01++) {
            CFileZIP::CentralDirectoryFileHeader directoryEntry;
            this->getCentralDirectoryFileHeader(directoryEntry);
            this->zipCentralDirectory.push_back(directoryEntry);
            this->bZIP64 = fieldOverflow(directoryEntry.compressedSize) ||
                    fieldOverflow(directoryEntry.uncompressedSize) ||
                    fieldOverflow(directoryEntry.fileHeaderOffset);
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

                if (fieldOverflow(directoryEntry.compressedSize) ||
                    fieldOverflow(directoryEntry.uncompressedSize) ||
                    fieldOverflow(directoryEntry.fileHeaderOffset)) {
                    getZip64ExtendedInformationExtraField(extendedInfo, directoryEntry.extraField);
                }

                this->positionInZIPFile(extendedInfo.fileHeaderOffset);
                this->getLocalFileHeader(fileHeader);

                if (directoryEntry.compression == 0x8) {
                    crc32 =  this->inflateFile(destFileNameStr, extendedInfo.compressedSize);
                    fileExtracted = true;
                } else if (directoryEntry.compression == 0) {
                    crc32 = this->extractFile(destFileNameStr, extendedInfo.originalSize);
                    fileExtracted = true;
                } else {
                    throw Exception("File uses unsupported compression = " + std::to_string(directoryEntry.compression));
                }

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

        this->openZIPFile(this->zipFileNameStr, std::ios::binary | std::ios_base::in | std::ios_base::out | std::ios_base::trunc);

        this->putEOCentralDirectoryRecord(this->zipEOCentralDirectory);

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

        this->zipEOCentralDirectory.startDiskNumber = 0;
        this->zipEOCentralDirectory.diskNumber = 0;
        this->zipEOCentralDirectory.startDiskNumber = 0;
        this->zipEOCentralDirectory.numberOfCentralDirRecords = 0;
        this->zipEOCentralDirectory.totalCentralDirRecords = 0;
        this->zipEOCentralDirectory.sizeOfCentralDirRecords = 0;
        this->zipEOCentralDirectory.offsetCentralDirRecords = 0;
        this->zipEOCentralDirectory.commentLength = 0;
        this->zipEOCentralDirectory.comment.clear();

        this->zip64EOCentralDirectory.totalRecordSize = 0;
        this->zip64EOCentralDirectory.creatorVersion = 0;
        this->zip64EOCentralDirectory.extractorVersion = 0;
        this->zip64EOCentralDirectory.diskNumber = 0;
        this->zip64EOCentralDirectory.startDiskNumber = 0;
        this->zip64EOCentralDirectory.numberOfCentralDirRecords = 0;
        this->zip64EOCentralDirectory.totalCentralDirRecords = 0;
        this->zip64EOCentralDirectory.sizeOfCentralDirRecords = 0;
        this->zip64EOCentralDirectory.offsetCentralDirRecords = 0;
        this->zip64EOCentralDirectory.extensibleDataSector.clear();

        this->zipCentralDirectory.clear();
        
        this->offsetToNextFileHeader = 0;

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

        for (auto& directoryEntry : this->zipCentralDirectory) {
            if (directoryEntry.fileNameStr.compare(zippedFileNameStr) == 0) {
                std::cerr << "File already present in archive [" << zippedFileNameStr << "]" << std::endl;
                return (false);
            }
        }

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

        return ((fileEntry.externalFileAttrib & 0x10) || (S_ISDIR(fileEntry.externalFileAttrib >> 16)));

    }
    
    //
    // If a ZIP64 archive return true
    //

    bool CFileZIP::isZIP64(void) {

        return (this->bZIP64);

    }

} // namespace Antik