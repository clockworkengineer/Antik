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
// Antik classes      : SSHChannel
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

        static void readShellInput(CSSHChannel &channel, std::atomic<bool> &stopShellInput, std::exception_ptr &thrownException) {

            try {

                struct termios terminalSettings, savedTerminalSettings;

                if (tcgetattr(0, &terminalSettings) == -1) {
                    throw system_error(errno, system_category(),  __func__);
                }

                savedTerminalSettings = terminalSettings;

                cfmakeraw(&terminalSettings);
                terminalSettings.c_cc[VMIN] = 0;
                terminalSettings.c_cc[VTIME] = 0;

                if (tcsetattr(0, TCSANOW, &terminalSettings) == -1) {
                    throw system_error(errno, system_category(), __func__);
                }

                while (!stopShellInput) {
                    char singleChar = std::getchar();
                    if (singleChar != EOF) {
                        char ioBuffer[1]{singleChar};
                        channel.write(ioBuffer, 1);
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

        // ================
        // PUBLIC FUNCTIONS
        // ================

        int interactiveShell(CSSHChannel &channel, int columns, int rows) {

            int bytesRead;
            char *ioBuffer = channel.getIoBuffer().get();
            uint32_t ioBufferSize = channel.getIoBufferSize();
            std::atomic<bool> stopShellInput{ false};
            std::exception_ptr thrownException {nullptr};

            channel.requestTerminal();
            channel.requestTerminalSize(columns, rows);
            channel.requestShell();

            std::thread shellInputThread{ readShellInput, std::ref(channel), std::ref(stopShellInput), std::ref(thrownException)};

            while (channel.isOpen() && !channel.isEndOfFile()) {
                bytesRead = channel.readNonBlocking(ioBuffer, ioBufferSize, 0);
                if (bytesRead > 0) {
                    std::cout.write(ioBuffer, bytesRead);
                    std::cout.flush();
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

    } // namespace SSH

} // namespace Antik

