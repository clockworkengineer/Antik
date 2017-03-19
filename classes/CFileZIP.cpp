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
// supports archive creation and extraction of files from an existing archives. 
// Files are either saved using store (file copy) or deflate compression. The current
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
    // Put a 32 bit word into buffer.
    //

    void CFileZIP::putField(const std::uint32_t& field, std::vector<std::uint8_t>& buffer) {
        buffer.push_back(static_cast<std::uint8_t> (field & 0x000000FF));
        buffer.push_back(static_cast<std::uint8_t> ((field & 0x0000FF00) >> 8));
        buffer.push_back(static_cast<std::uint8_t> ((field & 0x00FF0000) >> 16));
        buffer.push_back(static_cast<std::uint8_t> ((field >> 24)));
    }

    //
    // Put a 16 bit word into buffer.
    //

    void CFileZIP::putField(const std::uint16_t& field, std::vector<std::uint8_t>& buffer) {

        buffer.push_back(static_cast<std::uint8_t> (field & 0x00FF));
        buffer.push_back(static_cast<std::uint8_t> ((field & 0xFF00) >> 8));
    }

    //
    // Put Data Descriptor record into buffer.
    //

    void CFileZIP::putDataDescriptor(CFileZIP::DataDescriptor& entry) {

        std::vector<std::uint8_t> buffer;

        putField(entry.signature, buffer);
        putField(entry.crc32, buffer);
        putField(entry.compressedSize, buffer);
        putField(entry.uncompressedSize, buffer);

        this->zipFileStream.write((char *) &buffer[0], entry.size);

        if (this->zipFileStream.fail()) {
            throw Exception("Error in writing Data Descriptor Record.");
        }

    }

    //
    // Put Central Directory File Header record into buffer.
    //

    void CFileZIP::putCentralDirectoryFileHeader(CFileZIP::CentralDirectoryFileHeader& entry) {

        std::vector<std::uint8_t> buffer;

        putField(entry.signature, buffer);
        putField(entry.creatorVersion, buffer);
        putField(entry.extractorVersion, buffer);
        putField(entry.bitFlag, buffer);
        putField(entry.compression, buffer);
        putField(entry.modificationTime, buffer);
        putField(entry.modificationDate, buffer);
        putField(entry.crc32, buffer);
        putField(entry.compressedSize, buffer);
        putField(entry.uncompressedSize, buffer);
        putField(entry.fileNameLength, buffer);
        putField(entry.extraFieldLength, buffer);
        putField(entry.fileCommentLength, buffer);
        putField(entry.diskNoStart, buffer);
        putField(entry.internalFileAttrib, buffer);
        putField(entry.externalFileAttrib, buffer);
        putField(entry.fileHeaderOffset, buffer);

        this->zipFileStream.write((char *) &buffer[0], entry.size);

        if (entry.fileNameLength) {
            this->zipFileStream.write((char *) &entry.fileNameStr[0], entry.fileNameLength);
        }
        if (entry.extraFieldLength) {
            this->zipFileStream.write((char *) &entry.extraField[0], entry.extraFieldLength);
        }
        if (entry.fileCommentLength) {
            this->zipFileStream.write((char *) &entry.fileCommentStr[0], entry.fileCommentLength);
        }

        if (this->zipFileStream.fail()) {
            throw Exception("Error in writing Central Directory File Header record.");
        }

    }

    //
    // Put File Header record into buffer.
    //

    void CFileZIP::putFileHeader(CFileZIP::FileHeader& entry) {

        std::vector<std::uint8_t> buffer;

        putField(entry.signature, buffer);
        putField(entry.creatorVersion, buffer);
        putField(entry.bitFlag, buffer);
        putField(entry.compression, buffer);
        putField(entry.modificationTime, buffer);
        putField(entry.modificationDate, buffer);
        putField(entry.crc32, buffer);
        putField(entry.compressedSize, buffer);
        putField(entry.uncompressedSize, buffer);
        putField(entry.fileNameLength, buffer);
        putField(entry.extraFieldLength, buffer);

        this->zipFileStream.write((char *) &buffer[0], entry.size);

        if (entry.fileNameLength) {
            this->zipFileStream.write((char *) &entry.fileNameStr[0], entry.fileNameLength);
        }
        if (entry.extraFieldLength) {
            this->zipFileStream.write((char *) &entry.extraField[0], entry.extraFieldLength);
        }

        if (this->zipFileStream.fail()) {
            throw Exception("Error in writing File Header record.");
        }

    }

    //
    // Put End Of Central Directory record into buffer.
    //

    void CFileZIP::putEOCentralDirectoryRecord(CFileZIP::EOCentralDirectoryRecord& entry) {

        std::vector<std::uint8_t> buffer;

        putField(entry.signature, buffer);
        putField(entry.diskNnumber, buffer);
        putField(entry.centralDirectoryStartDisk, buffer);
        putField(entry.numberOfCentralDirRecords, buffer);
        putField(entry.totalCentralDirRecords, buffer);
        putField(entry.sizeOfCentralDirRecords, buffer);
        putField(entry.offsetCentralDirRecords, buffer);
        putField(entry.commentLength, buffer);

        this->zipFileStream.write((char *) &buffer[0], entry.size);

        if (entry.commentLength) {
            this->zipFileStream.write((char *) &entry.comment[0], entry.commentLength);
        }

        if (this->zipFileStream.fail()) {
            throw Exception("Error in writing End Of Central Directory File Header record.");
        }

    }

    //
    // Get 32 bit word from buffer.
    //

    void CFileZIP::getField(std::uint32_t& field, std::uint8_t *buffer) {
        std::uint32_t byte1 = buffer[0], byte2 = buffer[1], byte3 = buffer[2], byte4 = buffer[3];
        field = byte4 << 24;
        field |= byte3 << 16;
        field |= byte2 << 8;
        field |= byte1;
    }

    //
    // Get 16 bit word value from buffer.
    //

    void CFileZIP::getField(std::uint16_t& field, std::uint8_t *buffer) {
        std::uint16_t byte1 = buffer[0], byte2 = buffer[1];
        field = byte2 << 8;
        field |= byte1;
    }

    //
    // Get Data Descriptor record from buffer. 
    //

    void CFileZIP::getDataDescriptor(CFileZIP::DataDescriptor& entry) {

        std::vector<std::uint8_t> buffer(entry.size);
        std::uint32_t tag;
        this->zipFileStream.read((char *) &buffer[0], 4);
        getField(tag, &buffer[0]);

        if (tag == entry.signature) {
            this->zipFileStream.read((char *) &buffer[4], entry.size - 4);
            getField(entry.crc32, &buffer[4]);
            getField(entry.compressedSize, &buffer[8]);
            getField(entry.uncompressedSize, &buffer[12]);

        } else {
            throw Exception("No Data Descriptor record found.");
        }

    }

    //
    // Get Central Directory File Header record from buffer.
    //

    void CFileZIP::getCentralDirectoryFileHeader(CFileZIP::CentralDirectoryFileHeader& entry) {

        std::vector<std::uint8_t> buffer(entry.size);
        std::uint32_t tag;
        ;
        this->zipFileStream.read((char *) &buffer[0], 4);
        getField(tag, &buffer[0]);

        if (tag == entry.signature) {

            this->zipFileStream.read((char *) &buffer[4], entry.size - 4);

            getField(entry.creatorVersion, &buffer[4]);
            getField(entry.extractorVersion, &buffer[6]);
            getField(entry.bitFlag, &buffer[8]);
            getField(entry.compression, &buffer[10]);
            getField(entry.modificationTime, &buffer[12]);
            getField(entry.modificationDate, &buffer[14]);
            getField(entry.crc32, &buffer[16]);
            getField(entry.compressedSize, &buffer[20]);
            getField(entry.uncompressedSize, &buffer[24]);
            getField(entry.fileNameLength, &buffer[28]);
            getField(entry.extraFieldLength, &buffer[30]);
            getField(entry.fileCommentLength, &buffer[32]);
            getField(entry.diskNoStart, &buffer[34]);
            getField(entry.internalFileAttrib, &buffer[36]);
            getField(entry.externalFileAttrib, &buffer[38]);
            getField(entry.fileHeaderOffset, &buffer[42]);

            if ((entry.fileNameLength + entry.extraFieldLength + entry.fileCommentLength) > buffer.size()) {
                buffer.resize(entry.fileNameLength + entry.extraFieldLength + entry.fileCommentLength);
            }

            this->zipFileStream.read((char *) &buffer[0], entry.fileNameLength + entry.extraFieldLength + entry.fileCommentLength);
            
            if (entry.fileNameLength) {
                entry.fileNameStr.append((char *) &buffer[0], entry.fileNameLength);
            }
            if (entry.extraFieldLength) {
                entry.extraField.resize(entry.extraFieldLength);
                std::memcpy(&entry.extraField[0], &buffer[entry.fileNameLength], entry.extraFieldLength);
            }
            if (entry.fileCommentLength) {
                entry.fileNameStr.append((char *) &buffer[entry.fileNameLength + entry.extraFieldLength], entry.fileCommentLength);
            }

        } else {
            throw Exception("No Central Directory File Header found.");
        }

    }

    //
    // Get File Header record from buffer.
    //

    void CFileZIP::getFileHeader(CFileZIP::FileHeader& entry) {

        std::vector<std::uint8_t> buffer(entry.size);
        std::uint32_t tag;
        this->zipFileStream.read((char *) &buffer[0], 4);
        getField(tag, &buffer[0]);

        if (tag == entry.signature) {

            this->zipFileStream.read((char *) &buffer[4], entry.size - 4);
            getField(entry.creatorVersion, &buffer[4]);
            getField(entry.bitFlag, &buffer[6]);
            getField(entry.compression, &buffer[8]);
            getField(entry.modificationTime, &buffer[10]);
            getField(entry.modificationDate, &buffer[12]);
            getField(entry.crc32, &buffer[14]);
            getField(entry.compressedSize, &buffer[18]);
            getField(entry.uncompressedSize, &buffer[22]);
            getField(entry.fileNameLength, &buffer[26]);
            getField(entry.extraFieldLength, &buffer[28]);

            if ((entry.fileNameLength + entry.extraFieldLength) > buffer.size()) {
                buffer.resize(entry.fileNameLength + entry.extraFieldLength);
            }
            
            this->zipFileStream.read((char *) &buffer[0], entry.fileNameLength + entry.extraFieldLength);
            
            if (entry.fileNameLength) {
                entry.fileNameStr.append((char *) &buffer[0], entry.fileNameLength);
            }
            if (entry.extraFieldLength) {
                entry.extraField.resize(entry.extraFieldLength);
                std::memcpy(&entry.extraField[0], &buffer[entry.fileNameLength], entry.extraFieldLength);
            }

            // Read any data descriptor and discard

            if (entry.bitFlag & 0x8) {
                DataDescriptor dataDesc;
                CFileZIP::getDataDescriptor(dataDesc);
            }

        } else {
            throw Exception("No File Header record found.");
        }


    }

    //
    // Get End Of Central Directory File Header record from buffer.
    //

    void CFileZIP::getEOCentralDirectoryRecord(CFileZIP::EOCentralDirectoryRecord& entry) {

        this->zipFileStream.seekg(0, std::ios_base::end);
        uint64_t fileLength = this->zipFileStream.tellg();
        int64_t filePosition = fileLength - 1;
        std::uint32_t signature = 0;

        // Read file in reverse looking for End Of Central Directory File Header signature

        while (filePosition) {
            char curr;
            this->zipFileStream.seekg(filePosition, std::ios_base::beg);
            this->zipFileStream.get(curr);
            signature <<= 8;
            signature |= curr;
            if (signature == entry.signature) {
                break;
            }
            filePosition--;
        }

        // If record found then get

        if (filePosition != -1) {
            std::vector<std::uint8_t> buffer(entry.size);
            this->zipFileStream.seekg(filePosition, std::ios_base::beg);
            this->zipFileStream.read((char *) &buffer[0], entry.size);
            getField(entry.diskNnumber, &buffer[4]);
            getField(entry.centralDirectoryStartDisk, &buffer[6]);
            getField(entry.numberOfCentralDirRecords, &buffer[8]);
            getField(entry.totalCentralDirRecords, &buffer[10]);
            getField(entry.sizeOfCentralDirRecords, &buffer[12]);
            getField(entry.offsetCentralDirRecords, &buffer[16]);
            getField(entry.commentLength, &buffer[20]);
            if (entry.commentLength != 0) {
                if (entry.commentLength > buffer.size()) {
                    buffer.resize(entry.commentLength);
                }
                this->zipFileStream.read((char *) &buffer[0], entry.commentLength);
                entry.comment.resize(entry.commentLength);
                std::memcpy(&entry.comment[0], &buffer[0], entry.commentLength);
            }

        } else {
            throw Exception("No End Of Central Directory record found.");
        }
    }

    //
    // Convert stat based modified date/time to ZIP format.
    //

    void CFileZIP::convertModificationDateTime(std::tm& modificationDateTime, std::uint16_t dateWord, std::uint16_t timeWord) {

        std::time_t rawtime = 0;

        std::time(&rawtime);
        std::memcpy(&modificationDateTime, std::localtime(&rawtime), sizeof (std::tm));

        modificationDateTime.tm_sec = (timeWord & 0b11111) >> 2;
        modificationDateTime.tm_min = (timeWord & 0b11111100000) >> 5;
        modificationDateTime.tm_hour = (timeWord & 0b1111100000000000) >> 11;
        modificationDateTime.tm_mday = (dateWord & 0b11111);
        modificationDateTime.tm_mon = ((dateWord & 0b111100000) >> 5) - 1;
        modificationDateTime.tm_year = ((dateWord & 0b1111111000000000) >> 9) + 80;

        mktime(&modificationDateTime);

    }

    //
    // Uncompress ZIP file entry data to file.
    //

    bool CFileZIP::inflateFile(std::ofstream& destFileStream, std::uint32_t fileSize) {

        int inflateResult = Z_OK;
        std::uint32_t inflatedBytes = 0;
        z_stream inlateZIPStream{0};

        if (fileSize == 0) {
            return (true);
        }

        inflateResult = inflateInit2(&inlateZIPStream, -MAX_WBITS);
        if (inflateResult != Z_OK) {
            throw Exception("inflateInit2() Error = " + std::to_string(inflateResult));
        }

        do {

            this->zipFileStream.read((char *) & this->zipInBuffer[0], ((CFileZIP::kZIPBufferSize > fileSize) ? fileSize : CFileZIP::kZIPBufferSize));

            if (this->zipFileStream.fail()) {
                inflateEnd(&inlateZIPStream);
                throw Exception("Error reading zip file during inflate.");
            }

            inlateZIPStream.avail_in = this->zipFileStream.gcount();
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
                        std::cerr << "Error inflating ZIP archive file. = " + std::to_string(inflateResult) << std::endl;
                        inflateEnd(&inlateZIPStream);
                        return (false);
                }

                inflatedBytes = CFileZIP::kZIPBufferSize - inlateZIPStream.avail_out;
                destFileStream.write((char *) & this->zipOutBuffer[0], inflatedBytes);
                if (destFileStream.fail()) {
                    inflateEnd(&inlateZIPStream);
                    throw Exception("Error writing to file during inflate.");
                }

            } while (inlateZIPStream.avail_out == 0);

            fileSize -= CFileZIP::kZIPBufferSize;

        } while (inflateResult != Z_STREAM_END);

        inflateEnd(&inlateZIPStream);

        return (inflateResult == Z_STREAM_END ? true : false);

    }

    //
    // Compress source file and write as part of ZIP file header record.
    //

    bool CFileZIP::deflateFile(std::ifstream& sourceFileStream, std::uint32_t uncompressedSize, std::uint32_t& compressedSize) {

        int deflateResult = 0, flushRemainder = 0;
        std::uint32_t bytesDeflated = 0;
        z_stream deflateZIPStream{0};

        deflateResult = deflateInit2(&deflateZIPStream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
        if (deflateResult != Z_OK) {
            throw Exception("deflateInit2() Error = " + std::to_string(deflateResult));
        }

        do {

            sourceFileStream.read((char *) & this->zipInBuffer[0], std::min(uncompressedSize, CFileZIP::kZIPBufferSize));
            if (sourceFileStream.fail() && !sourceFileStream.eof()) {
                deflateEnd(&deflateZIPStream);
                throw Exception("Error reading source file to deflate.");
                deflateEnd(&deflateZIPStream);
            }

            deflateZIPStream.avail_in = sourceFileStream.gcount();
            uncompressedSize -= deflateZIPStream.avail_in;

            flushRemainder = ((sourceFileStream.eof() || uncompressedSize == 0)) ? Z_FINISH : Z_NO_FLUSH;

            deflateZIPStream.next_in = &this->zipInBuffer[0];

            do {

                deflateZIPStream.avail_out = CFileZIP::kZIPBufferSize;
                deflateZIPStream.next_out = &this->zipOutBuffer[0];
                deflateResult = deflate(&deflateZIPStream, flushRemainder); /* no bad return value */

                bytesDeflated = CFileZIP::kZIPBufferSize - deflateZIPStream.avail_out;
                this->zipFileStream.write((char *) & this->zipOutBuffer[0], bytesDeflated);
                if (this->zipFileStream.fail()) {
                    deflateEnd(&deflateZIPStream);
                    throw Exception("Error writing deflated data to ZIP archive.");
                }

                compressedSize += bytesDeflated;

            } while (deflateZIPStream.avail_out == 0);


        } while (flushRemainder != Z_FINISH);

        deflateEnd(&deflateZIPStream);

        return (true);

    }

    //
    // Copy uncompressed (stored) ZIP file entry data to file.
    //

    bool CFileZIP::copyFile(std::ofstream& destFileStream, std::uint32_t fileSize) {

        bool bCopied = true;

        while (fileSize) {
            this->zipFileStream.read((char *) & this->zipInBuffer[0], std::min(fileSize, CFileZIP::kZIPBufferSize));
            if (this->zipFileStream.fail()) {
                bCopied = false;
                break;
            }
            destFileStream.write((char *) & this->zipInBuffer[0], this->zipFileStream.gcount());
            if (destFileStream.fail()) {
                bCopied = false;
                break;
            }

            fileSize -= (std::min(fileSize, CFileZIP::kZIPBufferSize));

        }

        return (bCopied);

    }

    //
    // Calculate a files CRC232 value.
    //

    std::uint32_t CFileZIP::calculateCRC32(std::ifstream& sourceFileStream, std::uint32_t fileSize) {

        uLong crc = crc32(0L, Z_NULL, 0);

        while (fileSize) {

            sourceFileStream.read((char *) & this->zipInBuffer[0], std::min(fileSize, CFileZIP::kZIPBufferSize));
            if (sourceFileStream.fail()) {
                break;
            }

            crc = crc32(crc, &this->zipInBuffer[0], sourceFileStream.gcount());

            fileSize -= (std::min(fileSize, CFileZIP::kZIPBufferSize));

        }

        return (crc);

    }

    //
    // Store file data as part of ZIP file header.
    //

    void CFileZIP::getFileData(const std::string& fileNameStr, std::uint32_t fileSize) {

        std::ifstream fileStream(fileNameStr, std::ios::binary);
        while (fileSize) {

            fileStream.read((char *) & this->zipInBuffer[0], std::min(fileSize, CFileZIP::kZIPBufferSize));
            if (fileStream.fail()) {
                break;
            }

            this->zipFileStream.write((char *) & this->zipInBuffer[0], fileStream.gcount());
            if (this->zipFileStream.fail()) {
                break;
            }
            fileSize -= (std::min(fileSize, CFileZIP::kZIPBufferSize));

        }

    }

    //
    // Write deflated file data as part of ZIP file header.
    //

    void CFileZIP::getFileDataCompressed(const std::string& fileNameStr, std::uint32_t uncompressedSize, std::uint32_t& compressedSize) {

        std::ifstream fileStream(fileNameStr, std::ios::binary);

        this->deflateFile(fileStream, uncompressedSize, compressedSize);


    }

    //
    // Get file attributes.
    //

    void CFileZIP::getFileAttributes(const std::string& fileNameStr, std::uint32_t& attributes) {

        struct stat fileStat {0};

        int rc = stat(fileNameStr.c_str(), &fileStat);

        if (rc == 0) {
            attributes = fileStat.st_mode;
            attributes <<= 16;
        } else {
            attributes = 0;
        }

    }

    //
    // Get a files size.
    //

    void CFileZIP::getFileSize(const std::string& fileNameStr, std::uint32_t& fileSize) {

        struct stat fileStat {0};

        int rc = stat(fileNameStr.c_str(), &fileStat);
        if (rc == 0) {
            if (S_ISDIR(fileStat.st_mode)) {
                fileSize = 0;
            } else {
                fileSize = fileStat.st_size;
            }
        } else {
            fileSize = 0;
        }
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
    // Get a files CRC32 value.
    //

    void CFileZIP::getFileCRC32(const std::string& fileNameStr, std::uint32_t fileSize, std::uint32_t& crc32) {

        std::ifstream fileStream(fileNameStr, std::ios::binary);
        crc32 = this->calculateCRC32(fileStream, fileSize);

    }

    //
    // Get files stat based modified date/time and convert to ZIP format.
    //

    void CFileZIP::getFileModificationDateTime(const std::string& fileNameStr, std::uint16_t& modificatioDate, std::uint16_t& modificationTime) {

        struct stat fileStat { 0 };

        int rc = stat(fileNameStr.c_str(), &fileStat);
        if (rc == 0) {
            struct std::tm * fileTimeInfo = std::localtime(&fileStat.st_mtime);
            modificationTime = (fileTimeInfo->tm_sec & 0b11111) | ((fileTimeInfo->tm_min & 0b111111) << 5) | ((fileTimeInfo->tm_hour & 0b11111) << 11);
            modificatioDate = (fileTimeInfo->tm_mday & 0b11111) | ((((fileTimeInfo->tm_mon + 1) & 0b1111)) << 5) | (((fileTimeInfo->tm_year - 80)& 0b1111111) << 9);
        } else {
            modificationTime = modificatioDate = 0;
        }
    }

    //
    // Write a File Header record (including file contents) to ZIP file. The filenames pair
    // are the original files name and the ZIP file name it is stored under for later extraction.
    //

    void CFileZIP::writeFileHeaderAndData(const std::pair<std::string, std::string>& fileNames) {

        FileHeader fileHeader = FileHeader();
        CentralDirectoryFileHeader fileEntry = CentralDirectoryFileHeader();
        std::uint32_t fileOffset = 0;

        fileEntry.creatorVersion = 0x0314; // Unix
        fileEntry.extractorVersion = 0x0014;
        fileEntry.fileNameStr = fileNames.second;
        fileEntry.fileNameLength = fileEntry.fileNameStr.length();
        fileEntry.bitFlag = 0; // None
        fileEntry.compression = 8; // Deflated
        getFileModificationDateTime(fileNames.first, fileEntry.modificationDate, fileEntry.modificationTime);
        getFileSize(fileNames.first, fileEntry.uncompressedSize);
        getFileCRC32(fileNames.first, fileEntry.uncompressedSize, fileEntry.crc32);
        fileEntry.compressedSize = 0;

        fileEntry.internalFileAttrib = 0; // ???
        getFileAttributes(fileNames.first, fileEntry.externalFileAttrib);
        fileEntry.fileHeaderOffset = this->zipFileStream.tellp();

        fileHeader.creatorVersion = fileEntry.creatorVersion;
        fileHeader.bitFlag = fileEntry.bitFlag;
        fileHeader.compression = fileEntry.compression;
        fileHeader.modificationTime = fileEntry.modificationTime;
        fileHeader.modificationDate = fileEntry.modificationDate;
        fileHeader.crc32 = fileEntry.crc32;
        fileHeader.compressedSize = fileEntry.compressedSize;
        fileHeader.uncompressedSize = fileEntry.uncompressedSize;
        fileHeader.fileNameLength = fileEntry.fileNameLength;
        fileHeader.extraFieldLength = fileEntry.extraFieldLength;
        fileHeader.fileNameStr = fileEntry.fileNameStr;
        fileHeader.extraField = fileEntry.extraField;

        this->putFileHeader(fileHeader);

        // Calculate files compressed size while deflating it and then either modify its
        // File Header entry to have the correct compressed size or if its compressed size
        // is greater then or equal to its original size then store file instead of compress.

        this->getFileDataCompressed(fileNames.first, fileEntry.uncompressedSize, fileEntry.compressedSize);
        fileOffset = this->zipFileStream.tellp();
        this->zipFileStream.seekg(fileEntry.fileHeaderOffset, std::ios_base::beg);
        if (fileEntry.compressedSize < fileEntry.uncompressedSize) {
            fileHeader.compressedSize = fileEntry.compressedSize;
            this->putFileHeader(fileHeader);
            this->zipFileStream.seekg(fileOffset, std::ios_base::beg);
        } else {
            fileHeader.compression = fileEntry.compression = 0;
            fileHeader.compressedSize = fileEntry.compressedSize = fileEntry.uncompressedSize;
            this->putFileHeader(fileHeader);
            this->getFileData(fileNames.first, fileEntry.uncompressedSize);
        }

        this->zipCentralDirectory.push_back(fileEntry);

    }
    
    //
    // Update a ZIP archives Central Directory.
    //

    void CFileZIP::UpdateCentralDiectory() {

        this->zipEOCentralDirectory.numberOfCentralDirRecords = this->zipCentralDirectory.size();
        this->zipEOCentralDirectory.totalCentralDirRecords = this->zipCentralDirectory.size();
        this->zipEOCentralDirectory.offsetCentralDirRecords = this->zipFileStream.tellp();

        for (auto& fileEntry : this->zipCentralDirectory) {
            this->putCentralDirectoryFileHeader(fileEntry);
        }

        this->zipEOCentralDirectory.sizeOfCentralDirRecords = this->zipFileStream.tellp();
        this->zipEOCentralDirectory.sizeOfCentralDirRecords -= this->zipEOCentralDirectory.offsetCentralDirRecords;

        this->putEOCentralDirectoryRecord(this->zipEOCentralDirectory);

    }

    // ==============
    // PUBLIC METHODS
    // ==============

    //
    // Constructor
    //

    CFileZIP::CFileZIP(const std::string& zipFileNameStr) : zipFileNameStr{zipFileNameStr}
    {

        zipInBuffer.resize(CFileZIP::kZIPBufferSize);
        zipOutBuffer.resize(CFileZIP::kZIPBufferSize);

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
    // Open zip file and read in Central Directory Header records.
    //

    void CFileZIP::open(void) {

        if (this->bOpen) {
            throw Exception("ZIP archive has not been opened.");
        }

        this->zipFileStream.open(this->zipFileNameStr, std::ios::binary | std::ios_base::in | std::ios_base::out);

        if (this->zipFileStream.is_open()) {

            this->getEOCentralDirectoryRecord(this->zipEOCentralDirectory);

            this->zipFileStream.seekg(this->zipEOCentralDirectory.offsetCentralDirRecords, std::ios_base::beg);

            for (auto cnt01 = 0; cnt01 < this->zipEOCentralDirectory.numberOfCentralDirRecords; cnt01++) {
                CFileZIP::CentralDirectoryFileHeader centDirFileHeader;
                this->getCentralDirectoryFileHeader(centDirFileHeader);
                this->zipCentralDirectory.push_back(centDirFileHeader);
            }
            
            this->bOpen=true;

        }

    }

    //
    // Read Central Directory and return a list of ZIP archive contents.
    //

    std::vector<CFileZIP::FileDetail> CFileZIP::contents(void) {


        FileDetail fileEntry = FileDetail();
        std::vector<CFileZIP::FileDetail> fileDetailList;

        if (!this->bOpen) {
            throw Exception("ZIP archive has not been opened.");
        }

        for (auto entry : this->zipCentralDirectory) {
            fileEntry.fileNameStr = entry.fileNameStr;
            fileEntry.fileCommentStr = entry.fileCommentStr;
            fileEntry.uncompressedSize = entry.uncompressedSize;
            fileEntry.compressedSize = entry.compressedSize;
            fileEntry.compression = entry.compression;
            fileEntry.externalFileAttrib = entry.externalFileAttrib;
            fileEntry.creatorVersion = entry.creatorVersion;
            fileEntry.extraField = entry.extraField;
            this->convertModificationDateTime(fileEntry.modificationDateTime, entry.modificationDate, entry.modificationTime);
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

        for (auto entry : this->zipCentralDirectory) {

            if (entry.fileNameStr.compare(fileNameStr) == 0) {

                this->zipFileStream.seekg(entry.fileHeaderOffset, std::ios_base::beg);
                std::ofstream extractedFileStream(destFileNameStr, std::ios::binary);

                if (extractedFileStream.is_open()) {

                    CFileZIP::FileHeader fileHeader;
                    this->getFileHeader(fileHeader);

                    if (fileHeader.compression == 0x8) {
                        fileExtracted = this->inflateFile(extractedFileStream, fileHeader.compressedSize);
                    } else if (fileHeader.compression == 0) {
                        fileExtracted = this->copyFile(extractedFileStream, fileHeader.uncompressedSize);
                    } else {
                        throw Exception("File uses unsupported compression = " + std::to_string(fileHeader.compression));
                    }

                    extractedFileStream.close();

                    std::ifstream crc32FileStream(destFileNameStr, std::ios::binary);
                    std::uint32_t crc32 = this->calculateCRC32(crc32FileStream, fileHeader.uncompressedSize);
                    if (crc32 != fileHeader.crc32) {
                        throw Exception("File " + destFileNameStr + " has an invalid CRC.");
                    }

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

        this->zipFileStream.open(this->zipFileNameStr, std::ios::binary | std::ios_base::in | std::ios_base::out | std::ios_base::trunc);

        if (this->zipFileStream.is_open()) {

            this->zipEOCentralDirectory.centralDirectoryStartDisk = 0;
            this->zipEOCentralDirectory.diskNnumber = 0;
            this->zipEOCentralDirectory.centralDirectoryStartDisk = 0;
            this->zipEOCentralDirectory.numberOfCentralDirRecords = 0;
            this->zipEOCentralDirectory.totalCentralDirRecords = 0;
            this->zipEOCentralDirectory.sizeOfCentralDirRecords = 0;
            this->zipEOCentralDirectory.offsetCentralDirRecords = 0;
            this->zipEOCentralDirectory.commentLength = 0;
            this->zipEOCentralDirectory.comment.clear();

            this->putEOCentralDirectoryRecord(this->zipEOCentralDirectory);

            this->zipFileStream.close();

        } else {
            throw Exception("Could not create ZIP archive " + this->zipFileNameStr);
        }

    }

    //
    // Add file to list of ZIP archive files.
    //

    void CFileZIP::add(const std::string& fileNameStr, const std::string& zipFileNameStr) {

        if (this->bOpen) {
            throw Exception("ZIP archive should not be open.");
        }

        if (this->fileExists(fileNameStr)) {
            this->zipFileContentsList.push_back(std::make_pair(fileNameStr, zipFileNameStr));
        } else {
            std::cerr << "File does not exist [" << fileNameStr << "]" << std::endl;
        }
    }

    //
    // Close ZIP archive
    //

    void CFileZIP::close(void) {

        if (!this->bOpen) {
            throw Exception("ZIP archive has not been opened.");
        }

        this->zipEOCentralDirectory.centralDirectoryStartDisk = 0;
        this->zipEOCentralDirectory.diskNnumber = 0;
        this->zipEOCentralDirectory.centralDirectoryStartDisk = 0;
        this->zipEOCentralDirectory.numberOfCentralDirRecords = 0;
        this->zipEOCentralDirectory.totalCentralDirRecords = 0;
        this->zipEOCentralDirectory.sizeOfCentralDirRecords = 0;
        this->zipEOCentralDirectory.offsetCentralDirRecords = 0;
        this->zipEOCentralDirectory.commentLength = 0;
        this->zipEOCentralDirectory.comment.clear();

        this->zipCentralDirectory.clear();
        this->zipFileContentsList.clear();

        this->zipFileStream.close();

        this->bOpen = false;

    }

    //
    // Save current ZIP contents to archive.
    //

    void CFileZIP::save(void) {

        this->zipFileStream.open(this->zipFileNameStr, std::ios::binary | std::ios_base::in | std::ios_base::out);

        if (this->zipFileStream.is_open()) {

            this->bOpen = true;

            this->zipFileStream.seekg(0, std::ios_base::beg);

            for (auto& fileEntry : this->zipFileContentsList) {
                this->writeFileHeaderAndData(fileEntry);
            }

            UpdateCentralDiectory();

            this->close();

        }

    }

    //
    // Append file to ZIP archive.
    //

    bool CFileZIP::append(const std::string& fileNameStr, const std::string& zipFileNameStr) {

        if (!this->bOpen) {
            throw Exception("ZIP archive has not been opened.");
        }

        for (auto& fileEntry : this->zipCentralDirectory) {
            if (fileEntry.fileNameStr.compare(zipFileNameStr) == 0) {
                return (false);
            }
        }

        if (this->fileExists(fileNameStr)) {

            this->zipFileStream.seekg(this->zipEOCentralDirectory.offsetCentralDirRecords, std::ios_base::beg);
            this->writeFileHeaderAndData(std::make_pair(fileNameStr, zipFileNameStr));
            this->UpdateCentralDiectory();
            return (true);

        } else {
            std::cerr << "File does not exist [" << fileNameStr << "]" << std::endl;
        }
        
        return (false);
        
    }

} // namespace Antik