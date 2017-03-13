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
// Description:
//
// Dependencies:   C11++     - Language standard features used.

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

#include <zlib.h>

// =========
// NAMESPACE
// =========

namespace Antik {

    // ===========================
    // PRIVATE TYPES AND CONSTANTS
    // ===========================

    const std::uint32_t CFileZIP::kFileHeaderSize;
    const std::uint32_t CFileZIP::kDataDescriptorSize;
    const std::uint32_t CFileZIP::kCentralDirectoryFileHeaderSize;
    const std::uint32_t CFileZIP::kEOCentralDirectoryRecordSize;

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

    void CFileZIP::getField(std::uint32_t& field, std::uint8_t *buffer) {
        std::uint32_t byte1 = buffer[0], byte2 = buffer[1], byte3 = buffer[2], byte4 = buffer[3];
        field = byte4 << 24;
        field |= byte3 << 16;
        field |= byte2 << 8;
        field |= byte1;
    }

    void CFileZIP::getField(std::uint16_t& field, std::uint8_t *buffer) {
        std::uint16_t byte1 = buffer[0], byte2 = buffer[1];
        field = byte2 << 8;
        field |= byte1;
    }

    void CFileZIP::getDataDescriptor(std::ifstream& zipFileStream, CFileZIP::DataDescriptor& entry) {

        std::vector<std::uint8_t> buffer(CFileZIP::kDataDescriptorSize);
        uint32_t tag;
        zipFileStream.read((char *) &buffer[0], 4);
        getField(tag, &buffer[0]);

        if (tag == entry.signature) {
            zipFileStream.read((char *) &buffer[4], CFileZIP::CFileZIP::kDataDescriptorSize - 4);
            getField(entry.crc32, &buffer[4]);
            getField(entry.compressedSize, &buffer[8]);
            getField(entry.uncompressedSize, &buffer[12]);

        } else {
            std::cerr << "No Data Descriptor found" << std::endl;
        }

    }

    void CFileZIP::getCentralDirectoryFileHeader(std::ifstream& zipFileStream, CFileZIP::CentralDirectoryFileHeader& entry) {

        std::vector<std::uint8_t> buffer(CFileZIP::kCentralDirectoryFileHeaderSize);
        uint32_t tag;
        zipFileStream.read((char *) &buffer[0], 4);
        getField(tag, &buffer[0]);

        if (tag == entry.signature) {

            zipFileStream.read((char *) &buffer[4], CFileZIP::kCentralDirectoryFileHeaderSize - 4);

            getField(entry.versionExtract, &buffer[4]);
            getField(entry.versionMadeBys, &buffer[6]);
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

            if (entry.fileNameLength + entry.extraFieldLength > buffer.size()) {
                buffer.resize(entry.fileNameLength + entry.extraFieldLength + entry.fileCommentLength);
            }

            zipFileStream.read((char *) &buffer[0], entry.fileNameLength + entry.extraFieldLength + entry.fileCommentLength);
            entry.fileNameStr.append((char *) &buffer[0], entry.fileNameLength);
            if (entry.extraFieldLength != 0) {
                entry.extraField.reset(new std::uint8_t [entry.extraFieldLength]);
                std::memcpy(entry.extraField.get(), &buffer[entry.fileNameLength], entry.extraFieldLength);
            }

            if (entry.fileCommentLength != 0) {
                entry.fileNameStr.append((char *) &buffer[entry.fileNameLength + entry.extraFieldLength], entry.fileCommentLength);
            }

        } else {
            std::cerr << "Central Directory File Header not found." << std::endl;
        }

    }

    void CFileZIP::getFileHeader(std::ifstream& zipFileStream, CFileZIP::FileHeader& entry) {

        std::vector<std::uint8_t> buffer(CFileZIP::kFileHeaderSize);
        uint32_t tag;
        zipFileStream.read((char *) &buffer[0], 4);
        getField(tag, &buffer[0]);

        if (tag == entry.signature) {

            zipFileStream.read((char *) &buffer[4], CFileZIP::kFileHeaderSize - 4);
            getField(entry.versionExtract, &buffer[4]);
            getField(entry.bitFlag, &buffer[6]);
            getField(entry.compression, &buffer[8]);
            getField(entry.modificationTime, &buffer[10]);
            getField(entry.modificationDate, &buffer[12]);
            getField(entry.crc32, &buffer[14]);
            getField(entry.compressedSize, &buffer[18]);
            getField(entry.uncompressedSize, &buffer[22]);
            getField(entry.fileNameLength, &buffer[26]);
            getField(entry.extraFieldLength, &buffer[28]);

            if (entry.fileNameLength + entry.extraFieldLength > buffer.size()) {
                buffer.resize(entry.fileNameLength + entry.extraFieldLength);
            }
            zipFileStream.read((char *) &buffer[0], entry.fileNameLength + entry.extraFieldLength);
            entry.fileNameStr.append((char *) &buffer[0], entry.fileNameLength);

            if (entry.extraFieldLength != 0) {
                entry.extraField.reset(new std::uint8_t [entry.extraFieldLength]);
                std::memcpy(entry.extraField.get(), &buffer[entry.fileNameLength], entry.extraFieldLength);
            }

            if (entry.bitFlag & 0x8) {
                CFileZIP::DataDescriptor dataDesc;
                CFileZIP::getDataDescriptor(zipFileStream, dataDesc);
            }

        } else {
            std::cerr << "No file header found." << std::endl;
        }


    }

