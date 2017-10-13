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

#include <vector>
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

class CSocket {
    
public:
    
    CSocket() {}
    CSocket(const CSocket& orig) = delete;
    virtual ~CSocket();
    
   // Socket I/O functions

    void socketConnect(SSLSocket &socket, const std::string &hostAddress, const std::string &hostPort);
    size_t socketRead(SSLSocket &socket);
    size_t socketWrite(SSLSocket &socket, const char *writeBuffer, size_t writeLength);
    void socketClose(SSLSocket &socket);
    void socketSwitchOnSSL(SSLSocket &socket);
    bool socketClosedByServer(SSLSocket &socket);
    void socketListenForConnection(SSLSocket &socket);
    void socketIsConnected(SSLSocket &socket);
    void socketConnectionListener(SSLSocket &socket);
    void socketCleanup(SSLSocket &socket);
    
private:

    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> SSLSocket;

    boost::asio::io_service m_ioService;                            // io Service
    boost::system::error_code m_ioSocketError;                      // io last socket error
    boost::asio::ip::tcp::resolver m_ioQueryResolver{ m_ioService}; // io name resolver

    std::atomic<bool> m_isListenThreadRunning{ false }; // Listen thread running flag
    std::shared_ptr<std::thread> m_dataChannelListenThread{ nullptr}; // Active mode connection listen thread

    boost::asio::ssl::context sslContext{ m_ioService, boost::asio::ssl::context::tlsv12};

    SSLSocket m_socket{m_ioService, sslContext};

    bool m_sslConnectionActive{ false};

};

#endif /* CSOCKET_HPP */

