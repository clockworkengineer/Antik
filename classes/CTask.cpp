#include "HOST.hpp"
//
// Class: CTask
//
// Description: This class uses the CFileApprise class to generate file add events
// on a watch folder and to process each file added with a task action function
// provided as a parameter in its constructor.
//
// Dependencies: C17++               - Language standard features used.
//               Class CLogger       - Logging functionality.
//               Class CFileApprise  - File event handling abstraction.
//

// =================
// CLASS DEFINITIONS
// =================

#include "CTask.hpp"
#include "CFileEventNotifier.hpp"

// ====================
// CLASS IMPLEMENTATION
// ====================

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
// Task object constructor.
//

CTask::CTask(
    const std::string &watchFolder,         // Watch folder path
    std::shared_ptr<CTask::IAction> action, // Action object
    int watchDepth,                         // Watch depth -1= all, 0=just watch folder
    int killCount                           // Kill count
    )
    : m_taskAction{action}, m_killCount{killCount}

{

    // ASSERT if passed parameters invalid

    assert(watchFolder.length() != 0); // Length == 0
    assert(watchDepth >= -1);          // < -1
    assert(action != nullptr);         // nullptr
    assert(killCount >= 0);            // < 0

    // Create CFileApprise watcher object.

    m_watcher = std::make_unique<CApprise>(watchFolder, watchDepth);
    
}

//
// Destructor
//

CTask::~CTask()
{
}

//
// Check whether termination of CTask was the result of any thrown exception
//

std::exception_ptr CTask::getThrownException(void)
{

    return (m_thrownException);
}

//
// Flag watcher and task loops to stop.
//

void CTask::stop(void)
{

    m_watcher->stopWatching();
}

//
// Loop calling the action process() for each add file event.
//

void CTask::monitor(void)
{

    try
    {

        m_taskAction->init();

        m_watcher->startWatching(false);

        // Loop until watcher stopped

        while (m_watcher->stillWatching())
        {

            IApprise::Event evt;

            m_watcher->getNextEvent(evt);

            if ((evt.id == IApprise::Event_add) && !evt.message.empty())
            {

                m_taskAction->process(evt.message);

                if ((m_killCount != 0) && (--(m_killCount) == 0))
                {
                    break;
                }
            }
        }

        // Pass any CFileApprise exceptions up chain

        if (m_watcher->getThrownException())
        {
            m_thrownException = m_watcher->getThrownException();
        }
    }
    catch (...)
    {
        // Pass any CTask thrown exceptions up chain
        m_thrownException = std::current_exception();
    }

    // Stop file watcher

    m_watcher->stopWatching();

    m_taskAction->term();
}

} // namespace Antik::File