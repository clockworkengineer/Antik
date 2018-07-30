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

#ifndef CAPPRISE_HPP
#define CAPPRISE_HPP

//
// C++ STL
//

#include <stdexcept>
#include <thread>

//
// Antik classes
//

#include "CommonAntik.hpp"
#include "IApprise.hpp"
#include "IFileEventNotifier.hpp"

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace File {

        // ================
        // CLASS DEFINITION
        // ================

        class CApprise : public IApprise {
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

            explicit CApprise
            (
                const std::string& watchFolder = "", // Watch folder path;;
                int watchDepth = -1,                 // Watch depth -1=all,0=just watch folder,1=next level down etc.
                IFileEventNotifier *fileEventNotifier = nullptr // File event notifier
            );

            // ==========
            // DESTRUCTOR
            // ==========

            virtual ~CApprise();

            // ==============
            // PUBLIC METHODS
            // ==============

            //
            // Event control
            //

            void startWatching(bool clearQueue=true) override;
            void stopWatching(void) override;
            bool stillWatching(void) override;
            void getNextEvent(CApprise::Event& message) override;

            //
            // Watch handling
            //

            void addWatch(const std::string& filePath) override; // Add directory/file to be watched
            void removeWatch(const std::string& filePath) override; // Remove directory/file being watched

            //
            // Get any thrown exceptions
            //

            std::exception_ptr getThrownException(void) override; // Get any exception thrown by watcher to pass down chain

            // ================
            // PUBLIC VARIABLES
            // ================

        private:

            // ===========================
            // PRIVATE TYPES AND CONSTANTS
            // ===========================

            // ===========================================
            // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
            // ===========================================

            CApprise(const CApprise & orig) = delete;
            CApprise(const CApprise && orig) = delete;
            CApprise& operator=(CApprise other) = delete;

            // ===============
            // PRIVATE METHODS
            // ===============

            // =================
            // PRIVATE VARIABLES
            // =================

            //
            // Constructor passed in and intialised
            //

            std::string m_watchFolder; // Watch Folder
            int m_watchDepth{ -1}; // Watch depth -1=all,0=just watch folder,1=next level down etc.
            std::shared_ptr<IFileEventNotifier> m_fileEventNotifier; // File event notifier

            //
            // Watcher thread
            //

            std::unique_ptr<std::thread> m_watcherThread;

        };


    } // namespace File
} // namespace Antik

#endif /* CAPPRISE_HPP */

