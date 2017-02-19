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
    CRedirect(std::FILE* stdStream);

    //
    // Set stream to redirect and start redirect
    //
    
    CRedirect(std::ostream& outStream, std::string outfileName, std::ios_base::openmode mode = std::ios_base::out); 
    CRedirect(std::FILE* stdStream,  std::string outfileName, const char *mode = "w");

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
    void change(std::string outfileName, const char* mode = "w");
 
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
    CRedirect(const CRedirect & orig) = delete;
    CRedirect(const CRedirect && orig) = delete;
    CRedirect& operator=(CRedirect other) = delete;

    // ===============
    // PRIVATE METHODS
    // ===============

    // =================
    // PRIVATE VARIABLES
    // =================

    std::unique_ptr<std::ofstream> fileStream = nullptr;    // New file stream
    std::ostream *outStream = nullptr;                      // saved stream
    std::streambuf *outBuffer = nullptr;                    // Saved readbuffer
    std::FILE *stdStream=nullptr;                           // Saved stdout/stderr

};
#endif /* REDIRECT_HPP */

