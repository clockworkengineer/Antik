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

                Exception(std::string const& message)
                : std::runtime_error("CTask Failure: " + message) {
                }

            };

            //
            // Task action function
            //

     //       typedef std::function<bool (const std::string&, const std::shared_ptr<void>) > TaskActionFcn;

            //
            // Task options structure (optionally pass to CTask constructor)
            // Note: After killCount files processed stop task (0 = disabled)
            //

            struct TaskOptions {
                int killCount; // file kill count
                Antik::Util::CLogger::LogStringsFn coutstr; // coutstr output
                Antik::Util::CLogger::LogStringsFn cerrstr; // cerrstr output
            };
            
            class Action {
            public:
                Action(const std::string &taskName) : name {taskName} {}
                std::string name;
                virtual void init(void)=0;
                virtual bool process(const std::string &file, const std::shared_ptr<void> fnData)=0;
                virtual void term(void)=0;
                virtual ~Action() {};
            };

            // ===========
            // CONSTRUCTOR
            // ===========

            //
            // Main constructor
            //

            explicit CTask
            (
                const std::string& taskName,    // Task name
                const std::string& watchFolder, // Watch folder path
                std::shared_ptr<Action> taskActFcn,  // Task action function
                std::shared_ptr<void> fnData,   // Task file process function data
                int watchDepth,                 // Watch depth -1= all, 0=just watch folder
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

            std::string m_taskName;         // Task name
            std::string m_watchFolder;      // Watch Folder
            std::shared_ptr<Action> m_taskActFcn;      // Task action function 
            std::shared_ptr<void> m_fnData; // Task action function data   
            int m_killCount { 0 };          // Task Kill Count

            //
            // CFileApprise file watcher
            //

            std::shared_ptr<CApprise> m_watcher; // Folder watcher

            //
            // Publicly accessed via accessors
            //

            std::exception_ptr m_thrownException { nullptr }; // Pointer to any exception thrown

            //
            // Trace functions default do nothing
            //

            Antik::Util::CLogger::LogStringsFn m_coutstr { Antik::Util::CLogger::noOp };
            Antik::Util::CLogger::LogStringsFn m_cerrstr { Antik::Util::CLogger::noOp };

            std::string m_prefix; // Task trace prefix

        };

    } // namespace File
} // namespace Antik

#endif /* CTASK_HPP */

