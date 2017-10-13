/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CSocket.hpp
 * Author: robt
 *
 * Created on 13 October 2017, 12:16
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
#include <iostream>

//
// Boost ASIO
//

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

namespace asio = boost::asio;
namespace ip = asio::ip;

class CSocket {
public:

    //
    // Class exception
    //

    struct Exception : public std::runtime_error {

        Exception(std::string const& message)
        : std::runtime_error("CSocket Failure: " + message) {
        }

    };

    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> SSLSocket;

    CSocket() {
    }

    CSocket(const CSocket& orig) = delete;

    virtual ~CSocket() {
    }

    // Socket I/O functions

    void socketConnect(const std::string &hostAddress, const std::string &hostPort);
    size_t socketRead(char *readBuffer, size_t bufferLength);
    size_t socketWrite(const char *writeBuffer, size_t writeLength);
    void socketClose();
    void socketSwitchOnSSL();
    bool socketClosedByServer();
    void socketListenForConnection();
    void socketIsConnected();
    void socketConnectionListener();
    void socketCleanup();
    
    void setSslActive(bool sslActive);
    bool isSslActive() const;
    boost::system::error_code getSocketError() const;
    void setHostAddress(std::string hostAddress);
    std::string getHostAddress() const;
    void setHostPort(std::string hostPort);
    std::string getHostPort() const;

private:

    bool m_sslActive { false };
    
    std::string m_hostAddress; // Host ip address
    std::string m_hostPort;    // Host port address

    boost::system::error_code m_socketError; // Last socket error

    boost::asio::io_service m_ioService;                             // io Service
    boost::asio::ip::tcp::resolver m_ioQueryResolver{ m_ioService};  // io name resolver

    std::atomic<bool> m_isListenThreadRunning{ false};                    // Listen thread running flag
    std::shared_ptr<std::thread> m_dataChannelListenThread { nullptr };   // Active mode connection listen thread

    boost::asio::ssl::context sslContext{ m_ioService, boost::asio::ssl::context::tlsv12};
    SSLSocket m_socket{m_ioService, sslContext};

};

//
// Data channel socket listener thread method for incoming data 
// channel connections.
//

inline void CSocket::socketConnectionListener() {

    ip::tcp::acceptor acceptor(m_ioService, ip::tcp::endpoint(ip::tcp::v4(), 0));

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

inline void CSocket::socketCleanup() {

    if (m_isListenThreadRunning && m_dataChannelListenThread) {
        m_isListenThreadRunning = false;
        try {
            ip::tcp::socket socket{ m_ioService};
            ip::tcp::resolver::query query(m_hostAddress, m_hostPort);
            asio::connect(socket, m_ioQueryResolver.resolve(query));
        } catch (std::exception &e) {
            std::cerr << "Listener thread running when it should not be." << std::endl;
        }
        m_dataChannelListenThread->join();
    }

    socketClose();

}

//
// Listen for connections
//

inline void CSocket::socketListenForConnection() {

    m_dataChannelListenThread.reset(new std::thread(&CSocket::socketConnectionListener, this));
    while (!m_isListenThreadRunning) { // Wait for until listening before sending PORT command
        continue; // Could use conditional but use existing flag for now
    }

}

//
// Socket connected processings
//

inline void CSocket::socketIsConnected() {

    // Listener thread is running (wait for it to finish)

    if (m_dataChannelListenThread) {
        m_dataChannelListenThread->join();
    }

    // TLS handshake

    if (m_sslActive) {
        socketSwitchOnSSL();

    }

}

//
// Connect to a given host and port.
//

inline void CSocket::socketConnect(const std::string &hostAddress, const std::string &hostPort) {

    ip::tcp::resolver::query query(hostAddress, hostPort);
    m_socket.next_layer().connect(*m_ioQueryResolver.resolve(query), m_socketError);
    if (m_socketError) {
        throw Exception(m_socketError.message());
    }

}

//
// Read data from socket into io buffer
//

inline size_t CSocket::socketRead(char *readBuffer, size_t bufferLength) {

    if (m_sslActive) {
        return (m_socket.read_some(asio::buffer(readBuffer, bufferLength), m_socketError));
    } else {
        return (m_socket.next_layer().read_some(asio::buffer(readBuffer, bufferLength), m_socketError));
    }

}

//
// Write data to socket
//

inline size_t CSocket::socketWrite(const char *writeBuffer, size_t writeLength) {

    size_t bytesWritten=0;
    
    if (m_sslActive) {
        bytesWritten = m_socket.write_some(asio::buffer(writeBuffer, writeLength), m_socketError);
    } else {
        bytesWritten = m_socket.next_layer().write_some(asio::buffer(writeBuffer, writeLength), m_socketError);
    }

    if (getSocketError()) {
        throw Exception(getSocketError().message());
    }
    
    return(bytesWritten);

}

//
// Perform TLS handshake to enable SSL
//

inline void CSocket::socketSwitchOnSSL() {
    m_socket.handshake(SSLSocket::client, m_socketError);
    if (m_socketError) {
        throw Exception(m_socketError.message());
    }
    m_sslActive = true;
}

//
// Closedown any running SSL and close m_socket.
//

inline void CSocket::socketClose() {

    if (m_socket.next_layer().is_open()) {
        if (m_sslActive) {
            m_socket.shutdown(m_socketError);
            // m_sslConnectionActive=false; 
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

inline bool CSocket::socketClosedByServer() {

    if (m_socketError == asio::error::eof) {
        return (true); // Connection closed cleanly by peer.
    } else if (m_socketError) {
        throw Exception(m_socketError.message());
    }

    return (false);

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

#endif /* CSOCKET_HPP */

