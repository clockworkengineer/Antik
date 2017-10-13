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

//
// Class: CSocket
// 
// Description: 
//
// Dependencies:   C11++     - Language standard features used.
//

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

            void connect(const std::string &hostAddress, const std::string &hostPort);
            size_t read(char *readBuffer, size_t bufferLength);
            size_t write(const char *writeBuffer, size_t writeLength);
            void close();
            void tlsHandshake();
            bool closedByRemotePeer();
            void listenForConnection();
            void connected();
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

            std::string m_hostAddress; // Host ip address
            std::string m_hostPort; // Host port address

            boost::system::error_code m_socketError; // Last socket error

            boost::asio::io_service m_ioService; // io Service
            boost::asio::ip::tcp::resolver m_ioQueryResolver{ m_ioService}; // io name resolver

            std::atomic<bool> m_isListenThreadRunning{ false}; // Listen thread running flag
            std::shared_ptr<std::thread> m_dataChannelListenThread{ nullptr}; // Active mode connection listen thread

            boost::asio::ssl::context sslContext{ m_ioService, boost::asio::ssl::context::tlsv12};
            SSLSocket m_socket{m_ioService, sslContext};

        };

        //
        // Data channel socket listener thread method for incoming data 
        // channel connections.
        //

        inline void CSocket::connectionListener() {

            boost::asio::ip::tcp::acceptor acceptor(m_ioService, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0));

            m_hostPort = std::to_string(acceptor.local_endpoint().port());

            m_isListenThreadRunning = true;
            acceptor.accept(m_socket.next_layer(), m_socketError);
            if (m_socketError) {
                m_isListenThreadRunning = false;
                throw Exception(m_socketError.message());
            }

            m_isListenThreadRunning = false;


        }

        //
        // Cleanup after data channel transfer. This includes stopping any unused listener
        // thread and closing the socket if still open.
        //

        inline void CSocket::cleanup() {

            if (m_isListenThreadRunning && m_dataChannelListenThread) {
                m_isListenThreadRunning = false;
                try {
                    boost::asio::ip::tcp::socket socket{ m_ioService};
                    boost::asio::ip::tcp::resolver::query query(m_hostAddress, m_hostPort);
                    boost::asio::connect(socket, m_ioQueryResolver.resolve(query));
                    socket.close();
                } catch (std::exception &e) {
                    throw Exception ("Listener thread running when it should not be.");
                }
                m_dataChannelListenThread->join();
            }

            close();

        }

        //
        // Listen for connections
        //

        inline void CSocket::listenForConnection() {

            m_dataChannelListenThread.reset(new std::thread(&CSocket::connectionListener, this));
            while (!m_isListenThreadRunning) { // Wait for until listening before sending PORT command
                continue; // Could use conditional but use existing flag for now
            }

        }

        //
        // Socket connected processing.
        //

        inline void CSocket::connected() {

            // Listener thread is running (wait for it to finish)

            if (m_dataChannelListenThread) {
                m_dataChannelListenThread->join();
            }

            // TLS handshake

            if (m_sslActive) {
                tlsHandshake();
            }

        }

        //
        // Connect to a given host and port.
        //

        inline void CSocket::connect(const std::string &hostAddress, const std::string &hostPort) {

            boost::asio::ip::tcp::resolver::query query(hostAddress, hostPort);
            m_socket.next_layer().connect(*m_ioQueryResolver.resolve(query), m_socketError);
            if (m_socketError) {
                throw Exception(m_socketError.message());
            }

        }

        //
        // Read data from socket into buffer
        //

        inline size_t CSocket::read(char *readBuffer, size_t bufferLength) {

            if (m_sslActive) {
                return (m_socket.read_some(boost::asio::buffer(readBuffer, bufferLength), m_socketError));
            } else {
                return (m_socket.next_layer().read_some(boost::asio::buffer(readBuffer, bufferLength), m_socketError));
            }

        }

        //
        // Write data to socket
        //

        inline size_t CSocket::write(const char *writeBuffer, size_t writeLength) {

            size_t bytesWritten = 0;

            if (m_sslActive) {
                bytesWritten = m_socket.write_some(boost::asio::buffer(writeBuffer, writeLength), m_socketError);
            } else {
                bytesWritten = m_socket.next_layer().write_some(boost::asio::buffer(writeBuffer, writeLength), m_socketError);
            }

            if (getSocketError()) {
                throw Exception(getSocketError().message());
            }

            return (bytesWritten);

        }

        //
        // Perform TLS handshake on to enable SSL
        //

        inline void CSocket::tlsHandshake() {
            m_socket.handshake(SSLSocket::client, m_socketError);
            if (m_socketError) {
                throw Exception(m_socketError.message());
            }
            m_sslActive = true;
        }

        //
        // Closedown any running SSL and close m_socket.
        //

        inline void CSocket::close() {

            if (m_socket.next_layer().is_open()) {
                if (m_sslActive) {
                    m_socket.shutdown(m_socketError);
                }
                m_socket.next_layer().close();
            }

            if (m_dataChannelListenThread) {
                m_dataChannelListenThread.reset();
            }

        }

        //
        // Return true if socket closed by server otherwise false.
        // Also throw exception for any socket error detected,
        //

        inline bool CSocket::closedByRemotePeer() {

            if (m_socketError == boost::asio::error::eof) {
                return (true); // Connection closed cleanly by peer.
            } else if (m_socketError) {
                throw Exception(m_socketError.message());
            }

            return (false);

        }

        //
        // Work out ip address for local machine. This is quite difficult to achieve but
        // this is the best code i have seen for doing it. It just tries to connect to
        // google.com with a udp connect to get the local socket endpoint.
        // Note: Fall back of localhost on failure.
        // 

        inline std::string CSocket::localIPAddress() {

            static std::string localIPAddress;

            if (localIPAddress.empty()) {
                try {
                    boost::asio::io_service ioService;
                    boost::asio::ip::udp::resolver resolver(ioService);
                    boost::asio::ip::udp::resolver::query query(boost::asio::ip::udp::v4(), "google.com", "");
                    boost::asio::ip::udp::socket socket(ioService);
                    socket.connect(*resolver.resolve(query));
                    localIPAddress = socket.local_endpoint().address().to_string();
                    socket.close();
                } catch (std::exception &e) {
                    return ("127.0.0.1");
                }
            }

            return (localIPAddress);

        }

        inline void CSocket::setSslActive(bool sslActive) {
            m_sslActive = sslActive;
        }

        inline bool CSocket::isSslActive() const {
            return m_sslActive;
        }

        inline boost::system::error_code CSocket::getSocketError() const {
            return m_socketError;
        }

        inline void CSocket::setHostAddress(std::string hostAddress) {
            m_hostAddress = hostAddress;
        }

        inline std::string CSocket::getHostAddress() const {
            return m_hostAddress;
        }

        inline void CSocket::setHostPort(std::string hostPort) {
            m_hostPort = hostPort;
        }

        inline std::string CSocket::getHostPort() const {
            return m_hostPort;
        }

     
    } // namespace Network
} // namespace Antik

#endif /* CSOCKET_HPP */

