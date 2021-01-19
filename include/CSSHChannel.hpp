#ifndef CSSHCHANNEL_HPP
#define CSSHCHANNEL_HPP
//
// C++ STL
//
#include <stdexcept>
#include <cassert>
//
// Antik classes
//
#include "CommonAntik.hpp"
#include "CSSHSession.hpp"
// =========
// NAMESPACE
// =========
namespace Antik::SSH
{
    class CSSHSession;
    // ==========================
    // PUBLIC TYPES AND CONSTANTS
    // ==========================
    // ================
    // CLASS DEFINITION
    // ================
    class CSSHChannel
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
            Exception(CSSHChannel &channel, const std::string &functionName) : m_errorCode{channel.getSession().getErrorCode()},
                                                                               m_errorMessage{channel.getSession().getError()}, m_functionName{functionName}
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
                return static_cast<std::string>("CSSHChannel Failure: (") + m_functionName + ") [" + m_errorMessage + "]";
            }

        private:
            int m_errorCode{SSH_OK};    // SSH error code
            std::string m_errorMessage; // SSH error message
            std::string m_functionName; // Current function name
        };
        // ============
        // CONSTRUCTORS
        // ============
        //
        // Main constructor
        //
        explicit CSSHChannel(CSSHSession &session);
        // ==========
        // DESTRUCTOR
        // ==========
        virtual ~CSSHChannel();
        // ==============
        // PUBLIC METHODS
        // ==============
        //
        // Channel I/O
        //
        void open();
        void close();
        void sendEndOfFile();
        int read(void *buffer, uint32_t bytesToRead, bool isStdErr = false);
        int readNonBlocking(void *buffer, uint32_t bytesToRead, bool isStdErr = false);
        int write(void *buffer, uint32_t bytesToWrite);
        //
        // Terminal and shell.
        //
        void requestTerminalOfTypeSize(const std::string &termialType, int columns, int rows);
        void requestTerminal();
        void changeTerminalSize(int columns, int rows);
        void requestShell();
        void execute(const std::string &commandToRun);
        void setEnvironmentVariable(const std::string &variable, const std::string &value);
        //
        // Channel status.
        //
        bool isOpen();
        bool isClosed();
        bool isEndOfFile();
        int getExitStatus();
        //
        // Channel direct and reverse forwarding. Three of these functions are static as they need a session parameter
        // and not a channel (the libssh functions seemed to be grouped under channel rather than session so try to
        // keep consistent).
        //
        void openForward(const std::string &remoteHost, int remotePort, const std::string &localHost, int localPort);
        static void listenForward(CSSHSession &session, const std::string &address, int port, int *boundPort);
        static void cancelForward(CSSHSession &session, const std::string &address, int port);
        static std::unique_ptr<CSSHChannel> acceptForward(CSSHSession &session, int timeout, int *port);
        //
        // Set IO buffer parameters.
        //
        std::shared_ptr<char[]> getIoBuffer();
        void setIoBufferSize(std::uint32_t ioBufferSize);
        std::uint32_t getIoBufferSize() const;
        //
        // Get CSSHSession for channel and libssh channel structure
        //
        CSSHSession &getSession() const;
        ssh_channel getChannel() const;
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
        CSSHChannel() = delete;
        CSSHChannel(const CSSHChannel &orig) = delete;
        CSSHChannel(const CSSHChannel &&orig) = delete;
        CSSHChannel &operator=(CSSHChannel other) = delete;
        // ===============
        // PRIVATE METHODS
        // ===============
        //
        // Private constructor (used to return channel from acceptForward)
        //
        explicit CSSHChannel(CSSHSession &session, ssh_channel channel);
        // =================
        // PRIVATE VARIABLES
        // =================
        CSSHSession &m_session;                      // Channel session
        ssh_channel m_channel{NULL};                 // libssh channel structure.
        std::shared_ptr<char[]> m_ioBuffer{nullptr}; // IO buffer
        std::uint32_t m_ioBufferSize{32 * 1024};     // IO buffer size
    };
} // namespace Antik::SSH
#endif /* CSSHCHANNEL_HPP */
