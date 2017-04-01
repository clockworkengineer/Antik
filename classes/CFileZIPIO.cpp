#include "HOST.hpp"
/*
 * File:   CFileZIPIO.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on March 24, 2017, 2:33 PM
 *
 * Copyright 2017.
 *
 */

//
// Class: CFileZIPIO
// 
// Description: Class to provide ZIP archive record/data I/O.
// It is the base class for CFileZIP but may be used standalone for 
// reading/writing ZIP archive information as and when required.
//
// Dependencies:   C11++     - Language standard features used.
//

// =================
// CLASS DEFINITIONS
// =================

// ====================
// CLASS IMPLEMENTATION
// ====================

#include "CFileZIPIO.hpp"

//
// C++ STL definitions
//

#include <cstring>

// =========
// NAMESPACE
// =========

namespace Antik {

    // ===========================
    // PRIVATE TYPES AND CONSTANTS
    // ===========================

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

    // ==============
    // PUBLIC METHODS
    // ==============

    //
    // Constructor
    //

    CFileZIPIO::CFileZIPIO() {

    }

    //
    // Destructor
    //

    CFileZIPIO::~CFileZIPIO() {

    }

    //
    // Open ZIP archive for I/O..
    //

    void CFileZIPIO::openZIPFile(const std::string fileNameStr, std::ios_base::openmode mode) {

        this->zipFileStream.open(fileNameStr, mode);

        if (this->zipFileStream.fail()) {
            throw Exception("Could not open ZIP archive " + fileNameStr);
        }

    }

    //
    // Close ZIP archive.
    //

    void CFileZIPIO::closeZIPFile(void) {
        this->zipFileStream.close();
    }

    //
    // Move to position in ZIP archive.
    //

    void CFileZIPIO::positionInZIPFile(std::uint64_t offset) {
        this->zipFileStream.seekg(offset, std::ios::beg);
    }

    //
    // Return current position within ZIP archive.
    //

    std::uint64_t CFileZIPIO::currentPositionZIPFile(void) {
        return (this->zipFileStream.tellg());
    }

    //
    // Write data to ZIP archive.
    //

    void CFileZIPIO::writeZIPFile(std::vector<std::uint8_t>& buffer, std::uint64_t count) {
        this->zipFileStream.write((char *) &buffer[0], count);
    }

    //
    // Read data from ZIP archive.
    //

    void CFileZIPIO::readZIPFile(std::vector<std::uint8_t>& buffer, std::uint64_t count) {
        this->zipFileStream.read((char *) &buffer[0], count);
    }

    //
    // Return amount of data returned from last read.
    //

    std::uint64_t CFileZIPIO::readCountZIPFile() {
        return (this->zipFileStream.gcount());
    }

    //
    // Return true if error in ZIP archive I/O..
    //

    bool CFileZIPIO::errorInZIPFile(void) {
        return (this->zipFileStream.fail());
    }
    
    //
    // Put Data Descriptor record into buffer and write to disk.
    //

