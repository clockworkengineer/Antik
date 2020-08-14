//
// Class: CFTP
//
// Description: A class to connect to an FTP server using provided credentials
// and enable the uploading/downloading of files along with assorted other commands.
//
// Note: TLS/SSL connections are supported.
//
// Dependencies:   C17++        - Language standard features used.
//                 CSocket   -  - Used to talk to FTP server.
//

// =================
// CLASS DEFINITIONS
// =================

#include "CFTP.hpp"

// ====================
// CLASS IMPLEMENTATION
// ====================

//
// C++ STL
//

#include <iostream>
#include <fstream>

// =======
// IMPORTS
// =======

// =========
// NAMESPACE
// =========

namespace Antik::FTP
{

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

//
// Extract host ip address and port information from passive command reply.
//

void CFTP::extractPassiveAddressPort(std::string &pasvResponse)
{

    try
    {

        std::string passiveParams = pasvResponse.substr(pasvResponse.find('(') + 1);
        passiveParams = passiveParams.substr(0, passiveParams.find(')'));

        std::uint32_t port = std::stoi(passiveParams.substr(passiveParams.find_last_of(',') + 1));
        passiveParams = passiveParams.substr(0, passiveParams.find_last_of(','));
        port |= (std::stoi(passiveParams.substr(passiveParams.find_last_of(',') + 1)) << 8);

        passiveParams = passiveParams.substr(0, passiveParams.find_last_of(','));
        for (auto &byte : passiveParams)
        {
            if (byte == ',')
                byte = '.';
        }

        m_dataChannelSocket.setHostAddress(passiveParams);
        m_dataChannelSocket.setHostPort(std::to_string(port));
    }
    catch (const std::exception &e)
    {
        throw;
    }
}

//
// Create PORT command to send over control channel.
//

std::string CFTP::createPortCommand()
{

    std::string portCommand{"PORT "};

    try
    {

        std::uint16_t port = std::stoi(m_dataChannelSocket.getHostPort());
        portCommand += m_dataChannelSocket.getHostAddress();
        for (auto &byte : portCommand)
        {
            if (byte == '.')
                byte = ',';
        }
        portCommand += "," + std::to_string((port & 0xFF00) >> 8) + "," + std::to_string(port & 0xFF);
    }
    catch (const std::exception &e)
    {
        throw;
    }

    return (portCommand);
}

//
// Download a file from FTP server to local system.
//

void CFTP::downloadFile(const std::string &file)
{

    std::ofstream localFile{file, std::ofstream::trunc | std::ofstream::binary};

    do
    {

        size_t bytesRead = m_dataChannelSocket.read(m_ioBuffer.get(), m_ioBufferSize);
        if (bytesRead)
        {
            localFile.write(m_ioBuffer.get(), bytesRead);
        }

    } while (!m_dataChannelSocket.closedByRemotePeer());

    localFile.close();
}

//
// Upload file from local system to FTP server,
//

void CFTP::uploadFile(const std::string &file)
{

    std::ifstream localFile{file, std::ifstream::binary};

    if (localFile)
    {

        do
        {

            localFile.read(m_ioBuffer.get(), m_ioBufferSize);

            size_t bytesToWrite = localFile.gcount();
            if (bytesToWrite)
            {
                for (;;)
                {
                    bytesToWrite -= m_dataChannelSocket.write(&m_ioBuffer.get()[localFile.gcount() - bytesToWrite], bytesToWrite);
                    if ((bytesToWrite == 0) || m_dataChannelSocket.closedByRemotePeer())
                    {
                        break;
                    }
                }
            }

        } while (localFile && !m_dataChannelSocket.closedByRemotePeer());

        localFile.close();
    }
}

//
// Download response to command over data channel.
//

void CFTP::downloadCommandResponse(std::string &commandResponse)
{

    do
    {

        size_t bytesRead = m_dataChannelSocket.read(m_ioBuffer.get(), m_ioBufferSize - 1);
        if (bytesRead)
        {
            m_ioBuffer.get()[bytesRead] = '\0';
            commandResponse.append(m_ioBuffer.get());
        }

    } while (!m_dataChannelSocket.closedByRemotePeer());
}

//
// Transfer (upload/download) file over data channel.
//

void CFTP::transferOnDataChannel(const std::string &file, DataTransferType transferType)
{

    std::string unusedResponse;

    transferOnDataChannel(file, unusedResponse, transferType);
}

//
// Transfer (command response) file over data channel.
//

void CFTP::transferOnDataChannel(std::string &commandRespnse)
{

    std::string unusedFile;

    transferOnDataChannel(unusedFile, commandRespnse, DataTransferType::commandResponse);
}

//
// Transfer (file upload/ file download/ command response) over data channel.
//

void CFTP::transferOnDataChannel(const std::string &file, std::string &commandRespnse, DataTransferType transferType)
{

    try
    {

        if ((m_commandStatusCode == 125) || (m_commandStatusCode == 150))
        {

            m_dataChannelSocket.waitUntilConnected();

            switch (transferType)
            {
            case DataTransferType::download:
                downloadFile(file);
                break;
            case DataTransferType::upload:
                uploadFile(file);
                break;
            case DataTransferType::commandResponse:
                downloadCommandResponse(commandRespnse);
                break;
            }

            m_dataChannelSocket.close();

            ftpResponse();
        }
    }
    catch (const std::exception &e)
    {
        m_dataChannelSocket.cleanup();
        throw;
    }

    m_dataChannelSocket.cleanup();
}

//
// Send FTP over control channel. Append "\r\n" to command for transmission then remove
// from m_lastCommand.
//

void CFTP::ftpCommand(const std::string &command)
{

    m_lastCommand = command + "\r\n";

    size_t commandLength = m_lastCommand.size();

    do
    {
        commandLength -= m_controlChannelSocket.write(&m_lastCommand[m_lastCommand.size() - commandLength], commandLength);
    } while (commandLength != 0);

    m_lastCommand.pop_back();
    m_lastCommand.pop_back();

    ftpResponse();
}

//
// Read FTP command response from control channel (return its status code).
// It gathers the whole response even if it is extended (ie. starts with "ddd-"
// and ends with a line starting ddd. Can get multiple replies in a single read
// so just read single characters.
//

void CFTP::ftpResponse()
{

    m_commandResponse.clear();

    do
    {

        do
        {
            char byte[1];
            if (m_controlChannelSocket.read(&byte[0], 1))
            {
                m_commandResponse.append(1, byte[0]);
                if (byte[0] == '\n')
                {
                    break;
                }
            }
        } while (!m_controlChannelSocket.closedByRemotePeer());

        if (m_commandResponse[3] == '-')
        {
            if (m_commandResponse.rfind("\r\n" + m_commandResponse.substr(0, 3) + " ") != std::string::npos)
            {
                break;
            }
        }

    } while ((m_commandResponse[3] == '-') && (!m_controlChannelSocket.closedByRemotePeer()));

    if (m_controlChannelSocket.closedByRemotePeer())
    {
        throw std::runtime_error("Control channel connection closed by peer.");
    }

    try
    {
        m_commandStatusCode = std::stoi(m_commandResponse);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Invalid FTP command response status code.");
    }
}

//
// Send transfer mode to used over data channel.
//

bool CFTP::sendTransferMode()
{

    if (m_passiveMode)
    {
        ftpCommand("PASV");
        if (m_commandStatusCode == 227)
        {
            extractPassiveAddressPort(m_commandResponse);
            m_dataChannelSocket.connect();
        }
        return (m_commandStatusCode == 227);
    }
    else
    {
        m_dataChannelSocket.setHostAddress(Antik::Network::CSocket::localIPAddress());
        m_dataChannelSocket.listenForConnection();
        ftpCommand(createPortCommand());
        return (m_commandStatusCode == 200);
    }
}

//
// Get FTP server features list.
//

void CFTP::ftpServerFeatures(void)
{

    ftpCommand("FEAT");

    if (m_commandStatusCode == 211)
    {
        std::string feature;
        std::istringstream featureResponseStream{m_commandResponse};
        std::getline(featureResponseStream, feature, '\n');
        m_serverFeatures.clear();
        while (std::getline(featureResponseStream, feature, '\n'))
        {
            feature.pop_back();
            m_serverFeatures.push_back(feature.substr(1));
        }
        m_serverFeatures.pop_back();
    }
}

// ==============
// PUBLIC METHODS
// ==============

//
// Set FTP account details
//

void CFTP::setUserAndPassword(const std::string &userName, const std::string &userPassword)
{

    m_userName = userName;
    m_userPassword = userPassword;
}

//
// Set FTP server name and port
//

void CFTP::setServerAndPort(const std::string &serverName, const std::string &serverPort)
{

    m_serverName = serverName;
    m_serverPort = serverPort;
}

//
// Get current connection status with server
//

bool CFTP::isConnected(void) const
{

    return (m_connected);
}

//
// Enabled/disable SSL
//

void CFTP::setSslEnabled(bool sslEnabled)
{
    if (!m_connected)
    {
        m_sslEnabled = sslEnabled;
    }
    else
    {
        throw Exception("Cannot set SSL mode while connected.");
    }
}

bool CFTP::isSslEnabled() const
{
    return m_sslEnabled;
}

//
// Get last raw FTP command
//

std::string CFTP::getLastCommand() const
{
    return m_lastCommand;
}

//
// Get last FTP response status code
//

std::uint16_t CFTP::getCommandStatusCode() const
{
    return m_commandStatusCode;
}

//
// Get last FTP full response string
//

std::string CFTP::getCommandResponse() const
{
    return m_commandResponse;
}

//
// Setup connection to server
//

std::uint16_t CFTP::connect(void)
{

    try
    {

        if (m_connected)
        {
            throw std::logic_error("Already connected to a server.");
        }

        // Allocate IO Buffer

        m_ioBuffer = std::make_unique<char[]>(m_ioBufferSize);

        m_dataChannelSocket.setHostAddress(Antik::Network::CSocket::localIPAddress());
        ;

        m_controlChannelSocket.setHostAddress(m_serverName);
        m_controlChannelSocket.setHostPort(m_serverPort);
        m_controlChannelSocket.connect();

        ftpResponse();

        if (m_commandStatusCode == 220)
        {

            // Fetch FTP server features list

            ftpServerFeatures();

            if (m_sslEnabled)
            {
                ftpCommand("AUTH TLS");
                if (m_commandStatusCode == 234)
                {
                    m_controlChannelSocket.setSslEnabled(true);
                    m_controlChannelSocket.tlsHandshake();
                    m_dataChannelSocket.setSslEnabled(true);
                    ftpCommand("PBSZ 0");
                    if (m_commandStatusCode == 200)
                    {
                        ftpCommand("PROT P");
                    }
                }
            }

            m_connected = true;

            ftpCommand("USER " + m_userName);

            if (m_commandStatusCode == 331)
            {
                ftpCommand("PASS " + m_userPassword);
            }
        }

        return (m_commandStatusCode);
    }
    catch (const std::exception &e)
    {
        throw Exception(e.what());
    }
}

//
// Disconnect from server
//

std::uint16_t CFTP::disconnect(void)
{

    try
    {

        if (!m_connected)
        {
            throw std::logic_error("Already connected to a server.");
        }

        ftpCommand("QUIT");

        m_connected = false;

        m_controlChannelSocket.close();

        m_controlChannelSocket.setSslEnabled(false);
        m_dataChannelSocket.setSslEnabled(false);

        // Free IO Buffer

        m_ioBuffer.reset();

        return (m_commandStatusCode);
    }
    catch (const std::exception &e)
    {
        throw Exception(e.what());
    }
}

//
// Set passive transfer mode.
// == true passive enabled, == false active mode
//

void CFTP::setPassiveTransferMode(bool passiveEnabled)
{

    m_passiveMode = passiveEnabled;
}

//
// Transfer a file from the server to a local file.
//

std::uint16_t CFTP::getFile(const std::string &remoteFilePath, const std::string &localFilePath)
{

    try
    {

        std::ofstream localFile{localFilePath, std::ofstream::binary};

        if (!m_connected)
        {
            throw std::logic_error("Already connected to a server.");
        }

        if (localFile)
        {
            localFile.close();
            if (sendTransferMode())
            {
                ftpCommand("RETR " + remoteFilePath);
                transferOnDataChannel(localFilePath, DataTransferType::download);
            }
        }
        else
        {
            m_commandStatusCode = 550;
            throw std::runtime_error("Local file " + localFilePath + " could not be created.");
        }

        return (m_commandStatusCode);
    }
    catch (const std::exception &e)
    {
        throw Exception(e.what());
    }
}

//
// Transfer a file to the server from a local file.
//

std::uint16_t CFTP::putFile(const std::string &remoteFilePath, const std::string &localFilePath)
{

    try
    {

        std::ifstream localFile{localFilePath, std::ifstream::binary};

        if (!m_connected)
        {
            throw std::logic_error("Already connected to a server.");
        }

        if (localFile)
        {
            localFile.close();
            if (sendTransferMode())
            {
                ftpCommand("STOR " + remoteFilePath);
                transferOnDataChannel(localFilePath, DataTransferType::upload);
            }
        }
        else
        {
            m_commandStatusCode = 550;
            throw std::runtime_error("Local file " + localFilePath + " does not exist.");
        }

        return (m_commandStatusCode);
    }
    catch (const std::exception &e)
    {
        throw Exception(e.what());
    }
}

//
// Produce a directory listing for the file/directory passed in or for the current
// working directory if none is.
//

std::uint16_t CFTP::list(const std::string &directoryPath, std::string &listOutput)
{

    try
    {

        if (!m_connected)
        {
            throw std::logic_error("Already connected to a server.");
        }

        listOutput.clear();

        if (sendTransferMode())
        {
            ftpCommand("LIST " + directoryPath);
            transferOnDataChannel(listOutput);
        }

        return (m_commandStatusCode);
    }
    catch (const std::exception &e)
    {
        throw Exception(e.what());
    }
}

//
// Produce a file list for the file/directory passed in or for the current
// working directory if none is.
//

std::uint16_t CFTP::listFiles(const std::string &directoryPath, FileList &fileList)
{

    try
    {

        if (!m_connected)
        {
            throw std::logic_error("Already connected to a server.");
        }

        fileList.clear();

        if (sendTransferMode())
        {
            std::string listOutput;
            ftpCommand("NLST " + directoryPath);
            transferOnDataChannel(listOutput);
            if (m_commandStatusCode == 226)
            {
                std::string file;
                std::istringstream listOutputStream{listOutput};
                while (std::getline(listOutputStream, file, '\n'))
                {
                    file.pop_back();
                    fileList.push_back(file);
                }
            }
        }

        return (m_commandStatusCode);
    }
    catch (const std::exception &e)
    {
        throw Exception(e.what());
    }
}

//
// Produce a file information list for the directory passed in or for the current
// working directory if none is.
//

std::uint16_t CFTP::listDirectory(const std::string &directoryPath, std::string &listOutput)
{

    try
    {

        if (!m_connected)
        {
            throw std::logic_error("Already connected to a server.");
        }

        listOutput.clear();

        if (sendTransferMode())
        {
            ftpCommand("MLSD " + directoryPath);
            transferOnDataChannel(listOutput);
        }

        return (m_commandStatusCode);
    }
    catch (const std::exception &e)
    {
        throw Exception(e.what());
    }
}

//
// Produce file information for the file passed in or for the current
// working directory if none is. Note: Reply is sent on control and not
// the data channel.
//

std::uint16_t CFTP::listFile(const std::string &filePath, std::string &listOutput)
{

    try
    {

        if (!m_connected)
        {
            throw std::logic_error("Already connected to a server.");
        }

        listOutput.clear();

        ftpCommand("MLST " + filePath);

        if (m_commandStatusCode == 250)
        {
            listOutput = m_commandResponse.substr(m_commandResponse.find('\n') + 1);
            listOutput = listOutput.substr(0, listOutput.find('\r'));
        }

        return (m_commandStatusCode);
    }
    catch (const std::exception &e)
    {
        throw Exception(e.what());
    }
}

//
// Make remote FTP server directory
//

std::uint16_t CFTP::makeDirectory(const std::string &directoryName)
{

    try
    {

        if (!m_connected)
        {
            throw std::logic_error("Already connected to a server.");
        }

        ftpCommand("MKD " + directoryName);

        return (m_commandStatusCode);
    }
    catch (const std::exception &e)
    {
        throw Exception(e.what());
    }
}

//
// Remove remote FTP server directory
//

std::uint16_t CFTP::removeDirectory(const std::string &directoryName)
{

    try
    {

        if (!m_connected)
        {
            throw std::logic_error("Already connected to a server.");
        }

        ftpCommand("RMD " + directoryName);

        return (m_commandStatusCode);
    }
    catch (const std::exception &e)
    {
        throw Exception(e.what());
    }
}

//
// Remove remote FTP server directory
//

std::uint16_t CFTP::fileSize(const std::string &fileName, size_t &fileSize)
{

    try
    {

        if (!m_connected)
        {
            throw std::logic_error("Already connected to a server.");
        }

        ftpCommand("SIZE " + fileName);

        if (m_commandStatusCode == 213)
        {
            fileSize = std::stoi(m_commandResponse.substr(m_commandResponse.find(' ') + 1));
        }

        return (m_commandStatusCode);
    }
    catch (const std::exception &e)
    {
        throw Exception(e.what());
    }
}

//
// Delete remote FTP server file
//

std::uint16_t CFTP::deleteFile(const std::string &fileName)
{

    try
    {

        if (!m_connected)
        {
            throw std::logic_error("Already connected to a server.");
        }

        ftpCommand("DELE " + fileName);

        return (m_commandStatusCode);
    }
    catch (const std::exception &e)
    {
        throw Exception(e.what());
    }
}

//
// Rename remote FTP server file
//

std::uint16_t CFTP::renameFile(const std::string &srcFileName, const std::string &dstFileName)
{

    try
    {

        if (!m_connected)
        {
            throw std::logic_error("Already connected to a server.");
        }

        ftpCommand("RNFR " + srcFileName);
        if (m_commandStatusCode == 350)
        {
            ftpCommand("RNTO " + dstFileName);
        }

        return (m_commandStatusCode);
    }
    catch (const std::exception &e)
    {
        throw Exception(e.what());
    }
}

//
// Change current working directory on server.
//

std::uint16_t CFTP::changeWorkingDirectory(const std::string &workingDirectoryPath)
{

    try
    {

        if (!m_connected)
        {
            throw std::logic_error("Already connected to a server.");
        }

        ftpCommand("CWD " + workingDirectoryPath);

        return (m_commandStatusCode);
    }
    catch (const std::exception &e)
    {
        throw Exception(e.what());
    }
}

//
// Fetch current working directory on server and return path as string.
//

std::uint16_t CFTP::getCurrentWoringDirectory(std::string &currentWoringDirectoryPath)
{

    try
    {

        if (!m_connected)
        {
            throw std::logic_error("Already connected to a server.");
        }

        currentWoringDirectoryPath.clear();

        ftpCommand("PWD");

        if (m_commandStatusCode == 257)
        {
            currentWoringDirectoryPath = m_commandResponse.substr(m_commandResponse.find_first_of('\"') + 1);
            currentWoringDirectoryPath = currentWoringDirectoryPath.substr(0, currentWoringDirectoryPath.find_first_of('\"'));
        }

        return (m_commandStatusCode);
    }
    catch (const std::exception &e)
    {
        throw Exception(e.what());
    }
}

//
// Fetch files last modified date/time.
//

std::uint16_t CFTP::getModifiedDateTime(const std::string &filePath, DateTime &modifiedDateTime)
{

    try
    {

        if (!m_connected)
        {
            throw std::logic_error("Already connected to a server.");
        }

        ftpCommand("MDTM " + filePath);

        if (m_commandStatusCode == 213)
        {
            std::string dateTime = m_commandResponse.substr(m_commandResponse.find(' ') + 1);
            modifiedDateTime.year = std::stoi(dateTime.substr(0, 4));
            modifiedDateTime.month = std::stoi(dateTime.substr(4, 2));
            modifiedDateTime.day = std::stoi(dateTime.substr(6, 2));
            modifiedDateTime.hour = std::stoi(dateTime.substr(8, 2));
            modifiedDateTime.minute = std::stoi(dateTime.substr(10, 2));
            modifiedDateTime.second = std::stoi(dateTime.substr(12, 2));
        }

        return (m_commandStatusCode);
    }
    catch (const std::exception &e)
    {
        throw Exception(e.what());
    }
}

//
// Return true if passed in file is a directory false for a file.
//

bool CFTP::isDirectory(const std::string &fileName)
{

    try
    {

        if (!m_connected)
        {
            throw std::logic_error("Already connected to a server.");
        }

        // Try MLST first then STAT

        ftpCommand("MLST " + fileName);

        if (m_commandStatusCode == 250)
        {

            size_t dirPosition = m_commandResponse.find("Type=dir;");
            if (dirPosition != std::string::npos)
            {
                return (true);
            }
        }
        else if (m_commandStatusCode == 500)
        {

            ftpCommand("STAT " + fileName);

            if ((m_commandStatusCode == 213) || (m_commandStatusCode == 212))
            {
                size_t dirPosition = m_commandResponse.find("\r\n") + 2;
                if ((dirPosition != std::string::npos) &&
                    (m_commandResponse[dirPosition] == 'd'))
                {
                    return (true);
                }
            }
        }

        return (false);
    }
    catch (const std::exception &e)
    {
        throw Exception(e.what());
    }
}

//
// Return true if passed in file exists false otherwise.
//

bool CFTP::fileExists(const std::string &fileName)
{

    try
    {

        if (!m_connected)
        {
            throw std::logic_error("Already connected to a server.");
        }

        // Try MLST first then STAT

        ftpCommand("MLST " + fileName);

        if (m_commandStatusCode == 250)
        {
            return (true);
        }
        else if (m_commandStatusCode == 500)
        {

            ftpCommand("STAT " + fileName);

            // If 212/213 returned check the response is not empty; if it is
            // file does not exist.

            if ((m_commandStatusCode == 213) || (m_commandStatusCode == 212))
            {
                size_t statusCodePosition = m_commandResponse.find("\r\n") + 2;
                if ((statusCodePosition != std::string::npos) &&
                    (m_commandResponse[statusCodePosition] != '2'))
                {
                    return (true);
                }
            }
        }

        return (false);
    }
    catch (const std::exception &e)
    {
        throw Exception(e.what());
    }
}

//
// Move up a directory
//

std::uint16_t CFTP::cdUp()
{

    try
    {

        if (!m_connected)
        {
            throw std::logic_error("Already connected to a server.");
        }

        ftpCommand("CDUP");

        return (m_commandStatusCode);
    }
    catch (const std::exception &e)
    {
        throw Exception(e.what());
    }
}

//
// BinaryTransfer == true set binary transfer otherwise set ASCII
//

void CFTP::setBinaryTransfer(bool binaryTransfer)
{

    try
    {

        if (!m_connected)
        {
            throw std::logic_error("Already connected to a server.");
        }

        if (binaryTransfer)
        {
            ftpCommand("TYPE I");
        }
        else
        {
            ftpCommand("TYPE A");
        }

        if (m_commandStatusCode == 200)
        {
            m_binaryTransfer = binaryTransfer;
        }
    }
    catch (const std::exception &e)
    {
        throw Exception(e.what());
    }
}

bool CFTP::isBinaryTransfer() const
{
    return m_binaryTransfer;
}

//
// Return a vector of strings representing FTP server features. If empty
// try to get again as server may require to be logged in.
//

std::vector<std::string> CFTP::getServerFeatures()
{

    try
    {

        if (!m_connected)
        {
            throw std::logic_error("Already connected to a server.");
        }

        if (m_serverFeatures.empty())
        {
            ftpServerFeatures();
        }

        return (m_serverFeatures);
    }
    catch (const std::exception &e)
    {
        throw Exception(e.what());
    }
}

//
// Main CFTP object constructor.
//

CFTP::CFTP()
{
}

//
// CFTP Destructor
//

CFTP::~CFTP()
{
}

} // namespace Antik::FTP
