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

        static void readShellInput(CSSHChannel &channel, std::atomic<bool> &stopShellInput) {

            struct termios terminalSettings, savedTerminalSettings;

            tcgetattr(0, &terminalSettings);
            savedTerminalSettings = terminalSettings;
            cfmakeraw(&terminalSettings);
            terminalSettings.c_cc[VMIN] = 0;
            terminalSettings.c_cc[VTIME] = 0;
            tcsetattr(0, TCSANOW, &terminalSettings);

            while (!stopShellInput) {
                char singleChar = std::getchar();
                if (singleChar != EOF) {
                    char ioBuffer[1]{singleChar};
                    channel.write(ioBuffer, 1);
                }
                std::this_thread::sleep_for(std::chrono::microseconds(5));
            }

            tcsetattr(0, TCSANOW, &savedTerminalSettings);

        }
              
        // ================
        // PUBLIC FUNCTIONS
        // ================

        int interactiveShell(CSSHChannel &channel, int columns, int rows) {

            int bytesRead;
            char *ioBuffer = channel.getIoBuffer().get();
            uint32_t ioBufferSize = channel.getIoBufferSize();
            std::atomic<bool> stopShellInput{ false};

            channel.requestTerminal();
            channel.requestTerminalSize(columns, rows);
            channel.requestShell();

            std::thread shellInputThread{ readShellInput, std::ref(channel), std::ref(stopShellInput)};

            while (channel.isOpen() && !channel.isEndOfFile()) {
                bytesRead = channel.read(ioBuffer, ioBufferSize, 0);
                if (bytesRead > 0) {
                    std::cout.write(ioBuffer, bytesRead);
                    std::cout.flush();
                }
            }

            stopShellInput = true;

            shellInputThread.join();

        }

        void executeCommand(CSSHChannel &channel, const std::string &command) {

            int nbytes;
            char *ioBuffer = channel.getIoBuffer().get();
            uint32_t ioBufferSize = channel.getIoBufferSize();

            channel.execute(command.c_str());

            while ((nbytes = channel.read(ioBuffer, ioBufferSize)) > 0) {
                std::cout.write(ioBuffer, nbytes);
            }
            std::cout << std::flush;
            while ((nbytes = channel.read(ioBuffer, ioBufferSize, true)) > 0) {
                std::cerr.write(ioBuffer, nbytes);
            }

        }

    } // namespace SSH

} // namespace Antik

