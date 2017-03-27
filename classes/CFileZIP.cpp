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
// archives. ZIP archives in ZIP64 format may have their contents extracted but 
// at present the creation of archives and the adding of files are not supported.
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
    // Return true if field contains all 1s.
    //

    bool CFileZIP::fieldOverflow(const std::uint64_t& field) {
        return (field == static_cast<std::uint64_t> (~0));
    }

    //
    // Return true if field contains all 1s.
    //

    bool CFileZIP::fieldOverflow(const std::uint32_t& field) {
        return (field == static_cast<std::uint32_t> (~0));
    }

    //
    // Return true if field contains all 1s.
    //

    bool CFileZIP::fieldOverflow(const std::uint16_t& field) {
        return (field == static_cast<std::uint16_t> (~0));
    }

    //
    // Put a 64 bit word into buffer.
    //

    void CFileZIP::putField(const std::uint64_t& field, std::vector<std::uint8_t>& buffer) {
        buffer.push_back(static_cast<std::uint8_t> (field & 0x000000FF));
        buffer.push_back(static_cast<std::uint8_t> ((field & 0x0000FF00) >> 8));
        buffer.push_back(static_cast<std::uint8_t> ((field & 0x00FF0000) >> 16));
        buffer.push_back(static_cast<std::uint8_t> ((field & 0xFF000000) >> 24));
        buffer.push_back(static_cast<std::uint8_t> ((field & 0xFF00000000) >> 32));
        buffer.push_back(static_cast<std::uint8_t> ((field & 0xFF0000000000) >> 40));
        buffer.push_back(static_cast<std::uint8_t> ((field & 0xFF000000000000) >> 48));
        buffer.push_back(static_cast<std::uint8_t> ((field >> 56)));

    }

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
            throw Exception("Error in writing Central Directory Local File Header record.");
        }

    }

    //
    // Put Local File Header record into buffer.
    //

    void CFileZIP::putFileHeader(CFileZIP::LocalFileHeader& entry) {

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
            throw Exception("Error in writing Local File Header record.");
        }

    }

    //
    // Put End Of Central Directory record into buffer.
    //

    void CFileZIP::putEOCentralDirectoryRecord(CFileZIP::EOCentralDirectoryRecord& entry) {

        std::vector<std::uint8_t> buffer;

        putField(entry.signature, buffer);
        putField(entry.diskNumber, buffer);
        putField(entry.startDiskNumber, buffer);
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
            throw Exception("Error in writing End Of Central Directory Local File Header record.");
        }

    }

    //
    // Get 64 bit word from buffer.
    //

    void CFileZIP::getField(std::uint64_t& field, std::uint8_t *buffer) {
        field = static_cast<std::uint64_t> (buffer[7]) << 56;
        field |= static_cast<std::uint64_t> (buffer[6]) << 48;
        field |= static_cast<std::uint64_t> (buffer[5]) << 40;
        field |= static_cast<std::uint64_t> (buffer[4]) << 32;
        field |= static_cast<std::uint64_t> (buffer[3]) << 24;
        field |= static_cast<std::uint64_t> (buffer[2]) << 16;
        field |= static_cast<std::uint64_t> (buffer[1]) << 8;
        field |= static_cast<std::uint64_t> (buffer[0]);
    }

    //
    // Get 32 bit word from buffer.
    //

    void CFileZIP::getField(std::uint32_t& field, std::uint8_t *buffer) {
        field = static_cast<std::uint32_t> (buffer[3]) << 24;
        field |= static_cast<std::uint32_t> (buffer[2]) << 16;
        field |= static_cast<std::uint32_t> (buffer[1]) << 8;
        field |= static_cast<std::uint32_t> (buffer[0]);
    }

    //
    // Get 16 bit word value from buffer.
    //

    void CFileZIP::getField(std::uint16_t& field, std::uint8_t *buffer) {
        field = static_cast<std::uint16_t> (buffer[1]) << 8;
        field |= static_cast<std::uint16_t> (buffer[0]);
    }

    //
    // Get Data Descriptor record from buffer. 
    //

    void CFileZIP::getDataDescriptor(CFileZIP::DataDescriptor& entry) {

        std::vector<std::uint8_t> buffer(entry.size);
        std::uint32_t signature;

        this->zipFileStream.read((char *) &buffer[0], sizeof (signature));
        getField(signature, &buffer[0]);

        if (signature == entry.signature) {

            this->zipFileStream.read((char *) &buffer[sizeof (signature)], entry.size - sizeof (signature));
            getField(entry.crc32, &buffer[sizeof (signature)]);
            getField(entry.compressedSize, &buffer[8]);
            getField(entry.uncompressedSize, &buffer[12]);

            if (this->zipFileStream.fail()) {
                throw Exception("Error in reading Data Descriptor Record.");
            }

        } else {
            throw Exception("No Data Descriptor record found.");
        }

    }

    //
    // Get Central Directory File Header record from buffer.
    //

    void CFileZIP::getCentralDirectoryFileHeader(CFileZIP::CentralDirectoryFileHeader& entry) {

        std::vector<std::uint8_t> buffer(entry.size);
        std::uint32_t signature;

        this->zipFileStream.read((char *) &buffer[0], sizeof (signature));
        getField(signature, &buffer[0]);

        if (signature == entry.signature) {

            this->zipFileStream.read((char *) &buffer[sizeof (signature)], entry.size - sizeof (signature));

            getField(entry.creatorVersion, &buffer[sizeof (signature)]);
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

            if (this->zipFileStream.fail()) {
                throw Exception("Error in reading Central Directory Local File Header record.");
            }

        } else {
            throw Exception("No Central Directory File Header found.");
        }

    }

    //
    // Get Local File Header record from buffer.
    //

    void CFileZIP::getFileHeader(CFileZIP::LocalFileHeader& entry) {

        std::vector<std::uint8_t> buffer(entry.size);
        std::uint32_t signature;
        this->zipFileStream.read((char *) &buffer[0], sizeof (signature));
        getField(signature, &buffer[0]);

        if (signature == entry.signature) {

            this->zipFileStream.read((char *) &buffer[sizeof (signature)], entry.size - sizeof (signature));
            getField(entry.creatorVersion, &buffer[sizeof (signature)]);
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

            if (this->zipFileStream.fail()) {
                throw Exception("Error in reading Local File Header record.");
            }


        } else {
            throw Exception("No Local File Header record found.");
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
            char nextByte;
            this->zipFileStream.seekg(filePosition, std::ios_base::beg);
            this->zipFileStream.get(nextByte);
            signature <<= 8;
            signature |= nextByte;
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
            getField(entry.diskNumber, &buffer[4]);
            getField(entry.startDiskNumber, &buffer[6]);
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

            if (this->zipFileStream.fail()) {
                throw Exception("Error in reading End Of Central Directory record.");
            }

        } else {
            throw Exception("No End Of Central Directory record found.");
        }
    }

    //
    // Get ZIP64 End Of Central Directory record
    //

    void CFileZIP::getZip64EOCentralDirectoryRecord(CFileZIP::Zip64EOCentralDirectoryRecord& entry) {

        std::vector<std::uint8_t> buffer(entry.size);
        std::uint32_t signature;
        std::uint64_t extensionSize;

        this->zipFileStream.read((char *) &buffer[0], sizeof (signature));
        getField(signature, &buffer[0]);

        if (signature == entry.signature) {

            this->zipFileStream.read((char *) &buffer[sizeof (signature)], entry.size - sizeof (signature));
            getField(entry.totalRecordSize, &buffer[sizeof (signature)]);
            getField(entry.creatorVersion, &buffer[12]);
            getField(entry.extractorVersion, &buffer[14]);
            getField(entry.diskNumber, &buffer[16]);
            getField(entry.startDiskNumber, &buffer[20]);
            getField(entry.numberOfCentralDirRecords, &buffer[24]);
            getField(entry.totalCentralDirRecords, &buffer[32]);
            getField(entry.sizeOfCentralDirRecords, &buffer[40]);
            getField(entry.offsetCentralDirRecords, &buffer[48]);

            extensionSize = entry.totalRecordSize - entry.size + 12;
            if (extensionSize) {
                entry.extensibleDataSector.resize(extensionSize);
                this->zipFileStream.read((char *) &entry.extensibleDataSector[0], extensionSize);
            }

            if (this->zipFileStream.fail()) {
                throw Exception("Error in reading ZIP64 End Of Central Directory record.");
            }

        } else {
            throw Exception("No ZIP64 End Of Central Directory record found.");
        }

    }

    //
    // Get ZIP64 End Of Central Directory record locator
    //

    void CFileZIP::zip64EOCentDirRecordLocator(CFileZIP::Zip64EOCentDirRecordLocator& entry) {

        this->zipFileStream.seekg(0, std::ios_base::end);
        uint64_t fileLength = this->zipFileStream.tellg();
        int64_t filePosition = fileLength - 1;
        std::uint32_t signature = 0;

        // Read file in reverse looking for End Of Central Directory File Header signature

        while (filePosition) {
            char nextByte;
            this->zipFileStream.seekg(filePosition, std::ios_base::beg);
            this->zipFileStream.get(nextByte);
            signature <<= 8;
            signature |= nextByte;
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
            this->getField(entry.startDiskNumber, &buffer[4]);
            this->getField(entry.offset, &buffer[8]);
            this->getField(entry.numberOfDisks, &buffer[12]);

            if (this->zipFileStream.fail()) {
                throw Exception("Error in reading ZIP64 End Of Central Directory Locator records.");
            }

        } else {
            throw Exception("No ZIP64 End Of Central Directory Locator record found.");
        }

    }

    //
    // Get any ZIP64 extended information
    //

    void CFileZIP::getZip64ExtendedInformationExtraField(Zip64ExtendedInformationExtraField& extendedInfo, std::vector<std::uint8_t>& info) {

        std::int16_t fieldOffset = 0;
        std::uint16_t signature = 0;
        std::uint16_t fieldSize = 0;

        while (fieldOffset < info.size()) {

            this->getField(signature, &info[fieldOffset + 0]);
            this->getField(fieldSize, &info[fieldOffset + 2]);
            if (signature == extendedInfo.signature) {
                fieldOffset += (sizeof (std::uint16_t)*2);
                if (fieldOverflow(extendedInfo.originalSize)) {
                    this->getField(extendedInfo.originalSize, &info[fieldOffset]);
                    fieldSize -= sizeof (std::uint64_t);
                    fieldOffset += sizeof (std::uint64_t);
                    if (!fieldSize) break;
                }
                if (fieldOverflow(extendedInfo.compressedSize)) {
                    this->getField(extendedInfo.compressedSize, &info[fieldOffset]);
                    fieldSize -= sizeof (std::uint64_t);
                    fieldOffset += sizeof (std::uint64_t);
                    if (!fieldSize) break;
                }
                if (fieldOverflow(extendedInfo.fileHeaderOffset)) {
                    this->getField(extendedInfo.fileHeaderOffset, &info[fieldOffset]);
                    fieldSize -= sizeof (std::uint64_t);
                    fieldOffset += sizeof (std::uint64_t);
                    if (!fieldSize) break;
                }
                this->getField(extendedInfo.diskNo, &info[fieldOffset + 28]);
                break;
            }

            fieldOffset += (fieldSize + (sizeof (std::uint16_t)*2));

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
    // Uncompress ZIP file entry data to file. Note: The crc32 is calculated 
    // while the data is inflated.
    //

    void CFileZIP::inflateFile(const std::string& fileNameStr, std::uint64_t fileSize, std::uint32_t& crc) {

        int inflateResult = Z_OK;
        std::uint64_t inflatedBytes = 0;
        z_stream inlateZIPStream{0};
        std::ofstream fileStream(fileNameStr, std::ios::binary | std::ios::trunc);

        if (fileStream.fail()) {
            throw Exception("Could not open destination file for inflate.");
        }

        crc = crc32(0L, Z_NULL, 0);

        if (fileSize == 0) {
            return;
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
                        inflateEnd(&inlateZIPStream);
                        throw Exception("Error inflating ZIP archive file. = " + std::to_string(inflateResult));
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

    }

    //
    // Compress source file and write as part of ZIP file header record.
    //

    void CFileZIP::deflateFile(const std::string& fileNameStr, std::uint32_t uncompressedSize, std::uint32_t& compressedSize, std::uint32_t& crc) {

        int deflateResult = 0, flushRemainder = 0;
        std::uint32_t bytesDeflated = 0;
        z_stream deflateZIPStream{0};
        std::ifstream fileStream(fileNameStr, std::ios::binary);

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
                this->zipFileStream.write((char *) & this->zipOutBuffer[0], bytesDeflated);
                if (this->zipFileStream.fail()) {
                    deflateEnd(&deflateZIPStream);
                    throw Exception("Error writing deflated data to ZIP archive.");
                }

                compressedSize += bytesDeflated;

            } while (deflateZIPStream.avail_out == 0);


        } while (flushRemainder != Z_FINISH);

        deflateEnd(&deflateZIPStream);

        fileStream.close();

    }

    //
    // Extract uncompressed (stored) ZIP file entry data to file. Note: The crc32 
    // is calculated while the data is copied.
    //

    void CFileZIP::extractFile(const std::string& fileNameStr, std::uint64_t fileSize, std::uint32_t& crc) {

        crc = crc32(0L, Z_NULL, 0);
        std::ofstream fileStream(fileNameStr, std::ios::binary | std::ios::trunc);

        if (fileStream.fail()) {
            throw Exception("Could not open destination file for extract.");
        }

        while (fileSize) {
            this->zipFileStream.read((char *) & this->zipInBuffer[0], std::min(fileSize, static_cast<std::uint64_t> (CFileZIP::kZIPBufferSize)));
            if (this->zipFileStream.fail()) {
                throw Exception("Error in reading ZIP archive file.");
            }
            crc = crc32(crc, &this->zipInBuffer[0], this->zipFileStream.gcount());
            fileStream.write((char *) & this->zipInBuffer[0], this->zipFileStream.gcount());
            if (fileStream.fail()) {
                throw Exception("Error in writing extracted file.");
            }
            fileSize -= (std::min(fileSize, static_cast<std::uint64_t> (CFileZIP::kZIPBufferSize)));

        }

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

            this->zipFileStream.write((char *) & this->zipInBuffer[0], fileStream.gcount());
            if (this->zipFileStream.fail()) {
                throw Exception("Error writing to ZIP archive.");
            }

            fileSize -= (std::min(fileSize, CFileZIP::kZIPBufferSize));

        }

    }

    //
    // Get file attributes.
    //

    void CFileZIP::getFileAttributes(const std::string& fileNameStr, std::uint32_t& attributes) {

        struct stat fileStat {
            0
        };

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

        struct stat fileStat {
            0
        };

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

        struct stat fileStat {
            0
        };

        int rc = stat(fileNameStr.c_str(), &fileStat);
        return (rc == 0);

    }

    //
    // Get files stat based modified date/time and convert to ZIP format.
    //

    void CFileZIP::getFileModificationDateTime(const std::string& fileNameStr, std::uint16_t& modificatioDate, std::uint16_t& modificationTime) {

        struct stat fileStat {
            0
        };

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
            modificationTime = modificatioDate = 0;
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
        getFileSize(fileNameStr, directoryEntry.uncompressedSize);
        getFileAttributes(fileNameStr, directoryEntry.externalFileAttrib);
        
        // File directory so add trailing delimeter, no compression and extractor version  1.0
        
        if (S_ISDIR(directoryEntry.externalFileAttrib >> 16)) {
            if (directoryEntry.fileNameStr.back()!='/') { 
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

        this->zipFileStream.seekg(this->offsetToNextFileHeader, std::ios_base::beg);
        this->putFileHeader(fileHeader);

        if (directoryEntry.uncompressedSize) {

            // Calculate files compressed size while deflating it and then either modify its
            // Local File Header record to have the correct compressed size and CRC or if its 
            // compressed size is greater then or equal to its original size then store file 
            // instead of compress.

            this->deflateFile(fileNameStr, directoryEntry.uncompressedSize, directoryEntry.compressedSize, directoryEntry.crc32);

            fileHeader.crc32 = directoryEntry.crc32;

            this->offsetToNextFileHeader = this->zipFileStream.tellp();

            this->zipFileStream.seekg(directoryEntry.fileHeaderOffset, std::ios_base::beg);
            if (directoryEntry.compressedSize < directoryEntry.uncompressedSize) {
                fileHeader.compressedSize = directoryEntry.compressedSize;
                this->putFileHeader(fileHeader);
            } else {
                directoryEntry.extractorVersion = 0x000a;
                fileHeader.compression = directoryEntry.compression = 0;
                fileHeader.compressedSize = directoryEntry.compressedSize = directoryEntry.uncompressedSize;
                this->putFileHeader(fileHeader);
                this->storeFile(fileNameStr, directoryEntry.uncompressedSize);
                this->offsetToNextFileHeader = this->zipFileStream.tellp();
            }

        } else {
           this->offsetToNextFileHeader = this->zipFileStream.tellp();
        }

        this->zipCentralDirectory.push_back(directoryEntry);

        this->bModified = true;

    }

    //
    // Update a ZIP archives Central Directory.
    //

    void CFileZIP::UpdateCentralDirectory(void) {

        if (this->bModified) {

            this->zipFileStream.seekg(this->offsetToNextFileHeader, std::ios_base::beg);

            this->zipEOCentralDirectory.numberOfCentralDirRecords = this->zipCentralDirectory.size();
            this->zipEOCentralDirectory.totalCentralDirRecords = this->zipCentralDirectory.size();
            this->zipEOCentralDirectory.offsetCentralDirRecords = this->zipFileStream.tellp();

            for (auto& directoryEntry : this->zipCentralDirectory) {
                this->putCentralDirectoryFileHeader(directoryEntry);
            }

            this->zipEOCentralDirectory.sizeOfCentralDirRecords = this->zipFileStream.tellp();
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
            throw Exception("ZIP archive has already been opened.");
        }

        this->zipFileStream.open(this->zipFileNameStr, std::ios::binary | std::ios_base::in | std::ios_base::out);

        if (this->zipFileStream.is_open()) {

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

                this->zip64EOCentDirRecordLocator(this->zip64EOCentralDirLocator);
                this->zipFileStream.seekg(this->zip64EOCentralDirLocator.offset, std::ios::beg);
                this->getZip64EOCentralDirectoryRecord(this->zip64EOCentralDirectory);
                this->zipFileStream.seekg(this->zip64EOCentralDirectory.offsetCentralDirRecords, std::ios_base::beg);
                noOfFileRecords = this->zip64EOCentralDirectory.numberOfCentralDirRecords;
                this->offsetToNextFileHeader = this->zip64EOCentralDirectory.offsetCentralDirRecords;

            } else {
                this->zipFileStream.seekg(this->zipEOCentralDirectory.offsetCentralDirRecords, std::ios_base::beg);
                noOfFileRecords = this->zipEOCentralDirectory.numberOfCentralDirRecords;
                this->offsetToNextFileHeader = this->zipEOCentralDirectory.offsetCentralDirRecords;
            }

            for (auto cnt01 = 0; cnt01 < noOfFileRecords; cnt01++) {
                CFileZIP::CentralDirectoryFileHeader centDirFileHeader;
                this->getCentralDirectoryFileHeader(centDirFileHeader);
                this->zipCentralDirectory.push_back(centDirFileHeader);
            }

            this->bOpen = true;

        } else {
            throw Exception("ZIP archive " + this->zipFileNameStr + " could not be opened.");
        }

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
            this->convertModificationDateTime(fileEntry.modificationDateTime,
                    directoryEntry.modificationDate, directoryEntry.modificationTime);
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
                CFileZIP::LocalFileHeader fileHeader;
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

                this->zipFileStream.seekg(extendedInfo.fileHeaderOffset, std::ios_base::beg);
                this->getFileHeader(fileHeader);

                if (directoryEntry.compression == 0x8) {
                    this->inflateFile(destFileNameStr, extendedInfo.compressedSize, crc32);
                    fileExtracted = true;
                } else if (directoryEntry.compression == 0) {
                    this->extractFile(destFileNameStr, extendedInfo.originalSize, crc32);
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

        this->zipFileStream.open(this->zipFileNameStr, std::ios::binary | std::ios_base::in | std::ios_base::out | std::ios_base::trunc);

        if (this->zipFileStream.is_open()) {

            this->zipEOCentralDirectory.startDiskNumber = 0;
            this->zipEOCentralDirectory.diskNumber = 0;
            this->zipEOCentralDirectory.startDiskNumber = 0;
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

        this->zipCentralDirectory.clear();

        this->zipFileStream.close();

        this->bOpen = false;
        this->bModified = false;

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

} // namespace Antik