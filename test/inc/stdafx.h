#ifndef __STDAFX_H__
#define __STDAFX_H__

#ifndef _OSF1
#ifndef	_AIX
#ifndef _XOPEN_SOURCE_EXTENDED
//#define _XOPEN_SOURCE_EXTENDED
#endif
#endif
#endif

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE		// file64, stat64
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE				// file64, stat64
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <winnt.h>
#endif

#include "mydefs.h"
#include "snprintf.h"
#include "debug.h"

#endif
