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
                try {
                    return(boost::filesystem::exists(filePath.toString()));
                } catch (const boost::filesystem::filesystem_error & e) {
                    throw Exception(e.what());
                } 
            }
            
            static bool isFile(const CPath &filePath) {
                try {
                    return(boost::filesystem::is_regular(filePath.toString()));
                } catch (const boost::filesystem::filesystem_error & e) {
                    throw Exception(e.what());
                } 
            }
            
           static bool isDirectory(const CPath &filePath) {
                try {
                    return(boost::filesystem::is_directory(filePath.toString()));
                } catch (const boost::filesystem::filesystem_error & e) {
                    throw Exception(e.what());
                } 
            }
            
            static bool createDirectory(const CPath &directoryPath) {
                try {
                    return (boost::filesystem::create_directories(directoryPath.toString()));
                } catch (const boost::filesystem::filesystem_error & e) {
                    throw Exception(e.what());
                }
            }

            static void remove(const CPath &filePath) {
                try {
                    boost::filesystem::remove(filePath.toString());
                } catch (const boost::filesystem::filesystem_error & e) {
                    throw Exception(e.what());
                }
            }
                        
            static void copy(const CPath &sourcePath, const CPath &destinationPath) {
                try {
                    boost::filesystem::copy_file(sourcePath.toString(), destinationPath.toString(),
                            boost::filesystem::copy_option::none);
                } catch (const boost::filesystem::filesystem_error & e) {
                    throw Exception(e.what());
                }
                  
            }
            
            static void rename(const CPath &sourcePath, const CPath &destinationPath) {
                try {
                    boost::filesystem::rename(sourcePath.toString(), destinationPath.toString());
                } catch (const boost::filesystem::filesystem_error & e) {
                    throw Exception(e.what());
                }
                  
            }
            
            static FileList directoryContentsList(const CPath &localDirectory) {

                FileList fileList;

                try {

                    for (auto &directoryEntry : boost::filesystem::
                            recursive_directory_iterator{localDirectory.toString()}) {
                        fileList.emplace_back(directoryEntry.path().string());
                    }

                } catch (const boost::filesystem::filesystem_error & e) {
                    throw Exception(e.what());
                }
                
                return(fileList);
            
            }
            
            static time_t lastWriteTime(const CPath &filePath) {

                try {

                    return(boost::filesystem::last_write_time(filePath.toString()));

                } catch (const boost::filesystem::filesystem_error & e) {
                    throw Exception(e.what());
                }
                
            
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

