/*
 * File:   IFileEventNotifier.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2016, 2:33 PM
 *
 * Copyright 2016.
 *
 */

#ifndef IFILEEVENTNOTIFIER_HPP
#define IFILEEVENTNOTIFIER_HPP

//
// C++ STL
//

#include <string>
#include <stdexcept>

//
// Antik classes
//

#include "IApprise.hpp"

// =========
// NAMESPACE
// =========

namespace Antik::File
{

// ================
// CLASS DEFINITION
// ================

class IFileEventNotifier
{
public:
    // ==========================
    // PUBLIC TYPES AND CONSTANTS
    // ==========================

    // ============
    // CONSTRUCTORS
    // ============

    // ==============
    // PUBLIC METHODS
    // ==============

    //
    // Event queue
    //

    virtual void generateEvents(void) = 0;                   // Watch folder(s) for file events
    virtual void stopEventGeneration(void) = 0;              // Stop watch loop/thread
    virtual void getNextEvent(IApprise::Event &message) = 0; // Get next queued event
    virtual bool stillWatching() const = 0;                  // Events still being generated
    virtual void clearEventQueue() = 0;                      // Clear event queue

    //
    // Watch processing
    //

    virtual void addWatch(const std::string &filePath) = 0;    // Add path to be watched
    virtual void removeWatch(const std::string &filePath) = 0; // Remove path being watched
    virtual void setWatchDepth(int watchDepth) = 0;            // Set maximum watch depth

    //
    // Get any thrown exceptions
    //

    virtual std::exception_ptr getThrownException() const = 0;

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
};

} // namespace Antik::File

#endif /* IFILEEVENTNOTIFIER_HPP */
