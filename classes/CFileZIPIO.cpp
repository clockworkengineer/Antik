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
    namespace File {

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

        //
        // Put Data Descriptor record into buffer and write to disk.
        //

        void CFileZIPIO::writeDataDescriptor(std::fstream &zipFileStream, CFileZIPIO::DataDescriptor& entry) {

            std::vector<std::uint8_t> buffer;

            putField(entry.signature, buffer);
            putField(entry.crc32, buffer);
            putField(entry.compressedSize, buffer);
            putField(entry.uncompressedSize, buffer);

            zipFileStream.write((char *) &buffer[0], entry.size);

            if (zipFileStream.fail()) {
                throw Exception("Error in writing Data Descriptor Record.");
            }

        }

        //
        // Put Central Directory File Header record into buffer and write to disk.
        //

        void CFileZIPIO::writeCentralDirectoryFileHeader(std::fstream &zipFileStream, CFileZIPIO::CentralDirectoryFileHeader& entry) {

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

            zipFileStream.write((char *) &buffer[0], entry.size);

            if (entry.fileNameLength) {
                zipFileStream.write((char *) &entry.fileNameStr[0], entry.fileNameLength);
            }
            if (entry.extraFieldLength) {
                zipFileStream.write((char *) &entry.extraField[0], entry.extraFieldLength);
            }
            if (entry.fileCommentLength) {
                zipFileStream.write((char *) &entry.fileCommentStr[0], entry.fileCommentLength);
            }

            if (zipFileStream.fail()) {
                throw Exception("Error in writing Central Directory Local File Header record.");
            }

        }

        //
        // Put Local File Header record into buffer and write to disk.
        //

        void CFileZIPIO::writeFileHeader(std::fstream &zipFileStream, CFileZIPIO::LocalFileHeader& entry) {

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

            zipFileStream.write((char *) &buffer[0], entry.size);

            if (entry.fileNameLength) {
                zipFileStream.write((char *) &entry.fileNameStr[0], entry.fileNameLength);
            }
            if (entry.extraFieldLength) {
                zipFileStream.write((char *) &entry.extraField[0], entry.extraFieldLength);
            }

            if (zipFileStream.fail()) {
                throw Exception("Error in writing Local File Header record.");
            }

        }

        //
        // Put End Of Central Directory record into buffer and write to disk.
        //

        void CFileZIPIO::writeEOCentralDirectoryRecord(std::fstream &zipFileStream, CFileZIPIO::EOCentralDirectoryRecord& entry) {

            std::vector<std::uint8_t> buffer;

            putField(entry.signature, buffer);
            putField(entry.diskNumber, buffer);
            putField(entry.startDiskNumber, buffer);
            putField(entry.numberOfCentralDirRecords, buffer);
            putField(entry.totalCentralDirRecords, buffer);
            putField(entry.sizeOfCentralDirRecords, buffer);
            putField(entry.offsetCentralDirRecords, buffer);
            putField(entry.commentLength, buffer);

            zipFileStream.write((char *) &buffer[0], entry.size);

            if (entry.commentLength) {
                zipFileStream.write((char *) &entry.commentStr[0], entry.commentLength);
            }

            if (zipFileStream.fail()) {
                throw Exception("Error in writing End Of Central Directory Local File Header record.");
            }

        }

        //
        // Put ZIP64 End Of Central Directory record into buffer and write to disk.
        //

        void CFileZIPIO::writeZip64EOCentralDirectoryRecord(std::fstream &zipFileStream, CFileZIPIO::Zip64EOCentralDirectoryRecord& entry) {

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
            zipFileStream.write((char *) &buffer[0], entry.size);

            if (entry.extensibleDataSector.size()) {
                zipFileStream.write((char *) &entry.extensibleDataSector[0], entry.extensibleDataSector.size());
            }

            if (zipFileStream.fail()) {
                throw Exception("Error in writing ZIP64 End Of Central Directory record.");
            }

        }

        //
        // Put ZIP64 End Of Central Directory record locator into buffer and write to disk.
        //

        void CFileZIPIO::writeZip64EOCentDirRecordLocator(std::fstream &zipFileStream, CFileZIPIO::Zip64EOCentDirRecordLocator& entry) {

            std::vector<std::uint8_t> buffer;

            putField(entry.signature, buffer);
            putField(entry.startDiskNumber, buffer);
            putField(entry.offset, buffer);
            putField(entry.numberOfDisks, buffer);
            zipFileStream.write((char *) &buffer[0], entry.size);

            if (zipFileStream.fail()) {
                throw Exception("Error in writing ZIP64 End Of Central Directory record locator.");
            }

        }

        //
        // Read Data Descriptor record from ZIP archive. 
        //

        void CFileZIPIO::readDataDescriptor(std::fstream &zipFileStream, CFileZIPIO::DataDescriptor& entry) {

            std::vector<std::uint8_t> buffer(entry.size);
            std::uint8_t *buffptr = &buffer[0];
            std::uint32_t signature;

            zipFileStream.read((char *) buffptr, sizeof (signature));
            buffptr = getField(signature, buffptr);

            if (signature == entry.signature) {

                zipFileStream.read((char *) buffptr, entry.size - sizeof (signature));

                buffptr = getField(entry.crc32, buffptr);
                buffptr = getField(entry.compressedSize, buffptr);
                buffptr = getField(entry.uncompressedSize, buffptr);

                if (zipFileStream.fail()) {
                    throw Exception("Error in reading Data Descriptor Record.");
                }

            } else {
                throw Exception("No Data Descriptor record found.");
            }

        }

        //
        // Read Central Directory File Header record from ZIP archive.
        //

        void CFileZIPIO::readCentralDirectoryFileHeader(std::fstream &zipFileStream, CFileZIPIO::CentralDirectoryFileHeader& entry) {

            std::vector<std::uint8_t> buffer(entry.size);
            std::uint8_t *buffptr = &buffer[0];
            std::uint32_t signature;

            zipFileStream.read((char *) buffptr, sizeof (signature));
            buffptr = getField(signature, buffptr);

            if (signature == entry.signature) {

                zipFileStream.read((char *) buffptr, entry.size - sizeof (signature));

                buffptr = getField(entry.creatorVersion, buffptr);
                buffptr = getField(entry.extractorVersion, buffptr);
                buffptr = getField(entry.bitFlag, buffptr);
                buffptr = getField(entry.compression, buffptr);
                buffptr = getField(entry.modificationTime, buffptr);
                buffptr = getField(entry.modificationDate, buffptr);
                buffptr = getField(entry.crc32, buffptr);
                buffptr = getField(entry.compressedSize, buffptr);
                buffptr = getField(entry.uncompressedSize, buffptr);
                buffptr = getField(entry.fileNameLength, buffptr);
                buffptr = getField(entry.extraFieldLength, buffptr);
                buffptr = getField(entry.fileCommentLength, buffptr);
                buffptr = getField(entry.diskNoStart, buffptr);
                buffptr = getField(entry.internalFileAttrib, buffptr);
                buffptr = getField(entry.externalFileAttrib, buffptr);
                buffptr = getField(entry.fileHeaderOffset, buffptr);

                if ((entry.fileNameLength + entry.extraFieldLength + entry.fileCommentLength) > buffer.size()) {
                    buffer.resize(entry.fileNameLength + entry.extraFieldLength + entry.fileCommentLength);
                }

                zipFileStream.read((char *) &buffer[0], entry.fileNameLength + entry.extraFieldLength + entry.fileCommentLength);

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

                if (zipFileStream.fail()) {
                    throw Exception("Error in reading Central Directory Local File Header record.");
                }

            } else {
                throw Exception("No Central Directory File Header found.");
            }

        }

        //
        // Read Local File Header record from ZIP archive.
        //

        void CFileZIPIO::readLocalFileHeader(std::fstream &zipFileStream, CFileZIPIO::LocalFileHeader& entry) {

            std::vector<std::uint8_t> buffer(entry.size);
            std::uint8_t *buffptr = &buffer[0];
            std::uint32_t signature;

            zipFileStream.read((char *) buffptr, sizeof (signature));
            buffptr = getField(signature, buffptr);

            if (signature == entry.signature) {

                zipFileStream.read((char *) buffptr, entry.size - sizeof (signature));

                buffptr = getField(entry.creatorVersion, buffptr);
                buffptr = getField(entry.bitFlag, buffptr);
                buffptr = getField(entry.compression, buffptr);
                buffptr = getField(entry.modificationTime, buffptr);
                buffptr = getField(entry.modificationDate, buffptr);
                buffptr = getField(entry.crc32, buffptr);
                buffptr = getField(entry.compressedSize, buffptr);
                buffptr = getField(entry.uncompressedSize, buffptr);
                buffptr = getField(entry.fileNameLength, buffptr);
                buffptr = getField(entry.extraFieldLength, buffptr);

                if ((entry.fileNameLength + entry.extraFieldLength) > buffer.size()) {
                    buffer.resize(entry.fileNameLength + entry.extraFieldLength);
                }

                zipFileStream.read((char *) &buffer[0], entry.fileNameLength + entry.extraFieldLength);

                if (entry.fileNameLength) {
                    entry.fileNameStr.append((char *) &buffer[0], entry.fileNameLength);
                }

                if (entry.extraFieldLength) {
                    entry.extraField.resize(entry.extraFieldLength);
                    std::memcpy(&entry.extraField[0], &buffer[entry.fileNameLength], entry.extraFieldLength);
                }

                if (zipFileStream.fail()) {
                    throw Exception("Error in reading Local File Header record.");
                }


            } else {
                throw Exception("No Local File Header record found.");
            }


        }

        //
        // Read End Of Central Directory File Header record from ZIP archive.
        //

        void CFileZIPIO::readEOCentralDirectoryRecord(std::fstream &zipFileStream, CFileZIPIO::EOCentralDirectoryRecord& entry) {

            zipFileStream.seekg(0, std::ios_base::end);
            uint64_t fileLength = zipFileStream.tellg();
            int64_t filePosition = fileLength - 1;
            std::uint32_t signature = 0;

            // Read file in reverse looking for End Of Central Directory File Header signature

            while (filePosition) {
                char nextByte;
                zipFileStream.seekg(filePosition, std::ios_base::beg);
                zipFileStream.get(nextByte);
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
                std::uint8_t *buffptr = &buffer[0];

                zipFileStream.seekg(filePosition + sizeof (signature), std::ios_base::beg);
                zipFileStream.read((char *) buffptr, entry.size - sizeof (signature));

                buffptr = getField(entry.diskNumber, buffptr);
                buffptr = getField(entry.startDiskNumber, buffptr);
                buffptr = getField(entry.numberOfCentralDirRecords, buffptr);
                buffptr = getField(entry.totalCentralDirRecords, buffptr);
                buffptr = getField(entry.sizeOfCentralDirRecords, buffptr);
                buffptr = getField(entry.offsetCentralDirRecords, buffptr);
                buffptr = getField(entry.commentLength, buffptr);

                if (entry.commentLength != 0) {
                    if (entry.commentLength > buffer.size()) {
                        buffer.resize(entry.commentLength);
                    }
                    zipFileStream.read((char *) &buffer[0], entry.commentLength);
                    entry.commentStr.resize(entry.commentLength);
                    entry.commentStr.append((char *) &buffer[0], entry.commentLength);
                }

                if (zipFileStream.fail()) {
                    throw Exception("Error in reading End Of Central Directory record.");
                }

            } else {
                throw Exception("No End Of Central Directory record found.");
            }
        }

        //
        // Read ZIP64 End Of Central Directory record from ZIP archive.
        //

        void CFileZIPIO::readZip64EOCentralDirectoryRecord(std::fstream &zipFileStream, CFileZIPIO::Zip64EOCentralDirectoryRecord& entry) {

            std::vector<std::uint8_t> buffer(entry.size);
            std::uint8_t *buffptr = &buffer[0];
            std::uint32_t signature;
            std::uint64_t extensionSize;
            Zip64EOCentDirRecordLocator zip64EOCentralDirLocator;

            readZip64EOCentDirRecordLocator(zipFileStream, zip64EOCentralDirLocator);
            zipFileStream.seekg(zip64EOCentralDirLocator.offset, std::ios::beg);

            zipFileStream.read((char *) buffptr, sizeof (signature));
            buffptr = getField(signature, buffptr);

            if (signature == entry.signature) {

                zipFileStream.read((char *) buffptr, entry.size - sizeof (signature));

                buffptr = getField(entry.totalRecordSize, buffptr);
                buffptr = getField(entry.creatorVersion, buffptr);
                buffptr = getField(entry.extractorVersion, buffptr);
                buffptr = getField(entry.diskNumber, buffptr);
                buffptr = getField(entry.startDiskNumber, buffptr);
                buffptr = getField(entry.numberOfCentralDirRecords, buffptr);
                buffptr = getField(entry.totalCentralDirRecords, buffptr);
                buffptr = getField(entry.sizeOfCentralDirRecords, buffptr);
                buffptr = getField(entry.offsetCentralDirRecords, buffptr);

                extensionSize = entry.totalRecordSize - entry.size + 12;
                if (extensionSize) {
                    entry.extensibleDataSector.resize(extensionSize);
                    zipFileStream.read((char *) &entry.extensibleDataSector[0], extensionSize);
                }

                if (zipFileStream.fail()) {
                    throw Exception("Error in reading ZIP64 End Of Central Directory record.");
                }

            } else {
                throw Exception("No ZIP64 End Of Central Directory record found.");
            }

        }

        //
        // Read ZIP64 End Of Central Directory record locator from ZIP archive
        //

        void CFileZIPIO::readZip64EOCentDirRecordLocator(std::fstream &zipFileStream, CFileZIPIO::Zip64EOCentDirRecordLocator& entry) {

            zipFileStream.seekg(0, std::ios_base::end);
            uint64_t fileLength = zipFileStream.tellg();
            int64_t filePosition = fileLength - 1;
            std::uint32_t signature = 0;

            // Read file in reverse looking for End Of Central Directory File Header signature

            while (filePosition) {
                char nextByte;
                zipFileStream.seekg(filePosition, std::ios_base::beg);
                zipFileStream.get(nextByte);
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
                std::uint8_t *buffptr = &buffer[0];

                zipFileStream.seekg(filePosition + sizeof (signature), std::ios_base::beg);
                zipFileStream.read((char *) buffptr, entry.size - sizeof (signature));

                buffptr = getField(entry.startDiskNumber, buffptr);
                buffptr = getField(entry.offset, buffptr);
                buffptr = getField(entry.numberOfDisks, buffptr);

                if (zipFileStream.fail()) {
                    throw Exception("Error in reading ZIP64 End Of Central Directory Locator records.");
                }

            } else {
                throw Exception("No ZIP64 End Of Central Directory Locator record found.");
            }

        }

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
        // Write Data Descriptor record into buffer to disk.
        //

        void CFileZIPIO::putDataDescriptor(CFileZIPIO::DataDescriptor& entry) {

            writeDataDescriptor(this->zipFileStream, entry);

        }

        //
        // Write Central Directory File Header record to disk.
        //

        void CFileZIPIO::putCentralDirectoryFileHeader(CFileZIPIO::CentralDirectoryFileHeader& entry) {

            writeCentralDirectoryFileHeader(this->zipFileStream, entry);

        }

        //
        // Write Local File Header record to disk.
        //

        void CFileZIPIO::putFileHeader(CFileZIPIO::LocalFileHeader& entry) {

            writeFileHeader(this->zipFileStream, entry);

        }

        //
        // Write End Of Central Directory record to disk.
        //

        void CFileZIPIO::putEOCentralDirectoryRecord(CFileZIPIO::EOCentralDirectoryRecord& entry) {

            writeEOCentralDirectoryRecord(this->zipFileStream, entry);

        }

        //
        // Write ZIP64 End Of Central Directory record to disk.
        //

        void CFileZIPIO::putZip64EOCentralDirectoryRecord(CFileZIPIO::Zip64EOCentralDirectoryRecord& entry) {

            writeZip64EOCentralDirectoryRecord(this->zipFileStream, entry);

        }

        //
        // Write ZIP64 End Of Central Directory record locator to disk.
        //

        void CFileZIPIO::putZip64EOCentDirRecordLocator(CFileZIPIO::Zip64EOCentDirRecordLocator& entry) {

            writeZip64EOCentDirRecordLocator(this->zipFileStream, entry);

        }

        //
        // Put any ZIP64 extended information record into byte array. Only perform if the
        // value is too large for its default storage. Sizes are stored as pair because
        // of the requirement for Local file headers.
        //

        void CFileZIPIO::putZip64ExtendedInfoExtraField(Zip64ExtendedInfoExtraField& extendedInfo, std::vector<std::uint8_t>& info) {

            std::uint16_t fieldSize = 0;

            info.clear();

            if (fieldRequires64bits(extendedInfo.originalSize)) {
                fieldSize += sizeof (std::uint64_t); // Store sizes as a pair.
                fieldSize += sizeof (std::uint64_t);
            }

            if (fieldRequires64bits(extendedInfo.fileHeaderOffset)) {
                fieldSize += sizeof (std::uint64_t);
            }

            if (fieldRequires32bits(extendedInfo.diskNo)) {
                fieldSize += sizeof (std::uint32_t);
            }

            putField(extendedInfo.signature, info);
            putField(fieldSize, info);

            if (fieldRequires64bits(extendedInfo.originalSize)) {
                putField(extendedInfo.originalSize, info);
                putField(extendedInfo.compressedSize, info);
            }

            if (fieldRequires64bits(extendedInfo.fileHeaderOffset)) {
                putField(extendedInfo.fileHeaderOffset, info);
            }
            if (fieldRequires32bits(extendedInfo.diskNo)) {
                putField(extendedInfo.diskNo, info);
            }

        }

        //
        // Get Data Descriptor record from ZIP archive. 
        //

        void CFileZIPIO::getDataDescriptor(CFileZIPIO::DataDescriptor& entry) {

            readDataDescriptor(this->zipFileStream, entry);

        }

        //
        // Get Central Directory File Header record from ZIP archive.
        //

        void CFileZIPIO::getCentralDirectoryFileHeader(CFileZIPIO::CentralDirectoryFileHeader& entry) {

            readCentralDirectoryFileHeader(this->zipFileStream, entry);

        }

        //
        // Get Local File Header record from ZIP archive.
        //

        void CFileZIPIO::getLocalFileHeader(CFileZIPIO::LocalFileHeader& entry) {

            readLocalFileHeader(this->zipFileStream, entry);

        }

        //
        // Get End Of Central Directory File Header record from ZIP archive.
        //

        void CFileZIPIO::getEOCentralDirectoryRecord(CFileZIPIO::EOCentralDirectoryRecord& entry) {

            readEOCentralDirectoryRecord(this->zipFileStream, entry);

        }

        //
        // Get ZIP64 End Of Central Directory record from ZIP archive
        //

        void CFileZIPIO::getZip64EOCentralDirectoryRecord(CFileZIPIO::Zip64EOCentralDirectoryRecord& entry) {

            readZip64EOCentralDirectoryRecord(this->zipFileStream, entry);

        }

        //
        // Get any ZIP64 extended information from byte array.
        //

        void CFileZIPIO::getZip64ExtendedInfoExtraField(Zip64ExtendedInfoExtraField& zip64ExtendedInfo, std::vector<std::uint8_t>& info) {

            std::uint16_t signature = 0;
            std::uint16_t fieldSize = 0;
            std::uint16_t fieldCount = 0;
            std::uint8_t *buffptr = &info[0];

            while (fieldCount < info.size()) {

                buffptr = getField(signature, buffptr);
                buffptr = getField(fieldSize, buffptr);

                if (signature == zip64ExtendedInfo.signature) {
                    if (fieldOverflow(static_cast<std::uint32_t> (zip64ExtendedInfo.originalSize))) {
                        buffptr = getField(zip64ExtendedInfo.originalSize, buffptr);
                        fieldSize -= sizeof (std::uint64_t);
                        if (!fieldSize) break;
                    }
                    if (fieldOverflow(static_cast<std::uint32_t> (zip64ExtendedInfo.compressedSize))) {
                        buffptr = getField(zip64ExtendedInfo.compressedSize, buffptr);
                        fieldSize -= sizeof (std::uint64_t);
                        if (!fieldSize) break;
                    }
                    if (fieldOverflow(static_cast<std::uint32_t> (zip64ExtendedInfo.fileHeaderOffset))) {
                        buffptr = getField(zip64ExtendedInfo.fileHeaderOffset, buffptr);
                        fieldSize -= sizeof (std::uint64_t);
                        if (!fieldSize) break;
                    }
                    buffptr = getField(zip64ExtendedInfo.diskNo, buffptr);
                    break;
                }

                fieldCount += (fieldSize + (sizeof (std::uint16_t)*2));
                buffptr += fieldSize;

            }

        }

    } // namespace File
} // namespace Antik