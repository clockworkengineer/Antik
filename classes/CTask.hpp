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

//
// CLogger trace output, CFileApprise file event watcher
//

#include "CLogger.hpp"
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

                Exception(std::string const& messageStr)
                : std::runtime_error("CTask Failure: " + messageStr) {
                }

            };

            //
            // Task action function
            //

            typedef std::function<bool (const std::string&, const std::shared_ptr<void>) > TaskActionFcn;

            //
            // Task options structure (optionally pass to CTask constructor)
            // Note: After killCount files processed stop task (0 = disabled)
            //

            struct TaskOptions {
                int killCount; // file kill count
                Antik::Util::CLogger::LogStringsFn coutstr; // coutstr output
                Antik::Util::CLogger::LogStringsFn cerrstr; // cerrstr output
            };

            // ===========
            // CONSTRUCTOR
            // ===========

            //
            // Main constructor
            //

            CTask
            (
                const std::string& taskNameStr, // Task name
                const std::string& watchFolderStr, // Watch folder path
                TaskActionFcn taskActFcn, // Task action function
                std::shared_ptr<void> fnData, // Task file process function data
                int watchDepth, // Watch depth -1= all, 0=just watch folder
                std::shared_ptr<TaskOptions> options = nullptr // Task options. 
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
            void stop(void); // Stop task

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

            std::string taskNameStr; // Task name
            std::string watchFolderStr; // Watch Folder
            TaskActionFcn taskActFcn; // Task action function 
            std::shared_ptr<void> fnData; // Task action function data   
            int killCount { 0 }; // Task Kill Count

            //
            // CFileApprise file watcher
            //

            std::shared_ptr<CApprise> watcher; // Folder watcher
            std::shared_ptr<CApprise::Options> watcherOptions; // folder watcher options
            std::unique_ptr<std::thread> watcherThread; // Folder watcher thread

            //
            // Publicly accessed via accessors
            //

            std::exception_ptr thrownException { nullptr }; // Pointer to any exception thrown

            //
            // Trace functions default do nothing
            //

            Antik::Util::CLogger::LogStringsFn coutstr { Antik::Util::CLogger::noOp };
            Antik::Util::CLogger::LogStringsFn cerrstr { Antik::Util::CLogger::noOp };

            std::string prefix; // Task trace prefix

        };

    } // namespace File
} // namespace Antik

#endif /* CTASK_HPP */

