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
            throw Exception("Error in writing End Of Central Directory File Header record.");
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

        } else {
            throw Exception("No End Of Central Directory record found.");
        }
    }

    //
    // Get ZIP64 End Of Central Directory record
    //

    void CFileZIP::getZip64EOCentralDirectoryRecord(CFileZIP::Zip64EOCentralDirectoryRecord& entry) {

        std::vector<std::uint8_t> buffer(entry.size);
        std::uint32_t tag;
        std::uint64_t extensionSize;

        this->zipFileStream.read((char *) &buffer[0], 4);
        getField(tag, &buffer[0]);

        if (tag == entry.signature) {

            this->zipFileStream.read((char *) &buffer[4], entry.size - 4);
            getField(entry.totalRecordSize, &buffer[4]);
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
            this->getField(entry.startDiskNumber, &buffer[4]);
            this->getField(entry.offset, &buffer[8]);
            this->getField(entry.numberOfDisks, &buffer[12]);
        } else {
            throw Exception("No ZIP64 End Of Central Directory Locator record found.");
        }
    }

    //
    // Get any ZIP64 extended information
    //
    
    void CFileZIP::getZip64ExtendedInformationExtraField(Zip64ExtendedInformationExtraField& extendedInfo, std::vector<std::uint8_t>& info) {

        std::uint16_t signature = 0;
        std::int16_t currentField = 0;
        std::uint16_t fieldSize;

        while (currentField < info.size()) {

            this->getField(signature, &info[currentField + 0]);
            this->getField(fieldSize, &info[currentField + 2]);
            if (signature == extendedInfo.signature) {
                currentField += 4;
                if (extendedInfo.originalSize == 0xFFFFFFFF) {
                    this->getField(extendedInfo.originalSize, &info[currentField]);
                    fieldSize -= 8;
                    currentField += 8;
                    if (!fieldSize) break;
                }
                if (extendedInfo.compressedSize == 0xFFFFFFFF) {
                    this->getField(extendedInfo.compressedSize, &info[currentField]);
                    fieldSize -= 8;
                    currentField += 8;
                    if (!fieldSize) break;
                }
                if (extendedInfo.fileHeaderOffset == 0xFFFFFFFF) {
                    this->getField(extendedInfo.fileHeaderOffset, &info[currentField]);
                    fieldSize -= 8;
                    currentField += 8;
                    if (!fieldSize) break;
                }
                this->getField(extendedInfo.diskNo, &info[currentField + 28]);
                break;
            }

            currentField += (fieldSize + 4);
            
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

    bool CFileZIP::inflateFile(std::ofstream& destFileStream, std::uint64_t fileSize, std::uint32_t& crc) {

        int inflateResult = Z_OK;
        std::uint64_t inflatedBytes = 0;
        z_stream inlateZIPStream{0};
        
        crc = crc32(0L, Z_NULL, 0);

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

                crc = crc32(crc, &this->zipOutBuffer[0], inflatedBytes);
                          
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
    // Copy uncompressed (stored) ZIP file entry data to file. Note: The crc32 
    // is calculated while the data is copied.
    //

    bool CFileZIP::copyFile(std::ofstream& destFileStream, std::uint64_t fileSize, std::uint32_t& crc) {

        bool bCopied = true;
        
        crc = crc32(0L, Z_NULL, 0);
        
        while (fileSize) {
            this->zipFileStream.read((char *) & this->zipInBuffer[0], std::min(fileSize, static_cast<std::uint64_t> (CFileZIP::kZIPBufferSize)));
            if (this->zipFileStream.fail()) {
                bCopied = false;
                break;
            }
            destFileStream.write((char *) & this->zipInBuffer[0], this->zipFileStream.gcount());
            if (destFileStream.fail()) {
                bCopied = false;
                break;
            }
            crc = crc32(crc, &this->zipInBuffer[0], this->zipFileStream.gcount());
            fileSize -= (std::min(fileSize, static_cast<std::uint64_t> (CFileZIP::kZIPBufferSize)));

        }

        return (bCopied);

    }

    //
    // Store file data as part of ZIP file header.
    //

    void CFileZIP::storeFileData(const std::string& fileNameStr, std::uint32_t fileSize) {

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
    // Get a files CRC32 value.
    //

    void CFileZIP::getFileCRC32(const std::string& fileNameStr, std::uint64_t fileSize, std::uint32_t& crc) {

        std::ifstream fileStream(fileNameStr, std::ios::binary);

        crc = crc32(0L, Z_NULL, 0);

        while (fileSize) {

            fileStream.read((char *) & this->zipInBuffer[0], std::min(fileSize, static_cast<std::uint64_t> (CFileZIP::kZIPBufferSize)));
            if (fileStream.fail()) {
                break;
            }

            crc = crc32(crc, &this->zipInBuffer[0], fileStream.gcount());

            fileSize -= (std::min(fileSize, static_cast<std::uint64_t> (CFileZIP::kZIPBufferSize)));

        }

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
            modificationTime = (fileTimeInfo->tm_sec & 0b11111) | ((fileTimeInfo->tm_min & 0b111111) << 5) | ((fileTimeInfo->tm_hour & 0b11111) << 11);
            modificatioDate = (fileTimeInfo->tm_mday & 0b11111) | ((((fileTimeInfo->tm_mon + 1) & 0b1111)) << 5) | (((fileTimeInfo->tm_year - 80)& 0b1111111) << 9);
        } else {
            modificationTime = modificatioDate = 0;
        }
    }

    //
    // Write a File Header record (including file contents) to ZIP file.
    //

    void CFileZIP::writeFileHeaderAndData(const std::string& fileNameStr, const std::string& zippedFileNameStr) {

        FileHeader fileHeader = FileHeader();
        CentralDirectoryFileHeader fileEntry = CentralDirectoryFileHeader();
        std::uint64_t fileOffset = 0;

        this->zipFileStream.seekg(this->offsetToNextFileHeader, std::ios_base::beg);

        fileEntry.creatorVersion = 0x0314; // Unix / PK 2.0
        fileEntry.extractorVersion = 0x0014; // PK 2.0
        fileEntry.fileNameStr = zippedFileNameStr;
        fileEntry.fileNameLength = fileEntry.fileNameStr.length();
        fileEntry.bitFlag = 0; // None
        fileEntry.compression = 8; // Deflated
        getFileModificationDateTime(fileNameStr, fileEntry.modificationDate, fileEntry.modificationTime);
        getFileSize(fileNameStr, fileEntry.uncompressedSize);
        getFileCRC32(fileNameStr, fileEntry.uncompressedSize, fileEntry.crc32);
        fileEntry.compressedSize = 0;

        fileEntry.internalFileAttrib = 0; // ???
        getFileAttributes(fileNameStr, fileEntry.externalFileAttrib);
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

        this->getFileDataCompressed(fileNameStr, fileEntry.uncompressedSize, fileEntry.compressedSize);
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
            this->storeFileData(fileNameStr, fileEntry.uncompressedSize);
        }

        this->zipCentralDirectory.push_back(fileEntry);

        this->offsetToNextFileHeader = this->zipFileStream.tellp();
        
        this->bModified = true;

    }

    //
    // Update a ZIP archives Central Directory.
    //

    void CFileZIP::UpdateCentralDiectory() {

        if (this->bModified) {

            this->zipFileStream.seekg(this->offsetToNextFileHeader, std::ios_base::beg);
                   
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

            // One of the central directory fields is to large to store so ZIP64
            
            if ((this->zipEOCentralDirectory.totalCentralDirRecords==0xFFFF) ||
                (this->zipEOCentralDirectory.numberOfCentralDirRecords==0xFFFF) ||
                (this->zipEOCentralDirectory.sizeOfCentralDirRecords==0xFFFFFFFF) ||
                (this->zipEOCentralDirectory.totalCentralDirRecords==0xFFFF) ||
                (this->zipEOCentralDirectory.startDiskNumber==0xFFFF) ||
                (this->zipEOCentralDirectory.diskNumber==0xFFFF)||
                (this->zipEOCentralDirectory.offsetCentralDirRecords == 0xFFFFFFFF)) {

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
            this->convertModificationDateTime(fileEntry.modificationDateTime, 
                              entry.modificationDate, entry.modificationTime);
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

                std::ofstream extractedFileStream(destFileNameStr, std::ios::binary | std::ios::trunc);
                Zip64ExtendedInformationExtraField extendedInfo;
                std::uint32_t crc32;
                
                if (extractedFileStream.is_open()) {

                    CFileZIP::FileHeader fileHeader;
    
                    // Set up 64 bit data values if needed
                    
                    extendedInfo.compressedSize = entry.compressedSize;
                    extendedInfo.originalSize = entry.uncompressedSize;
                    extendedInfo.fileHeaderOffset = entry.fileHeaderOffset;

                    // If dealing with ZIP64 extract full 64 bit values from extended field
                    
                    if ((entry.compressedSize == 0xFFFFFFFF) ||
                        (entry.uncompressedSize == 0xFFFFFFFF) ||
                        (entry.fileHeaderOffset == 0xFFFFFFFF)) {
                        getZip64ExtendedInformationExtraField(extendedInfo, entry.extraField);
                    }

                    this->zipFileStream.seekg(extendedInfo.fileHeaderOffset, std::ios_base::beg);
                    this->getFileHeader(fileHeader);

                    if (entry.compression == 0x8) {
                        fileExtracted = this->inflateFile(extractedFileStream, extendedInfo.compressedSize, crc32);
                    } else if (entry.compression == 0) {
                        fileExtracted = this->copyFile(extractedFileStream, extendedInfo.originalSize, crc32);
                    } else {
                        throw Exception("File uses unsupported compression = " + std::to_string(entry.compression));
                    }

                    extractedFileStream.close();

                    if (crc32 != entry.crc32) {
                        throw Exception("File " + destFileNameStr + " has an invalid CRC.");
                    }

                } else {
                    throw Exception("Could not open destination file " + destFileNameStr + " for output.");
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

        this->UpdateCentralDiectory();

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

        for (auto& fileEntry : this->zipCentralDirectory) {
            if (fileEntry.fileNameStr.compare(zippedFileNameStr) == 0) {
                std::cerr << "File already present in archive [" << zippedFileNameStr << "]" << std::endl;
                return (false);
            }
        }

        if (this->fileExists(fileNameStr)) {
            this->writeFileHeaderAndData(fileNameStr, zippedFileNameStr);
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