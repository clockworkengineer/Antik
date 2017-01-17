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

    CRedirect(std::ostream& outStream);
    CRedirect(std::ostream& outStream, std::string outfileName, std::ios_base::openmode mode = std::ios_base::out);

    // ==========
    // DESTRUCTOR
    // ==========

    ~CRedirect();

    // ==============
    // PUBLIC METHODS
    // ==============

    void change(std::string outfileName, std::ios_base::openmode mode = std::ios_base::out);
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

    std::unique_ptr<std::ofstream> fileStream = nullptr;
    std::ostream *outStream = nullptr;
    std::streambuf *outBuffer = nullptr;

};
#endif /* REDIRECT_HPP */

