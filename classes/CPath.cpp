#include "HOST.hpp"
/*
 * File:   CPath.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2016, 2:33 PM
 *
 * Copyright 2016.
 *
 */

//
// Class: CPath
// 
// Description: File path interrogation and manipulation.  At present this is 
// just a simple adapter class for any boost file system path functionality that 
// is required.
//
// Dependencies:   C11++ - Language standard features used.
//                 Boost - Filesystem.
//

// =================
// CLASS DEFINITIONS
// =================

#include "CPath.hpp"

// ====================
// CLASS IMPLEMENTATION
// ====================

//
// C++ STL
//

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace File {

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

        // ==============
        // PUBLIC METHODS
        // ==============

        //
        // Return path value as string
        //
        
        std::string CPath::toString(void) const {
            return m_path.string();
        };

        //
        // Return parent directory CPath
        //
        
        CPath CPath::parentPath(void) {
            return (m_path.parent_path().string());
        }

        //
        // Return path filename (including extension).
        //
        
        std::string CPath::fileName(void) const {
            return m_path.filename().string();
        };

        //
        // Return path base file name (without extension).
        //
        
        std::string CPath::baseName(void) const {
            return m_path.stem().string();
        };

        //
        // Return path extension.
        //
        
        std::string CPath::extension(void) const {
            return m_path.extension().string();
        };

        //
        // Append partial path to path.
        //
        
        void CPath::join(const std::string &partialPath) {
            m_path /= partialPath;
        }

        //
        // Replace paths extension.
        //
        
        void CPath::replaceExtension(const std::string &extension) {
            m_path.replace_extension(extension);
        }

        //
        // Normalize path.
        //
        
        void CPath::normalize(void) {
            m_path = m_path.normalize();
        }

        //
        // Return absolute path value.
        //
        
        std::string CPath::absolutePath() {
            std::string path = boost::filesystem::absolute(m_path).lexically_normal().string();
            if (path.back() == '.') path.pop_back();
            return (path);
        }

        //
        // Return current set path value.
        //
        
        std::string CPath::currentPath() {
            return (boost::filesystem::current_path().string());
        }

    } // namespace File
} // namespace Antik
