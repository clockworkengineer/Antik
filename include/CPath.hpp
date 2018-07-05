/*
 * File:   CPath.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2016, 2:33 PM
 *
 * Copyright 2016.
 *
 */

#ifndef CPATH_HPP
#define CPATH_HPP

//
// C++ STL
//

#include <stdexcept>

//
// Antik classes
//

#include "CommonAntik.hpp"

//
// BOOST filesystem, iterators
//

#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace File {

        // ================
        // CLASS DEFINITION
        // ================

        class CPath {
        public:

            // ==========================
            // PUBLIC TYPES AND CONSTANTS
            // ==========================

            //
            // Class exception
            //

            struct Exception : public std::runtime_error {

                Exception(std::string const& message)
                : std::runtime_error("CPath Failure: " + message) {
                }

            };

            // ============
            // CONSTRUCTORS
            // ============

            CPath(const std::string &path) : m_path(path) {};
                        
            // ==========
            // DESTRUCTOR
            // ==========
            
            virtual ~CPath() {};

            // ==============
            // PUBLIC METHODS
            // ==============
            
            std::string toString() const {
                return m_path.string();
            };
            
            CPath  parentPath () {
                return(m_path.parent_path().string());
            }
            
            std::string fileName() const {
                return m_path.filename().string();
            };
            
            std::string baseName() const {
                return m_path.stem().string();
            };
            
            std::string extension() const {
                return m_path.extension().string();
            };
            
            void join(const std::string &partialPath) {
                m_path /= partialPath;
            }
            
            void replaceExtension(const std::string &extension) {
                m_path.replace_extension(extension);
            }
            
            void normalize(void) {
                m_path = m_path.normalize();
            }
            
            std::string absolutePath() {
                std::string path =  boost::filesystem::absolute(m_path).lexically_normal().string();
                if (path.back() == '.') path.pop_back();
                return(path);
            }
            
            static std::string currentPath() {
                return(boost::filesystem::current_path().string());
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

            // ===============
            // PRIVATE METHODS
            // ===============

            // =================
            // PRIVATE VARIABLES
            // =================
            
            boost::filesystem::path m_path;
            
        };

    } // namespace File
} // namespace Antik

#endif /* CPATH_HPP */

