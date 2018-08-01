#include "HOST.hpp"
/*
 * File:   CSocket.cpp
 * 
 * Author: Robert Tizzard
 * 
 * Created on October 10, 2017, 2:33 PM
 *
 * Copyright 2017.
 *
 */

//
// Class: CSocket
// 
// Description: Class for connecting to / listening for connections from remote peers
// and the reading/writing of data using sockets. It supports both plain and TLS/SSL 
// connections and  is implemented using BOOST:ASIO synchronous API calls. At present it
// only has basic TLS/SSL support and is geared more towards client support but this may
// change in future.
//
// Dependencies:   C11++        - Language standard features used.
//                 BOOST ASIO   - Used to talk to FTP server.
//

// =================
// CLASS DEFINITIONS
// =================

#include "CSocket.hpp"

// ====================
// CLASS IMPLEMENTATION
// ====================

//
// C++ STL
//

#include <iostream>
#include <fstream>

// =======
// IMPORTS
// =======

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace Network {

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

        //
        // Socket listener thread method for incoming connections. At present it listens
        // on a random port but sets m_hostPort to its value. The connected socket is created
        // local and moved to m_socket on success.
        //

        void CSocket::connectionListener() {

            try {

                boost::asio::ip::tcp::acceptor acceptor(m_ioService, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0));

                m_hostPort = std::to_string(acceptor.local_endpoint().port());

                m_isListenThreadRunning = true;

                std::unique_ptr<SSLSocket> socket{ new SSLSocket(m_ioService, *m_sslContext)};
                if (!socket) {
                    throw std::runtime_error("Could not create socket.");
                }

                acceptor.accept(socket->next_layer(), m_socketError);
                if (m_socketError) {
                    throw std::runtime_error(m_socketError.message());
                }

                m_socket = std::move(socket);

            } catch (const std::exception &e) {
                m_thrownException = std::current_exception();
            }

            m_isListenThreadRunning = false;
            
        }

        // ==============
        // PUBLIC METHODS
        // ==============

        //
        // Cleanup after socket connection. This includes stopping any unused listener
        // thread and closing the socket if still open. Note: if the thread is still waiting
        // for a connect it is woken up with a fake connect.
        //

        void CSocket::cleanup() {

            try {

                if (m_isListenThreadRunning && m_socketListenThread) {
                    m_isListenThreadRunning = false;
                    try {
                        boost::asio::ip::tcp::socket socket{ m_ioService};
                        boost::asio::ip::tcp::resolver::query query(m_hostAddress, m_hostPort);
                        boost::asio::connect(socket, m_ioQueryResolver.resolve(query));
                        socket.close();
                    } catch (std::exception &e) {
                        throw std::runtime_error("Could not wake listener thread with fake connect.");
                    }
                    m_socketListenThread->join();
                }

                close();

            } catch (const std::exception &e) {
                throw Exception(e.what());
            }

        }

        //
        // Listen for connections
        //

        void CSocket::listenForConnection() {

            try {

                // Start connection listener thread and wait until its listening

                m_socketListenThread.reset(new std::thread(&CSocket::connectionListener, this));
                while (!m_isListenThreadRunning) {
                    continue;
                }
                
                if (m_thrownException) {
                    std::rethrow_exception(m_thrownException); 
                }

            } catch (const std::exception &e) {
                throw Exception(e.what());
            }

        }

        //
        // Wait until a socket is connected.
        //

        void CSocket::waitUntilConnected() {

            try {

                // Listener thread is running (wait for it to finish)

                if (m_socketListenThread) {
                    m_socketListenThread->join();
                }

                // TLS handshake if SSL enabled

                tlsHandshake();

            } catch (const std::exception &e) {
                throw Exception(e.what());
            }

        }

        //
        // Connect to a given host and port.The connecting socket is created
        // local and moved to m_socket on success.
        //

        void CSocket::connect() {

            try {

                std::unique_ptr<SSLSocket> socket{ new SSLSocket(m_ioService, *m_sslContext)};
                if (!socket) {
                    throw std::logic_error("Could not create socket.");
                }

                boost::asio::ip::tcp::resolver::query query{ m_hostAddress, m_hostPort};
                socket->next_layer().connect(*m_ioQueryResolver.resolve(query), m_socketError);
                if (m_socketError) {
                    throw std::runtime_error(m_socketError.message());
                }

                m_socket = std::move(socket);

            } catch (const std::exception &e) {
                throw Exception(e.what());
            }
            
        }

        //
        // Read data from socket into buffer
        //

        size_t CSocket::read(char *readBuffer, size_t bufferLength) {

            try {

                size_t bytesRead{ 0};

                // No socket present

                if (!m_socket) {
                    throw std::logic_error("No socket present.");
                }

                // Read data

                if (m_sslActive) {
                    bytesRead = m_socket->read_some(boost::asio::buffer(readBuffer, bufferLength), m_socketError);
                } else {
                    bytesRead = m_socket->next_layer().read_some(boost::asio::buffer(readBuffer, bufferLength), m_socketError);
                }

                // Signal any non end of file  error

                if (m_socketError && m_socketError != boost::asio::error::eof) {
                    throw std::runtime_error(m_socketError.message());
                }

                return (bytesRead);

            } catch (const std::exception &e) {
                throw Exception(e.what());
            }
            
        }

        //
        // Write data to socket
        //

        size_t CSocket::write(const char *writeBuffer, size_t writeLength) {

            try {

                size_t bytesWritten{ 0};

                // No socket present

                if (!m_socket) {
                    throw std::logic_error("No socket present.");
                }

                // Write data

                if (m_sslActive) {
                    bytesWritten = m_socket->write_some(boost::asio::buffer(writeBuffer, writeLength), m_socketError);
                } else {
                    bytesWritten = m_socket->next_layer().write_some(boost::asio::buffer(writeBuffer, writeLength), m_socketError);
                }

                // Signal any non end of file error

                if (m_socketError && m_socketError != boost::asio::error::eof) {
                    throw std::runtime_error(m_socketError.message());
                }

                return (bytesWritten);

            } catch (const std::exception &e) {
                throw Exception(e.what());
            }
            
        }

        //
        // Perform TLS handshake if SSL enabled
        //

        void CSocket::tlsHandshake() {

            try {

                // If SSL not enabled return

                if (!m_sslEnabled) {
                    return;
                }

                // No socket present

                if (!m_socket) {
                    throw std::logic_error("No socket present.");
                }

                m_socket->handshake(SSLSocket::client, m_socketError);
                if (m_socketError) {
                    throw std::runtime_error(m_socketError.message());
                }

                m_sslActive = true;

            } catch (const std::exception &e) {
                throw Exception(e.what());
            }

        }

        //
        // Closedown any running SSL and close socket. Move m_socket to local
        // socket and close it down.
        //

        void CSocket::close() {

            try {

                std::unique_ptr<SSLSocket> socket{ std::move(m_socket)};

                // Socket exists and is open

                if (socket && socket->next_layer().is_open()) {

                    // Shutdown TLS

                    if (m_sslActive) {
                        m_sslActive = false;
                        socket->shutdown(m_socketError);
                    }

                    // Close socket

                    socket->next_layer().close(m_socketError);
                    if (m_socketError) {
                        throw std::runtime_error(m_socketError.message());
                    }

                }

                // Remove any listen thread

                if (m_socketListenThread) {
                    m_socketListenThread.reset();
                }

            } catch (const std::exception &e) {
                throw Exception(e.what());
            }
            
        }
        
        //
        // Set SSL context for the TLS version se
        //

        void CSocket::setTLSVersion(TLSVerion version) {
        
            m_tlsVersion = version;

            switch (m_tlsVersion) {
                case TLSVerion::v1_0:
                    m_sslContext.reset(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv1));
                    break;
                case TLSVerion::v1_1:
                    m_sslContext.reset(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv11));
                    break;
                case TLSVerion::v1_2:
                    m_sslContext.reset(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv12));
                    break;
            }
            
        }
        
        //
        // Work out ip address for local machine. This is quite difficult to achieve but
        // this is the best code i have seen for doing it. It just tries to connect to
        // google.com with a udp connect to get the local socket endpoint.
        // Note: Fall back of localhost on failure.
        // 

        std::string CSocket::localIPAddress() {

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

        // ============================
        // CLASS PRIVATE DATA ACCESSORS
        // ============================

        void CSocket::setSslEnabled(bool sslEnabled) {
            m_sslEnabled = sslEnabled;
        }

        bool CSocket::isSslEnabled() const {
            return m_sslEnabled;
        }

        void CSocket::setHostAddress(std::string hostAddress) {
            m_hostAddress = hostAddress;
        }

        std::string CSocket::getHostAddress() const {
            return m_hostAddress;
        }

        void CSocket::setHostPort(std::string hostPort) {
            m_hostPort = hostPort;
        }

        std::string CSocket::getHostPort() const {
            return m_hostPort;
        }


    } // namespace Network
} // namespace Antik
