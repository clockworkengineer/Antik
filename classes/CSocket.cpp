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
            
            boost::asio::ip::tcp::acceptor acceptor(m_ioService, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0));
            
            m_hostPort = std::to_string(acceptor.local_endpoint().port());

            m_isListenThreadRunning = true;
            
            std::unique_ptr<SSLSocket> socket { new SSLSocket(m_ioService, *m_sslContext) };
            if (!socket) {
                m_isListenThreadRunning = false;
                throw Exception("Could not create socket.");
            }

            acceptor.accept(socket->next_layer(), m_socketError);
            if (m_socketError) {
                m_isListenThreadRunning = false;
                throw Exception(m_socketError.message());
            }
            
            m_socket = std::move(socket);

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

            if (m_isListenThreadRunning && m_socketListenThread) {
                m_isListenThreadRunning = false;
                try {
                    boost::asio::ip::tcp::socket socket{ m_ioService};
                    boost::asio::ip::tcp::resolver::query query(m_hostAddress, m_hostPort);
                    boost::asio::connect(socket, m_ioQueryResolver.resolve(query));
                    socket.close();
                } catch (std::exception &e) {
                    throw Exception("Could not wake listener thread with fake connect.");
                }
                m_socketListenThread->join();
            }

            close();

        }

        //
        // Listen for connections
        //

        void CSocket::listenForConnection() {

            // Start connection listener thread and wait until its listening
            
            m_socketListenThread.reset(new std::thread(&CSocket::connectionListener, this));
            while (!m_isListenThreadRunning) {
                continue; 
            }

        }

        //
        // Wait until a socket is connected.
        //

        void CSocket::waitUntilConnected() {

            // Listener thread is running (wait for it to finish)

            if (m_socketListenThread) {
                m_socketListenThread->join();
            }

            // TLS handshake if SSL enabled

            tlsHandshake();

        }

        //
        // Connect to a given host and port.The connecting socket is created
        // local and moved to m_socket on success.
        //

        void CSocket::connect() {
     
            std::unique_ptr<SSLSocket> socket { new SSLSocket(m_ioService, *m_sslContext) };
            if (!socket) {
                throw Exception("Could not create socket.");
            }
            
            boost::asio::ip::tcp::resolver::query query { m_hostAddress, m_hostPort };        
            socket->next_layer().connect(*m_ioQueryResolver.resolve(query), m_socketError);
            if (m_socketError) {
                throw Exception(m_socketError.message());
            }
            
            m_socket = std::move(socket);

        }

        //
        // Read data from socket into buffer
        //

        size_t CSocket::read(char *readBuffer, size_t bufferLength) {

            size_t bytesRead { 0 };
            
            // No socket present

            if (!m_socket) {
                throw Exception("No socket present.");
            }

            // Read data
            
            if (m_sslActive) {
                bytesRead = m_socket->read_some(boost::asio::buffer(readBuffer, bufferLength), m_socketError);
            } else {
                bytesRead = m_socket->next_layer().read_some(boost::asio::buffer(readBuffer, bufferLength), m_socketError);
            }

            // Signal any non end of file  error
            
            if (m_socketError && m_socketError != boost::asio::error::eof) {
                throw Exception(m_socketError.message());
            }
            
            return(bytesRead);
            
        }

        //
        // Write data to socket
        //

        size_t CSocket::write(const char *writeBuffer, size_t writeLength) {

            size_t bytesWritten  { 0 };

            // No socket present

            if (!m_socket) {
                throw Exception("No socket present.");
            }

            // Write data
            
            if (m_sslActive) {
                bytesWritten = m_socket->write_some(boost::asio::buffer(writeBuffer, writeLength), m_socketError);
            } else {
                bytesWritten = m_socket->next_layer().write_some(boost::asio::buffer(writeBuffer, writeLength), m_socketError);
            }

            // Signal any non end of file error
            
            if (m_socketError && m_socketError != boost::asio::error::eof) {
                throw Exception(m_socketError.message());
            }

            return (bytesWritten);

        }

        //
        // Perform TLS handshake if SSL enabled
        //

        void CSocket::tlsHandshake() {
            
            // If SSL not enabled return

            if (!m_sslEnabled) {
                return;
            }
                       
            // No socket present

            if (!m_socket) {
                throw Exception("No socket present.");
            }

            m_socket->handshake(SSLSocket::client, m_socketError);
            if (m_socketError) {
                throw Exception(m_socketError.message());
            }
            
            m_sslActive = true;
        
        }

        //
        // Closedown any running SSL and close socket. Move m_socket to local
        // socket and close it down.
        //

        void CSocket::close() {

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
                    throw Exception(m_socketError.message());
                }

            }

            // Remove any listen thread
            
            if (m_socketListenThread) {
                m_socketListenThread.reset();
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
