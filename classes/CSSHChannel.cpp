#include "HOST.hpp"
/*
 * File:   CSSHChannel.cpp
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
// Description:   A class for the creation of channels over an SSH session for the
// transport of data/commands to and from a remote host. The protocol used on the channel
// can be any standard internet protocol from IMAP/HTTP etc or a custom designed one.
// It also tries to hide as much of its implementation using libssh as possible and use/return 
// C11++ data structures/ exceptions. It is not complete by any means but may be updated to 
// future to use more libssh features.
//
// Dependencies: 
//
// C11++        - Language standard features used.
// libssh       - Used to talk to SSH server (https://www.libssh.org/) (0.7.5)
//

// =================
// CLASS DEFINITIONS
// =================

#include "CSSHChannel.hpp"

// ====================
// CLASS IMPLEMENTATION
// ====================

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
        // Main CSSHChannel object constructors. The passed in session has to be
        // connected and authorized for a channel  to be created.
        //

        CSSHChannel::CSSHChannel(CSSHSession &session) : m_session{session}
        {

            assert(session.isConnected() && session.isAuthorized());

            if ((m_channel = ssh_channel_new(m_session.getSession())) == NULL) {
                throw Exception("Could not allocate new channel.", __func__);
            }

        }

        CSSHChannel::CSSHChannel(CSSHSession &session, ssh_channel channel) : m_session{session}, m_channel{channel}
        {

            assert(session.isConnected() && session.isAuthorized());

            if ((m_channel = ssh_channel_new(m_session.getSession())) == NULL) {
                throw Exception("Could not allocate new channel.", __func__);
            }

        }

        //
        // CSSHChannel Destructor
        //

        CSSHChannel::~CSSHChannel() {

        }

        //
        // Open a channel for reading/writing of data.
        //

        void CSSHChannel::open() {

            if (ssh_channel_open_session(m_channel) != SSH_OK) {
                throw Exception(*this, __func__);
            }

        }

        //
        // Close an open channel and free its resources.
        //

        void CSSHChannel::close() {

            if (m_channel) {
                ssh_channel_close(m_channel);
                ssh_channel_free(m_channel);
                m_channel = NULL;
            }

            m_ioBuffer.reset();

        }

        //
        // Send end of file on channel to remote host.
        //

        void CSSHChannel::sendEndOfFile() {

            ssh_channel_send_eof(m_channel);

        }

        //
        // Execute a shell command on the remote host associated with a channel.
        //

        void CSSHChannel::execute(const std::string &commandToRun) {

            if (ssh_channel_request_exec(m_channel, commandToRun.c_str()) != SSH_OK) {
                throw Exception(*this, __func__);
            }

        }

        //
        // Read data from a channel. isStdErr== true then data comes from stderr.
        //

        int CSSHChannel::read(void *buffer, uint32_t bytesToRead, bool isStdErr) {

            uint32_t bytesRead = ssh_channel_read(m_channel, buffer, bytesToRead, isStdErr);

            if (static_cast<int> (bytesRead) == SSH_ERROR) {
                throw Exception(*this, __func__);
            }

            return (bytesRead);

        }


        //
        // Read data from a channel (non-blocking). isStdErr== true then data comes from stderr.
        //

        int CSSHChannel::readNonBlocking(void *buffer, uint32_t bytesToRead, bool isStdErr) {
 
            uint32_t bytesRead = ssh_channel_read_nonblocking(m_channel, buffer, bytesToRead, isStdErr);

            if (static_cast<int> (bytesRead) == SSH_ERROR) {
                throw Exception(*this, __func__);
            }

            return (bytesRead);

        }

        //
        // Write data to a channel.
        //

        int CSSHChannel::write(void *buffer, uint32_t bytesToWrite) {
 
            uint32_t bytesWritten = ssh_channel_write(m_channel, buffer, bytesToWrite);

            if (static_cast<int> (bytesWritten) == SSH_ERROR) {
                throw Exception(*this, __func__);
            }

            return (bytesWritten);
        }

        //
        // Request a PTY (pseudoterminal) of given type and size is attached to channel.
        //

        void CSSHChannel::requestTerminalOfTypeSize(const std::string &termialType, int columns, int rows) {

            int returnCode = ssh_channel_request_pty_size(m_channel, termialType.c_str(), columns, rows);

            if (returnCode == SSH_ERROR) {
                throw Exception(*this, __func__);
            }

        }

        //
        // Request a PTY (pseudoterminal) is attached to channel.
        //

        void CSSHChannel::requestTerminal() {

            int returnCode = ssh_channel_request_pty(m_channel);

            if (returnCode == SSH_ERROR) {
                throw Exception(*this, __func__);
            }

        }

        //
        // Set PTY (pseudoterminal) terminal size.
        //

        void CSSHChannel::changeTerminalSize(int columns, int rows) {

            int returnCode = ssh_channel_change_pty_size(m_channel, columns, rows);

            if (returnCode == SSH_ERROR) {
                throw Exception(*this, __func__);
            }

        }

        //
        // Request that a remote shell is attache to a channel.
        //

        void CSSHChannel::requestShell() {

            int returnCode = ssh_channel_request_shell(m_channel);

            if (returnCode == SSH_ERROR) {
                throw Exception(*this, __func__);
            }

        }

        //
        // Return true if a channel is open.
        //

        bool CSSHChannel::isOpen() {

            return (ssh_channel_is_open(m_channel));

        }

        //
        // Return true if a channel is closed.
        //

        bool CSSHChannel::isClosed() {

            return (ssh_channel_is_closed(m_channel));

        }

        //
        // Return true if end of file has been sent by remote host on channel.
        //

        bool CSSHChannel::isEndOfFile() {

            return (ssh_channel_is_eof(m_channel));

        }

        //
        // Return the exit status of a channel.
        //

        int CSSHChannel::getExitStatus() {

            return (ssh_channel_get_exit_status(m_channel));

        }

        //
        // Set an environment variable for remote shell attached to channel.
        //

        void CSSHChannel::setEnvironmentVariable(const std::string &variable, const std::string &value) {

            int returnCode = ssh_channel_request_env(m_channel, variable.c_str(), value.c_str());

            if (returnCode == SSH_ERROR) {
                throw Exception(*this, __func__);
            }

        }

        //
        // Open a direct forwarding channel on the remote host.
        //

        void CSSHChannel::openForward(const std::string &remoteHost, int remotePort, const std::string &localHost, int localPort) {

            int returnCode = ssh_channel_open_forward(m_channel, remoteHost.c_str(), remotePort, localHost.c_str(), localPort);

            if (returnCode == SSH_ERROR) {
                throw Exception(*this, __func__);
            }

        }

        //
        // Setup a reverse forwarding channel from remote host.
        //

        void CSSHChannel::listenForward(CSSHSession &session, const std::string &address, int port, int *boundPort) {

            int returnCode = ssh_channel_listen_forward(session.getSession(), address.c_str(), port, boundPort);

            if (returnCode == SSH_ERROR) {
                throw CSSHSession::Exception(session, __func__);
            }

        }

        //
        // Cancel a reverse forwarding channel from remote host.
        //

        void CSSHChannel::cancelForward(CSSHSession &session, const std::string &address, int port) {
                   
            int returnCode = ssh_channel_cancel_forward(session.getSession(), address.c_str(), port);

            if (returnCode == SSH_ERROR) {
                throw CSSHSession::Exception(session, __func__);
            }

        }

        //
        // Wait for a reverse forwarding channel from remote host (with timeout).
        //

        std::unique_ptr<CSSHChannel> CSSHChannel::acceptForward(CSSHSession &session, int timeout, int *port) {
                   
            ssh_channel forwardChannel = ssh_channel_accept_forward(session.getSession(), timeout, port);

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

        //
        // Return CSSHSession reference associated with channel.
        //

        CSSHSession& CSSHChannel::getSession() const {

            return m_session;

        }

        //
        // Return internal libssh channel reference,
        //

        ssh_channel CSSHChannel::getChannel() const {

            return m_channel;

        }


    } // namespace SSH
} // namespace Antik
