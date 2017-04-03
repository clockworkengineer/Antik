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

    //
    // ZIP archive compression methods.
    //

    const std::uint16_t CFileZIPIO::kZIPCompressionStore;
    const std::uint16_t CFileZIPIO::kZIPCompressionDeflate;

    //
    // ZIP archive versions
    //

    const std::uint8_t CFileZIPIO::kZIPVersion10;
    const std::uint8_t CFileZIPIO::kZIPVersion20;
    const std::uint8_t CFileZIPIO::kZIPVersion45;

    //
    // ZIP archive creator
    //

    const std::uint8_t CFileZIPIO::kZIPCreatorUnix;
    
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
    // Open ZIP archive for I/O.
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

        this->putField(entry.signature, buffer);
        this->putField(entry.crc32, buffer);
        this->putField(entry.compressedSize, buffer);
        this->putField(entry.uncompressedSize, buffer);

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

        this->putField(entry.signature, buffer);
        this->putField(entry.creatorVersion, buffer);
        this->putField(entry.extractorVersion, buffer);
        this->putField(entry.bitFlag, buffer);
        this->putField(entry.compression, buffer);
        this->putField(entry.modificationTime, buffer);
        this->putField(entry.modificationDate, buffer);
        this->putField(entry.crc32, buffer);
        this->putField(entry.compressedSize, buffer);
        this->putField(entry.uncompressedSize, buffer);
        this->putField(entry.fileNameLength, buffer);
        this->putField(entry.extraFieldLength, buffer);
        this->putField(entry.fileCommentLength, buffer);
        this->putField(entry.diskNoStart, buffer);
        this->putField(entry.internalFileAttrib, buffer);
        this->putField(entry.externalFileAttrib, buffer);
        this->putField(entry.fileHeaderOffset, buffer);

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

        this->putField(entry.signature, buffer);
        this->putField(entry.creatorVersion, buffer);
        this->putField(entry.bitFlag, buffer);
        this->putField(entry.compression, buffer);
        this->putField(entry.modificationTime, buffer);
        this->putField(entry.modificationDate, buffer);
        this->putField(entry.crc32, buffer);
        this->putField(entry.compressedSize, buffer);
        this->putField(entry.uncompressedSize, buffer);
        this->putField(entry.fileNameLength, buffer);
        this->putField(entry.extraFieldLength, buffer);

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

        this->putField(entry.signature, buffer);
        this->putField(entry.diskNumber, buffer);
        this->putField(entry.startDiskNumber, buffer);
        this->putField(entry.numberOfCentralDirRecords, buffer);
        this->putField(entry.totalCentralDirRecords, buffer);
        this->putField(entry.sizeOfCentralDirRecords, buffer);
        this->putField(entry.offsetCentralDirRecords, buffer);
        this->putField(entry.commentLength, buffer);

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
                       
        this->putField(entry.signature, buffer);
        this->putField(entry.totalRecordSize, buffer);
        this->putField(entry.creatorVersion, buffer);
        this->putField(entry.extractorVersion, buffer);
        this->putField(entry.diskNumber, buffer);
        this->putField(entry.startDiskNumber, buffer);
        this->putField(entry.numberOfCentralDirRecords, buffer);
        this->putField(entry.totalCentralDirRecords, buffer);
        this->putField(entry.sizeOfCentralDirRecords, buffer);
        this->putField(entry.offsetCentralDirRecords, buffer);
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

        this->putField(entry.signature, buffer);
        this->putField(entry.startDiskNumber, buffer);
        this->putField(entry.offset, buffer);
        this->putField(entry.numberOfDisks, buffer);
        this->zipFileStream.write((char *) &buffer[0], entry.size);

        if (this->zipFileStream.fail()) {
            throw Exception("Error in writing ZIP64 End Of Central Directory record locator.");
        }

    }

    //
    // Put any ZIP64 extended information record into byte array If a field is zero
    // (not present) then it is not placed in the buffer.
    //

    void CFileZIPIO::putZip64ExtendedInformationExtraField(Zip64ExtendedInformationExtraField& extendedInfo, std::vector<std::uint8_t>& info) {

        std::uint16_t fieldSize = 0;

        info.clear();

        if (extendedInfo.originalSize) fieldSize += sizeof(std::uint64_t);
        if (extendedInfo.compressedSize) fieldSize += sizeof(std::uint64_t);
        if (extendedInfo.fileHeaderOffset) fieldSize += sizeof(std::uint64_t);
        if (extendedInfo.fileHeaderOffset) fieldSize += sizeof(std::uint32_t);
        
        this->putField(extendedInfo.signature, info);
        this->putField(fieldSize, info);
        
        if (extendedInfo.originalSize) this->putField(extendedInfo.originalSize, info);
        if (extendedInfo.compressedSize) this->putField(extendedInfo.compressedSize, info);
        if (extendedInfo.fileHeaderOffset)this->putField(extendedInfo.fileHeaderOffset, info);
        if (extendedInfo.diskNo)this->putField(extendedInfo.diskNo, info);

    }

    //
    // Get Data Descriptor record from ZIP archive. 
    //

    void CFileZIPIO::getDataDescriptor(CFileZIPIO::DataDescriptor& entry) {

        std::vector<std::uint8_t> buffer(entry.size);
        std::uint8_t *buffptr=&buffer[0];
        std::uint32_t signature;

        this->zipFileStream.read((char *) buffptr, sizeof (signature));
        this->getField(signature, buffptr);

        if (signature == entry.signature) {

            this->zipFileStream.read((char *) buffptr, entry.size - sizeof (signature));
            
            this->getField(entry.crc32, buffptr);
            this->getField(entry.compressedSize, buffptr);
            this->getField(entry.uncompressedSize, buffptr);

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
        std::uint8_t *buffptr=&buffer[0];
        std::uint32_t signature;

        this->zipFileStream.read((char *) buffptr, sizeof (signature));
        this->getField(signature, buffptr);

        if (signature == entry.signature) {

            this->zipFileStream.read((char *) buffptr, entry.size - sizeof (signature));

            this->getField(entry.creatorVersion, buffptr);
            this->getField(entry.extractorVersion, buffptr);
            this->getField(entry.bitFlag, buffptr);
            this->getField(entry.compression, buffptr);
            this->getField(entry.modificationTime, buffptr);
            this->getField(entry.modificationDate, buffptr);
            this->getField(entry.crc32, buffptr);
            this->getField(entry.compressedSize, buffptr);
            this->getField(entry.uncompressedSize, buffptr);
            this->getField(entry.fileNameLength, buffptr);
            this->getField(entry.extraFieldLength, buffptr);
            this->getField(entry.fileCommentLength, buffptr);
            this->getField(entry.diskNoStart, buffptr);
            this->getField(entry.internalFileAttrib, buffptr);
            this->getField(entry.externalFileAttrib, buffptr);
            this->getField(entry.fileHeaderOffset, buffptr);

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
        std::uint8_t *buffptr=&buffer[0];
        std::uint32_t signature;
        
        this->zipFileStream.read((char *) buffptr, sizeof (signature));
        this->getField(signature, buffptr);

        if (signature == entry.signature) {

            this->zipFileStream.read((char *) buffptr, entry.size - sizeof (signature));
            
            this->getField(entry.creatorVersion, buffptr);
            this->getField(entry.bitFlag, buffptr);
            this->getField(entry.compression, buffptr);
            this->getField(entry.modificationTime, buffptr);
            this->getField(entry.modificationDate, buffptr);
            this->getField(entry.crc32, buffptr);
            this->getField(entry.compressedSize, buffptr);
            this->getField(entry.uncompressedSize, buffptr);
            this->getField(entry.fileNameLength, buffptr);
            this->getField(entry.extraFieldLength, buffptr);

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
            std::uint8_t *buffptr=&buffer[0];
            
            this->zipFileStream.seekg(filePosition+sizeof(signature), std::ios_base::beg);
            this->zipFileStream.read((char *) buffptr, entry.size-sizeof(signature));
            
            this->getField(entry.diskNumber, buffptr);
            this->getField(entry.startDiskNumber, buffptr);
            this->getField(entry.numberOfCentralDirRecords, buffptr);
            this->getField(entry.totalCentralDirRecords, buffptr);
            this->getField(entry.sizeOfCentralDirRecords, buffptr);
            this->getField(entry.offsetCentralDirRecords, buffptr);
            this->getField(entry.commentLength, buffptr);
            
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
        std::uint8_t *buffptr=&buffer[0];        
        std::uint32_t signature;
        std::uint64_t extensionSize;
        Zip64EOCentDirRecordLocator zip64EOCentralDirLocator;

        this->getZip64EOCentDirRecordLocator(zip64EOCentralDirLocator);
        this->zipFileStream.seekg(zip64EOCentralDirLocator.offset, std::ios::beg);

        this->zipFileStream.read((char *) buffptr, sizeof (signature));
        this->getField(signature, buffptr);

        if (signature == entry.signature) {

            this->zipFileStream.read((char *) buffptr, entry.size - sizeof (signature));
            
            this->getField(entry.totalRecordSize, buffptr);
            this->getField(entry.creatorVersion, buffptr);
            this->getField(entry.extractorVersion, buffptr);
            this->getField(entry.diskNumber, buffptr);
            this->getField(entry.startDiskNumber, buffptr);
            this->getField(entry.numberOfCentralDirRecords, buffptr);
            this->getField(entry.totalCentralDirRecords, buffptr);
            this->getField(entry.sizeOfCentralDirRecords, buffptr);
            this->getField(entry.offsetCentralDirRecords, buffptr);

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
            std::uint8_t *buffptr=&buffer[0];    
                   
            this->zipFileStream.seekg(filePosition+sizeof(signature), std::ios_base::beg);
            this->zipFileStream.read((char *) buffptr, entry.size-sizeof(signature));
            
            this->getField(entry.startDiskNumber, buffptr);
            this->getField(entry.offset, buffptr);
            this->getField(entry.numberOfDisks, buffptr);

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

    void CFileZIPIO::getZip64ExtendedInformationExtraField(Zip64ExtendedInformationExtraField& zip64ExtendedInfo, std::vector<std::uint8_t>& info) {

        std::uint16_t signature = 0;
        std::uint16_t fieldSize = 0;
        std::uint16_t fieldCount = 0;
        std::uint8_t *buffptr=&info[0];
          
        while (fieldCount < info.size()) {
            
            this->getField(signature, buffptr);
            this->getField(fieldSize, buffptr);
            
            if (signature == zip64ExtendedInfo.signature) {
                if (fieldOverflow(static_cast<std::uint32_t>(zip64ExtendedInfo.originalSize))) {
                    this->getField(zip64ExtendedInfo.originalSize, buffptr);
                    fieldSize -= sizeof (std::uint64_t);
                    if (!fieldSize) break;
                }
                if (fieldOverflow(static_cast<std::uint32_t>(zip64ExtendedInfo.compressedSize))) {
                    this->getField(zip64ExtendedInfo.compressedSize, buffptr);
                    fieldSize -= sizeof (std::uint64_t);
                    if (!fieldSize) break;
                }
                if (fieldOverflow(static_cast<std::uint32_t>(zip64ExtendedInfo.fileHeaderOffset))) {
                    this->getField(zip64ExtendedInfo.fileHeaderOffset, buffptr);
                    fieldSize -= sizeof (std::uint64_t);
                    if (!fieldSize) break;
                }
                this->getField(zip64ExtendedInfo.diskNo, buffptr);
                break;
            }

            fieldCount += (fieldSize + (sizeof (std::uint16_t)*2));
            buffptr += fieldSize;

        }

    }


} // namespace Antik