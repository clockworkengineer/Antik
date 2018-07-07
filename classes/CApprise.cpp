#include "HOST.hpp"
/*
 * File:   CApprise.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2016, 2:33 PM
 *
 * Copyright 2016.
 *
 */

//
// Class: CApprise
// 
// Description: A simple C++ class to enable files/folders to be watched and 
// events generated. Supported events include the addition/deletion of files and
// directories and the modification of files with a change event. It is recursive 
// by default and any directories added/removed from the hierarchy will cause new 
// watches to be added/removed respectively. The current implementation is for 
// POSIX only or any platform that has inotify or a third party equivalent.
//
// Dependencies: C11++               - Language standard features used.    
//               inotify/Linux       - Linux file system events
//

// =================
// CLASS DEFINITIONS
// =================

#include "CApprise.hpp"

// ====================
// CLASS IMPLEMENTATION
// ====================

//
// C++ STL
//

#include <mutex>
#include <system_error>
#include <cassert>
#include <algorithm>

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace File {

        // ===========================
        // PRIVATE TYPES AND CONSTANTS
        // ===========================

        // inotify events to recieve

        const std::uint32_t CApprise::kInofityEvents{
            IN_ISDIR | IN_CREATE | IN_MOVED_TO | IN_MOVED_FROM |
            IN_DELETE_SELF | IN_CLOSE_WRITE | IN_DELETE | IN_MODIFY};

        // inotify event structure size

        const std::uint32_t CApprise::kInotifyEventSize{ (sizeof (struct inotify_event))};

        // inotify event read buffer size

        const std::uint32_t CApprise::kInotifyEventBuffLen{ (1024 * (CApprise::kInotifyEventSize + 16))};

        // CApprise logging prefix

        const std::string CApprise::kLogPrefix{ "[CApprise] "};

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

        //
        // Clean up inotify. Note: closing the inotify file descriptor cleans up all
        // used resources including watch descriptors but removing them all before
        // hand will cause any pending read for events to return and the watcher loop
        // to stop. 
        //

        void CApprise::destroyWatchTable(void) {

            for (auto it = m_watchMap.begin(); it != m_watchMap.end(); ++it) {

                if (inotify_rm_watch(m_inotifyFd, it->first) == -1) {
                    throw std::system_error(std::error_code(errno, std::system_category()), "inotify_rm_watch() error");
                }

            }

            if (close(m_inotifyFd) == -1) {
                throw std::system_error(std::error_code(errno, std::system_category()), "inotify close() error");
            }

        }

        //
        // Initialize inotify and add watch for watchFolder.
        //

        void CApprise::initWatchTable(void) {

            // Initialize inotify 

            if ((m_inotifyFd = inotify_init()) == -1) {
                throw std::system_error(std::error_code(errno, std::system_category()), "inotify_init() error");
            }

            // Add non empty watch folder

            if (!m_watchFolder.empty()) {
                addWatch(m_watchFolder);
            }

        }

        //
        // Add watch for file/directory
        //

        void CApprise::addWatch(const std::string& filePath) {

            std::string fileName{ filePath};
            int watch{ 0};

            // Remove path trailing '/'

            if (fileName.back() == '/') {
                fileName.pop_back();
            }

            // Deeper than max watch depth so ignore.

            if ((m_watchDepth != -1) && (std::count(fileName.begin(), fileName.end(), '/') > m_watchDepth)) {
                return;
            }

            // Add watch to inotify 

            if ((watch = inotify_add_watch(m_inotifyFd, fileName.c_str(), m_inotifyWatchMask)) == -1) {
                throw std::system_error(std::error_code(errno, std::system_category()), "inotify_add_watch() error");
            }

            // Add watch to map

            m_watchMap.insert({watch, fileName});

        }

        //
        //  Remove watch for file/directory
        //

        void CApprise::removeWatch(const std::string& filePath) {

            try {

                std::string fileName{ filePath};
                int32_t watch{ 0};

                // Remove path trailing '/'

                if (fileName.back() == '/') {
                    fileName.pop_back();
                }

                // Find Watch value

                for (auto watchMapEntry : m_watchMap) {
                    if (watchMapEntry.second.compare(filePath) == 0) {
                        watch = watchMapEntry.first;
                        break;
                    }
                }

                if (watch) {

                    m_watchMap.erase(watch);

                    if (inotify_rm_watch(m_inotifyFd, watch) == -1) {
                        throw std::system_error(std::error_code(errno, std::system_category()), "inotify_rm_watch() error");
                    }

                }


            } catch (std::system_error &e) {
                // Ignore error 22 and carry on. From the documentation on this error the kernel has removed the watch for us.
                if (e.code() != std::error_code(EINVAL, std::system_category())) {
                    throw; // Throw exception back up the chain.
                }
            }

            // No more watches so closedown

            if (m_watchMap.size() == 0) {
                stopEventGeneration();
            }

        }

        //
        // Queue CApprise event
        //

        void CApprise::sendEvent(CApprise::EventId id, const std::string& fileName) {

            std::unique_lock<std::mutex> locker(m_queuedEventsMutex);
            m_queuedEvents.push(Event(id, fileName));
            m_queuedEventsWaiting.notify_one();

        }

        //
        // Flag watch loop to stop.
        //

        void CApprise::stopEventGeneration(void) {

            // If still active then need to close down

            if (m_doWork.load()) {

                std::unique_lock<std::mutex> locker(m_queuedEventsMutex);
                m_doWork = false;
                m_queuedEventsWaiting.notify_one();

                destroyWatchTable();

            }

        }

        //
        // Loop adding/removing watches for directory hierarchy  changes
        // and also generating CApprise events from inotify; until stopped.
        //

        void CApprise::generateEvents(void) {

            std::uint8_t * buffer{ m_inotifyBuffer.get()};

            struct inotify_event * event {
                nullptr
            };
            std::string filePath;

            try {

                // Loop until told to stop

                while (m_doWork.load()) {

                    int readLen{ 0};
                    int currentPos{ 0};

                    // Read in events

                    if ((readLen = read(m_inotifyFd, buffer, kInotifyEventBuffLen)) == -1) {
                        throw std::system_error(std::error_code(errno, std::system_category()), "inotify read() error");
                    }

                    // Loop until all read processed

                    while (currentPos < readLen) {

                        // Point to next event & display if necessary

                        event = (struct inotify_event *) &buffer[ currentPos ];
                        currentPos += kInotifyEventSize + event->len;

                        // IGNORE so move onto next event

                        if (event->mask == IN_IGNORED) {
                            continue;
                        }

                        // Create full file name path

                        filePath = m_watchMap[event->wd];

                        if (event->len > 0) {
                            filePath += ("/" + static_cast<std::string> (event->name));
                        }

                        // Process event

                        switch (event->mask) {

                                // Flag file as being created

                            case IN_CREATE:
                            {
                                m_inProcessOfCreation.insert(filePath);
                                break;
                            }

                                // If file not being created send Event_change

                            case IN_MODIFY:
                            {
                                auto beingCreated = m_inProcessOfCreation.find(filePath);
                                if (beingCreated == m_inProcessOfCreation.end()) {
                                    sendEvent(Event_change, filePath);
                                }
                                break;
                            }

                                // Add watch for new directory and send Event_addir

                            case (IN_ISDIR | IN_CREATE):
                            case (IN_ISDIR | IN_MOVED_TO):
                            {
                                sendEvent(Event_addir, filePath);
                                addWatch(filePath);
                                break;
                            }

                                // Directory deleted send Event_unlinkdir

                            case (IN_ISDIR | IN_DELETE):
                            {
                                sendEvent(Event_unlinkdir, filePath);
                                break;
                            }

                                // Remove watch for deleted/moved directory

                            case (IN_ISDIR | IN_MOVED_FROM):
                            case IN_DELETE_SELF:
                            {
                                removeWatch(filePath);
                                break;
                            }

                                // File deleted send Event_unlink

                            case IN_DELETE:
                            {
                                sendEvent(Event_unlink, filePath);
                                break;
                            }

                                // File moved into directory send Event_add.

                            case IN_MOVED_TO:
                            {
                                sendEvent(Event_add, filePath);
                                break;
                            }

                                // File closed. If being created send Event_add otherwise Event_change.

                            case IN_CLOSE_WRITE:
                            {
                                auto beingCreated = m_inProcessOfCreation.find(filePath);
                                if (beingCreated == m_inProcessOfCreation.end()) {
                                    sendEvent(Event_change, filePath);
                                } else {
                                    m_inProcessOfCreation.erase(filePath);
                                    sendEvent(Event_add, filePath);
                                }
                                break;
                            }

                            default:
                                break;

                        }

                    }

                }

                //
                // Generate event for any exceptions and also store to be passed up the chain
                //

            } catch (std::system_error &e) {
                sendEvent(Event_error, kLogPrefix + "Caught a system_error exception: [" + e.what() + "]");
                m_thrownException = std::current_exception();
            } catch (std::exception &e) {
                sendEvent(Event_error, kLogPrefix + "General exception occured: [" + e.what() + "]");
                m_thrownException = std::current_exception();
            }

            stopEventGeneration(); // If not asked to stop then call anyway (cleanup)

        }

        // ==============
        // PUBLIC METHODS
        // ==============

        //
        // Main CApprise object constructor. 
        //

        CApprise::CApprise(const std::string& watchFolder, int watchDepth)
        : m_watchFolder{watchFolder}, m_watchDepth{watchDepth}, m_doWork{true}
        {

            // ASSERT if passed parameters invalid

            assert(watchDepth >= -1); // < -1

            if (!watchFolder.empty()) {

                // Remove path trailing '/'

                if ((m_watchFolder).back() == '/') {
                    (m_watchFolder).pop_back();
                }

                // Save away max watch depth and modify with watch folder depth value if not all (-1).

                m_watchDepth = watchDepth;
                if (watchDepth != -1) {
                    m_watchDepth += std::count(watchFolder.begin(), watchFolder.end(), '/');
                }

            }

            // Allocate inotify read buffer

            m_inotifyBuffer.reset(new std::uint8_t [kInotifyEventBuffLen]);

            // Create watch table

            initWatchTable();

        }

        //
        // CApprise Destructor
        //

        CApprise::~CApprise() {

        }

        //
        // CApprise still watching folder(s)
        //

        bool CApprise::stillWatching(void) {

            return (m_doWork.load());

        }

        //
        // Check whether termination of CApprise was the result of any thrown exception
        //

        std::exception_ptr CApprise::getThrownException(void) {

            return (m_thrownException);

        }

        //
        // Add watch (file or directory)
        //

        void CApprise::addWatchFile(const std::string& filePath) {

            addWatch(filePath);

        }

        //
        // Remove watch
        //

        void CApprise::removeWatchFile(const std::string& filePath) {

            removeWatch(filePath);

        }

        //
        // Get next CApprise event in queue.
        //

        void CApprise::getEvent(CApprise::Event& evt) {

            std::unique_lock<std::mutex> locker(m_queuedEventsMutex);

            // Wait for something to happen. Either an event or stop running

            m_queuedEventsWaiting.wait(locker, [&]() {
                return (!m_queuedEvents.empty() || !m_doWork.load());
            });

            // return next event from queue

            if (!m_queuedEvents.empty()) {
                evt = m_queuedEvents.front();
                m_queuedEvents.pop();
            } else {
                evt.id = Event_none;
                evt.message = "";
            }

        }

        //
        // Start watching for file events
        //

        void CApprise::startWatching(void) {

            m_watcherThread.reset(new std::thread(&CApprise::generateEvents, this));

        }

        //
        // Stop watching for file events
        //

        void CApprise::stopWatching(void) {

            stopEventGeneration();

            if (m_watcherThread) {
                m_watcherThread->join();
            }

        }


    } // namespace File
} // namespace Antik
