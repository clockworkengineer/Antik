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

            //
            // Option / value used in setOption(s).
            //
            
            class IOptionValue {
            public:
                virtual long getValue() = 0;
            };

            template <typename T>
            class OptionValue : public IOptionValue {
            public:
                OptionValue(T value) : m_value{value}
                {
                }

                long getValue() override {
                    return (reinterpret_cast<long> (m_value));
                }
            private:
                T m_value;
            };
            
            //
            // String list
            //
            
            class StringList : public IOptionValue {
            public:

                StringList() {
                }

                ~StringList() {
                    if (m_stringList) curl_slist_free_all(m_stringList);
                    m_stringList = NULL;
                }

                StringList(const StringList &other) = delete;
                StringList(const StringList &&other) = delete;
                
                void append(const char *string) {
                    m_stringList = curl_slist_append(m_stringList, string);
                    if (m_stringList == NULL) {
                        throw Exception("Failed to append to string list.");
                    }
                }

                long getValue() override {
                    return (reinterpret_cast<long> (m_stringList));
                }

            private:

                struct curl_slist *m_stringList { NULL };
                
            };
            
            struct OptionAndValue {
                OptionAndValue(CURLoption option, IOptionValue* value) : m_option{option}, m_value{value}
                {
                }
                CURLoption m_option;
                std::shared_ptr<IOptionValue> m_value;
            };

            
            //
            // Curl return status code and get info.
            //

            using StatusCode = CURLcode;
            using Info = CURLINFO;

            // ============
            // CONSTRUCTORS
            // ============

            //
            // Main constructor.
            //

            CCurl(void);

            // ==========
            // DESTRUCTOR
            // ==========

            virtual ~CCurl();

            // ==============
            // PUBLIC METHODS
            // ==============

            //
            // Set extended error buffer size.
            //
            
            void setErrorBuffer(size_t errorBufferSize);

            //
            // Set connection options.
            //
            
            void setOption(const OptionAndValue &option);
            void setOptions(const std::vector<OptionAndValue> &options);

            //
            // Get connection information.
            //
            
            template <typename T> T getInfo(const Info &info);

            //
            // Perform connection transfer.
            //
            
            void transfer();

            //
            // Curl global closedown(cleanup).
            //
            
            static void globalCleanup() {
                curl_global_cleanup();
            }

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

            std::string m_errorBuffer;          // Extended error buffer
            CURL *m_curlConnection;             // Curl connection
            static bool m_globalInitialised;    // == true global intialisation performed.

        };

        //
        // Get connection information.
        //
        
        template <typename T>
        T CCurl::getInfo(const Info &info) {
            T infoValue;
            auto code = curl_easy_getinfo(m_curlConnection, info, &infoValue);
            if (code != CURLE_OK) {
                if (m_errorBuffer[0]) {
                    throw Exception("Failed to get information." + m_errorBuffer);
                } else {
                    throw Exception(std::string("Failed to get information.") + curl_easy_strerror(code) + ".");
                }
            }
            return (infoValue);
        }

    } // namespace Network
} // namespace Antik

#endif /* CURL_HPP */

