/*
 * File:   CFileApprise.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2016, 2:33 PM
 *
 * Copyright 2016.
 
 */

#ifndef IAPPRISE_HPP
#define IAPPRISE_HPP

//
// C++ STL definitions
//

#include <unordered_map>
#include <thread>
#include <queue>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <system_error>
#include <cassert>
#include <algorithm>
#include <set>

//
// CLogger trace output
//

#include "CLogger.hpp"

//
// inotify definitions
//

#include <sys/inotify.h>

// ================
// CLASS DEFINITION
// ================

class CFileApprise {
    
public:
    
    // ==========================
    // PUBLIC TYPES AND CONSTANTS
    // ==========================

    //
    // CFileApprise options structure (optionally passed to CFileApprise constructor)
    //

    struct Options {
        uint32_t inotifyWatchMask;        // inotify watch event mask
        bool bDisplayInotifyEvent;        // ==true then display inotify event to coutstr
        CLogger::LogStringsFn coutstr;    // coutstr output
        CLogger::LogStringsFn cerrstr;    // cerrstr output
     };
    
    //
    // CFileApprise event identifiers
    //

    enum EventId {
        Event_none = 0,     // None
        Event_add,          // File added to watched folder hierarchy
        Event_change,       // File changed
        Event_unlink,       // File deleted from watched folder hierarchy
        Event_addir,        // Directory added to watched folder hierarchy
        Event_unlinkdir,    // Directory deleted from watched folder hierarchy
        Event_error         // Exception error
    };

    //
    // CFileApprise event structure
    //
    
    struct Event {
        CFileApprise::EventId id;  // Event id
        std::string message;       // Event file name / error message string
    };
    
    // ============
    // CONSTRUCTORS
    // ============
    
    //
    // Main constructor
    //
    
    CFileApprise
    (
        const std::string& watchFolder,                            // Watch folder path
        int watchDepth,                                            // Watch depth -1=all,0=just watch folder,1=next level down etc.
        std::shared_ptr<CFileApprise::Options> options = nullptr   // CFileApprise Options (OPTIONAL)
    );
    
    //
    // Need to add/remove watches manually
    //
    
    CFileApprise
    (
         std::shared_ptr<CFileApprise::Options> options = nullptr   // CFileApprise Options (OPTIONAL)
    );
  
    
    // ==========
    // DESTRUCTOR
    // ==========
    
    virtual ~CFileApprise();

    // ==============
    // PUBLIC METHODS
    // ==============

    //
    // Control
    //
    
    void watch(void);    // Watch folder(s) for file events to convert for CFileApprise.
    void stop(void);     // Stop watch loop/thread
    
    //
    // Queue access
    //
    
    void getEvent(CFileApprise::Event& message);    // Get CFileApprise event (waiting if necessary)
    
    //
    // Watch handling
    //
    
    void addWatchFile(const std::string& filePath);     // Add directory/file to be watched
    void removeWatchFile(const std::string& filePath);  // Remove directory/file being watched
    
    //
    // Private data accessors
    //
    
    bool stillWatching(void);                       // Is watcher loop till active ?.
    std::exception_ptr getThrownException(void);    // Get any exception thrown by watcher to pass down chain
    
    // ================
    // PUBLIC VARIABLES
    // ================
    
private:

    // ===========================
    // PRIVATE TYPES AND CONSTANTS
    // ===========================
    
    //
    // Logging prefix
    //

    static const std::string kLogPrefix;        // Logging output prefix 

    //
    // inotify
    //
    
    static const uint32_t kInofityEvents;       // inotify events to monitor
    static const uint32_t kInotifyEventSize;    // inotify read event size
    static const uint32_t kInotifyEventBuffLen; // inotify read buffer length

    // =====================
    // DISABLED CONSTRUCTORS
    // =====================
    
    CFileApprise() = delete;
    CFileApprise(const CFileApprise & orig) = delete;
    CFileApprise(const CFileApprise && orig) = delete;
 
    // ===============
    // PRIVATE METHODS
    // ===============

    //
    // Display inotify
    //
    
    void displayInotifyEvent(struct inotify_event *event);
    
    //
    // Watch processing
    //
    
    void addWatch(const std::string& filePath);     // Add path to be watched
    void removeWatch(const std::string& filePath);  // Remove path being watched
    void initWatchTable(void);                      // Initialise table for watched folders
    void destroyWatchTable(void);                   // Tare down watch table

    //
    // Queue CFileApprise event
    //
    
    void sendEvent(
        CFileApprise::EventId id,   // Event id
        const std::string& message  // Filename/message
    ); 

    // =================
    // PRIVATE VARIABLES
    // =================
    
    //
    // Constructor passed in and intialised
    //
    
    std::string watchFolder;    // Watch Folder
    int watchDepth;             // Watch depth -1=all,0=just watch folder,1=next level down etc.

    //
    // Inotify
    //
    
    int inotifyFd;                                              // file descriptor for read
    uint32_t inotifyWatchMask = CFileApprise::kInofityEvents;   // watch event mask
    std::unique_ptr<std::uint8_t> inotifyBuffer;                // read buffer
    std::unordered_map<int32_t, std::string> watchMap;          // Watch table indexed by watch variable
    std::set<std::string> inProcessOfCreation;                  // Set to hold files being created.
    bool bDisplayInotifyEvent=false;                            // ==true then display inotify event to coutstr
 
    //
    // Publicly accessed via accessors
    //
    
    std::exception_ptr thrownException = nullptr;   // Pointer to any exception thrown
    std::atomic<bool> bDoWork;                      // doWork=true (run watcher loop) false=(stop watcher loop)

    //
    // Event queue
    //
    
    std::condition_variable queuedEventsWaiting;    // Queued events conditional
    std::mutex queuedEventsMutex;                   // Queued events mutex
    std::queue <CFileApprise::Event> queuedEvents;  // Queue of CFileApprise events

    // Trace functions default (do nothing).
    
    CLogger::LogStringsFn coutstr = CLogger::noOp;
    CLogger::LogStringsFn cerrstr = CLogger::noOp; 

};
#endif /* IAPPRISE_HPP */

