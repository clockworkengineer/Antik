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
// C++ STL
//

#include <functional>
#include <mutex>
#include <sstream>

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace Util {
        
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

        static const LogStringsFn  noOp;

        // ============
        // CONSTRUCTORS
        // ============

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

        template <typename T> static std::string toing(T value);

        // ===========================
        // PRIVATE TYPES AND CONSTANTS
        // ===========================

        // ===========================================
        // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
        // ===========================================

        CLogger() = delete;
        virtual ~CLogger() = delete;
        CLogger(const CLogger & orig) = delete;
        CLogger(const CLogger && orig) = delete;
        CLogger& operator=(CLogger other) = delete;

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

        static std::mutex m_outputMutex; // Stream output mutex
        static bool m_dateTimeStamped; // ==true output date/time stamped

    };

    //
    // Convert value to string for output
    //

    template <typename T>
    std::string CLogger::toing(T value) {
        std::ostringstream ss;
        ss << value;
        return ss.str();
    }

    } // nmespace Util
} // namespace Antik

#endif /* CLOGGER_HPP */

