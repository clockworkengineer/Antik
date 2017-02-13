/* 
 * File:   CLogger.hpp
 * 
 * Author: Robert Tizzard
 *
 * Created on January 6, 2017, 6:37 PM
 * 
 * Copyright 2016.
 * 
 */

#ifndef CLOGGER_HPP
#define CLOGGER_HPP

//
// C++ STL definitions
//

#include <functional>
#include <mutex>
#include <sstream>

// ================
// CLASS DEFINITION
// ================

class CLogger {
public:

    // ==========================
    // PUBLIC TYPES AND CONSTANTS
    // ==========================

    //
    // Logging output function
    //

    typedef std::function<void (const std::initializer_list<std::string>&) > LogStringsFn;

    //
    // NoOp output function
    //

    static const LogStringsFn noOp;

    // ============
    // CONSTRUCTORS
    // ============

    CLogger();

    // ==========
    // DESTRUCTOR
    // ==========

    virtual ~CLogger();

    // ==============
    // PUBLIC METHODS
    // ==============

    //
    // Log to std::cout/std::cerr
    //

    static void coutstr(const std::initializer_list<std::string>& outstr);
    static void cerrstr(const std::initializer_list<std::string>& errstr);

    //
    // Set output as date time stamped
    // 

    static void setDateTimeStamped(const bool bDateTimeStamped);

    //
    // Template for string conversion method
    //

    template <typename T> static std::string toString(T value);

    // ===========================
    // PRIVATE TYPES AND CONSTANTS
    // ===========================

    // =====================
    // DISABLED CONSTRUCTORS
    // =====================

    CLogger(const CLogger& orig) = delete;
    CLogger(const CLogger&& orig) = delete;

private:

    // ===============
    // PRIVATE METHODS
    // ===============

    //
    // Return current date/time as a string
    //

    static const std::string currentDateAndTime(void);

    // =================
    // PRIVATE VARIABLES
    // =================

    static std::mutex mOutput;        // Stream output mutex
    static bool bDateTimeStamped;     // ==true output date/time stamped

};

//
// Convert value to string for output
//

template <typename T>
std::string CLogger::toString(T value) {
    std::ostringstream ss;
    ss << value;
    return ss.str();
}

#endif /* CLOGGER_HPP */

