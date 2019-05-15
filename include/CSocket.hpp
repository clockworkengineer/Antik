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
// Antik classes
//

#include "CommonAntik.hpp"

//
// Boost ASIO
//

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

namespace Antik::Network
{

// ==========================
// PUBLIC TYPES AND CONSTANTS
// ==========================

// ================
// CLASS DEFINITION
// ================

class CSocket
{
public:
    // ==========================
    // PUBLIC TYPES AND CONSTANTS
    // ==========================

    //
    // Class exception
    //

    struct Exception : public std::runtime_error
    {

        Exception(std::string const &message)
            : std::runtime_error("CSocket Failure: " + message)
        {
        }
    };

    //
    // TLS versions
    //

    enum TLSVerion
    {
        v1_0 = 0,
        v1_1,
        v1_2
    };

    // ============
    // CONSTRUCTORS
    // ============

    //
    // Main constructor
    //

    CSocket()
    {
        // Default SSL context use TLS v1.2
        m_sslContext.reset(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv12));
    }

    // ==========
    // DESTRUCTOR
    // ==========

    virtual ~CSocket()
    {
    }

    // ==============
    // PUBLIC METHODS
    // ==============

    // Determine local machines IP address

    static std::string localIPAddress();

    // Set TLS version to use

    void setTLSVersion(TLSVerion version);

    // Socket IO methods connect, read/write and close

    void connect();
    size_t read(char *readBuffer, size_t bufferLength);
    size_t write(const char *writeBuffer, size_t writeLength);
    void close();

    // Socket TLS handshake

    void tlsHandshake();

    // Socket closed by remote peer

    bool closedByRemotePeer();

    // Listen and wait for remote connections

    void listenForConnection();
    void waitUntilConnected();

    // Socket cleanup

    void cleanup();

    // Private data accessors

    void setSslEnabled(bool sslActive);
    bool isSslEnabled() const;
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

    CSocket(const CSocket &orig) = delete;
    CSocket(const CSocket &&orig) = delete;
    CSocket &operator=(CSocket other) = delete;

    // ===============
    // PRIVATE METHODS
    // ===============

    // Listen on a thread for a connection

    void connectionListener();

    // =================
    // PRIVATE VARIABLES
    // =================

    bool m_sslActive{false};  // == true SSL currently active
    bool m_sslEnabled{false}; // == true SSL enabled

    TLSVerion m_tlsVersion{TLSVerion::v1_2}; // SSL sockets TLS version

    std::string m_hostAddress; // Host ip address
    std::string m_hostPort;    // Host port address

    boost::system::error_code m_socketError; // Last socket error

    boost::asio::io_service m_ioService;                           // io Service
    boost::asio::ip::tcp::resolver m_ioQueryResolver{m_ioService}; // io name resolver

    std::atomic<bool> m_isListenThreadRunning{false};           // Listen thread running flag
    std::unique_ptr<std::thread> m_socketListenThread{nullptr}; // Connection listen thread

    std::unique_ptr<boost::asio::ssl::context> m_sslContext{nullptr}; // SSL context (initialised in constructor).
    std::unique_ptr<SSLSocket> m_socket{nullptr};                     // SSL socket allocated at run time

    std::exception_ptr m_thrownException{nullptr}; // Pointer to any exception thrown in connectionListener
};

//
// Return true if socket closed by server otherwise false.
//

inline bool CSocket::closedByRemotePeer()
{

    return (m_socketError == boost::asio::error::eof);
}

} // namespace Antik::Network

#endif /* CSOCKET_HPP */
