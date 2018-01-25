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
// C++ STL
//

#include <string>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <ctime>

//
// Antik classes
//

#include "CommonAntik.hpp"
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

                Exception(std::string const& message)
                : std::runtime_error("CZIP Failure: " + message) {
                }

            };

            //
            // ZIP file archive file details entry
            //

            struct FileDetail {
                std::string fileName; // Name
                std::string fileComment; // Comment
                std::tm modificationDateTime{0}; // Last modified date/time
                std::uint64_t uncompressedSize{0}; // Uncompressed size
                std::uint64_t compressedSize{0}; // Compressed size
                std::uint16_t compression{0}; // Compression stored as
                std::uint16_t creatorVersion{0}; // Archive creator
                std::uint32_t externalFileAttrib{0}; // Attributes
                std::vector<std::uint8_t>extraField; // Extra data field
                bool bZIP64{ false}; // true then in ZIP64 format
            };

            // ============
            // CONSTRUCTORS
            // ============

            explicit CZIP(const std::string& zipFileName);

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

            void name(const std::string& zipFileName);

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

            bool extract(const std::string& fileName, const std::string& destFolder);
            bool add(const std::string& fileName, const std::string& zippedFileName);

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

            static const std::uint64_t kZIPDefaultBufferSize{ 16384};


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

            std::uint32_t inflateFile(const std::string& fileName, std::uint64_t fileSize);
            std::uint32_t extractFile(const std::string& fileName, std::uint64_t fileSize);
            std::pair<std::uint32_t, std::uint64_t> deflateFile(const std::string& fileName, std::uint64_t fileSize);
            void storeFile(const std::string& fileName, std::uint64_t fileLength);

            bool fileExists(const std::string& fileName);
            std::uint32_t getFileAttributes(const std::string& fileName);
            std::uint64_t getFileSize(const std::string& fileName);
            std::pair<std::uint16_t, std::uint16_t> getFileModificationDateTime(const std::string& fileName);

            void addFileHeaderAndContents(const std::string& fileName, const std::string& zippedFileName);
            void UpdateCentralDirectory(void);

            // =================
            // PRIVATE VARIABLES
            // =================

            //
            // ZIP archive status
            //

            bool m_open{ false};
            bool m_modified{ false};
            bool m_ZIP64{ true};

            //
            // ZIP archive filename and added contents list
            //

            std::string m_zipFileName;

            //
            // Inflate/deflate buffers.
            //

            std::vector<std::uint8_t> m_zipInBuffer;
            std::vector<std::uint8_t> m_zipOutBuffer;

            //
            //  ZIP(64) archive End Of Central Directory record  and  Central Directory
            // 

            std::vector<CentralDirectoryFileHeader> m_zipCentralDirectory;

            //
            // Offset in ZIP archive to put next File Header added.
            //

            std::uint64_t m_offsetToEndOfLocalFileHeaders{ 0};


            //
            // Offset in ZIP archive to put next File Header added.
            //

            std::uint64_t m_zipIOBufferSize{ kZIPDefaultBufferSize};

        };

    } // namespace ZIP
} // namespace Antik

#endif /* CZIP_HPP */

