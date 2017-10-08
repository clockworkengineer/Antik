#include "HOST.hpp"
/*
 * File:   CZIPIO.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on March 24, 2017, 2:33 PM
 *
 * Copyright 2017.
 *
 */

//
// Class: CZIPIO
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

#include "CZIPIO.hpp"

//
// C++ STL
//

#include <cstring>

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace ZIP {

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

        //
        // Put Data Descriptor record into buffer and write to disk.
        //

        void CZIPIO::writeZIPRecord(std::fstream &zipFileeam, CZIPIO::DataDescriptor& entry) {

            std::vector<std::uint8_t> buffer;

            putField(entry.signature, buffer);
            putField(entry.crc32, buffer);
            putField(entry.compressedSize, buffer);
            putField(entry.uncompressedSize, buffer);

            zipFileeam.write((char *) &buffer[0], entry.size);

            if (zipFileeam.fail()) {
                throw Exception("Error in writing Data Descriptor Record.");
            }

        }

        //
        // Put Central Directory File Header record into buffer and write to disk.
        //

        void CZIPIO::writeZIPRecord(std::fstream &zipFileeam, CZIPIO::CentralDirectoryFileHeader& entry) {

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

            zipFileeam.write((char *) &buffer[0], entry.size);

            if (entry.fileNameLength) {
                zipFileeam.write((char *) &entry.fileName[0], entry.fileNameLength);
            }
            if (entry.extraFieldLength) {
                zipFileeam.write((char *) &entry.extraField[0], entry.extraFieldLength);
            }
            if (entry.fileCommentLength) {
                zipFileeam.write((char *) &entry.fileComment[0], entry.fileCommentLength);
            }

            if (zipFileeam.fail()) {
                throw Exception("Error in writing Central Directory Local File Header record.");
            }

        }

        //
        // Put Local File Header record into buffer and write to disk.
        //

        void CZIPIO::writeZIPRecord(std::fstream &zipFileeam, CZIPIO::LocalFileHeader& entry) {

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

            zipFileeam.write((char *) &buffer[0], entry.size);

            if (entry.fileNameLength) {
                zipFileeam.write((char *) &entry.fileName[0], entry.fileNameLength);
            }
            if (entry.extraFieldLength) {
                zipFileeam.write((char *) &entry.extraField[0], entry.extraFieldLength);
            }

            if (zipFileeam.fail()) {
                throw Exception("Error in writing Local File Header record.");
            }

        }

        //
        // Put End Of Central Directory record into buffer and write to disk.
        //

        void CZIPIO::writeZIPRecord(std::fstream &zipFileeam, CZIPIO::EOCentralDirectoryRecord& entry) {

            std::vector<std::uint8_t> buffer;

            putField(entry.signature, buffer);
            putField(entry.diskNumber, buffer);
            putField(entry.startDiskNumber, buffer);
            putField(entry.numberOfCentralDirRecords, buffer);
            putField(entry.totalCentralDirRecords, buffer);
            putField(entry.sizeOfCentralDirRecords, buffer);
            putField(entry.offsetCentralDirRecords, buffer);
            putField(entry.commentLength, buffer);

            zipFileeam.write((char *) &buffer[0], entry.size);

            if (entry.commentLength) {
                zipFileeam.write((char *) &entry.comment[0], entry.commentLength);
            }

            if (zipFileeam.fail()) {
                throw Exception("Error in writing End Of Central Directory Local File Header record.");
            }

        }

        //
        // Put ZIP64 End Of Central Directory record into buffer and write to disk.
        //

        void CZIPIO::writeZIPRecord(std::fstream &zipFileeam, CZIPIO::Zip64EOCentralDirectoryRecord& entry) {

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
            zipFileeam.write((char *) &buffer[0], entry.size);

            if (entry.extensibleDataSector.size()) {
                zipFileeam.write((char *) &entry.extensibleDataSector[0], entry.extensibleDataSector.size());
            }

            if (zipFileeam.fail()) {
                throw Exception("Error in writing ZIP64 End Of Central Directory record.");
            }

        }

        //
        // Put ZIP64 End Of Central Directory record locator into buffer and write to disk.
        //

        void CZIPIO::writeZIPRecord(std::fstream &zipFileeam, CZIPIO::Zip64EOCentDirRecordLocator& entry) {

            std::vector<std::uint8_t> buffer;

            putField(entry.signature, buffer);
            putField(entry.startDiskNumber, buffer);
            putField(entry.offset, buffer);
            putField(entry.numberOfDisks, buffer);
            zipFileeam.write((char *) &buffer[0], entry.size);

            if (zipFileeam.fail()) {
                throw Exception("Error in writing ZIP64 End Of Central Directory record locator.");
            }

        }

        //
        // Read Data Descriptor record from ZIP archive. 
        //

        void CZIPIO::readZIPRecord(std::fstream &zipFileeam, CZIPIO::DataDescriptor& entry) {

            std::vector<std::uint8_t> buffer(entry.size);
            std::uint8_t *buffptr = &buffer[0];
            std::uint32_t signature;

            zipFileeam.read((char *) buffptr, sizeof (signature));
            buffptr = getField(signature, buffptr);

            if (signature == entry.signature) {

                zipFileeam.read((char *) buffptr, entry.size - sizeof (signature));

                buffptr = getField(entry.crc32, buffptr);
                buffptr = getField(entry.compressedSize, buffptr);
                buffptr = getField(entry.uncompressedSize, buffptr);

                if (zipFileeam.fail()) {
                    throw Exception("Error in reading Data Descriptor Record.");
                }

            } else {
                throw Exception("No Data Descriptor record found.");
            }

        }

        //
        // Read Central Directory File Header record from ZIP archive.
        //

        void CZIPIO::readZIPRecord(std::fstream &zipFileeam, CZIPIO::CentralDirectoryFileHeader& entry) {

            std::vector<std::uint8_t> buffer(entry.size);
            std::uint8_t *buffptr = &buffer[0];
            std::uint32_t signature;

            zipFileeam.read((char *) buffptr, sizeof (signature));
            buffptr = getField(signature, buffptr);

            if (signature == entry.signature) {

                zipFileeam.read((char *) buffptr, entry.size - sizeof (signature));

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

                zipFileeam.read((char *) &buffer[0], entry.fileNameLength + entry.extraFieldLength + entry.fileCommentLength);

                if (entry.fileNameLength) {
                    entry.fileName.append((char *) &buffer[0], entry.fileNameLength);
                }
                if (entry.extraFieldLength) {
                    entry.extraField.resize(entry.extraFieldLength);
                    std::memcpy(&entry.extraField[0], &buffer[entry.fileNameLength], entry.extraFieldLength);
                }
                if (entry.fileCommentLength) {
                    entry.fileName.append((char *) &buffer[entry.fileNameLength + entry.extraFieldLength], entry.fileCommentLength);
                }

                if (zipFileeam.fail()) {
                    throw Exception("Error in reading Central Directory Local File Header record.");
                }

            } else {
                throw Exception("No Central Directory File Header found.");
            }

        }

        //
        // Read Local File Header record from ZIP archive.
        //

        void CZIPIO::readZIPRecord(std::fstream &zipFileeam, CZIPIO::LocalFileHeader& entry) {

            std::vector<std::uint8_t> buffer(entry.size);
            std::uint8_t *buffptr = &buffer[0];
            std::uint32_t signature;

            zipFileeam.read((char *) buffptr, sizeof (signature));
            buffptr = getField(signature, buffptr);

            if (signature == entry.signature) {

                zipFileeam.read((char *) buffptr, entry.size - sizeof (signature));

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

                zipFileeam.read((char *) &buffer[0], entry.fileNameLength + entry.extraFieldLength);

                if (entry.fileNameLength) {
                    entry.fileName.append((char *) &buffer[0], entry.fileNameLength);
                }

                if (entry.extraFieldLength) {
                    entry.extraField.resize(entry.extraFieldLength);
                    std::memcpy(&entry.extraField[0], &buffer[entry.fileNameLength], entry.extraFieldLength);
                }

                if (zipFileeam.fail()) {
                    throw Exception("Error in reading Local File Header record.");
                }


            } else {
                throw Exception("No Local File Header record found.");
            }


        }

        //
        // Read End Of Central Directory File Header record from ZIP archive.
        //

        void CZIPIO::readZIPRecord(std::fstream &zipFileeam, CZIPIO::EOCentralDirectoryRecord& entry) {

            zipFileeam.seekg(0, std::ios_base::end);
            std::uint64_t fileLength = zipFileeam.tellg();
            int64_t filePosition = fileLength - 1;
            std::uint32_t signature = 0;

            // Read file in reverse looking for End Of Central Directory File Header signature

            while (filePosition) {
                char nextByte;
                zipFileeam.seekg(filePosition, std::ios_base::beg);
                zipFileeam.get(nextByte);
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

                zipFileeam.seekg(filePosition + sizeof (signature), std::ios_base::beg);
                zipFileeam.read((char *) buffptr, entry.size - sizeof (signature));

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
                    zipFileeam.read((char *) &buffer[0], entry.commentLength);
                    entry.comment.resize(entry.commentLength);
                    entry.comment.append((char *) &buffer[0], entry.commentLength);
                }

                if (zipFileeam.fail()) {
                    throw Exception("Error in reading End Of Central Directory record.");
                }

            } else {
                throw Exception("No End Of Central Directory record found.");
            }
        }

        //
        // Read ZIP64 End Of Central Directory record from ZIP archive.
        //

        void CZIPIO::readZIPRecord(std::fstream &zipFileeam, CZIPIO::Zip64EOCentralDirectoryRecord& entry) {

            std::vector<std::uint8_t> buffer(entry.size);
            std::uint8_t *buffptr = &buffer[0];
            std::uint32_t signature;
            std::uint64_t extensionSize;
            Zip64EOCentDirRecordLocator zip64EOCentralDirLocator;

            readZIPRecord(zipFileeam, zip64EOCentralDirLocator);
            zipFileeam.seekg(zip64EOCentralDirLocator.offset, std::ios::beg);

            zipFileeam.read((char *) buffptr, sizeof (signature));
            buffptr = getField(signature, buffptr);

            if (signature == entry.signature) {

                zipFileeam.read((char *) buffptr, entry.size - sizeof (signature));

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
                    zipFileeam.read((char *) &entry.extensibleDataSector[0], extensionSize);
                }

                if (zipFileeam.fail()) {
                    throw Exception("Error in reading ZIP64 End Of Central Directory record.");
                }

            } else {
                throw Exception("No ZIP64 End Of Central Directory record found.");
            }

        }

        //
        // Read ZIP64 End Of Central Directory record locator from ZIP archive
        //

        void CZIPIO::readZIPRecord(std::fstream &zipFileeam, CZIPIO::Zip64EOCentDirRecordLocator& entry) {

            zipFileeam.seekg(0, std::ios_base::end);
            std::uint64_t fileLength = zipFileeam.tellg();
            int64_t filePosition = fileLength - 1;
            std::uint32_t signature = 0;

            // Read file in reverse looking for End Of Central Directory File Header signature

            while (filePosition) {
                char nextByte;
                zipFileeam.seekg(filePosition, std::ios_base::beg);
                zipFileeam.get(nextByte);
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

                zipFileeam.seekg(filePosition + sizeof (signature), std::ios_base::beg);
                zipFileeam.read((char *) buffptr, entry.size - sizeof (signature));

                buffptr = getField(entry.startDiskNumber, buffptr);
                buffptr = getField(entry.offset, buffptr);
                buffptr = getField(entry.numberOfDisks, buffptr);

                if (zipFileeam.fail()) {
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

        CZIPIO::CZIPIO() {

        }

        //
        // Destructor
        //

        CZIPIO::~CZIPIO() {

        }

        //
        // Open ZIP archive for I/O.
        //

        void CZIPIO::openZIPFile(const std::string fileName, std::ios_base::openmode mode) {

            m_zipFileeam.open(fileName, mode);

            if (m_zipFileeam.fail()) {
                throw Exception("Could not open ZIP archive " + fileName);
            }

        }

        //
        // Close ZIP archive.
        //

        void CZIPIO::closeZIPFile(void) {
            m_zipFileeam.close();
        }

        //
        // Move to position in ZIP archive.
        //

        void CZIPIO::positionInZIPFile(std::uint64_t offset) {
            m_zipFileeam.seekg(offset, std::ios::beg);
        }

        //
        // Return current position within ZIP archive.
        //

        std::uint64_t CZIPIO::currentPositionZIPFile(void) {
            return (m_zipFileeam.tellg());
        }

        //
        // Write data to ZIP archive.
        //

        void CZIPIO::writeZIPFile(std::vector<std::uint8_t>& buffer, std::uint64_t count) {
            m_zipFileeam.write((char *) &buffer[0], count);
        }

        //
        // Read data from ZIP archive.
        //

        void CZIPIO::readZIPFile(std::vector<std::uint8_t>& buffer, std::uint64_t count) {
            m_zipFileeam.read((char *) &buffer[0], count);
        }

        //
        // Return amount of data returned from last read.
        //

        std::uint64_t CZIPIO::readCountZIPFile() {
            return (m_zipFileeam.gcount());
        }

        //
        // Return true if error in ZIP archive I/O..
        //

        bool CZIPIO::errorInZIPFile(void) {
            return (m_zipFileeam.fail());
        }

        //
        // Write Data Descriptor record into buffer to disk.
        //

        void CZIPIO::putZIPRecord(CZIPIO::DataDescriptor& entry) {

            writeZIPRecord(m_zipFileeam, entry);

        }

        //
        // Write Central Directory File Header record to disk.
        //

        void CZIPIO::putZIPRecord(CZIPIO::CentralDirectoryFileHeader& entry) {

            writeZIPRecord(m_zipFileeam, entry);

        }

        //
        // Write Local File Header record to disk.
        //

        void CZIPIO::putZIPRecord(CZIPIO::LocalFileHeader& entry) {

            writeZIPRecord(m_zipFileeam, entry);

        }

        //
        // Write End Of Central Directory record to disk.
        //

        void CZIPIO::putZIPRecord(CZIPIO::EOCentralDirectoryRecord& entry) {

            writeZIPRecord(m_zipFileeam, entry);

        }

        //
        // Write ZIP64 End Of Central Directory record to disk.
        //

        void CZIPIO::putZIPRecord(CZIPIO::Zip64EOCentralDirectoryRecord& entry) {

            writeZIPRecord(m_zipFileeam, entry);

        }

        //
        // Write ZIP64 End Of Central Directory record locator to disk.
        //

        void CZIPIO::putZIPRecord(CZIPIO::Zip64EOCentDirRecordLocator& entry) {

            writeZIPRecord(m_zipFileeam, entry);

        }

        //
        // Put any ZIP64 extended information record into byte array. Only perform if the
        // value is too large for its default storage. Sizes are stored as pair because
        // of the requirement for Local file headers.
        //

        void CZIPIO::putZip64ExtendedInfoExtraField(Zip64ExtendedInfoExtraField& extendedInfo, std::vector<std::uint8_t>& info) {

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

        void CZIPIO::getZIPRecord(CZIPIO::DataDescriptor& entry) {

            readZIPRecord(m_zipFileeam, entry);

        }

        //
        // Get Central Directory File Header record from ZIP archive.
        //

        void CZIPIO::getZIPRecord(CZIPIO::CentralDirectoryFileHeader& entry) {

            readZIPRecord(m_zipFileeam, entry);

        }

        //
        // Get Local File Header record from ZIP archive.
        //

        void CZIPIO::getZIPRecord(CZIPIO::LocalFileHeader& entry) {

            readZIPRecord(m_zipFileeam, entry);

        }

        //
        // Get End Of Central Directory File Header record from ZIP archive.
        //

        void CZIPIO::getZIPRecord(CZIPIO::EOCentralDirectoryRecord& entry) {

            readZIPRecord(m_zipFileeam, entry);

        }

        //
        // Get ZIP64 End Of Central Directory record from ZIP archive
        //

        void CZIPIO::getZIPRecord(CZIPIO::Zip64EOCentralDirectoryRecord& entry) {

            readZIPRecord(m_zipFileeam, entry);

        }

        //
        // Get any ZIP64 extended information from byte array.
        //

        void CZIPIO::getZip64ExtendedInfoExtraField(Zip64ExtendedInfoExtraField& zip64ExtendedInfo, std::vector<std::uint8_t>& info) {

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

    } // namespace ZIP
} // namespace Antik