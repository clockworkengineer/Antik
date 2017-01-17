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
#include <initializer_list>
#include <sstream>
#include <thread>

// ================
// CLASS DEFINITION
// ================

class CLogger {
    
public:
    
    // ==========================
    // PUBLIC TYPES AND CONSTANTS
    // ==========================

    typedef  std::function<void (const std::initializer_list<std::string>& )> LogStringsFn;
    
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

    static void coutstr(const std::initializer_list<std::string>& outstr);
    static void cerrstr(const std::initializer_list<std::string>& errstr);
    
    static void setDateTimeStamped(const bool bDateTimeStamped );
    
    template <typename T> static  std::string toString ( T value );
     
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

    static const std::string currentDateAndTime(void);
        
    // =================
    // PRIVATE VARIABLES
    // =================

    static std::mutex mOutput;
    static bool bDateTimeStamped;
 
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

