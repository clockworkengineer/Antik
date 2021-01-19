//
// Class: CApprise
//
// Description: A simple C++ class to enable files/folders to be watched and
// events generated. Supported events include the addition/deletion of files and
// directories and the modification of files with a change event. It is recursive
// by default and any directories added/removed from the hierarchy will cause new
// watches to be added/removed respectively. If no file event handler is passed then
// it defaults to the Linux inotify implementation.
//
// Dependencies: C20++               - Language standard features used.
//
// =================
// CLASS DEFINITIONS
// =================
#include "CApprise.hpp"
#include "CFileEventNotifier.hpp"
// ====================
// CLASS IMPLEMENTATION
// ====================
//
// C++ STL
//
#include <cassert>
#include <algorithm>
// =========
// NAMESPACE
// =========
namespace Antik::File
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
    // ==============
    // PUBLIC METHODS
    // ==============
    //
    // Main CApprise object constructor.
    //
    CApprise::CApprise(const std::string &watchFolder, int watchDepth,
                       std::shared_ptr<IFileEventNotifier> fileEventNotifier) : m_watchFolder{watchFolder}, m_watchDepth{watchDepth}
    {
        // ASSERT if passed parameters invalid
        assert(watchDepth >= -1); // < -1
        try
        {
            // If no handler passed then use default
            if (fileEventNotifier.get())
            {
                m_fileEventNotifier = fileEventNotifier;
            }
            else
            {
                m_fileEventNotifier = std::make_shared<CFileEventNotifier>();
            }
            if (!watchFolder.empty())
            {
                // Remove path trailing '/'
                if ((m_watchFolder).back() == '/')
                {
                    (m_watchFolder).pop_back();
                }
                // Save away max watch depth and modify with watch folder depth value if not all (-1).
                m_watchDepth = watchDepth;
                if (watchDepth != -1)
                {
                    m_watchDepth += std::count(watchFolder.begin(), watchFolder.end(), '/');
                }
            }
            // Set watch depth for notifier
            m_fileEventNotifier->setWatchDepth(watchDepth);
            // Add non empty watch folder
            if (!m_watchFolder.empty())
            {
                m_fileEventNotifier->addWatch(m_watchFolder);
            }
        }
        catch (const std::exception &e)
        {
            throw Exception(e.what());
        }
    }
    //
    // CApprise Destructor
    //
    CApprise::~CApprise()
    {
    }
    //
    // CApprise still watching folder(s)
    //
    bool CApprise::stillWatching(void)
    {
        try
        {
            return (m_fileEventNotifier->stillWatching());
        }
        catch (const std::exception &e)
        {
            throw Exception(e.what());
        }
    }
    //
    // Check whether termination of CApprise was the result of any thrown exception
    //
    std::exception_ptr CApprise::getThrownException(void)
    {
        try
        {
            return (m_fileEventNotifier->getThrownException());
        }
        catch (const std::exception &e)
        {
            throw Exception(e.what());
        }
    }
    //
    // Add watch (file or directory)
    //
    void CApprise::addWatch(const std::string &filePath)
    {
        try
        {
            m_fileEventNotifier->addWatch(filePath);
        }
        catch (const std::exception &e)
        {
            throw Exception(e.what());
        }
    }
    //
    // Remove watch
    //
    void CApprise::removeWatch(const std::string &filePath)
    {
        try
        {
            m_fileEventNotifier->removeWatch(filePath);
        }
        catch (const std::exception &e)
        {
            throw Exception(e.what());
        }
    }
    //
    // Get next CApprise event in queue.
    //
    void CApprise::getNextEvent(CApprise::Event &evt)
    {
        try
        {
            m_fileEventNotifier->getNextEvent(evt);
        }
        catch (const std::exception &e)
        {
            throw Exception(e.what());
        }
    }
    //
    // Start watching for file events
    //
    void CApprise::startWatching(bool clearQueue)
    {
        try
        {
            if (clearQueue)
            {
                m_fileEventNotifier->clearEventQueue();
            }
            m_watcherThread = std::make_unique<std::thread>(&IFileEventNotifier::generateEvents, m_fileEventNotifier);
        }
        catch (const std::exception &e)
        {
            throw Exception(e.what());
        }
    }
    //
    // Stop watching for file events
    //
    void CApprise::stopWatching(void)
    {
        try
        {
            m_fileEventNotifier->stopEventGeneration();
            if (m_watcherThread)
            {
                m_watcherThread->join();
            }
        }
        catch (const std::exception &e)
        {
            throw Exception(e.what());
        }
    }
} // namespace Antik::File