    void CFileZIP::getEOCentralDirectoryRecord(std::ifstream& zipFileStream, CFileZIP::EOCentralDirectoryRecord& entry) {

        zipFileStream.seekg(0, std::ios_base::end);
        uint64_t fileLength = zipFileStream.tellg();
        int64_t filePosition = fileLength;
        uint32_t signature = 0;
        while (filePosition--) {
            char curr;
            zipFileStream.seekg(filePosition, std::ios_base::beg);
            zipFileStream.get(curr);
            signature <<= 8;
            signature |= curr;
            if (signature == entry.signature) {
                break;
            }
        }

        if (filePosition != 0) {
            std::vector<std::uint8_t> buffer(CFileZIP::kEOCentralDirectoryRecordSize);
            zipFileStream.seekg(filePosition, std::ios_base::beg);
            zipFileStream.read((char *) &buffer[0], CFileZIP::kEOCentralDirectoryRecordSize);
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
                zipFileStream.read((char *) &buffer[0], entry.commentLength);
                entry.comment.reset(new std::uint8_t [entry.commentLength]);
                std::memcpy(entry.comment.get(), &buffer[0], entry.commentLength);
            }

        } else {
            std::cerr << "No End of central directory record found." << std::endl;
        }
    }

    void CFileZIP::convertModificationDateTime(std::tm& modificationDateTime, std::uint16_t dateWord, std::uint16_t timeWord) {

        std::time_t rawtime;

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

    // ==============
    // PUBLIC METHODS
    // ==============

    CFileZIP::CFileZIP(std::string& zipFileNameStr) : zipFileNameStr{zipFileNameStr}
    {

        zipInBuffer.resize(this->kZIPBufferSize);
        zipOutBuffer.resize(this->kZIPBufferSize);

    }

    CFileZIP::~CFileZIP() {

    }

    void CFileZIP::open(void) {

        this->zipFileReadStream.open(this->zipFileNameStr, std::ios::binary);

        if (this->zipFileReadStream.is_open()) {

            this->getEOCentralDirectoryRecord(zipFileReadStream, EOCentDirRec);

            this->zipFileReadStream.seekg(this->EOCentDirRec.offsetCentralDirRecords, std::ios_base::beg);

            for (auto cnt01 = 0; cnt01 < this->EOCentDirRec.numberOfCentralDirRecords; cnt01++) {
                CFileZIP::CentralDirectoryFileHeader centDirFileHeader;
                this->getCentralDirectoryFileHeader(zipFileReadStream, centDirFileHeader);
                this->zipContentsList.push_back(centDirFileHeader);
            }

        }

    }

    std::vector<CFileZIP::FileDetail> CFileZIP::getZIPFileDetails(void) {

        CFileZIP::FileDetail fileEntry;
        std::vector<CFileZIP::FileDetail> fileDetailList;

        for (auto entry : this->zipContentsList) {
            fileEntry.fileNameStr = entry.fileNameStr;
            fileEntry.fileCommentStr = entry.fileCommentStr;
            fileEntry.uncompressedSize = entry.uncompressedSize;
            this->convertModificationDateTime(fileEntry.modificationDateTime, entry.modificationDate, entry.modificationTime);
            fileDetailList.push_back(fileEntry);
        }


        return (fileDetailList);

    }

    bool CFileZIP::inflateFile(std::ifstream& sourceFileStream, std::ofstream& destFileStream, std::uint32_t sourceLength) {

        int inflateResult;
        std::uint32_t inflatedBytes;
        z_stream inlateZIPStream{ 0};

        if (sourceLength == 0) {
            return (true);
        }

        inflateResult = inflateInit2(&inlateZIPStream, -MAX_WBITS);
        if (inflateResult != Z_OK) {
            return (false);
        }

        /* decompress until deflate stream ends or end of file */

        do {

            sourceFileStream.read((char *) & this->zipInBuffer[0], ((kZIPBufferSize > sourceLength) ? sourceLength : kZIPBufferSize));

            if (sourceFileStream.fail()) {
                (void) inflateEnd(&inlateZIPStream);
                return (false);
            }

            inlateZIPStream.avail_in = sourceFileStream.gcount();
            if (inlateZIPStream.avail_in == 0) {
                break;
            }
            inlateZIPStream.next_in = (Bytef *) & this->zipInBuffer[0];

            /* run inflate() on input until output buffer not full */
            do {

                inlateZIPStream.avail_out = kZIPBufferSize;
                inlateZIPStream.next_out = (Bytef *) & this->zipOutBuffer[0];

                inflateResult = inflate(&inlateZIPStream, Z_NO_FLUSH);
                switch (inflateResult) {
                    case Z_NEED_DICT:
                        inflateResult = Z_DATA_ERROR; /* and fall through */
                    case Z_DATA_ERROR:
                    case Z_MEM_ERROR:
                        (void) inflateEnd(&inlateZIPStream);
                        return (false);
                }

                inflatedBytes = kZIPBufferSize - inlateZIPStream.avail_out;
                destFileStream.write((char *) & this->zipOutBuffer[0], inflatedBytes);
                if (destFileStream.fail()) {
                    (void) inflateEnd(&inlateZIPStream);
                    return (false);
                }

            } while (inlateZIPStream.avail_out == 0);

            sourceLength -= kZIPBufferSize;

        } while (inflateResult != Z_STREAM_END); // 

        /* clean up and return */

        (void) inflateEnd(&inlateZIPStream);
        
        return (inflateResult == Z_STREAM_END ? true : false);

    }

    bool CFileZIP::copyFile(std::ifstream& sourceFileStream, std::ofstream& destFileStream, std::uint32_t sourceLength) {

        bool bCopied = true;

        while (sourceLength) {
            sourceFileStream.read((char *) & this->zipInBuffer[0], std::min(sourceLength, this->kZIPBufferSize));
            if (sourceFileStream.fail()) {
                bCopied = false;
                break;
            }
            destFileStream.write((char *) & this->zipInBuffer[0], sourceFileStream.gcount());
            if (destFileStream.fail()) {
                bCopied = false;
                break;
            }

            sourceLength -= (std::min(sourceLength, CFileZIP::kZIPBufferSize));

        }

        return (bCopied);

    }

    uint32_t CFileZIP::calculateCRC32(std::ifstream& sourceFileStream, std::uint32_t sourceLength) {

        uLong crc = crc32(0L, Z_NULL, 0);

        while (sourceLength) {
            sourceFileStream.read((char *) & this->zipInBuffer[0], std::min(sourceLength, this->kZIPBufferSize));
            if (sourceFileStream.fail()) {
                break;
            }

            crc = crc32(crc, &this->zipInBuffer[0], sourceFileStream.gcount());

            sourceLength -= (std::min(sourceLength, CFileZIP::kZIPBufferSize));

        }

        return (crc);

    }

    bool CFileZIP::extractZIPFile(std::string fileNameStr, std::string destFileNameStr) {

        CFileZIP::FileDetail fileEntry;
        bool fileExtracted = false;

        for (auto entry : this->zipContentsList) {

            if (entry.fileNameStr.compare(fileNameStr) == 0) {

                this->zipFileReadStream.seekg(entry.fileHeaderOffset, std::ios_base::beg);
                std::ofstream attachmentFileStream(destFileNameStr, std::ios::binary);

                if (attachmentFileStream.is_open()) {

                    CFileZIP::FileHeader fileHeader;
                    this->getFileHeader(this->zipFileReadStream, fileHeader);

                    if (fileHeader.compression == 0x8) {
                        fileExtracted = this->inflateFile(this->zipFileReadStream, attachmentFileStream, fileHeader.compressedSize);
                    } else if (fileHeader.compression == 0) {
                        fileExtracted = this->copyFile(this->zipFileReadStream, attachmentFileStream, fileHeader.compressedSize);
                    } else {
                        throw Exception("File uses unsupported compression = "+std::to_string(fileHeader.compression));
                    }

                    attachmentFileStream.close();

                    std::ifstream inflatedFileStream(destFileNameStr, std::ios::binary);
                    uint32_t crc32 = this->calculateCRC32(inflatedFileStream, fileHeader.uncompressedSize);
                    if (crc32 != fileHeader.crc32) {
                        throw Exception("File " + destFileNameStr + " has an invalid CRC.");
                    }

                }

                break;

            }


        }

        return (fileExtracted);

    }

} // namespace Antik