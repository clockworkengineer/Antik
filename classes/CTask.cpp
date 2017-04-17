#include "HOST.hpp"
/*
 * File:   CTask.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2016, 2:33 PM
 *
 * Copyright 2016.
 *
 */

//
// Class: CTask
// 
// Description: This class uses the CFileApprise class to generate file add events
// on a watch folder and to process each file added with a task action function 
// provided as a parameter in its constructor.
// 
// Dependencies: C11++               - Language standard features used.    
//               Class CLogger       - Logging functionality. 
//               Class CFileApprise  - File event handling abstraction.
//

// =================
// CLASS DEFINITIONS
// =================

#include "CTask.hpp"

// ====================
// CLASS IMPLEMENTATION
// ====================

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace File {


        // ===========================
        // PRIVATE TYPES AND CONSTANTS
        // ===========================

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

        // ==============
        // PUBLIC METHODS
        // ==============

        //
        // Task object constructor. 
        //

        CTask::CTask
        (
                const std::string& taskNameStr, // Task name
                const std::string& watchFolderStr, // Watch folder path
                TaskActionFcn taskActFcn, // Task action function
                std::shared_ptr<void> fnData, // Task file process function data
                int watchDepth, // Watch depth -1= all, 0=just watch folder
                std::shared_ptr<TaskOptions> options // Task options. 
                )
        : taskNameStr{taskNameStr}, taskActFcn{taskActFcn}, fnData{fnData}

        {

            // ASSERT if passed parameters invalid

            assert(taskNameStr.length() != 0); // Length == 0
            assert(watchFolderStr.length() != 0); // Length == 0
            assert(watchDepth >= -1); // < -1
            assert(taskActFcn != nullptr); // nullptr
            assert(fnData != nullptr); // nullptr

            // If options passed then setup trace functions and  kill count

            if (options) {
                if (options->coutstr) {
                    this->coutstr = options->coutstr;
                }
                if (options->cerrstr) {
                    this->cerrstr = options->cerrstr;
                }
                this->killCount = options->killCount;
            }

            // Task prefix

            this->prefix = "[TASK " + this->taskNameStr + "] ";

            // Create CFileApprise watcher object. Use same cout/cerr functions as Task.

            this->watcherOptions.reset(new CApprise::Options{0, false, this->coutstr, this->cerrstr});
            this->watcher.reset(new CApprise{watchFolderStr, watchDepth, watcherOptions});

            // Create CFileApprise object thread and start to watch

            this->watcherThread.reset(new std::thread(&CApprise::watch, this->watcher));

        }

        //
        // Destructor
        //

        CTask::~CTask() {

            this->coutstr({this->prefix, "CTask DESTRUCTOR CALLED."});

        }

        //
        // Check whether termination of CTask was the result of any thrown exception
        //

        std::exception_ptr CTask::getThrownException(void) {

            return (this->thrownException);

        }

        //
        // Flag watcher and task loops to stop.
        //

        void CTask::stop(void) {

            this->coutstr({this->prefix, "Stop task."});
            this->watcher->stop();

        }

        //
        // Loop calling the task action function for each add file event.
        //

        void CTask::monitor(void) {

            try {

                this->coutstr({this->prefix, "CTask monitor started."});

                // Loop until watcher stopped

                while (this->watcher->stillWatching()) {

                    CApprise::Event evt;

                    this->watcher->getEvent(evt);

                    if ((evt.id == CApprise::Event_add) && !evt.messageStr.empty()) {

                        this->taskActFcn(evt.messageStr, this->fnData);

                        if ((this->killCount != 0) && (--(this->killCount) == 0)) {
                            this->coutstr({this->prefix, "CTask kill count reached."});
                            break;
                        }

                    } else if ((evt.id == CApprise::Event_error) && !evt.messageStr.empty()) {
                        this->coutstr({evt.messageStr});
                    }

                }

                // Pass any CFileApprise exceptions up chain

                if (this->watcher->getThrownException()) {
                    this->thrownException = this->watcher->getThrownException();
                }

            } catch (...) {
                // Pass any CTask thrown exceptions up chain
                this->thrownException = std::current_exception();
            }

            // CFileApprise still flagged as running so close down 

            if (this->watcher->stillWatching()) {
                this->watcher->stop();
            }

            // Wait for CFileApprise thread

            this->watcherThread->join();

            this->coutstr({this->prefix, "CTask monitor on stopped."});

        }

    } // namespace File
} // namespace Antik