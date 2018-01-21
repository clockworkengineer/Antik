#include "HOST.hpp"
/*
 * File:   SSHChannelUtil.cpp(Work In Progress)
 * 
 * Author: Robert Tizzard
 *
 * Created on October 10, 2017, 2:34 PM
 * 
 * Copyright 2017.
 * 
 */

//
// Module: SSHChannelUtil
//
// Description: SSH CHannel utility functions for the Antik class SSHChannel.
// 
// Dependencies: 
// 
// C11++              : Use of C11++ features.
// Antik classes      : CSSHChannel
//

// =============
// INCLUDE FILES
// =============

//
// C++ STL
//

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <system_error>

// POSIX terminal control definitions

#include <termios.h> 

//
// SSH Channel utility definitions
//

#include "SSHChannelUtil.hpp"

// =========
// NAMESPACE
// =========

namespace Antik {
    namespace SSH {

        // =======
        // IMPORTS
        // =======

        using namespace std;

        // ===============
        // LOCAL FUNCTIONS
        // ===============
        
        //
        // Function run on a separate thread and used to read characters that are sent down a SSH chanel with an associated shell.
        //
        
        static void readShellInput(CSSHChannel &channel, std::atomic<bool> &stopShellInput, std::exception_ptr &thrownException) {

            try {

                struct termios terminalSettings, savedTerminalSettings;
                std::vector<char> ioBuffer;

                if (tcgetattr(0, &terminalSettings) == -1) {
                    throw system_error(errno, system_category(), __func__);
                }

                savedTerminalSettings = terminalSettings;

                cfmakeraw(&terminalSettings);
                terminalSettings.c_cc[VMIN] = 0;
                terminalSettings.c_cc[VTIME] = 0;

                if (tcsetattr(0, TCSANOW, &terminalSettings) == -1) {
                    throw system_error(errno, system_category(), __func__);
                }

                while (!stopShellInput) {

                    for (char singleChar; (singleChar = std::getchar()) != EOF; ioBuffer.push_back(singleChar));

                    if (!ioBuffer.empty()) {
                        channel.write(&ioBuffer[0], ioBuffer.size());
                        ioBuffer.clear();
                    } else {
                        std::this_thread::sleep_for(std::chrono::microseconds(5));
                    }

                }

                if (tcsetattr(0, TCSANOW, &savedTerminalSettings) == -1) {
                    throw system_error(errno, system_category(), __func__);
                }

            } catch (...) {
                thrownException = std::current_exception();
            }

        }
        
        //
        // Function run on a separate thread that reads data from a direct forwarded SSH channel and passed to 
        // write callback function. When the channel is closed the thread terminates.
        //

        static void readChannelThread(CSSHChannel &forwardingChannel, WriteCallBackFn writeFn) {

            uint32_t bytesRead;

            while (forwardingChannel.isOpen() && !forwardingChannel.isEndOfFile()) {
                while ((bytesRead = forwardingChannel.readNonBlocking(forwardingChannel.getIoBuffer().get(), forwardingChannel.getIoBufferSize(), false)) > 0) {
                    writeFn(forwardingChannel.getIoBuffer().get(), bytesRead);
                }
                std::this_thread::sleep_for(std::chrono::microseconds(5));
            }

        }
        
        // ================
        // PUBLIC FUNCTIONS
        // ================

        //
        // Default write callback functions.
        //
        
        void defaultCOUT(void*ioBuffer, uint32_t ioBufferSize) {
            std::cout.write(static_cast<char *> (ioBuffer), ioBufferSize);
            std::cout.flush();
        };

        void defaultCERR (void *ioBuffer, uint32_t ioBufferSize) {
            std::cerr.write(static_cast<char *> (ioBuffer), ioBufferSize);
        }
        
        //
        // Create an interactive shell on a channel, send commands and receive output back.
        //
        
        void interactiveShell(CSSHChannel &channel, int columns, int rows, WriteCallBackFn writeFn) {

            int bytesRead;
            char *ioBuffer = channel.getIoBuffer().get();
            uint32_t ioBufferSize = channel.getIoBufferSize();
            std::atomic<bool> stopShellInput{ false};
            std::exception_ptr thrownException{nullptr};

            channel.requestTerminal();
            channel.requestTerminalSize(columns, rows);
            channel.requestShell();

            std::thread shellInputThread{ readShellInput, std::ref(channel), std::ref(stopShellInput), std::ref(thrownException)};

            while (channel.isOpen() && !channel.isEndOfFile()) {

                if ((bytesRead = channel.read(ioBuffer, ioBufferSize, 0)) > 0) {
                    writeFn(ioBuffer, bytesRead);
                }

                if (thrownException) {
                    break;
                }

            }

            stopShellInput = true;

            shellInputThread.join();

            if (thrownException) {
                std::rethrow_exception(thrownException);
            }

        }

        //
        // Send a shell command down a channel to be executed and read any output
        // produced.
        //
        
        void executeCommand(CSSHChannel &channel, const std::string &command) {

            int bytesRead;
            char *ioBuffer = channel.getIoBuffer().get();
            uint32_t ioBufferSize = channel.getIoBufferSize();

            channel.execute(command.c_str());

            while ((bytesRead = channel.read(ioBuffer, ioBufferSize)) > 0) {
                std::cout.write(ioBuffer, bytesRead);
            }
            std::cout << std::flush;

            while ((bytesRead = channel.read(ioBuffer, ioBufferSize, true)) > 0) {
                std::cerr.write(ioBuffer, bytesRead);
            }

        }

        //
        // Set up a channel to be direct forwarded and specify a write callback for any output received on the channel.
        //
        
        std::thread directForwarding(CSSHChannel &forwardingChannel, const std::string &remoteHost, int remotePort, const std::string &localHost, int localPort, WriteCallBackFn writeFn) {

            forwardingChannel.openForward(remoteHost, remotePort, localHost, localPort);

            std::thread channelReadThread{ readChannelThread, std::ref(forwardingChannel), writeFn};

            return (channelReadThread);

        }
        
    } // namespace SSH

} // namespace Antik

