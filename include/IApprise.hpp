/*
 * File:   IApprise.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 24, 2016, 2:33 PM
 *
 * Copyright 2016.
 *
 */

#ifndef IAPPRISE_HPP
#define IAPPRISE_HPP

//
// C++ STL
//

#include <stdexcept>
#include <string>

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace File {

        //
        // Apprise interface
        //

        class IApprise {
        public:

            //
            // Class exception
            //

            struct Exception : public std::runtime_error {

                explicit Exception(std::string const& message)
                : std::runtime_error("CApprise Failure: " + message) {
                }

            };

            //
            // CApprise event identifiers
            //

            enum EventId {
                Event_none = 0,  // None
                Event_add,       // File added to watched folder hierarchy
                Event_change,    // File changed
                Event_unlink,    // File deleted from watched folder hierarchy
                Event_addir,     // Directory added to watched folder hierarchy
                Event_unlinkdir, // Directory deleted from watched folder hierarchy
                Event_error      // Exception error
            };

            //
            // CApprise event structure
            //

            struct Event {
                explicit Event(EventId id = EventId::Event_none, std::string message = "") : id{id}, message{message}
                {
                }
                EventId id;          // Event id
                std::string message; // Event file name / error message string
            };

            //
            // Event control
            //

            virtual void startWatching(bool clearQueue) = 0;
            virtual void stopWatching(void) = 0;
            virtual bool stillWatching(void) = 0;
            virtual void getNextEvent(IApprise::Event& message) = 0;      

            //
            // Watch handling
            //

            virtual void addWatch(const std::string& filePath) = 0;    // Add directory/file to be watched
            virtual void removeWatch(const std::string& filePath) = 0; // Remove directory/file being watched

            //
            // Get any thrown exceptions
            //

            virtual std::exception_ptr getThrownException(void) = 0; // Get any exception thrown by watcher to pass down chain          

        };

    } // namespace File
} // namespace Antik

#endif /* IAPPRISE_HPP */

