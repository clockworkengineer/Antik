#include "HOST.hpp"
/*
 * File:   CZIP.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on January 24, 2017, 2:33 PM
 *
 * Copyright 2016.
 *
 */

//
// Class: CZIP
//
// Description:  Class to create and manipulate ZIP file archives. At present it
// supports archive creation and addition/extraction of files from an existing
// archives; ZIP64 extensions are also supported. Files are either saved
// using store (file copy) or deflate compression. Use is made of the stat64 API
// instead of stat for 64 bit files. The current class compiles and works on
// Linux/CYGWIN and it marks the archives as created on Unix.
//
// Dependencies:   C11++     - Language standard features used.
//                 ziplib    - File compression/decompression
//                 Linux     - stat64 call for file information.
//

// =================
// CLASS DEFINITIONS
// =================

// ====================
// CLASS IMPLEMENTATION
// ====================

#include "CZIP.hpp"

//
// C++ STL
//

#include <iostream>
#include <cstring>

//
// Ziplib and Linux stat64 file interface
//

#include <zlib.h>
#include <sys/stat.h>

// =========
// NAMESPACE
// =========

namespace Antik::ZIP
{

// ===========================
// PRIVATE TYPES AND CONSTANTS
// ===========================

//
// ZIP deflate/inflate default buffer size
//

const std::uint64_t CZIP::kZIPDefaultBufferSize;

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
// Convert  ZIP format (MSDOS) based modified date/time to Linux tm format.
//

std::tm CZIP::convertModificationDateTime(std::uint16_t dateWord, std::uint16_t timeWord)
{

    std::time_t rawtime = 0;
    std::tm modificationDateTime;
    std::time(&rawtime);
    std::memcpy(&modificationDateTime, std::localtime(&rawtime), sizeof(std::tm));

    modificationDateTime.tm_sec = (timeWord & 0b11111) >> 2;
    modificationDateTime.tm_min = (timeWord & 0b11111100000) >> 5;
    modificationDateTime.tm_hour = (timeWord & 0b1111100000000000) >> 11;
    modificationDateTime.tm_mday = (dateWord & 0b11111);
    modificationDateTime.tm_mon = ((dateWord & 0b111100000) >> 5) - 1;
    modificationDateTime.tm_year = ((dateWord & 0b1111111000000000) >> 9) + 80;

    mktime(&modificationDateTime);

    return (modificationDateTime);
}

//
// Uncompress ZIP local file header  data to file. Note: The files crc32 is calculated
// while the data is being inflated and returned.
//

std::uint32_t CZIP::inflateFile(const std::string &fileName, std::uint64_t fileSize)
{

    int inflateResult = Z_OK;
    std::uint64_t inflatedBytes = 0;
    z_stream inlateZIPStream{};
    std::ofstream fileStream(fileName, std::ios::binary | std::ios::trunc);
    std::uint32_t crc;

    if (fileStream.fail())
    {
        throw Exception("Could not open destination file for inflate.");
    }

    crc = crc32(0L, Z_NULL, 0);

    if (fileSize == 0)
    {
        return (crc);
    }

    inflateResult = inflateInit2(&inlateZIPStream, -MAX_WBITS);
    if (inflateResult != Z_OK)
    {
        throw Exception("inflateInit2() Error = " + std::to_string(inflateResult));
    }

    do
    {

        readZIPFile(m_zipInBuffer, std::min(fileSize, m_zipIOBufferSize));

        if (errorInZIPFile())
        {
            inflateEnd(&inlateZIPStream);
            throw Exception("Error reading ZIP archive file during inflate.");
        }

        inlateZIPStream.avail_in = readCountZIPFile();
        if (inlateZIPStream.avail_in == 0)
        {
            break;
        }

        inlateZIPStream.next_in = (Bytef *)&m_zipInBuffer[0];

        do
        {

            inlateZIPStream.avail_out = m_zipIOBufferSize;
            inlateZIPStream.next_out = (Bytef *)&m_zipOutBuffer[0];

            inflateResult = inflate(&inlateZIPStream, Z_NO_FLUSH);
            switch (inflateResult)
            {
            case Z_NEED_DICT:
                inflateResult = Z_DATA_ERROR;
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                inflateEnd(&inlateZIPStream);
                throw Exception("Error inflating ZIP archive. = " + std::to_string(inflateResult));
            }

            inflatedBytes = m_zipIOBufferSize - inlateZIPStream.avail_out;
            fileStream.write((char *)&m_zipOutBuffer[0], inflatedBytes);
            if (fileStream.fail())
            {
                inflateEnd(&inlateZIPStream);
                throw Exception("Error writing to file during inflate.");
            }

            crc = crc32(crc, &m_zipOutBuffer[0], inflatedBytes);

        } while (inlateZIPStream.avail_out == 0);

        fileSize -= m_zipIOBufferSize;

    } while (inflateResult != Z_STREAM_END);

    inflateEnd(&inlateZIPStream);

    return (crc);
}

//
// Compress source file and write as part of ZIP local file header record. The files
// crc32 is calculated  while the data is being deflated. The crc32 and compressed
// size are returned though a pair.
//

std::pair<std::uint32_t, std::uint64_t> CZIP::deflateFile(const std::string &fileName, std::uint64_t fileSize)
{

    int deflateResult = 0, flushRemainder = 0;
    std::uint64_t bytesDeflated = 0;
    z_stream deflateZIPStream{};
    std::ifstream fileStream(fileName, std::ios::binary);
    std::uint32_t crc;
    std::uint64_t compressedSize = 0;

    if (fileStream.fail())
    {
        throw Exception("Could not open source file for deflate.");
    }

    crc = crc32(0L, Z_NULL, 0);

    deflateResult = deflateInit2(&deflateZIPStream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
    if (deflateResult != Z_OK)
    {
        throw Exception("deflateInit2() Error = " + std::to_string(deflateResult));
    }

    do
    {

        fileStream.read((char *)&m_zipInBuffer[0], std::min(fileSize, m_zipIOBufferSize));
        if (fileStream.fail() && !fileStream.eof())
        {
            deflateEnd(&deflateZIPStream);
            throw Exception("Error reading source file to deflate.");
        }

        deflateZIPStream.avail_in = fileStream.gcount();
        fileSize -= deflateZIPStream.avail_in;

        crc = crc32(crc, &m_zipInBuffer[0], deflateZIPStream.avail_in);

        flushRemainder = ((fileStream.eof() || fileSize == 0)) ? Z_FINISH : Z_NO_FLUSH;

        deflateZIPStream.next_in = &m_zipInBuffer[0];

        do
        {

            deflateZIPStream.avail_out = m_zipIOBufferSize;
            deflateZIPStream.next_out = &m_zipOutBuffer[0];
            deflateResult = deflate(&deflateZIPStream, flushRemainder); /* no bad return value */

            bytesDeflated = m_zipIOBufferSize - deflateZIPStream.avail_out;
            writeZIPFile(m_zipOutBuffer, bytesDeflated);
            if (errorInZIPFile())
            {
                deflateEnd(&deflateZIPStream);
                throw Exception("Error writing deflated data to ZIP archive.");
            }

            compressedSize += bytesDeflated;

        } while (deflateZIPStream.avail_out == 0);

    } while (flushRemainder != Z_FINISH);

    deflateEnd(&deflateZIPStream);

    fileStream.close();

    return (std::make_pair(crc, compressedSize));
}

//
// Extract uncompressed (stored) ZIP local file header  data to file. Note: The files
// crc32 is calculated while the data being is copied and returned.
//

std::uint32_t CZIP::extractFile(const std::string &fileName, std::uint64_t fileSize)
{

    std::uint32_t crc;
    crc = crc32(0L, Z_NULL, 0);
    std::ofstream fileStream(fileName, std::ios::binary | std::ios::trunc);

    if (fileStream.fail())
    {
        throw Exception("Could not open destination file for extract.");
    }

    while (fileSize)
    {
        readZIPFile(m_zipInBuffer, std::min(fileSize, m_zipIOBufferSize));
        if (errorInZIPFile())
        {
            throw Exception("Error in reading ZIP archive file.");
        }
        crc = crc32(crc, &m_zipInBuffer[0], readCountZIPFile());
        fileStream.write((char *)&m_zipInBuffer[0], readCountZIPFile());
        if (fileStream.fail())
        {
            throw Exception("Error in writing extracted file.");
        }
        fileSize -= (std::min(fileSize, m_zipIOBufferSize));
    }

    return (crc);
}

//
// Store file as part of ZIP archive local file header.
//

void CZIP::storeFile(const std::string &fileName, std::uint64_t fileSize)
{

    std::ifstream fileStream(fileName, std::ios::binary);

    if (fileStream.fail())
    {
        throw Exception("Could not open source file for store.");
    }

    while (fileSize)
    {

        fileStream.read((char *)&m_zipInBuffer[0], std::min(fileSize, m_zipIOBufferSize));
        if (fileStream.fail())
        {
            throw Exception("Error reading source file to store in ZIP archive.");
        }

        writeZIPFile(m_zipInBuffer, fileStream.gcount());
        if (errorInZIPFile())
        {
            throw Exception("Error writing to ZIP archive.");
        }

        fileSize -= (std::min(fileSize, m_zipIOBufferSize));
    }
}

//
// Get a files Linux attributes. Note: To convert to ZIP file  format just
// shift 16 bits left.
//

std::uint32_t CZIP::getFileAttributes(const std::string &fileName)
{

    struct stat64 fileStat
    {
    };
    std::uint32_t attributes = 0;

    int rc = lstat64(fileName.c_str(), &fileStat);

    if (rc == 0)
    {
        attributes = fileStat.st_mode;
        attributes <<= 16;
    }
    else
    {
        throw Exception("stat() error getting file attributes. ERRNO = " + std::to_string(errno));
    }

    return (attributes);
}

//
// Get a files size. Directories have size 0.
//

std::uint64_t CZIP::getFileSize(const std::string &fileName)
{

    struct stat64 fileStat
    {
    };
    std::uint64_t fileSize = 0;

    int rc = lstat64(fileName.c_str(), &fileStat);
    if (rc == 0)
    {
        if (S_ISDIR(fileStat.st_mode))
        {
            fileSize = 0;
        }
        else
        {
            fileSize = fileStat.st_size;
        }
    }
    else
    {
        throw Exception("stat() error getting file size. ERRNO = " + std::to_string(errno));
    }

    return (fileSize);
}

//
// Return true if a files exists.
//

bool CZIP::fileExists(const std::string &fileName)
{

    struct stat64 fileStat
    {
    };

    int rc = lstat64(fileName.c_str(), &fileStat);
    if (rc != 0)
    {
        throw Exception("stat() error getting file size. ERRNO = " + std::to_string(errno));
    }
    return (rc == 0);
}

//
// Get files stat64 based modified date/time and convert to ZIP format. The values
// are passed back through a std::pair.
//

std::pair<std::uint16_t, std::uint16_t> CZIP::getFileModificationDateTime(const std::string &fileName)
{

    struct stat64 fileStat
    {
    };
    std::uint16_t modificatioDate, modificationTime;

    int rc = lstat64(fileName.c_str(), &fileStat);
    if (rc == 0)
    {
        struct std::tm *fileTimeInfo = std::localtime(&fileStat.st_mtime);
        modificationTime = (fileTimeInfo->tm_sec & 0b11111) |
                           ((fileTimeInfo->tm_min & 0b111111) << 5) |
                           ((fileTimeInfo->tm_hour & 0b11111) << 11);
        modificatioDate = (fileTimeInfo->tm_mday & 0b11111) |
                          ((((fileTimeInfo->tm_mon + 1) & 0b1111)) << 5) |
                          (((fileTimeInfo->tm_year - 80) & 0b1111111) << 9);
    }
    else
    {
        throw Exception("stat() error getting file modified time. ERRNO = " + std::to_string(errno));
    }

    return (std::make_pair(modificatioDate, modificationTime));
}

//
// Add a Local File Header record and file contents to ZIP file. Note: Also add
// an entry to central directory for flushing out to the archive on close. Any files
// that are > 4GB are stored using ZIP64 format extensions.
//

void CZIP::addFileHeaderAndContents(const std::string &fileName, const std::string &zippedFileName)
{

    LocalFileHeader fileHeader;
    CentralDirectoryFileHeader directoryEntry;
    Zip64ExtendedInfoExtraField info;
    bool bZIP64 = false;

    // Work from extended information 64 bit sizes

    info.fileHeaderOffset = m_offsetToEndOfLocalFileHeaders;
    info.originalSize = getFileSize(fileName);
    info.compressedSize = info.originalSize;

    // Save filename details

    directoryEntry.fileName = zippedFileName;
    directoryEntry.fileNameLength = directoryEntry.fileName.length();

    // If current offset > 32 bits use ZIP64

    if (fieldRequires64bits(info.fileHeaderOffset))
    {
        directoryEntry.fileHeaderOffset = static_cast<std::uint32_t>(~0);
        bZIP64 = true;
    }
    else
    {
        directoryEntry.fileHeaderOffset = info.fileHeaderOffset;
    }

    // File size > 32 bits then use ZIP64

    if (fieldRequires64bits(info.originalSize))
    {
        directoryEntry.uncompressedSize = static_cast<std::uint32_t>(~0);
        directoryEntry.compressedSize = static_cast<std::uint32_t>(~0);
        bZIP64 = true;
    }
    else
    {
        directoryEntry.uncompressedSize = info.originalSize;
        directoryEntry.compressedSize = info.compressedSize;
    }

    // Get file modified time and attributes.

    std::pair<std::uint16_t, std::uint16_t> modification = getFileModificationDateTime(fileName);

    directoryEntry.modificationDate = modification.first;
    directoryEntry.modificationTime = modification.second;
    directoryEntry.externalFileAttrib = getFileAttributes(fileName);

    // File is a directory so add trailing delimeter, set no compression and extractor version  1.0

    if (S_ISDIR(directoryEntry.externalFileAttrib >> 16))
    {
        if (directoryEntry.fileName.back() != '/')
        {
            directoryEntry.fileName.push_back('/');
            directoryEntry.fileNameLength++;
        }
        directoryEntry.extractorVersion = kZIPVersion10;
        directoryEntry.creatorVersion = (kZIPCreatorUnix << 8) | kZIPVersion10;
        directoryEntry.compression = kZIPCompressionStore;
    }

    // > 4 GB Files so ZIP64. Values not able to be stored in 32 bits have
    // there fields set to all ones and values placed in the extended
    // information field where their format has more bits.

    if (bZIP64)
    {
        m_ZIP64 = true;
        directoryEntry.extractorVersion = kZIPVersion45;
        directoryEntry.creatorVersion = (kZIPCreatorUnix << 8) | (kZIPVersion45);
        putZip64ExtendedInfoExtraField(info, directoryEntry.extraField);
        directoryEntry.extraFieldLength = directoryEntry.extraField.size();
    }

    // Copy information for file header and write to disk

    fileHeader.creatorVersion = directoryEntry.creatorVersion;
    fileHeader.bitFlag = directoryEntry.bitFlag;
    fileHeader.compression = directoryEntry.compression;
    fileHeader.modificationTime = directoryEntry.modificationTime;
    fileHeader.modificationDate = directoryEntry.modificationDate;
    fileHeader.uncompressedSize = directoryEntry.uncompressedSize;
    fileHeader.compressedSize = directoryEntry.compressedSize;
    fileHeader.fileNameLength = directoryEntry.fileNameLength;
    fileHeader.extraFieldLength = directoryEntry.extraFieldLength;
    fileHeader.fileName = directoryEntry.fileName;
    fileHeader.extraField = directoryEntry.extraField;

    positionInZIPFile(m_offsetToEndOfLocalFileHeaders);
    putZIPRecord(fileHeader);

    // Write any file contents next

    if (info.originalSize)
    {

        // Calculate files compressed size while deflating it and then either modify its
        // Local File Header record to have the correct compressed size and CRC or if its
        // compressed size is greater then or equal to its original size then store file
        // instead of compress.

        std::pair<std::uint32_t, std::int64_t> deflateValues = deflateFile(fileName, info.originalSize);

        fileHeader.crc32 = directoryEntry.crc32 = deflateValues.first;
        info.compressedSize = deflateValues.second;

        // Save away current position next file header

        m_offsetToEndOfLocalFileHeaders = currentPositionZIPFile();

        // Back up to beginning of current local file header

        positionInZIPFile(info.fileHeaderOffset);

        // Rewrite local file header with compressed size if compressed file
        // smaller or if ZIP64 format.

        if ((info.compressedSize < info.originalSize) || bZIP64)
        {
            if (bZIP64)
            {
                putZip64ExtendedInfoExtraField(info, directoryEntry.extraField);
                fileHeader.extraField = directoryEntry.extraField;
            }
            else
            {
                fileHeader.compressedSize = directoryEntry.compressedSize = info.compressedSize;
            }
            putZIPRecord(fileHeader);
        }
        else
        {
            // Store non-compressed file.
            directoryEntry.extractorVersion = kZIPVersion10;
            fileHeader.creatorVersion = (kZIPCreatorUnix << 8) | kZIPVersion10;
            fileHeader.compression = directoryEntry.compression = kZIPCompressionStore;
            fileHeader.compressedSize = directoryEntry.compressedSize = info.originalSize;
            putZIPRecord(fileHeader);
            storeFile(fileName, info.originalSize);
            m_offsetToEndOfLocalFileHeaders = currentPositionZIPFile();
        }
    }
    else
    {
        m_offsetToEndOfLocalFileHeaders = currentPositionZIPFile();
    }

    // Save Central Directory File Entry

    m_zipCentralDirectory.push_back(directoryEntry);

    m_modified = true;
}

//
// Update a ZIP archives Central Directory.
//

void CZIP::UpdateCentralDirectory(void)
{

    if (m_modified)
    {

        EOCentralDirectoryRecord zipEOCentralDirectory;
        Zip64EOCentralDirectoryRecord zip64EOCentralDirectory;
        bool bZIP64 = false;

        // Position to end of local file headers

        positionInZIPFile(m_offsetToEndOfLocalFileHeaders);

        // Initialise central directory offset and size

        zip64EOCentralDirectory.numberOfCentralDirRecords = m_zipCentralDirectory.size();
        zip64EOCentralDirectory.totalCentralDirRecords = m_zipCentralDirectory.size();
        zip64EOCentralDirectory.offsetCentralDirRecords = currentPositionZIPFile();

        // Write Central Directory to ZIP archive

        for (auto &directoryEntry : m_zipCentralDirectory)
        {
            putZIPRecord(directoryEntry);
        }

        // Calculate Central Directory size in byes.

        zip64EOCentralDirectory.sizeOfCentralDirRecords = currentPositionZIPFile();
        zip64EOCentralDirectory.sizeOfCentralDirRecords -= zip64EOCentralDirectory.offsetCentralDirRecords;

        // Number of records 16 bit overflow so use ZIP64 ie. 32 bits

        if (fieldRequires32bits(zip64EOCentralDirectory.numberOfCentralDirRecords))
        {
            zipEOCentralDirectory.numberOfCentralDirRecords = static_cast<std::uint16_t>(~0);
            bZIP64 = true;
        }
        else
        {
            zipEOCentralDirectory.numberOfCentralDirRecords = zip64EOCentralDirectory.numberOfCentralDirRecords;
        }

        // Total number of records 16 bit overflow so use ZIP64 ie. 32 bits

        if (fieldRequires32bits(zip64EOCentralDirectory.totalCentralDirRecords))
        {
            zipEOCentralDirectory.totalCentralDirRecords = static_cast<std::uint16_t>(~0);
            bZIP64 = true;
        }
        else
        {
            zipEOCentralDirectory.totalCentralDirRecords = zip64EOCentralDirectory.totalCentralDirRecords;
        }

        // Offset 32 bit overflow so use ZIP64 ie. 64 bits

        if (fieldRequires64bits(zip64EOCentralDirectory.offsetCentralDirRecords))
        {
            zipEOCentralDirectory.offsetCentralDirRecords = static_cast<std::uint32_t>(~0);
            bZIP64 = true;
        }
        else
        {
            zipEOCentralDirectory.offsetCentralDirRecords = zip64EOCentralDirectory.offsetCentralDirRecords;
        }

        // Central Directory size 32 bit overflow so use ZIP64 ie. 64 bits

        if (fieldRequires64bits(zip64EOCentralDirectory.sizeOfCentralDirRecords))
        {
            zipEOCentralDirectory.sizeOfCentralDirRecords = static_cast<std::uint32_t>(~0);
            bZIP64 = true;
        }
        else
        {
            zipEOCentralDirectory.sizeOfCentralDirRecords = zip64EOCentralDirectory.sizeOfCentralDirRecords;
        }

        // Central Directory start disk 16 bit overflow so use ZIP64 ie. 32 bits

        if (fieldRequires32bits(zip64EOCentralDirectory.startDiskNumber))
        {
            zipEOCentralDirectory.startDiskNumber = static_cast<std::uint16_t>(~0);
            bZIP64 = true;
        }
        else
        {
            zipEOCentralDirectory.startDiskNumber = zip64EOCentralDirectory.startDiskNumber;
        }

        // Central Directory number of disks 16 bit overflow so use ZIP64 ie. 32 bits

        if (fieldRequires32bits(zip64EOCentralDirectory.diskNumber))
        {
            zipEOCentralDirectory.diskNumber = static_cast<std::uint16_t>(~0);
            bZIP64 = true;
        }
        else
        {
            zipEOCentralDirectory.diskNumber = zip64EOCentralDirectory.diskNumber;
        }

        // ZIP64 so write extension records

        if (bZIP64)
        {
            Zip64EOCentDirRecordLocator locator;
            locator.offset = currentPositionZIPFile();
            putZIPRecord(zip64EOCentralDirectory);
            putZIPRecord(locator);
        }

        // Write End Of Central Directory record

        putZIPRecord(zipEOCentralDirectory);
    }
}

// ==============
// PUBLIC METHODS
// ==============

//
// Constructor
//

CZIP::CZIP(const std::string &zipFileName) : m_zipFileName{zipFileName}
{

    m_zipInBuffer.resize(m_zipIOBufferSize);
    m_zipOutBuffer.resize(m_zipIOBufferSize);
}

//
// Destructor
//

CZIP::~CZIP()
{
}

//
// Set ZIP archive name
//

void CZIP::name(const std::string &zipFileName)
{

    m_zipFileName = zipFileName;
}

//
// Open ZIP archive and read in Central Directory Header records.
//

void CZIP::open(void)
{

    if (m_open)
    {
        throw Exception("ZIP archive has already been opened.");
    }

    EOCentralDirectoryRecord zipEOCentralDirectory;
    Zip64EOCentralDirectoryRecord zip64EOCentralDirectory;

    openZIPFile(m_zipFileName, std::ios::binary | std::ios_base::in | std::ios_base::out);

    std::int64_t noOfFileRecords = 0;

    getZIPRecord(zipEOCentralDirectory);

    // If one of the central directory fields is to large to store so ZIP64

    if (fieldOverflow(zipEOCentralDirectory.totalCentralDirRecords) ||
        fieldOverflow(zipEOCentralDirectory.numberOfCentralDirRecords) ||
        fieldOverflow(zipEOCentralDirectory.sizeOfCentralDirRecords) ||
        fieldOverflow(zipEOCentralDirectory.totalCentralDirRecords) ||
        fieldOverflow(zipEOCentralDirectory.startDiskNumber) ||
        fieldOverflow(zipEOCentralDirectory.diskNumber) ||
        fieldOverflow(zipEOCentralDirectory.offsetCentralDirRecords))
    {

        m_ZIP64 = true;
        getZIPRecord(zip64EOCentralDirectory);
        positionInZIPFile(zip64EOCentralDirectory.offsetCentralDirRecords);
        noOfFileRecords = zip64EOCentralDirectory.numberOfCentralDirRecords;
        m_offsetToEndOfLocalFileHeaders = zip64EOCentralDirectory.offsetCentralDirRecords;
    }
    else
    {
        // Normal Archive
        positionInZIPFile(zipEOCentralDirectory.offsetCentralDirRecords);
        noOfFileRecords = zipEOCentralDirectory.numberOfCentralDirRecords;
        m_offsetToEndOfLocalFileHeaders = zipEOCentralDirectory.offsetCentralDirRecords;
    }

    // Read in Central Directory

    for (auto cnt01 = 0; cnt01 < noOfFileRecords; cnt01++)
    {
        CentralDirectoryFileHeader directoryEntry;
        getZIPRecord(directoryEntry);
        m_zipCentralDirectory.push_back(directoryEntry);
        m_ZIP64 = fieldOverflow(directoryEntry.compressedSize) ||
                  fieldOverflow(directoryEntry.uncompressedSize) ||
                  fieldOverflow(directoryEntry.fileHeaderOffset);
    }

    m_open = true;
}

//
// Read Central Directory and return a list of ZIP archive contents.
//

std::vector<CZIP::FileDetail> CZIP::contents(void)
{

    std::vector<FileDetail> fileDetailList;

    if (!m_open)
    {
        throw Exception("ZIP archive has not been opened.");
    }

    for (auto &directoryEntry : m_zipCentralDirectory)
    {

        FileDetail fileEntry;

        fileEntry.fileName = directoryEntry.fileName;
        fileEntry.fileComment = directoryEntry.fileComment;
        fileEntry.uncompressedSize = directoryEntry.uncompressedSize;
        fileEntry.compressedSize = directoryEntry.compressedSize;
        fileEntry.compression = directoryEntry.compression;
        fileEntry.externalFileAttrib = directoryEntry.externalFileAttrib;
        fileEntry.creatorVersion = directoryEntry.creatorVersion;
        fileEntry.extraField = directoryEntry.extraField;
        fileEntry.modificationDateTime =
            convertModificationDateTime(directoryEntry.modificationDate,
                                        directoryEntry.modificationTime);

        // File size information stored in Extended information.

        if (fieldOverflow(directoryEntry.compressedSize) ||
            fieldOverflow(directoryEntry.uncompressedSize) ||
            fieldOverflow(directoryEntry.fileHeaderOffset))
        {

            Zip64ExtendedInfoExtraField extra;
            extra.compressedSize = directoryEntry.compressedSize;
            extra.fileHeaderOffset = directoryEntry.fileHeaderOffset;
            extra.originalSize = directoryEntry.uncompressedSize;
            getZip64ExtendedInfoExtraField(extra, fileEntry.extraField);
            fileEntry.uncompressedSize = extra.originalSize;
            fileEntry.compressedSize = extra.compressedSize;
            fileEntry.bZIP64 = true;
        }

        fileDetailList.push_back(fileEntry);
    }

    return (fileDetailList);
}

//
// Extract a ZIP archive file and create in a specified destination.
//

bool CZIP::extract(const std::string &fileName, const std::string &destFileName)
{

    bool fileExtracted = false;

    if (!m_open)
    {
        throw Exception("ZIP archive has not been opened.");
    }

    for (auto &directoryEntry : m_zipCentralDirectory)
    {

        if (directoryEntry.fileName.compare(fileName) == 0)
        {

            Zip64ExtendedInfoExtraField extendedInfo;
            LocalFileHeader fileHeader;
            std::uint32_t crc32;

            // Set up 64 bit data values if needed

            extendedInfo.compressedSize = directoryEntry.compressedSize;
            extendedInfo.originalSize = directoryEntry.uncompressedSize;
            extendedInfo.fileHeaderOffset = directoryEntry.fileHeaderOffset;

            // If dealing with ZIP64 extract full 64 bit values from extended field

            if (fieldOverflow(directoryEntry.compressedSize) ||
                fieldOverflow(directoryEntry.uncompressedSize) ||
                fieldOverflow(directoryEntry.fileHeaderOffset))
            {
                getZip64ExtendedInfoExtraField(extendedInfo, directoryEntry.extraField);
            }

            // Move to and read file header

            positionInZIPFile(extendedInfo.fileHeaderOffset);
            getZIPRecord(fileHeader);

            // Now positioned at file contents so extract

            if (directoryEntry.compression == kZIPCompressionDeflate)
            {
                crc32 = inflateFile(destFileName, extendedInfo.compressedSize);
                fileExtracted = true;
            }
            else if (directoryEntry.compression == kZIPCompressionStore)
            {
                crc32 = extractFile(destFileName, extendedInfo.originalSize);
                fileExtracted = true;
            }
            else
            {
                throw Exception("File uses unsupported compression = " + std::to_string(directoryEntry.compression));
            }

            // Check file CRC32

            if (crc32 != directoryEntry.crc32)
            {
                throw Exception("File " + destFileName + " has an invalid CRC.");
            }

            break;
        }
    }

    return (fileExtracted);
}

//
// Create an empty ZIP archive.
//

void CZIP::create(void)
{

    if (m_open)
    {
        throw Exception("ZIP archive should not be open.");
    }

    EOCentralDirectoryRecord zipEOCentralDirectory;

    openZIPFile(m_zipFileName, std::ios::binary | std::ios_base::in | std::ios_base::out | std::ios_base::trunc);

    putZIPRecord(zipEOCentralDirectory);

    closeZIPFile();
}

//
// Close ZIP archive
//

void CZIP::close(void)
{

    if (!m_open)
    {
        throw Exception("ZIP archive has not been opened.");
    }

    // Flush Central Directory to ZIP achive and clear

    UpdateCentralDirectory();
    m_zipCentralDirectory.clear();

    // Reset end of local file header and close archive.

    m_offsetToEndOfLocalFileHeaders = 0;
    closeZIPFile();

    // Reset object flags

    m_open = false;
    m_modified = false;
    m_ZIP64 = false;
}

//
// Add file to ZIP archive.
//

bool CZIP::add(const std::string &fileName, const std::string &zippedFileName)
{

    if (!m_open)
    {
        throw Exception("ZIP archive has not been opened.");
    }

    // Check that an entry does not already exist

    for (auto &directoryEntry : m_zipCentralDirectory)
    {
        if (directoryEntry.fileName.compare(zippedFileName) == 0)
        {
            std::cerr << "File already present in archive [" << zippedFileName << "]" << std::endl;
            return (false);
        }
    }

    // Add file if it exists

    if (fileExists(fileName))
    {
        addFileHeaderAndContents(fileName, zippedFileName);
        return (true);
    }
    else
    {
        std::cerr << "File does not exist [" << fileName << "]" << std::endl;
    }

    return (false);
}

//
// If a archive file entry is a directory return true
//

bool CZIP::isDirectory(const CZIP::FileDetail &fileEntry)
{

    return ((fileEntry.externalFileAttrib & 0x10) ||
            (S_ISDIR(fileEntry.externalFileAttrib >> 16)));
}

//
// If a ZIP64 archive return true. Note if any part of an archive contains
// ZIP64 format entry then this will be true.
//

bool CZIP::isZIP64(void)
{

    return (m_ZIP64);
}

//
// Set ZIP I/O buffer size.
//

void CZIP::setZIPBufferSize(std::uint64_t newBufferSize)
{

    m_zipIOBufferSize = newBufferSize;
    m_zipInBuffer.resize(m_zipIOBufferSize);
    m_zipOutBuffer.resize(m_zipIOBufferSize);
}

} // namespace Antik::ZIP