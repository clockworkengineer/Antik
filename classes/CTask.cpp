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

            // If options passed then setup kill count

            if (options) {
                m_killCount = options->killCount;
            }

            // Create CFileApprise watcher object. Use same cout/cerr functions as Task.

            m_watcher.reset(new CApprise{watchFolder, watchDepth});

        }

        //
        // Destructor
        //

        CTask::~CTask() {

        }

        //
        // Check whether termination of CTask was the result of any thrown exception
        //

        std::exception_ptr CTask::getThrownException(void) {

            return (m_thrownException);

        }

        //
        // Stop file watcher.
        //

        void CTask::stop(void) {

            m_watcher->stopWatching();

        }

        //
        // Loop calling the task action function for each add file event.
        //

        void CTask::monitor(void) {

            try {
                
                // Start file watcher

                m_watcher->startWatching();
                
                // Loop until watcher stopped

                while (m_watcher->stillWatching()) {

                    CApprise::Event evt;

                    m_watcher->getEvent(evt);

                    if ((evt.id == CApprise::Event_add) && !evt.message.empty()) {

                        m_taskActFcn(evt.message, m_fnData);

                        if ((m_killCount != 0) && (--(m_killCount) == 0)) { // Kill count reached so stop
                            break;
                        }

                    } else if ((evt.id == CApprise::Event_error) && !evt.message.empty()) {
//                        m_coutstr({evt.message});
                    }

                }

                // Pass any CFileApprise exceptions up chain

                if (m_watcher->getThrownException()) {
                    m_thrownException = m_watcher->getThrownException();
                }

            } catch (...) {
                // Pass any CTask thrown exceptions up chain
                m_thrownException = std::current_exception();
            }

            // Stop file watcher

            m_watcher->stopWatching();

        }

    } // namespace File
} // namespace Antik