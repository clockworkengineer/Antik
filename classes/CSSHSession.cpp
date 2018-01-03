#include "HOST.hpp"
/*
 * File:   CSSHSession.cpp(Work In Progress)
 * 
 * Author: Robert Tizzard
 * 
 * Created on September 23, 2017, 2:33 PM
 *
 * Copyright 2017.
 *
 */

//
// Class: CSSHSession
// 
// Description:
//
// Dependencies:   C11++        - Language standard features used.
//                 libssh       - Used to talk to SSH server (https://www.libssh.org/)
//

// =================
// CLASS DEFINITIONS
// =================

#include "CSSHSession.hpp"

// ====================
// CLASS IMPLEMENTATION
// ====================

//
// C++ STL
//

#include <cstring>

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
        // Main CSSHSession object constructor. 
        //

        CSSHSession::CSSHSession() {

            m_session = ssh_new();

        }

        //
        // CSSHSession Destructor
        //

        CSSHSession::~CSSHSession() {

            if (m_session) {
                ssh_free(m_session);
                m_session = NULL;
            }

        }

        void CSSHSession::setServer(const std::string &server) {

            m_server = server;
            if (ssh_options_set(m_session, SSH_OPTIONS_HOST, m_server.c_str()) != SSH_OK) {
                throw Exception(*this, __func__);
            }

        }

        void CSSHSession::setPort(unsigned int port) {

            m_port = port;
            if (ssh_options_set(m_session, SSH_OPTIONS_PORT, &m_port) != SSH_OK) {
                throw Exception(*this, __func__);
            }

        }

        void CSSHSession::setUser(const std::string &user) {

            m_user = user;
            if (ssh_options_set(m_session, SSH_OPTIONS_USER, m_user.c_str()) != SSH_OK) {
                throw Exception(*this, __func__);
            }

        }

        void CSSHSession::setUserPassword(const std::string &password) {

            m_password = password;

        }

        void CSSHSession::connect() {

            if (ssh_connect(m_session) != SSH_OK) {
                throw Exception(*this, __func__);
            }

        }

        void CSSHSession::disconnect(bool silent) {

            if (silent) {
                ssh_silent_disconnect(m_session);
            } else {
                ssh_disconnect(m_session);
            }

        }

        int CSSHSession::userAuthorizationList() {

            return (ssh_userauth_list(m_session, NULL));

        }

        int CSSHSession::userAuthorizationNone() {

            return (ssh_userauth_none(m_session, NULL));

        }

        int CSSHSession::userAuthorizationWithPassword() {

            return (ssh_userauth_password(m_session, NULL, m_password.c_str()));

        }

        int CSSHSession::userAuthorizationWithPublicKeyAuto() {

            return (ssh_userauth_publickey_auto(m_session, NULL, NULL));

        }

        int CSSHSession::userAuthorizationWithPublicKey() {

            return (userAuthorizationWithPublicKeyAuto());

        }

        int CSSHSession::userAuthorizationWithKeyboardInteractive() {

            return (SSH_AUTH_DENIED);

        }

        int CSSHSession::isServerKnown() {

            return (ssh_is_server_known(m_session));

        }

        Antik::SSH::CSSHSession::Key CSSHSession::getPublicKey() {
            Key serverPublicKey;
            if (ssh_get_publickey(m_session, &serverPublicKey) < 0) {
                return (serverPublicKey);
            }
            return (serverPublicKey);
        }

        void CSSHSession::freeKey(Key keyToFree) {

            ssh_key_free(keyToFree);

        }

        void CSSHSession::getPublicKeyHash(Antik::SSH::CSSHSession::Key serverPublicKey, std::vector<unsigned char> &keyHash) {

            unsigned char *hash = NULL;
            size_t hlen;
            if (ssh_get_publickey_hash(serverPublicKey, SSH_PUBLICKEY_HASH_SHA1, &hash, &hlen) >= 0) {
                keyHash.assign(&hash[0], &hash[hlen]);
                ssh_clean_pubkey_hash(&hash);
            } else {
                throw Exception(*this, __func__);
            }

        }

        std::string CSSHSession::convertKeyHashToHex(std::vector<unsigned char> &keyHash) {

            char *hexa = NULL;
            std::string convertedHash;
            hexa = ssh_get_hexa(&keyHash[0], keyHash.size());
            if (hexa >= 0) {
                convertedHash.assign(&hexa[0], &hexa[std::strlen(hexa)]);
                free(hexa);
            }
            return (convertedHash);

        }

        void CSSHSession::writeKnownHost() {

            if (ssh_write_knownhost(m_session) != SSH_OK) {
                throw Exception(*this, __func__);
            }

        }

        std::string CSSHSession::getBanner() const {

            std::string sessionBanner;
            char *banner = ssh_get_issue_banner(m_session);
            if (banner) {
                sessionBanner.assign(&banner[0], &banner[std::strlen(banner)]);
                free(banner);
            }
            return (sessionBanner);

        }

        std::string CSSHSession::getDisconnectMessage() const {

            std::string disconnectMessage;
            const char *message = ssh_get_disconnect_message(m_session);
            if (message) {
                disconnectMessage.assign(&message[0], &message[std::strlen(message)]);
            } else {
                disconnectMessage = getError();
            }
            return (disconnectMessage);

        }

        int CSSHSession::getSSHVersion() const {

            return (ssh_get_version(m_session));

        }

        int CSSHSession::getStatus() const {

            return (ssh_get_status(m_session));

        }

        bool CSSHSession::isConnected() const {

            return (ssh_is_connected(m_session));

        }

        std::string CSSHSession::getError() const {

            return (ssh_get_error(m_session));

        }

        int CSSHSession::getErrorCode() const {

            return (ssh_get_error_code(m_session));

        }

        ssh_session CSSHSession::getSession() const {

            return m_session;

        }

    } // namespace CSSH
} // namespace Antik
