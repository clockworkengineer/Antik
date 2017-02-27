#include "HOST.hpp"
/* 
 * File:   CLogger.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on January 6, 2017, 6:37 PM
 * 
 * Copyright 2016.
 * 
 */

//
// Class: CLogger
// 
// Description: Class to perform trace output. All methods are designed to
// perform output in a thread safe manor and each is guarded buy a single 
// mutex.
//
// Dependencies: C11++ - Language standard features used.    
// 

// =================
// CLASS DEFINITIONS
// =================

#include "CLogger.hpp"

// ====================
// CLASS IMPLEMENTATION
// ====================

//
// C++ STL definitions
//

#include <iostream>
#include <initializer_list>
#include <thread>
#include <ctime>

// ===========================
// PRIVATE TYPES AND CONSTANTS
// ===========================

// ==========================
// PUBLIC TYPES AND CONSTANTS
// ==========================

// Log output no op

const CLogger::LogStringsFn CLogger::noOp = [] (const std::initializer_list<std::string>& outstr) { };

// ========================
// PRIVATE STATIC VARIABLES
// ========================

std::mutex CLogger::mOutput;                // Mutex to control access to cout/cerr
bool CLogger::bDateTimeStamped = false;     // ==true then output timetamped

// =======================
// PUBLIC STATIC VARIABLES
// =======================

// ===============
// PRIVATE METHODS
// ===============

//
// Get string for current date time
//

const std::string CLogger::currentDateAndTime(void) {
    
   std::time_t rawtime;
   struct std::tm *info;
   std::string buffer(80, ' ');

   std::time( &rawtime );
   info = std::localtime( &rawtime );
   buffer.resize(std::strftime(&buffer[0],buffer.length(),"%F %T", info));
   return(buffer);

}

// ==============
// PUBLIC METHODS
// ==============
//

//
// CLogger object constructor. 
//

CLogger::CLogger() {
}

//
// CLogger Destructor
//

CLogger::~CLogger() {
}

//
// Set whether log output is to have a date and time stamp.
//

void CLogger::setDateTimeStamped(const bool bDateTimeStamped) {
    CLogger::bDateTimeStamped = bDateTimeStamped;
}
 
//
// Standard cout for intialiser list of strings. All calls to this function from different
// threads are guarded by mutex CLogger::mOutput.
//

void CLogger::coutstr(const std::initializer_list<std::string>& outstr) {

    std::lock_guard<std::mutex> locker(CLogger::mOutput);

    if (outstr.size() > 0) {
        if (CLogger::bDateTimeStamped) {
            std::cout << ("[" + currentDateAndTime() + "]");
        }
        for (auto str : outstr) {
            std::cout << str;
        }
        std::cout << std::endl;
    }

}

//
// Standard cerr for intialiser list of strings. All calls to this function from different
// threads are guarded by mutex CLogger::mOutput.
//

void CLogger::cerrstr(const std::initializer_list<std::string>& errstr) {

    std::lock_guard<std::mutex> locker(CLogger::mOutput);

    if (errstr.size() > 0) {
        if (CLogger::bDateTimeStamped) {
            std::cerr << ("[" + currentDateAndTime() + "]");
        }
        for (auto str : errstr) {
            std::cerr << str;
        }
        std::cerr << std::endl;
    }

}

//
// Thread id as Hex
//

template <>
std::string CLogger::toString( std::thread::id value) {
    std::ostringstream ss;
    ss << "0x" << std::hex << value;
    return ss.str();
}