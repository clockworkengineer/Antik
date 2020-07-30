#ifndef HOST_HPP
#define HOST_HPP

#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
	/* UNIX-style OS. ------------------------------------------- */
#include <unistd.h>
#if defined(_POSIX_VERSION)
	/* POSIX compliant */
#else
#ifdef __CYGWIN__
#include <sys/select.h>
#else
static_assert(false, "TRYING TO BUILD FOR NON-POSIX UNIX PLATFORM.");
#endif
#endif
#else
static_assert(false, "TRYING TO BUILD FOR NON-UNIX PLATFORM.");
#endif

#endif /* HOST_HPP */

