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
#include <sys/stat.h>

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

    void CFileZIP::putField(const std::uint32_t& field, std::vector<std::uint8_t>& buffer) {
        buffer.push_back(static_cast<std::uint8_t> (field & 0x000000FF));
        buffer.push_back(static_cast<std::uint8_t> ((field & 0x0000FF00) >> 8));
        buffer.push_back(static_cast<std::uint8_t> ((field & 0x00FF0000) >> 16));
        buffer.push_back(static_cast<std::uint8_t> ((field >> 24)));
     //   buffer.push_back(static_cast<std::uint8_t> ((field >> 24)));
     //   buffer.push_back(static_cast<std::uint8_t> ((field & 0x00FF0000) >> 16));
     //   buffer.push_back(static_cast<std::uint8_t> ((field & 0x0000FF00) >> 8));
     //   buffer.push_back(static_cast<std::uint8_t> (field & 0x000000FF));
    }

    void CFileZIP::putField(const std::uint16_t& field, std::vector<std::uint8_t>& buffer) {
   
        buffer.push_back(static_cast<std::uint8_t> (field & 0x00FF));
        buffer.push_back(static_cast<std::uint8_t> ((field & 0xFF00) >> 8));
    }

    void CFileZIP::putDataDescriptor(CFileZIP::DataDescriptor& entry) {

        std::vector<std::uint8_t> buffer;

        putField(entry.signature, buffer);
        putField(entry.crc32, buffer);
        putField(entry.compressedSize, buffer);
        putField(entry.uncompressedSize, buffer);

        this->zipFileStream.write((char *) &buffer[0], CFileZIP::kDataDescriptorSize);
        if (this->zipFileStream.fail()) std::cerr << "Write to ZIP fail" << std::endl;;

    }

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

        this->zipFileStream.write((char *) &buffer[0], CFileZIP::kCentralDirectoryFileHeaderSize);
        this->zipFileStream.write((char *) &entry.fileNameStr[0], entry.fileNameLength);
        if (entry.extraFieldLength)
            this->zipFileStream.write((char *) &entry.extraField[0], entry.extraFieldLength);
        if (entry.fileCommentLength)
            this->zipFileStream.write((char *) &entry.fileCommentStr[0], entry.fileCommentLength);

        if (this->zipFileStream.fail()) std::cerr << "Write to ZIP fail" << std::endl;;

    }

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

        this->zipFileStream.write((char *) &buffer[0], CFileZIP::kFileHeaderSize);
        this->zipFileStream.write((char *) &entry.fileNameStr[0], entry.fileNameLength);
        if (entry.extraFieldLength)
            this->zipFileStream.write((char *) &entry.extraField[0], entry.extraFieldLength);

        if (this->zipFileStream.fail()) std::cerr << "Write to ZIP fail" << std::endl;;
        
    }

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

        this->zipFileStream.write((char *) &buffer[0], CFileZIP::kEOCentralDirectoryRecordSize);
        if (entry.commentLength)
            this->zipFileStream.write((char *) &entry.comment[0], entry.commentLength);

        if (this->zipFileStream.fail()) std::cerr << "Write to ZIP fail" << std::endl;;
        
        
    }

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

    void CFileZIP::getDataDescriptor(CFileZIP::DataDescriptor& entry) {

        std::vector<std::uint8_t> buffer(CFileZIP::kDataDescriptorSize);
        uint32_t tag;
        this->zipFileStream.read((char *) &buffer[0], 4);
        getField(tag, &buffer[0]);

        if (tag == entry.signature) {
            this->zipFileStream.read((char *) &buffer[4], CFileZIP::kDataDescriptorSize - 4);
            getField(entry.crc32, &buffer[4]);
            getField(entry.compressedSize, &buffer[8]);
            getField(entry.uncompressedSize, &buffer[12]);

        } else {
            std::cerr << "No Data Descriptor found" << std::endl;
        }

    }

    void CFileZIP::getCentralDirectoryFileHeader(CFileZIP::CentralDirectoryFileHeader& entry) {

        std::vector<std::uint8_t> buffer(CFileZIP::kCentralDirectoryFileHeaderSize);
        uint32_t tag;
        this->zipFileStream.read((char *) &buffer[0], 4);
        getField(tag, &buffer[0]);

        if (tag == entry.signature) {

            this->zipFileStream.read((char *) &buffer[4], CFileZIP::kCentralDirectoryFileHeaderSize - 4);

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

            if (entry.fileNameLength + entry.extraFieldLength > buffer.size()) {
                buffer.resize(entry.fileNameLength + entry.extraFieldLength + entry.fileCommentLength);
            }

            this->zipFileStream.read((char *) &buffer[0], entry.fileNameLength + entry.extraFieldLength + entry.fileCommentLength);
            entry.fileNameStr.append((char *) &buffer[0], entry.fileNameLength);
            if (entry.extraFieldLength != 0) {
                entry.extraField.resize(entry.extraFieldLength);
                std::memcpy(&entry.extraField[0], &buffer[entry.fileNameLength], entry.extraFieldLength);
            }

            if (entry.fileCommentLength != 0) {
                entry.fileNameStr.append((char *) &buffer[entry.fileNameLength + entry.extraFieldLength], entry.fileCommentLength);
            }

        } else {
            std::cerr << "Central Directory File Header not found." << std::endl;
        }

    }

    void CFileZIP::getFileHeader(CFileZIP::FileHeader& entry) {

        std::vector<std::uint8_t> buffer(CFileZIP::kFileHeaderSize);
        uint32_t tag;
        this->zipFileStream.read((char *) &buffer[0], 4);
        getField(tag, &buffer[0]);

        if (tag == entry.signature) {

            this->zipFileStream.read((char *) &buffer[4], CFileZIP::kFileHeaderSize - 4);
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

            if (entry.fileNameLength + entry.extraFieldLength > buffer.size()) {
                buffer.resize(entry.fileNameLength + entry.extraFieldLength);
            }
            this->zipFileStream.read((char *) &buffer[0], entry.fileNameLength + entry.extraFieldLength);
            entry.fileNameStr.append((char *) &buffer[0], entry.fileNameLength);

            if (entry.extraFieldLength != 0) {
                entry.extraField.resize(entry.extraFieldLength);
                std::memcpy(&entry.extraField[0], &buffer[entry.fileNameLength], entry.extraFieldLength);
            }

            if (entry.bitFlag & 0x8) {
                CFileZIP::DataDescriptor dataDesc;
                CFileZIP::getDataDescriptor(dataDesc);
            }

        } else {
            std::cerr << "No file header found." << std::endl;
        }


    }

    void CFileZIP::getEOCentralDirectoryRecord(CFileZIP::EOCentralDirectoryRecord& entry) {

        this->zipFileStream.seekg(0, std::ios_base::end);
        uint64_t fileLength = this->zipFileStream.tellg();
        int64_t filePosition = fileLength-1;
        uint32_t signature = 0;
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

        if (filePosition != -1) {
            std::vector<std::uint8_t> buffer(CFileZIP::kEOCentralDirectoryRecordSize);
            this->zipFileStream.seekg(filePosition, std::ios_base::beg);
            this->zipFileStream.read((char *) &buffer[0], CFileZIP::kEOCentralDirectoryRecordSize);
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

    bool CFileZIP::inflateFile(std::ofstream& destFileStream, std::uint32_t sourceLength) {

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

            this->zipFileStream.read((char *) & this->zipInBuffer[0], ((CFileZIP::kZIPBufferSize > sourceLength) ? sourceLength : CFileZIP::kZIPBufferSize));

            if (this->zipFileStream.fail()) {
                (void) inflateEnd(&inlateZIPStream);
                return (false);
            }

            inlateZIPStream.avail_in = this->zipFileStream.gcount();
            if (inlateZIPStream.avail_in == 0) {
                break;
            }
            inlateZIPStream.next_in = (Bytef *) & this->zipInBuffer[0];

            /* run inflate() on input until output buffer not full */
            do {

                inlateZIPStream.avail_out = CFileZIP::kZIPBufferSize;
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

                inflatedBytes = CFileZIP::kZIPBufferSize - inlateZIPStream.avail_out;
                destFileStream.write((char *) & this->zipOutBuffer[0], inflatedBytes);
                if (destFileStream.fail()) {
                    (void) inflateEnd(&inlateZIPStream);
                    return (false);
                }

            } while (inlateZIPStream.avail_out == 0);

            sourceLength -= CFileZIP::kZIPBufferSize;

        } while (inflateResult != Z_STREAM_END); // 

        /* clean up and return */

        (void) inflateEnd(&inlateZIPStream);

        return (inflateResult == Z_STREAM_END ? true : false);

    }

    bool CFileZIP::copyFile(std::ofstream& destFileStream, std::uint32_t sourceLength) {

        bool bCopied = true;

        while (sourceLength) {
            this->zipFileStream.read((char *) & this->zipInBuffer[0], std::min(sourceLength, CFileZIP::kZIPBufferSize));
            if (this->zipFileStream.fail()) {
                bCopied = false;
                break;
            }
            destFileStream.write((char *) & this->zipInBuffer[0], this->zipFileStream.gcount());
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
            sourceFileStream.read((char *) & this->zipInBuffer[0], std::min(sourceLength, CFileZIP::kZIPBufferSize));
            if (sourceFileStream.fail()) {
                break;
            }

            crc = crc32(crc, &this->zipInBuffer[0], sourceFileStream.gcount());

            sourceLength -= (std::min(sourceLength, CFileZIP::kZIPBufferSize));

        }

        return (crc);

    }
    
    void CFileZIP::getFileData(std::string& fileNameStr, std::uint32_t fileLength) {

        std::ifstream fileStream(fileNameStr, std::ios::binary);
        while (fileLength) {
            
            fileStream.read((char *) & this->zipInBuffer[0], std::min(fileLength, CFileZIP::kZIPBufferSize));
            if (fileStream.fail()) {
                break;
            }

            this->zipFileStream.write((char *)&this->zipInBuffer[0], fileStream.gcount());
            if (this->zipFileStream.fail()) {
                break;
            }
            fileLength -= (std::min(fileLength, CFileZIP::kZIPBufferSize));

        }

    }

    void CFileZIP::getFileAttributes(const std::string& fileNameStr, std::uint32_t& attributes) {
        struct stat fileStat;
        int rc = stat(fileNameStr.c_str(), &fileStat);
        attributes = (rc == 0) ? fileStat.st_mode : -1;
        attributes <<= 16;
    }

    void CFileZIP::getFileSize(const std::string& fileNameStr, std::uint32_t& fileSize) {
        struct stat fileStat;
        int rc = stat(fileNameStr.c_str(), &fileStat);
        fileSize = (rc == 0) ? fileStat.st_size : -1;
    }

    void CFileZIP::getFileCRC32(const std::string& fileNameStr, std::uint32_t fileSize, std::uint32_t& crc32) {

        std::ifstream fileStream(fileNameStr, std::ios::binary);
        crc32 = this->calculateCRC32(fileStream, fileSize);

    }

    void CFileZIP::getFileModificationDateTime(const std::string& fileNameStr, std::uint16_t& modificatioDate, std::uint16_t& modificationTime) {

        struct stat fileStat;
        stat(fileNameStr.c_str(), &fileStat);
        struct std::tm * fileTimeInfo = std::localtime(&fileStat.st_mtime);
        modificationTime = (fileTimeInfo->tm_sec & 0b11111) | ((fileTimeInfo->tm_min & 0b111111) << 5) | ((fileTimeInfo->tm_hour & 0b11111) << 11);
        modificatioDate = (fileTimeInfo->tm_mday & 0b11111) | ((((fileTimeInfo->tm_mon + 1) & 0b1111)) << 5) | (((fileTimeInfo->tm_year - 80)& 0b1111111) << 9);

    }

    void CFileZIP::writeFileHeaderAndData(CFileZIP::AddedZIPContent& addedFile) {

        FileHeader fileHeader;
        CentralDirectoryFileHeader fileEntry;
        std::string fullFileNameStr = addedFile.pathNameStr+addedFile.baseFileNameStr;

        fileEntry.creatorVersion = 0x0314; // Unix
        fileEntry.extractorVersion = 0x0314; // Unix
        fileEntry.fileNameStr = addedFile.baseFileNameStr;
        fileEntry.fileNameLength = addedFile.baseFileNameStr.length();
        fileEntry.bitFlag = 0; // None
        fileEntry.compression = 0; // No compression at present
        getFileModificationDateTime(fullFileNameStr, fileEntry.modificationDate, fileEntry.modificationTime);
        getFileSize(fullFileNameStr, fileEntry.uncompressedSize);
        getFileCRC32(fullFileNameStr, fileEntry.uncompressedSize, fileEntry.crc32);
        fileEntry.compressedSize = fileEntry.uncompressedSize;

        fileEntry.internalFileAttrib = 0; // ???
        getFileAttributes(fullFileNameStr, fileEntry.externalFileAttrib);
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
        
        this->getFileData(fullFileNameStr, fileEntry.uncompressedSize);
        
        this->zipContentsList.push_back(fileEntry);

    }

    // ==============
    // PUBLIC METHODS
    // ==============

    CFileZIP::CFileZIP(const std::string& zipFileNameStr) : zipFileNameStr{zipFileNameStr}
    {

        zipInBuffer.resize(CFileZIP::kZIPBufferSize);
        zipOutBuffer.resize(CFileZIP::kZIPBufferSize);

    }

    CFileZIP::~CFileZIP() {

    }

    void CFileZIP::open(void) {

        this->zipFileStream.open(this->zipFileNameStr, std::ios::binary | std::ios_base::in | std::ios_base::out);

        if (this->zipFileStream.is_open()) {

            this->getEOCentralDirectoryRecord(this->EOCentDirRec);

            this->zipFileStream.seekg(this->EOCentDirRec.offsetCentralDirRecords, std::ios_base::beg);

            for (auto cnt01 = 0; cnt01 < this->EOCentDirRec.numberOfCentralDirRecords; cnt01++) {
                CFileZIP::CentralDirectoryFileHeader centDirFileHeader;
                this->getCentralDirectoryFileHeader(centDirFileHeader);
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

    bool CFileZIP::extractZIPFile(const std::string& fileNameStr, const std::string& destFileNameStr) {

        CFileZIP::FileDetail fileEntry;
        bool fileExtracted = false;

        for (auto entry : this->zipContentsList) {

            if (entry.fileNameStr.compare(fileNameStr) == 0) {

                this->zipFileStream.seekg(entry.fileHeaderOffset, std::ios_base::beg);
                std::ofstream attachmentFileStream(destFileNameStr, std::ios::binary);

                if (attachmentFileStream.is_open()) {

                    CFileZIP::FileHeader fileHeader;
                    this->getFileHeader(fileHeader);

                    if (fileHeader.compression == 0x8) {
                        fileExtracted = this->inflateFile(attachmentFileStream, fileHeader.compressedSize);
                    } else if (fileHeader.compression == 0) {
                        fileExtracted = this->copyFile(attachmentFileStream, fileHeader.compressedSize);
                    } else {
                        throw Exception("File uses unsupported compression = " + std::to_string(fileHeader.compression));
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

    void CFileZIP::create(void) {

        this->zipFileStream.open(this->zipFileNameStr, std::ios::binary | std::ios_base::in | std::ios_base::out | std::ios_base::trunc);

        if (this->zipFileStream.is_open()) {

            this->EOCentDirRec.centralDirectoryStartDisk = 0;
            this->EOCentDirRec.diskNnumber = 0;
            this->EOCentDirRec.centralDirectoryStartDisk = 0;
            this->EOCentDirRec.numberOfCentralDirRecords = 0;
            this->EOCentDirRec.totalCentralDirRecords = 0;
            this->EOCentDirRec.sizeOfCentralDirRecords = 0;
            this->EOCentDirRec.offsetCentralDirRecords = 0;
            this->EOCentDirRec.commentLength = 0;
            this->EOCentDirRec.comment.clear();

            this->putEOCentralDirectoryRecord(this->EOCentDirRec);

            this->zipFileStream.close();

        } else {
            throw Exception("Could not create ZIP archive " + this->zipFileNameStr);
        }

    }

    void CFileZIP::add(const std::string& fullFileNameStr) {

        AddedZIPContent fileEntry;

        fileEntry.pathNameStr = fullFileNameStr.substr(0, fullFileNameStr.find_last_of("/\\") + 1);
        fileEntry.baseFileNameStr = fullFileNameStr.substr(fullFileNameStr.find_last_of("/\\") + 1);

        this->addedZipFiles.push_back(fileEntry);

    }

    void CFileZIP::save(void) {

        this->zipFileStream.open(this->zipFileNameStr, std::ios::binary | std::ios_base::in | std::ios_base::out);

        if (this->zipFileStream.is_open()) {

            this->zipFileStream.seekg(0, std::ios_base::beg);
            
            for (auto& fileEntry : this->addedZipFiles) {
                this->writeFileHeaderAndData(fileEntry);
            }
            
            this->EOCentDirRec.centralDirectoryStartDisk = 0;
            this->EOCentDirRec.diskNnumber = 0;
            this->EOCentDirRec.centralDirectoryStartDisk = 0;
            this->EOCentDirRec.numberOfCentralDirRecords =  this->zipContentsList.size();
            this->EOCentDirRec.totalCentralDirRecords = this->zipContentsList.size();
            this->EOCentDirRec.sizeOfCentralDirRecords = 0;
            this->EOCentDirRec.offsetCentralDirRecords = this->zipFileStream.tellp();
            this->EOCentDirRec.commentLength = 0;
            this->EOCentDirRec.comment.clear();
            
            for (auto& fileEntry : this->zipContentsList) {
                this->putCentralDirectoryFileHeader(fileEntry);
            }

            this->EOCentDirRec.sizeOfCentralDirRecords = this->zipFileStream.tellp();
            this->EOCentDirRec.sizeOfCentralDirRecords -= this->EOCentDirRec.offsetCentralDirRecords;
            
            this->putEOCentralDirectoryRecord(this->EOCentDirRec);
            
        }
    }

} // namespace Antik