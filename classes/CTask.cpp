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
                const std::string& taskName, // Task name
                const std::string& watchFolder, // Watch folder path
                TaskActionFcn taskActFcn, // Task action function
                std::shared_ptr<void> fnData, // Task file process function data
                int watchDepth, // Watch depth -1= all, 0=just watch folder
                std::shared_ptr<TaskOptions> options // Task options. 
                )
        : m_taskName{taskName}, m_taskActFcn{taskActFcn}, m_fnData{fnData}

        {

            // ASSERT if passed parameters invalid

            assert(taskName.length() != 0); // Length == 0
            assert(watchFolder.length() != 0); // Length == 0
            assert(watchDepth >= -1); // < -1
            assert(taskActFcn != nullptr); // nullptr
            assert(fnData != nullptr); // nullptr

            // If options passed then setup trace functions and  kill count

            if (options) {
                if (options->coutstr) {
                    this->m_coutstr = options->coutstr;
                }
                if (options->cerrstr) {
                    this->m_cerrstr = options->cerrstr;
                }
                this->m_killCount = options->killCount;
            }

            // Task prefix

            this->m_prefix = "[TASK " + this->m_taskName + "] ";

            // Create CFileApprise watcher object. Use same cout/cerr functions as Task.

            this->m_watcherOptions.reset(new CApprise::Options{0, false, this->m_coutstr, this->m_cerrstr});
            this->m_watcher.reset(new CApprise{watchFolder, watchDepth, m_watcherOptions});

            // Create CFileApprise object thread and start to watch

            this->m_watcherThread.reset(new std::thread(&CApprise::watch, this->m_watcher));

        }

        //
        // Destructor
        //

        CTask::~CTask() {

            this->m_coutstr({this->m_prefix, "CTask DESTRUCTOR CALLED."});

        }

        //
        // Check whether termination of CTask was the result of any thrown exception
        //

        std::exception_ptr CTask::getThrownException(void) {

            return (this->m_thrownException);

        }

        //
        // Flag watcher and task loops to stop.
        //

        void CTask::stop(void) {

            this->m_coutstr({this->m_prefix, "Stop task."});
            this->m_watcher->stop();

        }

        //
        // Loop calling the task action function for each add file event.
        //

        void CTask::monitor(void) {

            try {

                this->m_coutstr({this->m_prefix, "CTask monitor started."});

                // Loop until watcher stopped

                while (this->m_watcher->stillWatching()) {

                    CApprise::Event evt;

                    this->m_watcher->getEvent(evt);

                    if ((evt.id == CApprise::Event_add) && !evt.message.empty()) {

                        this->m_taskActFcn(evt.message, this->m_fnData);

                        if ((this->m_killCount != 0) && (--(this->m_killCount) == 0)) {
                            this->m_coutstr({this->m_prefix, "CTask kill count reached."});
                            break;
                        }

                    } else if ((evt.id == CApprise::Event_error) && !evt.message.empty()) {
                        this->m_coutstr({evt.message});
                    }

                }

                // Pass any CFileApprise exceptions up chain

                if (this->m_watcher->getThrownException()) {
                    this->m_thrownException = this->m_watcher->getThrownException();
                }

            } catch (...) {
                // Pass any CTask thrown exceptions up chain
                this->m_thrownException = std::current_exception();
            }

            // CFileApprise still flagged as running so close down 

            if (this->m_watcher->stillWatching()) {
                this->m_watcher->stop();
            }

            // Wait for CFileApprise thread

            this->m_watcherThread->join();

            this->m_coutstr({this->m_prefix, "CTask monitor on stopped."});

        }

    } // namespace File
} // namespace Antik