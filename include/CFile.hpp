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

            // ============
            // CONSTRUCTORS
            // ============

            // ==========
            // DESTRUCTOR
            // ==========

            // ==============
            // PUBLIC METHODS
            // ==============

            static bool exists(const CPath &filePath) {
                return(boost::filesystem::exists(filePath.toString()));
            }
            
            static bool createDirectory(const CPath &directoryPath) {
                return(boost::filesystem::create_directories(directoryPath.toString()));
            }
            
            static void remove(const CPath &filePath) {
                boost::filesystem::remove(filePath.toString());
            }
            
            static void copy(const CPath &soutcePath, const CPath &destinationPath) {
                boost::filesystem::copy_file(soutcePath.toString(), destinationPath.toString(),
                        boost::filesystem::copy_option::none);
            }
            
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

