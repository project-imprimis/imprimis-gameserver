#ifndef __ENGINE_H__
#define __ENGINE_H__

#define _FILE_OFFSET_BITS 64

#ifdef WIN32
#define _USE_MATH_DEFINES
#endif

#ifdef WIN32
  #define WIN32_LEAN_AND_MEAN
  #ifdef _WIN32_WINNT
  #undef _WIN32_WINNT
  #endif
  #define _WIN32_WINNT 0x0500
  #include "windows.h"
  #ifndef _WINDOWS
    #define _WINDOWS
  #endif
  #ifndef __GNUC__
    #include <eh.h>
    #include <dbghelp.h>
    #include <intrin.h>
  #endif
  #define ZLIB_DLL
#endif

#endif