    void CFileZIPIO::putDataDescriptor(CFileZIPIO::DataDescriptor& entry) {

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
    // Put Central Directory File Header record into buffer and write to disk.
    //

    void CFileZIPIO::putCentralDirectoryFileHeader(CFileZIPIO::CentralDirectoryFileHeader& entry) {

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
    // Put Local File Header record into buffer and write to disk.
    //

    void CFileZIPIO::putFileHeader(CFileZIPIO::LocalFileHeader& entry) {

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
    // Put End Of Central Directory record into buffer and write to disk.
    //

    void CFileZIPIO::putEOCentralDirectoryRecord(CFileZIPIO::EOCentralDirectoryRecord& entry) {

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
    // Put ZIP64 End Of Central Directory record into buffer and write to disk.
    //

    void CFileZIPIO::putZip64EOCentralDirectoryRecord(CFileZIPIO::Zip64EOCentralDirectoryRecord& entry) {

        std::vector<std::uint8_t> buffer;

        entry.totalRecordSize = entry.size - 12 +
                entry.extensibleDataSector.size();
                       
        putField(entry.signature, buffer);
        putField(entry.totalRecordSize, buffer);
        putField(entry.creatorVersion, buffer);
        putField(entry.extractorVersion, buffer);
        putField(entry.diskNumber, buffer);
        putField(entry.startDiskNumber, buffer);
        putField(entry.numberOfCentralDirRecords, buffer);
        putField(entry.totalCentralDirRecords, buffer);
        putField(entry.sizeOfCentralDirRecords, buffer);
        putField(entry.offsetCentralDirRecords, buffer);
        this->zipFileStream.write((char *) &buffer[0], entry.size);

        if (entry.extensibleDataSector.size()) {
            this->zipFileStream.write((char *) &entry.extensibleDataSector[0], entry.extensibleDataSector.size());
        }

        if (this->zipFileStream.fail()) {
            throw Exception("Error in writing ZIP64 End Of Central Directory record.");
        }

    }

    //
    // Put ZIP64 End Of Central Directory record locator into buffer and write to disk.
    //

    void CFileZIPIO::putZip64EOCentDirRecordLocator(CFileZIPIO::Zip64EOCentDirRecordLocator& entry) {

        std::vector<std::uint8_t> buffer;

        putField(entry.signature, buffer);
        putField(entry.startDiskNumber, buffer);
        putField(entry.offset, buffer);
        putField(entry.numberOfDisks, buffer);
        this->zipFileStream.write((char *) &buffer[0], entry.size);

        if (this->zipFileStream.fail()) {
            throw Exception("Error in writing ZIP64 End Of Central Directory record locator.");
        }

    }

    //
    // Put any ZIP64 extended information record into byte array.
    //

    void CFileZIPIO::putZip64ExtendedInformationExtraField(Zip64ExtendedInformationExtraField& extendedInfo, std::vector<std::uint8_t>& info) {

        std::uint16_t fieldSize = 28;

        info.clear();

        this->putField(extendedInfo.signature, info);
        this->putField(fieldSize, info);
        this->putField(extendedInfo.originalSize, info);
        this->putField(extendedInfo.compressedSize, info);
        this->putField(extendedInfo.fileHeaderOffset, info);
        this->putField(extendedInfo.diskNo, info);
        


    }

    //
    // Get Data Descriptor record from ZIP archive. 
    //

    void CFileZIPIO::getDataDescriptor(CFileZIPIO::DataDescriptor& entry) {

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
    // Get Central Directory File Header record from ZIP archive.
    //

    void CFileZIPIO::getCentralDirectoryFileHeader(CFileZIPIO::CentralDirectoryFileHeader& entry) {

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
    // Get Local File Header record from ZIP archive.
    //

    void CFileZIPIO::getLocalFileHeader(CFileZIPIO::LocalFileHeader& entry) {

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
    // Get End Of Central Directory File Header record from ZIP archive.
    //

    void CFileZIPIO::getEOCentralDirectoryRecord(CFileZIPIO::EOCentralDirectoryRecord& entry) {

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
    // Get ZIP64 End Of Central Directory record from ZIP archive
    //

    void CFileZIPIO::getZip64EOCentralDirectoryRecord(CFileZIPIO::Zip64EOCentralDirectoryRecord& entry) {

        std::vector<std::uint8_t> buffer(entry.size);
        std::uint32_t signature;
        std::uint64_t extensionSize;
        Zip64EOCentDirRecordLocator zip64EOCentralDirLocator;

        this->getZip64EOCentDirRecordLocator(zip64EOCentralDirLocator);
        this->zipFileStream.seekg(zip64EOCentralDirLocator.offset, std::ios::beg);

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
    // Get ZIP64 End Of Central Directory record locator from ZIP archive
    //

    void CFileZIPIO::getZip64EOCentDirRecordLocator(CFileZIPIO::Zip64EOCentDirRecordLocator& entry) {

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
    // Get any ZIP64 extended information from byte array.
    //

    void CFileZIPIO::getZip64ExtendedInformationExtraField(Zip64ExtendedInformationExtraField& extendedInfo, std::vector<std::uint8_t>& info) {

        std::int16_t fieldOffset = 0;
        std::uint16_t signature = 0;
        std::uint16_t fieldSize = 0;

        while (fieldOffset < info.size()) {

            this->getField(signature, &info[fieldOffset + 0]);
            this->getField(fieldSize, &info[fieldOffset + 2]);
            if (signature == extendedInfo.signature) {
                fieldOffset += (sizeof (std::uint16_t)*2);
                if (fieldOverflow(static_cast<std::uint32_t>(extendedInfo.originalSize))) {
                    this->getField(extendedInfo.originalSize, &info[fieldOffset]);
                    fieldSize -= sizeof (std::uint64_t);
                    fieldOffset += sizeof (std::uint64_t);
                    if (!fieldSize) break;
                }
                if (fieldOverflow(static_cast<std::uint32_t>(extendedInfo.compressedSize))) {
                    this->getField(extendedInfo.compressedSize, &info[fieldOffset]);
                    fieldSize -= sizeof (std::uint64_t);
                    fieldOffset += sizeof (std::uint64_t);
                    if (!fieldSize) break;
                }
                if (fieldOverflow(static_cast<std::uint32_t>(extendedInfo.fileHeaderOffset))) {
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


} // namespace Antik