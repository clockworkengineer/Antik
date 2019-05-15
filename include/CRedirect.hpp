/*
 * File:   CRedirect.hpp
 * 
 * Author: Robert Tizzard
 *
 * Created on October 24, 2016, 2:34 PM
 *
 * Copyright 2016.
 *
 */

#ifndef REDIRECT_HPP
#define REDIRECT_HPP

//
// C++ STL
//

#include <iostream>
#include <memory>
#include <fstream>

//
// Antik classes
//

#include "CommonAntik.hpp"

// =========
// NAMESPACE
// =========

namespace Antik::Util
{

// ================
// CLASS DEFINITION
// ================

class CRedirect
{
public:
    // ==========================
    // PUBLIC TYPES AND CONSTANTS
    // ==========================

    // ============
    // CONSTRUCTORS
    // ============

    //
    // Set stream to redirect
    //

    explicit CRedirect(std::ostream &outStream);
    explicit CRedirect(std::FILE *stdStream);

    //
    // Set stream to redirect and start redirect
    //

    explicit CRedirect(std::ostream &outStream, const std::string &outfileName, std::ios_base::openmode mode = std::ios_base::out);
    explicit CRedirect(std::FILE *stdStream, const std::string &outfileName, const char *mode = "w");

    // ==========
    // DESTRUCTOR
    // ==========

    ~CRedirect();

    // ==============
    // PUBLIC METHODS
    // ==============

    //
    // Redirect stream to outfleName
    //

    void change(const std::string &outfileName, std::ios_base::openmode mode = std::ios_base::out);
    void change(const std::string &outfileName, const char *mode = "w");

    //
    // Restore original output stream
    //

    void restore(void);

private:
    // ===========================
    // PRIVATE TYPES AND CONSTANTS
    // ===========================

    // ===========================================
    // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
    // ===========================================

    CRedirect() = delete;
    CRedirect(const CRedirect &orig) = delete;
    CRedirect(const CRedirect &&orig) = delete;
    CRedirect &operator=(CRedirect other) = delete;

    // ===============
    // PRIVATE METHODS
    // ===============

    // =================
    // PRIVATE VARIABLES
    // =================

    std::unique_ptr<std::ofstream> m_newFileStream{nullptr}; // New file stream
    std::ostream *m_savedStream{nullptr};                    // saved stream
    std::streambuf *m_outputBuffer{nullptr};                 // Saved readbuffer
    std::FILE *m_savedStdOutErr{nullptr};                    // Saved stdout/stderr
};

} // namespace Antik::Util

#endif /* REDIRECT_HPP */
