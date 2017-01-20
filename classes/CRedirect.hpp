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
// C++ STL definitions
//

#include <iostream>
#include <memory>
#include <fstream>

// ================
// CLASS DEFINITION
// ================

class CRedirect {
    
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

    CRedirect(std::ostream& outStream);

    //
    // Set stream to redirect and start redirect
    //
    
    CRedirect(std::ostream& outStream, std::string outfileName, std::ios_base::openmode mode = std::ios_base::out);

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

    void change(std::string outfileName, std::ios_base::openmode mode = std::ios_base::out);

    //
    // Restore original output stream
    //
    
    void restore(void);

private:

    // ===========================
    // PRIVATE TYPES AND CONSTANTS
    // ===========================

    // =====================
    // DISABLED CONSTRUCTORS
    // =====================

    CRedirect() = delete;
    CRedirect(const CRedirect & orig) = delete;
    CRedirect(const CRedirect && orig) = delete;

    // ===============
    // PRIVATE METHODS
    // ===============

    // =================
    // PRIVATE VARIABLES
    // =================

    std::unique_ptr<std::ofstream> fileStream = nullptr;    // New file stream
    std::ostream *outStream = nullptr;                      // saved stream
    std::streambuf *outBuffer = nullptr;                    // Saved readbuffer

};
#endif /* REDIRECT_HPP */

