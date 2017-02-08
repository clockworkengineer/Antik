## Antikythera Mechanism C ++ Class Repository##

# Introduction #

This repository contains the master copies of the C++ based utility classes that I use in my projects.  Copies of these classes in other repositories while working will not usually be up to date.

# [Task Class](https://github.com/clockworkengineer/Antikythera_mechanism/blob/master/classes/CFileTask.cpp) #

The core for the file processing engine is provided by the CFileTask class whose constructor takes five arguments, 

- **taskName:** The task name (std::string).
- **watchFolder:** The folder to be watched (std::string).
- **taskActFcn:** A pointer to a task action function that is called for each file that is copied/moved into the watch folder hierarchy.
- **fnData:** A pointer to data that may be needed by the action function.
- **watchDepth:** An integer specifying the watch depth (-1=all,0=just watch folder,1=next level down etc.)
- **options:**(optional) This structure passes in values used in any low level functionality (ie. killCount) and can be implementation specific such as providing a trace routine for low level inotify events or  pointers to the generic coutsr/coutstr trace functions.

 To start watching/processing files call this classes monitor function; the code within FPE.cpp creates a separate thread for this but it can be run in the main programs thread by just calling task.monitor() without any thread creation wrappper code (--single  option).

At the center of the class is an event loop which waits for CFileApprise events and calls the passed action function to process any file name passed through as part of an add event. The CFileApprise class is new and is basically an encapsulation of all of the inotify file event handling  code that used to reside in the Task class with an abstraction layer to hide any platform specifics. In the future new platform specific versions of this class may be implemented for say MacOS or Windows; in creating this class the one last non portable component of the FPE has been isolated thus aiding porting in future. As a result of this new class the task class is a lot simpler and smaller with all of the functionality being offloaded. The idea of making Task a child of the CFileApprise base class had been thought about but for present a task just creates and uses an private CFileApprise watcher object.

It should be noted that a basic shutdown protocol is provided to close down any threads that the task class uses by calling task.stop(). This now in turn calls the CFileApprise objects stop method which stops its internal event reading loop and performs any closedown of the CFileApprise object and thread.  This is just to give some control over thread termination which the C++ STL doesn't really provide; well in a subtle manner anyways. The shutdown can be actuated as well by either deleting the watch folder or by specifying a kill count in the optional task options structure parameter that can be passed in the classes constructor.

The task options structure parameter also has two other members which are pointers to functions that handle all cout/cerr output from the class. These take as a parameter a vector of strings to output and if the option parameter is omitted or the pointers are nullptr then no output occurs. The FPE provides these two functions in the form of coutstr/coutstr which are passed in if --quiet is not specified nullptrs otherwise. All output is modeled this way was it enables the two functions in the FPE to use a mutex to control access to the output streams which are not thread safe and also to provide a --quiet mode and when it is implemented a output to log file option.

# [CFileApprise Class](https://github.com/clockworkengineer/Antikythera_mechanism/blob/master/classes/CFileApprise.cpp) #

This is class was created to be a standalone class / abstraction of the inotify file event handling code that used to be contained in CFileTask. 

Its constructor has 3 parameters:

- **watchFolder:** Folder to watch for files created or moved into.
- **watchDepth:**  The watch depth is how far down the directory hierarchy that will be watched (-1 the whole tree, 0 just the watcher folder, 1 the next level down etc).
- **options:**(optional) This structure passes in values used in any low level functionality and can be implementation specific such as providing a trace routine for low level inotify events or  pointers to the generic coutsr/coutstr trace functions.

Once the object is created then its core method CFileApprise::watch() is run on a separate thread that is used to generate events from actions on files using inotify. While this is happening the main application loops  waiting on events returned by method CFileApprise::getEvent().

The current supported event types being

    enum EventId { 
    	Event_none=0,   	// None
    	Event_add,  		// File added to watched folder hierachy
    	Event_change,   	// File changed
    	Event_unlink,   	// File deleted from watched folder hierachy
    	Event_addir,		// Directory added to watched folder hierachy
    	Event_unlinkdir,	// Directory deleted from watched folder hierachy
    	Event_error 		// Exception error
    };

and they are contained within a structure of form

    struct Event {
    	EventId id;				// Event id
    	std::string message;   	// Event file name / error message string
    };
    
Notes: 

- Events *addir*/unlinkdir will result in new watch folders being added/removed from the internal watch table maps (depending on the value of watchDepth).

# [Redirect Class](https://github.com/clockworkengineer/Antikythera_mechanism/blob/master/classes/CRedirect.cpp) #

This is a small self contained utility class designed for FPE logging output. Its prime functionality is to provide a wrapper for pretty generic code that saves away an output streams read buffer, creates a file stream and redirects the output stream to it. The code to restore the original output streams is called from the objects destructor thus providing convenient for restoring the original stream. Its primary use within the FPE is to redirect std::cout to a log file.

# Exceptions #

Both the CFileTask and CFileApprise classes are designed to run in a separate thread although the former can run in the main thread quite happily. As such any exceptions thrown by them could be lost and so that they are not a copy is taken inside each objects main catch clause and stored away in a std::exception_ptr. This value can then by retrieved with method getThrownException() and either re-thrown if the and of the chain has been reached or stored away again to retrieved by another getThrownException() when the enclosing object closes down (as in the case CFileTask and CFileApprise class having thrown the exception).


# [CMailSMTP Class](https://github.com/clockworkengineer/Antikythera_mechanism/blob/master/classes/CMailSMTP.cpp) #

CMailSMTP provides the ability to create an email, add file attachments (encoded either as 7-bit or base64) and then send the created email to a given recipient(s). It provides methods for setting various parameters required to send the email and also attach files and post the resulting email. It is state based so it is quite possible to create an email and send but then just change say the recipients and re-post. Library [libcurl](https://curl.haxx.se/libcurl/) is used to provide the SMTP server connect and message sending transport.

# [CMailIMAP Class](https://github.com/clockworkengineer/Antikythera_mechanism/blob/master/classes/CMailIMAP.cpp) #

CMailIMAP provides a way to connect to an IMAP server and send commands and receive decoded responses to process. It supports most of [rfc3501](https://tools.ietf.org/html/rfc3501) including the ability the ability to FETCH messages and also APPEND them to a mailbox. The decoded response are fairly simplistic and adding extra processing like encoding attachments or creating emails for append need to be done at a higher code layer; although every peace of data needed to achieve this should be in the return response. Library [libcurl](https://curl.haxx.se/libcurl/) is used to provide the SMTP server connect and command sending transport.

# [CLogger Class](https://github.com/clockworkengineer/Antikythera_mechanism/blob/master/classes/CLogger.cpp) #

Generic log trace class that will take a list of strings and output them either to cout or cerr with an optional time and date stamp. It also includes a template method for converting an arbitrary value to a string to be placed in the list of strings to be output. This class is very much a work in progress and will probably change until I find a solution that I like for my logging needs.

# To Do #

1. Finish this document.