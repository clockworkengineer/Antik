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

namespace Antik::File
{

// ================
// CLASS DEFINITION
// ================

class CPath
{
public:
    // ==========================
    // PUBLIC TYPES AND CONSTANTS
    // ==========================

    //
    // Class exception
    //

    struct Exception : public std::runtime_error
    {

        Exception(std::string const &message)
            : std::runtime_error("CPath Failure: " + message)
        {
        }
    };

    // ============
    // CONSTRUCTORS
    // ============

    CPath(const std::string &path = "") : m_path(path){};

    // ==========
    // DESTRUCTOR
    // ==========

    virtual ~CPath(){};

    // ==============
    // PUBLIC METHODS
    // ==============

    std::string toString(void) const;

    CPath parentPath(void);

    std::string fileName(void) const;

    std::string baseName(void) const;

    std::string extension(void) const;

    void join(const std::string &partialPath);

    void replaceExtension(const std::string &extension);

    void normalize(void);

    std::string absolutePath(void);

    static std::string currentPath(void);

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

} // namespace Antik::File

#endif /* CPATH_HPP */
