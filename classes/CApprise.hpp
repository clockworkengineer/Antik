/*
 * File:   CApprise.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2016, 2:33 PM
 *
 * Copyright 2016.
 *
 */

#ifndef IAPPRISE_HPP
#define IAPPRISE_HPP

//
// C++ STL definitions
//

#include <unordered_map>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <set>
#include <stdexcept>

//
// CLogger trace output
//

#include "CLogger.hpp"

//
// inotify definitions
//

#include <sys/inotify.h>

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace File {

        // ================
        // CLASS DEFINITION
        // ================

        class CApprise {
        public:

            // ==========================
            // PUBLIC TYPES AND CONSTANTS
            // ==========================

            //
            // Class exception
            //

            struct Exception : public std::runtime_error {

                Exception(std::string const& messageStr)
                : std::runtime_error("CApprise Failure: " + messageStr) {
                }

            };

            //
            // CApprise options structure (optionally passed to CApprise constructor)
            //

            struct Options {
                uint32_t inotifyWatchMask;                  // inotify watch event mask
                bool bDisplayInotifyEvent;                  // ==true then display inotify event to coutstr
                Antik::Util::CLogger::LogStringsFn coutstr; // coutstr output
                Antik::Util::CLogger::LogStringsFn cerrstr; // cerrstr output
            };

            //
            // CApprise event identifiers
            //

            enum EventId {
                Event_none = 0,  // None
                Event_add,       // File added to watched folder hierarchy
                Event_change,    // File changed
                Event_unlink,    // File deleted from watched folder hierarchy
                Event_addir,     // Directory added to watched folder hierarchy
                Event_unlinkdir, // Directory deleted from watched folder hierarchy
                Event_error      // Exception error
            };

            //
            // CApprise event structure
            //

            struct Event {
                EventId id;                 // Event id
                std::string messageStr;     // Event file name / error message string
            };

            // ============
            // CONSTRUCTORS
            // ============

            //
            // Main constructor
            //

            CApprise
            (
                const std::string& watchFolderStr, // Watch folder path
                int watchDepth, // Watch depth -1=all,0=just watch folder,1=next level down etc.
                std::shared_ptr<CApprise::Options> options = nullptr // CApprise Options (OPTIONAL)
            );

            //
            // Need to add/remove watches manually
            //

            CApprise
            (
                std::shared_ptr<CApprise::Options> options = nullptr // CApprise Options (OPTIONAL)
            );


            // ==========
            // DESTRUCTOR
            // ==========

            virtual ~CApprise();

            // ==============
            // PUBLIC METHODS
            // ==============

            //
            // Control
            //

            void watch(void); // Watch folder(s) for file events to convert for CApprise.
            void stop(void); // Stop watch loop/thread

            //
            // Queue access
            //

            void getEvent(CApprise::Event& message); // Get CApprise event (waiting if necessary)

            //
            // Watch handling
            //

            void addWatchFile(const std::string& filePathStr); // Add directory/file to be watched
            void removeWatchFile(const std::string& filePathStr); // Remove directory/file being watched

            //
            // Private data accessors
            //

            bool stillWatching(void); // Is watcher loop till active ?.
            std::exception_ptr getThrownException(void); // Get any exception thrown by watcher to pass down chain

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

            static const std::string kLogPrefixStr; // Logging output prefix 

            //
            // inotify
            //

            static const uint32_t kInofityEvents; // inotify events to monitor
            static const uint32_t kInotifyEventSize; // inotify read event size
            static const uint32_t kInotifyEventBuffLen; // inotify read buffer length

            // ===========================================
            // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
            // ===========================================

            CApprise() = delete;
            CApprise(const CApprise & orig) = delete;
            CApprise(const CApprise && orig) = delete;
            CApprise& operator=(CApprise other) = delete;

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

            void addWatch(const std::string& filePathStr); // Add path to be watched
            void removeWatch(const std::string& filePathStr); // Remove path being watched
            void initWatchTable(void); // Initialise table for watched folders
            void destroyWatchTable(void); // Tare down watch table

            //
            // Queue CApprise event
            //

            void sendEvent(
                CApprise::EventId id,           // Event id
                const std::string& messageStr   // Filename/message
            );

            // =================
            // PRIVATE VARIABLES
            // =================

            //
            // Constructor passed in and intialised
            //

            std::string watchFolderStr; // Watch Folder
            int watchDepth { -1 }; // Watch depth -1=all,0=just watch folder,1=next level down etc.

            //
            // Inotify
            //

            int inotifyFd { 0 }; // file descriptor for read
            uint32_t inotifyWatchMask { CApprise::kInofityEvents }; // watch event mask
            std::unique_ptr<std::uint8_t> inotifyBuffer; // read buffer
            std::unordered_map<int32_t, std::string> watchMap; // Watch table indexed by watch variable
            std::set<std::string> inProcessOfCreation; // Set to hold files being created.
            bool bDisplayInotifyEvent { false }; // ==true then display inotify event to coutstr

            //
            // Publicly accessed via accessors
            //

            std::exception_ptr thrownException { nullptr }; // Pointer to any exception thrown
            std::atomic<bool> bDoWork { false }; // doWork=true (run watcher loop) false=(stop watcher loop)

            //
            // Event queue
            //

            std::condition_variable queuedEventsWaiting; // Queued events conditional
            std::mutex queuedEventsMutex; // Queued events mutex
            std::queue <CApprise::Event> queuedEvents; // Queue of CApprise events

            // Trace functions default (do nothing).

            Antik::Util::CLogger::LogStringsFn coutstr { Antik::Util::CLogger::noOp };
            Antik::Util::CLogger::LogStringsFn cerrstr { Antik::Util::CLogger::noOp };

        };


    } // namespace File
} // namespace Antik

#endif /* IAPPRISE_HPP */

