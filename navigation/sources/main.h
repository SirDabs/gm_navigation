/*
    gm_navigation
    By Spacetech
*/

#include "interface.h"
#include "defines.h"

#ifdef USE_BOOST_THREADS
#include <boost/thread/thread.hpp>
#else
#include <vstdlib/jobthread.h>
IThreadPool* threadPool;
#endif

#ifdef FILEBUG
	FILE *pDebugFile = NULL;
#endif
