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
//               Class CLogger       - Logging functionality. 
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

#include <thread>
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

        const uint32_t CApprise::kInofityEvents { 
                IN_ISDIR | IN_CREATE | IN_MOVED_TO | IN_MOVED_FROM |
                IN_DELETE_SELF | IN_CLOSE_WRITE | IN_DELETE | IN_MODIFY 
        };

        // inotify event structure size

        const uint32_t CApprise::kInotifyEventSize { (sizeof (struct inotify_event)) };

        // inotify event read buffer size

        const uint32_t CApprise::kInotifyEventBuffLen { (1024 * (CApprise::kInotifyEventSize + 16)) };

        // CApprise logging prefix

        const std::string CApprise::kLogPrefixStr { "[CApprise] " };

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
        // Display Inotify event using coutstr
        //

        void CApprise::displayInotifyEvent(struct inotify_event *event) {

#ifdef DISPLAY_INOTIFY_EVENTS

            std::string outstr{("    wd = " + std::to_string(event->wd) + ";")};

            if (event->cookie > 0) {
                outstr += ("cookie = " + std::to_string(event->wd) + ";");
            }

            outstr += ("mask = ");

            if (event->mask & IN_ACCESS) outstr += ("IN_ACCESS ");
            if (event->mask & IN_ATTRIB) outstr += ("IN_ATTRIB ");
            if (event->mask & IN_CLOSE_NOWRITE) outstr += ("IN_CLOSE_NOWRITE ");
            if (event->mask & IN_CLOSE_WRITE) outstr += ("IN_CLOSE_WRITE ");
            if (event->mask & IN_CREATE) outstr += ("IN_CREATE ");
            if (event->mask & IN_DELETE) outstr += ("IN_DELETE ");
            if (event->mask & IN_DELETE_SELF) outstr += ("IN_DELETE_SELF ");
            if (event->mask & IN_IGNORED) outstr += ("IN_IGNORED ");
            if (event->mask & IN_ISDIR) outstr += ("IN_ISDIR ");
            if (event->mask & IN_MODIFY) outstr += ("IN_MODIFY ");
            if (event->mask & IN_MOVE_SELF) outstr += ("IN_MOVE_SELF ");
            if (event->mask & IN_MOVED_FROM) outstr += ("IN_MOVED_FROM ");
            if (event->mask & IN_MOVED_TO) outstr += ("IN_MOVED_TO ");
            if (event->mask & IN_OPEN) outstr += ("IN_OPEN ");
            if (event->mask & IN_Q_OVERFLOW) outstr += ("IN_Q_OVERFLOW ");
            if (event->mask & IN_UNMOUNT) outstr += ("IN_UNMOUNT ");

            if (event->len > 0) {
                outstr += ("\n        name = " + std::string(event->name));
            }

            this->coutstr({outstr});

#endif // DISPLAY_INOTIFY_EVENTS

        }

        //
        // Clean up inotify. Note: closing the inotify file descriptor cleans up all
        // used resources including watch descriptors but removing them all before
        // hand will cause any pending read for events to return and the watcher loop
        // to stop. 
        //

        void CApprise::destroyWatchTable(void) {

            for (auto it = this->watchMap.begin(); it != this->watchMap.end(); ++it) {

                if (inotify_rm_watch(this->inotifyFd, it->first) == -1) {
                    throw std::system_error(std::error_code(errno, std::system_category()), "inotify_rm_watch() error");
                } else {
                    coutstr({kLogPrefixStr, "Watch[", std::to_string(it->first), "] removed.", "\n"});
                }

            }

            if (close(this->inotifyFd) == -1) {
                throw std::system_error(std::error_code(errno, std::system_category()), "inotify close() error");
            }

        }

        //
        // Initialize inotify and add watch for watchFolder.
        //

        void CApprise::initWatchTable(void) {

            // Initialize inotify 

            if ((this->inotifyFd = inotify_init()) == -1) {
                throw std::system_error(std::error_code(errno, std::system_category()), "inotify_init() error");
            }

            // Add non empty watch folder

            if (!this->watchFolderStr.empty()) {
                this->addWatch(this->watchFolderStr);
            }

        }

        //
        // Add watch for file/directory
        //

        void CApprise::addWatch(const std::string& filePathStr) {

            std::string fileNameStr { filePathStr };
            int watch { 0 };

            // Remove path trailing '/'

            if (fileNameStr.back() == '/') {
                fileNameStr.pop_back();
            }

            // Deeper than max watch depth so ignore.

            if ((this->watchDepth != -1) && (std::count(fileNameStr.begin(), fileNameStr.end(), '/') > this->watchDepth)) {
                return;
            }

            // Add watch to inotify 

            if ((watch = inotify_add_watch(this->inotifyFd, fileNameStr.c_str(), this->inotifyWatchMask)) == -1) {
                throw std::system_error(std::error_code(errno, std::system_category()), "inotify_add_watch() error");
            }

            // Add watch to map

            this->watchMap.insert({watch, fileNameStr});

            this->coutstr({kLogPrefixStr, "Watch added [", fileNameStr, "] watch = [", std::to_string(watch), "]"});

        }

        //
        //  Remove watch for file/directory
        //

        void CApprise::removeWatch(const std::string& filePathStr) {

            try {

                std::string fileName { filePathStr };
                int32_t watch { 0 };

                // Remove path trailing '/'

                if (fileName.back() == '/') {
                    fileName.pop_back();
                }

                // Find Watch value

                for (auto watchMapEntry : this->watchMap) {
                    if (watchMapEntry.second.compare(filePathStr) == 0) {
                        watch = watchMapEntry.first;
                        break;
                    }
                }

                if (watch) {

                    this->coutstr({kLogPrefixStr, "Watch removed [", fileName, "] watch = [", std::to_string(watch), "]"});

                    this->watchMap.erase(watch);

                    if (inotify_rm_watch(this->inotifyFd, watch) == -1) {
                        throw std::system_error(std::error_code(errno, std::system_category()), "inotify_rm_watch() error");
                    }

                } else {
                    this->cerrstr({kLogPrefixStr, "Watch not found in local map. Remove failed [", fileName, "]"});
                }


            } catch (std::system_error &e) {
                // Ignore error 22 and carry on. From the documentation on this error the kernel has removed the watch for us.
                if (e.code() != std::error_code(EINVAL, std::system_category())) {
                    throw; // Throw exception back up the chain.
                }
            }

            // No more watches so closedown

            if (this->watchMap.size() == 0) {
                this->coutstr({kLogPrefixStr, "*** Last watch deleted so terminating watch loop. ***"});
                this->stop();
            }

        }

        //
        // Queue CApprise event
        //

        void CApprise::sendEvent(CApprise::EventId id, const std::string& fileNameStr) {

            std::unique_lock<std::mutex> locker(this->queuedEventsMutex);
            this->queuedEvents.push({id, fileNameStr});
            this->queuedEventsWaiting.notify_one();

        }

        // ==============
        // PUBLIC METHODS
        // ==============

        //
        // Main CApprise object constructor. 
        //

        CApprise::CApprise(const std::string& watchFolderStr, int watchDepth, std::shared_ptr<CApprise::Options> options) : watchFolderStr{watchFolderStr}, watchDepth{watchDepth}, bDoWork{true}
        {

            // ASSERT if passed parameters invalid

            assert(watchFolderStr.length() != 0); // Length == 0
            assert(watchDepth >= -1); // < -1

            // If options passed then setup trace functions and event mask

            if (options) {
                this->bDisplayInotifyEvent = options->bDisplayInotifyEvent;
                if (options->inotifyWatchMask) {
                    this->inotifyWatchMask = options->inotifyWatchMask;
                }
                if (options->coutstr) {
                    this->coutstr = options->coutstr;
                }
                if (options->cerrstr) {
                    this->cerrstr = options->cerrstr;
                }
            }

            // Remove path trailing '/'

            if ((this->watchFolderStr).back() == '/') {
                (this->watchFolderStr).pop_back();
            }

            this->coutstr({kLogPrefixStr, "Watch folder [", this->watchFolderStr, "]"});
            this->coutstr({kLogPrefixStr, "Watch Depth [", std::to_string(watchDepth), "]"});

            // Save away max watch depth and modify with watch folder depth value if not all (-1).

            this->watchDepth = watchDepth;
            if (watchDepth != -1) {
                this->watchDepth += std::count(watchFolderStr.begin(), watchFolderStr.end(), '/');
            }

            // Allocate inotify read buffer

            this->inotifyBuffer.reset(new std::uint8_t [kInotifyEventBuffLen]);

            // Create watch table

            this->initWatchTable();

        }

        //
        // CApprise object constructor (watches need to be added/removed). 
        //

        CApprise::CApprise(std::shared_ptr<CApprise::Options> options) : bDoWork{true}
        {


            // If options passed then setup trace functions and event mask

            if (options) {
                this->bDisplayInotifyEvent = options->bDisplayInotifyEvent;
                if (options->inotifyWatchMask) {
                    this->inotifyWatchMask = options->inotifyWatchMask;
                }
                if (options->coutstr) {
                    this->coutstr = options->coutstr;
                }
                if (options->cerrstr) {
                    this->cerrstr = options->cerrstr;
                }
            }

            // Allocate inotify read buffer

            this->inotifyBuffer.reset(new u_int8_t [kInotifyEventBuffLen]);

            // Create watch table

            this->initWatchTable();

        }

        //
        // CApprise Destructor
        //

        CApprise::~CApprise() {

            this->coutstr({kLogPrefixStr, "DESTRUCTOR CALLED."});

        }

        //
        // CApprise still watching folder(s)
        //

        bool CApprise::stillWatching(void) {

            return (this->bDoWork.load());

        }

        //
        // Check whether termination of CApprise was the result of any thrown exception
        //

        std::exception_ptr CApprise::getThrownException(void) {

            return (this->thrownException);

        }

        //
        // Add watch (file or directory)
        //

        void CApprise::addWatchFile(const std::string& filePathStr) {

            this->addWatch(filePathStr);

        }

        //
        // Remove watch
        //

        void CApprise::removeWatchFile(const std::string& filePathStr) {

            this->removeWatch(filePathStr);

        }

        //
        // Get next CApprise event in queue.
        //

        void CApprise::getEvent(CApprise::Event& evt) {

            std::unique_lock<std::mutex> locker(this->queuedEventsMutex);

            // Wait for something to happen. Either an event or stop running

            this->queuedEventsWaiting.wait(locker, [&]() {
                return (!this->queuedEvents.empty() || !this->bDoWork.load());
            });

            // return next event from queue

            if (!this->queuedEvents.empty()) {
                evt = this->queuedEvents.front();
                this->queuedEvents.pop();
            } else {
                evt.id = Event_none;
                evt.messageStr = "";
            }

        }

        //
        // Flag watch loop to stop.
        //

        void CApprise::stop(void) {

            this->coutstr({kLogPrefixStr, "Stop CApprise thread."});

            std::unique_lock<std::mutex> locker(this->queuedEventsMutex);
            this->bDoWork = false;
            this->queuedEventsWaiting.notify_one();

            this->destroyWatchTable();

        }

        //
        // Loop adding/removing watches for directory hierarchy  changes
        // and also generating CApprise events from inotify; until stopped.
        //

        void CApprise::watch(void) {

            std::uint8_t *buffer { this->inotifyBuffer.get() };
            struct inotify_event *event { nullptr };
            std::string filePathStr;

            this->coutstr({kLogPrefixStr, "CApprise watch loop started on thread [",
                Antik::Util::CLogger::toString(std::this_thread::get_id()), "]"});

            try {

                // Loop until told to stop

                while (this->bDoWork.load()) {

                    int readLen { 0 };
                    int currentPos { 0 };

                    // Read in events

                    if ((readLen = read(this->inotifyFd, buffer, kInotifyEventBuffLen)) == -1) {
                        throw std::system_error(std::error_code(errno, std::system_category()), "inotify read() error");
                    }

                    // Loop until all read processed

                    while (currentPos < readLen) {

                        // Point to next event & display if necessary

                        event = (struct inotify_event *) &buffer[ currentPos ];
                        currentPos += kInotifyEventSize + event->len;

                        // Display inotify event

                        if (this->bDisplayInotifyEvent) {
                            this->displayInotifyEvent(event);
                        }

                        // IGNORE so move onto next event

                        if (event->mask == IN_IGNORED) {
                            continue;
                        }

                        // Create full file name path

                        filePathStr = this->watchMap[event->wd];

                        if (event->len > 0) {
                            filePathStr += ("/" + static_cast<std::string>(event->name));
                        }

                        // Process event

                        switch (event->mask) {

                                // Flag file as being created

                            case IN_CREATE:
                            {
                                this->inProcessOfCreation.insert(filePathStr);
                                break;
                            }

                                // If file not being created send Event_change

                            case IN_MODIFY:
                            {
                                auto beingCreated = this->inProcessOfCreation.find(filePathStr);
                                if (beingCreated == this->inProcessOfCreation.end()) {
                                    this->sendEvent(Event_change, filePathStr);
                                }
                                break;
                            }

                                // Add watch for new directory and send Event_addir

                            case (IN_ISDIR | IN_CREATE):
                            case (IN_ISDIR | IN_MOVED_TO):
                            {
                                this->sendEvent(Event_addir, filePathStr);
                                this->addWatch(filePathStr);
                                break;
                            }

                                // Directory deleted send Event_unlinkdir

                            case (IN_ISDIR | IN_DELETE):
                            {
                                this->sendEvent(Event_unlinkdir, filePathStr);
                                break;
                            }

                                // Remove watch for deleted/moved directory

                            case (IN_ISDIR | IN_MOVED_FROM):
                            case IN_DELETE_SELF:
                            {
                                this->removeWatch(filePathStr);
                                break;
                            }

                                // File deleted send Event_unlink

                            case IN_DELETE:
                            {
                                this->sendEvent(Event_unlink, filePathStr);
                                break;
                            }

                                // File moved into directory send Event_add.

                            case IN_MOVED_TO:
                            {
                                this->sendEvent(Event_add, filePathStr);
                                break;
                            }

                                // File closed. If being created send Event_add otherwise Event_change.

                            case IN_CLOSE_WRITE:
                            {
                                auto beingCreated = this->inProcessOfCreation.find(filePathStr);
                                if (beingCreated == this->inProcessOfCreation.end()) {
                                    this->sendEvent(Event_change, filePathStr);
                                } else {
                                    this->inProcessOfCreation.erase(filePathStr);
                                    this->sendEvent(Event_add, filePathStr);
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
                this->sendEvent(Event_error, kLogPrefixStr + "Caught a system_error exception: [" + e.what() + "]");
                this->thrownException = std::current_exception();
            } catch (std::exception &e) {
                this->sendEvent(Event_error, kLogPrefixStr + "General exception occured: [" + e.what() + "]");
                this->thrownException = std::current_exception();
            }

            // If still active then need to close down

            if (this->bDoWork.load()) {
                this->stop();
            }

            this->coutstr({kLogPrefixStr, "CApprise watch loop stopped."});

        }

    } // namespace File
} // namespace Antik