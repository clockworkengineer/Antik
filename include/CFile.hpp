/*
 * File:   CFile.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2016, 2:33 PM
 *
 * Copyright 2016.
 *
 */

#ifndef CFILE_HPP
#define CFILE_HPP

//
// C++ STL
//

#include <stdexcept>

//
// Antik classes
//

#include "CommonAntik.hpp"
#include "CPath.hpp"

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace File {

        // ================
        // CLASS DEFINITION
        // ================

        class CFile {
        public:

            // ==========================
            // PUBLIC TYPES AND CONSTANTS
            // ==========================

            //
            // Class exception
            //

            struct Exception : public std::runtime_error {

                Exception(std::string const& message)
                : std::runtime_error("CFile Failure: " + message) {
                }

            };

            using Status = boost::filesystem::file_status;
            using Permissions = boost::filesystem::perms;
            using Time = time_t;
            
            // ============
            // CONSTRUCTORS
            // ============

            // ==========
            // DESTRUCTOR
            // ==========

            // ==============
            // PUBLIC METHODS
            // ==============

            static bool exists(const CPath &filePath);
            
            static bool isFile(const CPath &filePath);
            
            static Status fileStatus(const CPath &filePath);
            
            static bool isDirectory(const CPath &filePath);
            
            static bool createDirectory(const CPath &directoryPath);
            
            static void remove(const CPath &filePath);
            
            static void setPermissions(const CPath &filePath, Permissions permissions);
            
            static void copy(const CPath &sourcePath, const CPath &destinationPath);
            
            static void rename(const CPath &sourcePath, const CPath &destinationPath);
            
            static FileList directoryContentsList(const CPath &localDirectory);
            
            static Time lastWriteTime(const CPath &filePath);
            
            // ================
            // PUBLIC VARIABLES
            // ================

        private:

            // ===========================
            // PRIVATE TYPES AND CONSTANTS
            // ===========================

            // ===========================================
            // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
            // ===========================================

            CFile() = delete;
            virtual ~CFile() = delete;
            CFile(const CFile & orig) = delete;
            CFile(const CFile && orig) = delete;
            CFile& operator=(CFile other) = delete;
            
            // ===============
            // PRIVATE METHODS
            // ===============

            // =================
            // PRIVATE VARIABLES
            // =================
            
        };

    } // namespace File
} // namespace Antik

#endif /* CFILE_HPP */

