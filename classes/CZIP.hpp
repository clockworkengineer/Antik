/*
 * File:   CZIP.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on January 24, 2017, 2:33 PM
 *
 * Copyright 2016.
 *
 */

#ifndef CZIP_HPP
#define CZIP_HPP

//
// C++ STL definitions
//

#include <string>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <ctime>

//
// ZIP archive record IO.
//

#include "CZIPIO.hpp"

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace ZIP {

        // ================
        // CLASS DEFINITION
        // ================

        class CZIP : private CZIPIO {
        public:

            // ==========================
            // PUBLIC TYPES AND CONSTANTS
            // ==========================

            //
            // Class exception
            //

            struct Exception : public std::runtime_error {

                Exception(std::string const& messageStr)
                : std::runtime_error("CZIP Failure: " + messageStr) {
                }

            };

            //
            // ZIP file archive file details entry
            //

            struct FileDetail {
                std::string fileNameStr; // Name
                std::string fileCommentStr; // Comment
                std::tm modificationDateTime {0}; // Last modified date/time
                std::uint64_t uncompressedSize {0}; // Uncompressed size
                std::uint64_t compressedSize {0}; // Compressed size
                std::uint16_t compression {0}; // Compression stored as
                std::uint16_t creatorVersion {0}; // Archive creator
                std::uint32_t externalFileAttrib {0}; // Attributes
                std::vector<std::uint8_t>extraField; // Extra data field
                bool bZIP64 { false }; // true then in ZIP64 format
            };

            // ============
            // CONSTRUCTORS
            // ============

            CZIP(const std::string& zipFileNameStr);

            // ==========
            // DESTRUCTOR
            // ==========

            virtual ~CZIP();

            // ==============
            // PUBLIC METHODS
            // ==============

            //
            // Set ZIP archive name
            //

            void name(const std::string& zipFileNameStr);

            //
            // Create an empty archive file
            //
            void create(void);

            //
            // Open/close archive file
            //
            void open(void);
            void close(void);

            //
            // Add/extract files to archive
            //

            bool extract(const std::string& fileNameStr, const std::string& destFolderStr);
            bool add(const std::string& fileNameStr, const std::string& zippedFileNameStr);

            //
            // Get archives contents
            //

            std::vector<CZIP::FileDetail> contents(void);

            //
            // Return true if archive file entry is a directory
            //

            bool isDirectory(const CZIP::FileDetail& fileEntry);

            //
            // Return true if archive is in ZIP64 format.
            //

            bool isZIP64(void);

            //
            // Set ZIP I/O buffer size.
            //

            void setZIPBufferSize(std::uint64_t newBufferSize);

            // ================
            // PUBLIC VARIABLES
            // ================

        private:

            // ===========================
            // PRIVATE TYPES AND CONSTANTS
            // ===========================

            //
            // ZIP inflate/deflate buffer size.
            //

            static const std::uint64_t kZIPDefaultBufferSize { 16384 };


            // ===========================================
            // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
            // ===========================================

            CZIP() = delete;
            CZIP(const CZIP & orig) = delete;
            CZIP(const CZIP && orig) = delete;
            CZIP& operator=(CZIP other) = delete;

            // ===============
            // PRIVATE METHODS
            // ===============

            std::tm convertModificationDateTime(std::uint16_t dateWord, std::uint16_t timeWord);

            std::uint32_t inflateFile(const std::string& fileNameStr, std::uint64_t fileSize);
            std::uint32_t extractFile(const std::string& fileNameStr, std::uint64_t fileSize);
            std::pair<std::uint32_t, std::uint64_t> deflateFile(const std::string& fileNameStr, std::uint64_t fileSize);
            void storeFile(const std::string& fileNameStr, std::uint64_t fileLength);

            bool fileExists(const std::string& fileNameStr);
            std::uint32_t getFileAttributes(const std::string& fileNameStr);
            std::uint64_t getFileSize(const std::string& fileNameStr);
            std::pair<std::uint16_t, std::uint16_t> getFileModificationDateTime(const std::string& fileNameStr);

            void addFileHeaderAndContents(const std::string& fileNameStr, const std::string& zippedFileNameStr);
            void UpdateCentralDirectory(void);

            // =================
            // PRIVATE VARIABLES
            // =================

            //
            // ZIP archive status
            //

            bool bOpen { false };
            bool bModified { false };
            bool bZIP64 { true };

            //
            // ZIP archive filename and added contents list
            //

            std::string zipFileNameStr;

            //
            // Inflate/deflate buffers.
            //

            std::vector<uint8_t> zipInBuffer;
            std::vector<uint8_t> zipOutBuffer;

            //
            //  ZIP(64) archive End Of Central Directory record  and  Central Directory
            // 

            std::vector<CentralDirectoryFileHeader> zipCentralDirectory;

            //
            // Offset in ZIP archive to put next File Header added.
            //

            std::uint64_t offsetToEndOfLocalFileHeaders { 0 };


            //
            // Offset in ZIP archive to put next File Header added.
            //

            std::uint64_t zipIOBufferSize { kZIPDefaultBufferSize };

        };

    } // namespace ZIP
} // namespace Antik

#endif /* CZIP_HPP */

