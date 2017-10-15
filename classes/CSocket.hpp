/*
 * File:   CSocket.hpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on September 23, 2017, 2:33 PM
 *
 * Copyright 2017.
 *
 */

#ifndef CSOCKET_HPP
#define CSOCKET_HPP

//
// C++ STL
//

#include <string>
#include <stdexcept>
#include <thread>
#include <memory>
#include <mutex>

//
// Boost ASIO
//

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

namespace Antik {
    namespace Network {

        // ==========================
        // PUBLIC TYPES AND CONSTANTS
        // ==========================

        // ================
        // CLASS DEFINITION
        // ================

        class CSocket {
        public:

            // ==========================
            // PUBLIC TYPES AND CONSTANTS
            // ==========================

            //
            // Class exception
            //

            struct Exception : public std::runtime_error {

                Exception(std::string const& message)
                : std::runtime_error("CSocket Failure: " + message) {
                }

            };
            // ============
            // CONSTRUCTORS
            // ============

            //
            // Main constructor
            //

            CSocket() {
            }

            // ==========
            // DESTRUCTOR
            // ==========

            virtual ~CSocket() {
            }

            // ==============
            // PUBLIC METHODS
            // ==============

            // Determine local machines IP address
            
            static std::string localIPAddress();

            // Socket I/O functions

            void connect();
            size_t read(char *readBuffer, size_t bufferLength);
            size_t write(const char *writeBuffer, size_t writeLength);
            void close();
            void tlsHandshake();
            bool closedByRemotePeer();
            void listenForConnection();
            void waitUntilConnected();
            void cleanup();

            // Private data accessors
            
            void setSslActive(bool sslActive);
            bool isSslActive() const;
            boost::system::error_code getSocketError() const;
            void setHostAddress(std::string hostAddress);
            std::string getHostAddress() const;
            void setHostPort(std::string hostPort);
            std::string getHostPort() const;

            // ================
            // PUBLIC VARIABLES
            // ================

        private:

            // ===========================
            // PRIVATE TYPES AND CONSTANTS
            // ===========================


            typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> SSLSocket;

            // ===========================================
            // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
            // ===========================================

            CSocket(const CSocket & orig) = delete;
            CSocket(const CSocket && orig) = delete;
            CSocket& operator=(CSocket other) = delete;

            // ===============
            // PRIVATE METHODS
            // ===============

            void connectionListener();

            // =================
            // PRIVATE VARIABLES
            // =================

            bool m_sslActive{ false};

            std::string m_hostAddress;   // Host ip address
            std::string m_hostPort;      // Host port address

            boost::system::error_code m_socketError; // Last socket error

            boost::asio::io_service m_ioService;                             // io Service
            boost::asio::ip::tcp::resolver m_ioQueryResolver{ m_ioService};  // io name resolver

            std::atomic<bool> m_isListenThreadRunning{ false};           // Listen thread running flag
            std::unique_ptr<std::thread> m_socketListenThread{ nullptr}; // Connection listen thread

            boost::asio::ssl::context m_sslContext{ m_ioService, boost::asio::ssl::context::tlsv12};    // Default TLS 1.2
            
            std::unique_ptr<SSLSocket> m_socket { nullptr };    // Socket allocated at run time 

        };

     
    } // namespace Network
} // namespace Antik

#endif /* CSOCKET_HPP */

