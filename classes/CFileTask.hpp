/*
 * File:   CFileTask.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2016, 2:33 PM
 *
 * Copyright 2016.
 *
 */

#ifndef FPETASK_HPP
#define FPETASK_HPP

//
// C++ STL definitions
//

#include <cassert>
#include <thread>
#include <stdexcept>

//
// CLogger trace output, CFileApprise file event watcher
//

#include "CLogger.hpp"
#include "CFileApprise.hpp"

// ================
// CLASS DEFINITION
// ================

class CFileTask { 
    
public:

    // ==========================
    // PUBLIC TYPES AND CONSTANTS
    // ==========================

    //
    // Class exception
    //
    
    struct Exception : public std::runtime_error {

        Exception(std::string const& message)
        : std::runtime_error("CFileTask Failure: "+ message) { }
        
    };
    
    //
    // Task action function
    //
    
    typedef std::function<bool (const std::string&, const std::shared_ptr<void>)> TaskActionFcn;
 
    //
    // Task options structure (optionally pass to CFileTask constructor)
    // Note: After killCount files processed stop task (0 = disabled)
    //
    
    struct TaskOptions {
        int killCount;                // file kill count
        CLogger::LogStringsFn coutstr;  // coutstr output
        CLogger::LogStringsFn cerrstr;  // cerrstr output
    };
    
    // ===========
    // CONSTRUCTOR
    // ===========

    //
    // Main constructor
    //
   
    CFileTask
    (
        const std::string& taskName,                   // Task name
        const std::string& watchFolder,                // Watch folder path
        TaskActionFcn taskActFcn,                      // Task action function
        std::shared_ptr<void> fnData,                  // Task file process function data
        int watchDepth,                                // Watch depth -1= all, 0=just watch folder
        std::shared_ptr<TaskOptions> options=nullptr   // Task options. 
    );
    
    // ==========
    // DESTRUCTOR
    // ==========
    
    virtual ~CFileTask(); // Task class cleanup

    // ==============
    // PUBLIC METHODS
    //===============

    //
    // Control
    //
    
    void monitor(void);                         // Monitor watch folder for directory file events and process added files
    void stop(void);                            // Stop task

   //
   // Private data accessors
   //
    
   std::exception_ptr getThrownException(void);// Get any exception thrown by task to pass down chain
 
private:
    
    // ===========================
    // PRIVATE TYPES AND CONSTANTS
    // ===========================

    // =====================
    // DISABLED CONSTRUCTORS
    // =====================
    
    CFileTask() = delete;                            // Use only provided constructors
    CFileTask(const CFileTask & orig) = delete;
    CFileTask(const CFileTask && orig )= delete;   
    
    // ===============
    // PRIVATE METHODS
    // ===============
    
    // =================
    // PRIVATE VARIABLES
    // =================
  
    //
    // Constructor passed in and intialized
    //
    
    std::string  taskName;          // Task name
    std::string  watchFolder;       // Watch Folder
    TaskActionFcn taskActFcn;       // Task action function 
    std::shared_ptr<void> fnData;   // Task action function data   
    int killCount=0;                // Task Kill Count

    //
    // CFileApprise file watcher
    //
    
    std::shared_ptr<CFileApprise> watcher;                  // Folder watcher
    std::shared_ptr<CFileApprise::Options> watcherOptions;  // folder watcher options
    std::unique_ptr<std::thread> watcherThread;             // Folder watcher thread

    //
    // Publicly accessed via accessors
    //
    
    std::exception_ptr thrownException=nullptr;     // Pointer to any exception thrown
    
    //
    // Trace functions default do nothing
    //
    
    CLogger::LogStringsFn coutstr  = CLogger::noOp;
    CLogger::LogStringsFn cerrstr = CLogger::noOp; 
 
    std::string prefix; // Task trace prefix

};
#endif /* FPETASK_HPP */

