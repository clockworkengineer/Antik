#include "HOST.hpp"
/*
 * File:   CRedirect.cpp
 * 
 * Author: Robert Tizzard
 *
 * Created on October 24, 2016, 2:34 PM
 * 
 * Copyright 2016.
 *
 */

// Class: CRedirect
// 
// Description: This is a small self contained utility class designed 
// for logging output to a file. Its prime functionality is to provide 
// a wrapper for pretty generic code that saves away an output streams 
// read buffer, creates a file stream and redirects the output stream to it. 
// The code to restore the original output streams is called from the objects 
// destructor thus providing convenient for restoring the original stream.
//
// Dependencies: C11++ - Language standard features used. 
//

// =================
// CLASS DEFINITIONS
// =================

#include "CRedirect.hpp"

// ====================
// CLASS IMPLEMENTATION
// ====================

// =========
// NAMESPACE
// =========

namespace Antik {

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

    // ===============
    // PRIVATE METHODS
    // ===============

    // ==============
    // PUBLIC METHODS
    // ==============

    //
    // Create CRedirect specifying output stream
    //

    CRedirect::CRedirect(std::ostream& outStream) {
        this->outStream = &outStream;
    }

    //
    // Create CRedirect specifying output stream, output file and start the redirect
    //

    CRedirect::CRedirect(std::ostream& outStream, std::string outfileName, std::ios_base::openmode mode) {
        this->outStream = &outStream;
        this->change(outfileName, mode);
    }

    //
    // Create CRedirect specifying file stream (stdout/stderr), output file and start the redirect
    //

    CRedirect::CRedirect(std::FILE* stdStream, std::string outfileName, const char* mode) {
        std::freopen(outfileName.c_str(), mode, stdStream);
    }

    //
    // Create CRedirect specifying file stream (stdout/stderr)
    //

    CRedirect::CRedirect(std::FILE* stdStream) {
        this->stdStream = stdStream;
    }

    //
    // Restore old output stream
    //

    CRedirect::~CRedirect() {
        restore();
    }

    //
    // Change output for stream to file 
    //

    void CRedirect::change(std::string outfileName, std::ios_base::openmode mode) {
        this->fileStream.reset(new std::ofstream{outfileName, mode});
        this->outBuffer = outStream->rdbuf();
        outStream->rdbuf((this->fileStream)->rdbuf());
    }

    //
    // Change output for file stream (stdout/stderr) to file.
    //

    void CRedirect::change(std::string outfileName, const char* mode) {
        std::freopen(outfileName.c_str(), mode, this->stdStream);
    }

    //
    // Restore old output stream. Note that this will currently do nothing  for 
    // stdout/stderr (except close) as a dependable way of doing this hasn't 
    // been found yet.
    //

    void CRedirect::restore() {

        if (this->outBuffer) {
            outStream->rdbuf(this->outBuffer);
        }

        if (this->fileStream) {
            this->fileStream->close();
        }

        if (this->stdStream) {
            fclose(this->stdStream);
        }
    }

} // namespace Antik