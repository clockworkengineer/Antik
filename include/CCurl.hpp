/*
 * File:   Curl.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on August 04, 2018, 2:33 PM
 *
 * Copyright 2017.
 *
 */

#ifndef CURL_HPP
#define CURL_HPP

//
// C++ STL
//

#include <stdexcept>
#include <string>
#include <vector>
#include <memory>

//
// libcurl
//

#include <curl/curl.h>

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace Network {

        // ==========================
        // PUBLIC TYPES AND CONSTANTS
        // ==========================

        // ================

        // CLASS DEFINITION
        // ================

        class CCurl {
        public:

            // ==========================
            // PUBLIC TYPES AND CONSTANTS
            // ==========================

            //
            // Class exception
            //

            struct Exception : public std::runtime_error {

                Exception(std::string const& message)
                : std::runtime_error("CCurl Failure: " + message) {
                }

            };

            class OptionValue {
            public:
                virtual long getValue() = 0;
            };
            
            template <typename T>
            class Parameter : public OptionValue {
            public:
                Parameter(T value) : m_value{value}
                {
                }

                long getValue() override {
                    return (reinterpret_cast<long> (m_value));
                }
            private:
                T m_value;
            };
            
            struct Options {
                Options(CURLoption option, OptionValue* value) : m_option{option}, m_value{value}
                {
                }
                CURLoption m_option;
                std::shared_ptr<OptionValue> m_value;
            };

            using StatusCode = CURLcode;

            // ============
            // CONSTRUCTORS
            // ============

            //
            // Main constructor
            //

            CCurl(void);

            // ==========
            // DESTRUCTOR
            // ==========

            virtual ~CCurl();


            // ==============
            // PUBLIC METHODS
            // ==============

            void setErrorBuffer(size_t errorBufferSize);

            void setOptions(std::vector<Options> &options);

            void transfer();
            
            static void globalCleanup() {  curl_global_cleanup(); }

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

            CCurl(const CCurl & orig) = delete;
            CCurl(const CCurl && orig) = delete;
            CCurl& operator=(CCurl other) = delete;

            // ===============
            // PRIVATE METHODS
            // ===============

            // =================
            // PRIVATE VARIABLES
            // =================

            std::string m_errorBuffer;
            CURL *m_curlConnection;

            static bool m_globalInitialised;

        };

    } // namespace Network
} // namespace Antik

#endif /* CURL_HPP */

