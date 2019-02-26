/* @(#)root/clib:$Name:  $:$Id: snprintf.h,v 1.2 2007/10/22 06:37:41 kskim Exp $ */
/* Author: Fons Rademakers  10/12/2000 */

/*
   Write formatted text to buffer 'string', using format string 'format'.
   Returns number of characters written, or -1 if truncated.
   Format string is understood as defined in ANSI C.
*/

#ifndef __SNPRINTF_H__
#define __SNPRINTF_H__

#include <stdio.h>
#include <stdarg.h>

// added by kskim. 2006-11-29
#ifdef mysnprintf
#undef mysnprintf
#endif

//! 모든 OS(Unix, linux, windows)에서 사용가능하도록 만든 snprintf 함수
int myvsnprintf(char *string, size_t length, const char *format, va_list args);
int _mysnprintf(char *string, size_t length, const char *format, ...);

#define mysnprintf	_mysnprintf

#endif /* __SNPRINTF_H__ */

