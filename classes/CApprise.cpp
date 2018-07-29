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
// watches to be added/removed respectively. If no file event handler is passed then
// it defaults to the Linux inotify implementation.
//
// Dependencies: C11++               - Language standard features used.    
//

// =================
// CLASS DEFINITIONS
// =================

#include "CApprise.hpp"
#include "CFileEventNotifier.hpp"

// ====================
// CLASS IMPLEMENTATION
// ====================

//
// C++ STL
//

//#include <mutex>
//#include <system_error>
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
        // Main CApprise object constructor. 
        //

        CApprise::CApprise(const std::string& watchFolder, int watchDepth,  IFileEventNotifier *fileEventNotifier)
        : m_watchFolder{watchFolder}, m_watchDepth{watchDepth}
        {

            // ASSERT if passed parameters invalid

            assert(watchDepth >= -1); // < -1

            // If no handler passed then use default
            
            if (fileEventNotifier) {
                m_fileEventNotifier.reset(fileEventNotifier);
            } else {
                m_fileEventNotifier.reset(new CFileEventNotifier());
            }
            
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
            
            // Add non empty watch folder

            if (!m_watchFolder.empty()) {
                m_fileEventNotifier->addWatch(m_watchFolder);
            }

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

            return (m_fileEventNotifier->stillWatching());

        }

        //
        // Check whether termination of CApprise was the result of any thrown exception
        //

        std::exception_ptr CApprise::getThrownException(void) {

            return (m_fileEventNotifier->getThrownException());

        }

        //
        // Add watch (file or directory)
        //

        void CApprise::addWatch(const std::string& filePath) {

            m_fileEventNotifier->addWatch(filePath);

        }

        //
        // Remove watch
        //

        void CApprise::removeWatch(const std::string& filePath) {

            m_fileEventNotifier->removeWatch(filePath);

        }

        //
        // Get next CApprise event in queue.
        //

        void CApprise::getNextEvent(CApprise::Event& evt) {

            m_fileEventNotifier->getNextEvent(evt);

        }

        //
        // Start watching for file events
        //

        void CApprise::startWatching(void) {

            m_watcherThread.reset(new std::thread(&IFileEventNotifier::generateEvents, m_fileEventNotifier));

        }

        //
        // Stop watching for file events
        //

        void CApprise::stopWatching(void) {

            m_fileEventNotifier->stopEventGeneration();

            if (m_watcherThread) {
                m_watcherThread->join();
            }

        }


    } // namespace File
} // namespace Antik
