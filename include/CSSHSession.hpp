#ifndef CSSHSESSION_HPP
#define CSSHSESSION_HPP
//
// C++ STL
//
#include <stdexcept>
#include <vector>
#include <memory>
#include <cassert>
#include <mutex>
//
// Antik classes
//
#include "CommonAntik.hpp"
//
// Libssh
//
#include <libssh/libssh.h>
#include <libssh/callbacks.h> // Threading intialisation
// =========
// NAMESPACE
// =========
namespace Antik::SSH
{
    // ==========================
    // PUBLIC TYPES AND CONSTANTS
    // ==========================
    enum UserAuthorizationType
    {
        None = 0x1,
        Password = 0x2,
        PublicKey = 0x4,
        Interactice = 0x8
    };
    // ================
    // CLASS DEFINITION
    // ================
    class CSSHSession
    {
    public:
        // ==========================
        // PUBLIC TYPES AND CONSTANTS
        // ==========================
        //
        // Class exception
        //
        struct Exception
        {
            Exception(CSSHSession &session, const std::string &functionName) : m_errorCode{session.getErrorCode()},
                                                                               m_errorMessage{session.getError()}, m_functionName{functionName}
            {
            }
            Exception(const std::string &errorMessage, const std::string &functionName) : m_errorMessage{errorMessage},
                                                                                          m_functionName{functionName}
            {
            }
            int getCode() const
            {
                return m_errorCode;
            }
            std::string getMessage() const
            {
                return static_cast<std::string>("CSSHSession Failure: (") + m_functionName + ") [" + m_errorMessage + "]";
            }

        private:
            int m_errorCode{SSH_OK};    // SSH error code
            std::string m_errorMessage; // SSH error message
            std::string m_functionName; // Current function name
        };
        //
        // Custom deleter for re-mapped libssh session data structures.
        //
        struct Deleter
        {
            void operator()(ssh_key key) const
            {
                ssh_key_free(key);
            }
        };
        using Option = ssh_options_e;
        //
        // Encapsulate libssh session data in unique pointers.
        //
        using Key = std::unique_ptr<std::pointer_traits<ssh_key>::element_type, Deleter>;
        // ============
        // CONSTRUCTORS
        // ============
        //
        // Main constructors
        //
        explicit CSSHSession();
        CSSHSession(CSSHSession &session);
        // ==========
        // DESTRUCTOR
        // ==========
        virtual ~CSSHSession();
        // ==============
        // PUBLIC METHODS
        // ==============
        //
        // Set session details
        //
        void setServer(const std::string &server);
        void setPort(unsigned int port);
        void setUser(const std::string &user);
        void setUserPassword(const std::string &password);
        //
        // Connect/disconnect sessions
        //
        void connect();
        void disconnect(bool silent = false);
        //
        // User authorization functions
        //
        int userAuthorizationList();
        //
        // Overridable user authorization functions
        //
        virtual int userAuthorizationNone();
        virtual int userAuthorizationWithPublicKeyAuto();
        virtual int userAuthorizationWithPassword();
        virtual int userAuthorizationWithPublicKey();
        virtual int userAuthorizationWithKeyboardInteractive();
        //
        // Verify server
        //
        int isServerKnown();
        //
        // Write server details away to local config
        //
        void writeKnownHost();
        //
        // Get names of session cipher/HMAC in/out methods, key exchange algorithm
        //
        std::string getCipherIn();
        std::string getCipherOut();
        std::string getHMACIn();
        std::string getHMACOut();
        std::string getKeyExchangeAlgorithm();
        //
        // Public key methods
        //
        Key getPublicKey();
        void getPublicKeyHash(Key &serverPublicKey, std::vector<unsigned char> &keyHash);
        std::string convertKeyHashToHex(std::vector<unsigned char> &keyHash);
        //
        // Get various banners and disconnect message
        //
        std::string getBanner() const;
        std::string getClientBanner() const;
        std::string getServerBanner() const;
        std::string getDisconnectMessage() const;
        //
        // Get SSH/OpenSSH versions
        //
        int getSSHVersion() const;
        int getOpenSSHVersion() const;
        //
        // Session status methods
        //
        int getStatus() const;
        bool isConnected() const;
        bool isAuthorized() const;
        //
        // Get/Set option values for a session and also copy options,
        //
        void setOption(Option sessionOption, const void *optionValue);
        void getOption(Option sessionOption, std::string &optionValue);
        void copyOptions(CSSHSession &source);
        //
        // Get SSH error code and message
        //
        std::string getError() const;
        int getErrorCode() const;
        //
        // Get libssh session structure
        //
        ssh_session getSession() const;
        //
        // Get session user authorization type
        //
        std::uint32_t getAuthorizarionType() const;
        //
        // Set logging verbosity
        //
        void setLogging(int logging);
        // ================
        // PUBLIC VARIABLES
        // ================
    private:
        // ===========================
        // PRIVATE TYPES AND CONSTANTS
        // ===========================
        // ===========================================
        // DISABLED CONSTRUCTORS/DESTRUCTORS/OPERATORS
        // ===========================================
        CSSHSession(const CSSHSession &orig) = delete;
        CSSHSession(const CSSHSession &&orig) = delete;
        CSSHSession &operator=(CSSHSession other) = delete;
        // ===============
        // PRIVATE METHODS
        // ===============
        //
        // Initialisation
        //
        static void initialise();
        // =================
        // PRIVATE VARIABLES
        // =================
        ssh_session m_session;                                          // libssh session
        int m_logging{SSH_LOG_NOLOG};                                   // libssh logging
        std::string m_server;                                           // SSH server name
        unsigned int m_port{22};                                        // SSH server port
        std::string m_user;                                             // SSH server login account name
        std::string m_password;                                         // SSH server login account password
        bool m_authorized{false};                                       // SSH session authorised
        std::uint32_t m_authorizarionType{UserAuthorizationType::None}; // SSH session user authorization type
    };
} // namespace Antik::SSH
#endif /* CSSHSESSION_HPP */
