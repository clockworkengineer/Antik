/*
 * File:   CFileEventNotifier.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2016, 2:33 PM
 *
 * Copyright 2016.
 *
 */

#ifndef CFILEEVENTNOTIFIER_HPP
#define CFILEEVENTNOTIFIER_HPP

//
// C++ STL
//

#include <unordered_map>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <set>

//
// Antik classes
//

#include "CommonAntik.hpp"
#include "IApprise.hpp"
#include "IFileEventNotifier.hpp"

//
// inotify
//

#include <sys/inotify.h>

// =========
// NAMESPACE
// =========

namespace Antik::File {

    // ================
    // CLASS DEFINITION
    // ================

    class CFileEventNotifier : public IFileEventNotifier {
    public:
        // ==========================
        // PUBLIC TYPES AND CONSTANTS
        // ==========================

        // ============
        // CONSTRUCTORS
        // ============

        //
        // Main constructor
        //

        CFileEventNotifier();

        // ==========
        // DESTRUCTOR
        // ==========

        virtual ~CFileEventNotifier();

        // ==============
        // PUBLIC METHODS
        // ==============

        //
        // Event queue
        //

        void generateEvents(void) override; // Watch folder(s) for file events
        void stopEventGeneration(void) override; // Stop watch loop/thread
        void getNextEvent(IApprise::Event &message) override; // Get next queued event
        bool stillWatching() const override; // Events still being generated
        void clearEventQueue() override; // Clear event queue

        //
        // Watch processing
        //

        void setWatchDepth(int watchDepth) override; // Set maximum watch depth
        void addWatch(const std::string &filePath) override; // Add path to be watched
        void removeWatch(const std::string &filePath) override; // Remove path being watched

        // Exception handling

        std::exception_ptr getThrownException() const override; // Get last thrown exception

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

        static const std::string kLogPrefix; // Logging output prefix

        //
        // inotify
        //

        static const std::uint32_t kInofityEvents; // inotify events to monitor
        static const std::uint32_t kInotifyEventSize; // inotify read event size
        static const std::uint32_t kInotifyEventBuffLen; // inotify read buffer length

        // ===========================================
        // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
        // ===========================================

        CFileEventNotifier(const CFileEventNotifier &orig) = delete;
        CFileEventNotifier(const CFileEventNotifier &&orig) = delete;
        CFileEventNotifier &operator=(CFileEventNotifier other) = delete;

        // ===============
        // PRIVATE METHODS
        // ===============

        //
        // Watch processing
        //

        void initialiseWatchTable(void); // Initialise table for watched folders
        void destroyWatchTable(void); // Tare down watch table

        //
        // Queue IApprise event
        //

        void sendEvent(
                IApprise::EventId id, // Event id
                const std::string &message // Filename/message
                );

        // =================
        // PRIVATE VARIABLES
        // =================

        //
        // Inotify
        //

        int m_inotifyFd{0}; // file descriptor for read
        std::uint32_t m_inotifyWatchMask{CFileEventNotifier::kInofityEvents}; // watch event mask
        std::unique_ptr<std::uint8_t[] > m_inotifyBuffer; // read buffer
        std::unordered_map<int32_t, std::string> m_watchMap; // Watch table indexed by watch variable
        std::set<std::string> m_inProcessOfCreation; // Set to hold files being created.

        //
        // Publicly accessed via accessors
        //

        std::exception_ptr m_thrownException{nullptr}; // Pointer to any exception thrown
        std::atomic<bool> m_doWork{false}; // doWork=true (run watcher loop) false=(stop watcher loop)
        int m_watchDepth{-1}; // Watch depth -1=all,0=just watch folder,1=next level down etc.

        //
        // Event queue
        //

        std::condition_variable m_queuedEventsWaiting; // Queued events conditional
        std::mutex m_queuedEventsMutex; // Queued events mutex
        std::queue<IApprise::Event> m_queuedEvents; // Queue of CFileEventNotifier events
    };

} // namespace Antik::File

#endif /* CFILEEVENTNOTIFIER_HPP */
