/*
 * File:   CTask.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2016, 2:33 PM
 *
 * Copyright 2016.
 *
 */

#ifndef CTASK_HPP
#define CTASK_HPP

//
// C++ STL
//

#include <cassert>
#include <thread>
#include <stdexcept>
#include <memory>

//
// Antik classes
//

#include "CommonAntik.hpp"
#include "CApprise.hpp"

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace File {

        // ================
        // CLASS DEFINITION
        // ================

        class CTask {
        public:

            // ==========================
            // PUBLIC TYPES AND CONSTANTS
            // ==========================

            //
            // Class exception
            //

            struct Exception : public std::runtime_error {

                explicit Exception(std::string const& message)
                : std::runtime_error("CTask Failure: " + message) {
                }

            };

            //
            // Base action interface class
            //

            class IAction {
            public:            
                virtual void init(void) = 0;
                virtual bool process(const std::string &file) = 0;
                virtual void term(void) = 0;
            };

            // ===========
            // CONSTRUCTOR
            // ===========

            //
            // Main constructor
            //

            explicit CTask
            (
                    const std::string& watchFolder,  // Watch folder path
                    std::shared_ptr<IAction> action, // Task action function
                    int watchDepth, // Watch depth -1= all, 0=just watch folder
                    int killCount // Kill count
            );

            // ==========
            // DESTRUCTOR
            // ==========

            virtual ~CTask(); // Task class cleanup

            // ==============
            // PUBLIC METHODS
            //===============

            //
            // Control
            //

            void monitor(void); // Monitor watch folder for directory file events and process added files
            void stop(void);    // Stop task

            //
            // Private data accessors
            //

            std::exception_ptr getThrownException(void); // Get any exception thrown by task to pass down chain

        private:

            // ===========================
            // PRIVATE TYPES AND CONSTANTS
            // ===========================

            // ===========================================
            // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
            // ===========================================

            CTask() = delete;
            CTask(const CTask & orig) = delete;
            CTask(const CTask && orig) = delete;
            CTask& operator=(CTask other) = delete;

            // ===============
            // PRIVATE METHODS
            // ===============

            // =================
            // PRIVATE VARIABLES
            // =================

            //
            // Constructor passed in and intialized
            //

            std::string m_watchFolder;             // Watch Folder
            std::shared_ptr<IAction> m_taskAction; // Task action function 
            int m_killCount { 0 };                   // Task Kill Count

            //
            // CFileApprise file watcher
            //

            std::shared_ptr<CApprise> m_watcher; // Folder watcher

            //
            // Publicly accessed via accessors
            //

            std::exception_ptr m_thrownException{ nullptr}; // Pointer to any exception thrown

        };

    } // namespace File
} // namespace Antik

#endif /* CTASK_HPP */

