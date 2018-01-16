#include "HOST.hpp"
/*
 * File:   CSSHChannel.cpp(Work In Progress)
 * 
 * Author: Robert Tizzard
 * 
 * Created on September 23, 2017, 2:33 PM
 *
 * Copyright 2017.
 *
 */

//
// Class: CSSHChannel
// 
// Description:
//
// Dependencies:   
//
// C11++        - Language standard features used.
// libssh       - Used to talk to SSH server (https://www.libssh.org/) (0.6.3)
//

// =================
// CLASS DEFINITIONS
// =================

#include "CSSHChannel.hpp"

// ====================
// CLASS IMPLEMENTATION
// ====================

//
// C++ STL
//

// =======
// IMPORTS
// =======

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace SSH {

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

        // ==============
        // PUBLIC METHODS
        // ==============

        //
        // Main CSSHChannel object constructor. 
        //

        CSSHChannel::CSSHChannel(CSSHSession &session) : m_session{session}
        {
            m_channel = ssh_channel_new(m_session.getSession());
            assert(m_channel != NULL);

        }
        
        CSSHChannel::CSSHChannel(CSSHSession &session, ssh_channel channel) : m_session{session}, m_channel {channel}
        {
           
            assert(m_channel != NULL);

        }

        //
        // CSSHChannel Destructor
        //

        CSSHChannel::~CSSHChannel() {

        }

        void CSSHChannel::open() {

            if (ssh_channel_open_session(m_channel) != SSH_OK) {
                throw Exception(*this, __func__);
            }

        }

        void CSSHChannel::close() {
            if (m_channel) {
                ssh_channel_close(m_channel);
                ssh_channel_free(m_channel);
                m_channel = NULL;
            }
            m_ioBuffer.reset();
        }

        void CSSHChannel::sendEndOfFile() {
            ssh_channel_send_eof(m_channel);
        }

        void CSSHChannel::execute(const std::string &commandToRun) {
            if (ssh_channel_request_exec(m_channel, commandToRun.c_str()) != SSH_OK) {
                throw Exception(*this, __func__);
            }
        }

        int CSSHChannel::read(void *buffer, uint32_t bytesToRead, bool isStdErr) {
            uint32_t bytesRead = ssh_channel_read(m_channel, buffer, bytesToRead, isStdErr);
            if (static_cast<int>(bytesRead) == SSH_ERROR) {
                throw Exception(*this, __func__);
            }
            return (bytesRead);
        }

        int CSSHChannel::readNonBlocking(void *buffer, uint32_t bytesToRead, bool isStdErr) {
            uint32_t bytesRead = ssh_channel_read_nonblocking(m_channel, buffer, bytesToRead, isStdErr);
            if (static_cast<int>(bytesRead) == SSH_ERROR) {
                throw Exception(*this, __func__);
            }
            return (bytesRead);
        }

        int CSSHChannel::write(void *buffer, uint32_t bytesToWrite) {
            uint32_t bytesWritten = ssh_channel_write(m_channel, buffer, bytesToWrite);
            if (static_cast<int>(bytesWritten) == SSH_ERROR) {
                throw Exception(*this, __func__);
            }
            return (bytesWritten);
        }

        void CSSHChannel::requestTerminal() {
            int returnCode = ssh_channel_request_pty(m_channel);
            if (returnCode == SSH_ERROR) {
                throw Exception(*this, __func__);
            }
        }

        void CSSHChannel::requestTerminalSize(int columns, int rows) {
            int returnCode = ssh_channel_change_pty_size(m_channel, columns, rows);
            if (returnCode == SSH_ERROR) {
                throw Exception(*this, __func__);
            }
        }

        void CSSHChannel::requestShell() {
            int returnCode = ssh_channel_request_shell(m_channel);
            if (returnCode == SSH_ERROR) {
                throw Exception(*this, __func__);
            }
        }

        bool CSSHChannel::isOpen() {
            return (ssh_channel_is_open(m_channel));
        }

        bool CSSHChannel::isClosed() {
            return (ssh_channel_is_closed(m_channel));
        }

        bool CSSHChannel::isEndOfFile() {
            return (ssh_channel_is_eof(m_channel));
        }

        int CSSHChannel::getExitStatus() {
            return (ssh_channel_get_exit_status(m_channel));
        }

        void CSSHChannel::setEnvironmentVariable(const std::string &variable, const std::string &value) {
            int returnCode = ssh_channel_request_env(m_channel, variable.c_str(), value.c_str());
            if (returnCode == SSH_ERROR) {
                throw Exception(*this, __func__);
            }
        }

        void CSSHChannel::openForward(const std::string &remoteHost, int remotePort, const std::string &localHost, int localPort) {

            int returnCode = ssh_channel_open_forward(m_channel, remoteHost.c_str(), remotePort, localHost.c_str(), localPort);
            if (returnCode == SSH_ERROR) {
                throw Exception(*this, __func__);
            }

        }

        void CSSHChannel::listenForward(CSSHSession &session, const std::string &address, int port, int *boundPort) {
            int returnCode = ssh_forward_listen(session.getSession(), address.c_str(), port, boundPort);
            if (returnCode == SSH_ERROR) {
                throw CSSHSession::Exception(session, __func__);
            }
         
        }

        void CSSHChannel::cancelForward(CSSHSession &session, const std::string &address, int port) {
            int returnCode = ssh_forward_cancel(session.getSession(), address.c_str(), port);
            if (returnCode == SSH_ERROR) {
                  throw CSSHSession::Exception(session, __func__);
            }

        }

        std::unique_ptr<CSSHChannel> CSSHChannel::acceptForward(CSSHSession &session, int timeout, int *port) {

            ssh_channel forwardChannel = ssh_channel_accept_forward(session.getSession(), timeout, port);
            if (forwardChannel == NULL) {
                throw CSSHSession::Exception(session, __func__);
            }

            std::unique_ptr<CSSHChannel> returnChannel;

            if (forwardChannel) {
                returnChannel.reset(new CSSHChannel(session, forwardChannel));
            }

            return (returnChannel);

        }
             
        //
        // Set/Get IO buffer parameters.
        //

        std::shared_ptr<char> CSSHChannel::getIoBuffer() {
            if (!m_ioBuffer) {
                setIoBufferSize(m_ioBufferSize);
            }
            return m_ioBuffer;
        }

        void CSSHChannel::setIoBufferSize(std::uint32_t ioBufferSize) {
            m_ioBufferSize = ioBufferSize;
            m_ioBuffer.reset(new char[m_ioBufferSize]);
        }

        std::uint32_t CSSHChannel::getIoBufferSize() const {
            return m_ioBufferSize;
        }

        CSSHSession& CSSHChannel::getSession() const {
            return m_session;
        }

        ssh_channel CSSHChannel::getChannel() const {
            return m_channel;
        }


    } // namespace SSH
} // namespace Antik
