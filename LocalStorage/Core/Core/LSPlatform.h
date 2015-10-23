//
//  Platform.h
//  Core
//
//  Created by void on 10/23/15.
//  Copyright © 2015 Timur Losev. All rights reserved.
//

#pragma once

// define supported target platform macro which LS uses.
#define LS_PLATFORM_UNKNOWN            0
#define LS_PLATFORM_IOS                1
#define LS_PLATFORM_ANDROID            2
#define LS_PLATFORM_WIN32              3
#define LS_PLATFORM_MARMALADE          4
#define LS_PLATFORM_LINUX              5
#define LS_PLATFORM_BADA               6
#define LS_PLATFORM_BLACKBERRY         7
#define LS_PLATFORM_MAC                8
#define LS_PLATFORM_NACL               9
#define LS_PLATFORM_EMSCRIPTEN        10
#define LS_PLATFORM_TIZEN             11
#define LS_PLATFORM_QT5               12
#define LS_PLATFORM_WINRT             13

// Determine target platform by compile environment macro.
#define LS_TARGET_PLATFORM             LS_PLATFORM_UNKNOWN

// mac
#if defined(LS_TARGET_OS_MAC) || defined(__APPLE__)
#undef  LS_TARGET_PLATFORM
#define LS_TARGET_PLATFORM         LS_PLATFORM_MAC
#define LS_TARGET_PLATFORM_APPLE 1
#endif

// iphone
#if defined(CC_TARGET_OS_IPHONE)
#undef  LS_TARGET_PLATFORM
#define LS_TARGET_PLATFORM         LS_PLATFORM_IOS
#define LS_TARGET_PLATFORM_APPLE 1
#endif

// android
#if defined(ANDROID)
#undef  LS_TARGET_PLATFORM
#define LS_TARGET_PLATFORM         LS_PLATFORM_ANDROID
#endif

// win32
#if defined(_WIN32)
#undef  LS_TARGET_PLATFORM
#define LS_TARGET_PLATFORM         LS_PLATFORM_WIN32
#endif

// linux
#if defined(LINUX) && !defined(__APPLE__)
#undef  LS_TARGET_PLATFORM
#define LS_TARGET_PLATFORM         LS_PLATFORM_LINUX
#endif

// marmalade
#if defined(MARMALADE)
#undef  LS_TARGET_PLATFORM
#define LS_TARGET_PLATFORM         LS_PLATFORM_MARMALADE
#endif

// bada
#if defined(SHP)
#undef  LS_TARGET_PLATFORM
#define LS_TARGET_PLATFORM         LS_PLATFORM_BADA
#endif

// qnx
#if defined(__QNX__)
#undef  LS_TARGET_PLATFORM
#define LS_TARGET_PLATFORM     LS_PLATFORM_BLACKBERRY
#endif

// native client
#if defined(__native_client__)
#undef  LS_TARGET_PLATFORM
#define LS_TARGET_PLATFORM     LS_PLATFORM_NACL
#endif

// Emscripten
#if defined(EMSCRIPTEN)
#undef  LS_TARGET_PLATFORM
#define LS_TARGET_PLATFORM     LS_PLATFORM_EMSCRIPTEN
#endif

// tizen
#if defined(TIZEN)
#undef  LS_TARGET_PLATFORM
#define LS_TARGET_PLATFORM     LS_PLATFORM_TIZEN
#endif

// WinRT (Windows 8.1 Store/Phone App)
#if defined(WINRT)
#undef  LS_TARGET_PLATFORM
#define LS_TARGET_PLATFORM     LS_PLATFORM_WINRT
#endif

//////////////////////////////////////////////////////////////////////////
// post configure
//////////////////////////////////////////////////////////////////////////

// check user set platform
#if ! LS_TARGET_PLATFORM
#error  "Cannot recognize the target platform; are you targeting an unsupported platform?"
#endif 

#if (LS_TARGET_PLATFORM == LS_PLATFORM_WIN32)
#ifndef __MINGW32__
#pragma warning (disable:4127) 
#endif 
#endif  // LS_PLATFORM_WIN32


#ifdef __GNUC__
    #include "GNUAttributes.h"
#else
    #include "MSVCAttributes.h"
#endif

#if defined(__LP64__) || defined(_M_X64)
    #define LS_PLATFORM_64BITS 1
#else
    #define LS_PLATFORM_64BITS 0
#endif

#define LS_PLATFORM_32BITS (!LS_PLATFORM_64BITS)

#if !__GNUC__
    #define typeof(x) decltype((x))
#endif

#ifdef __GNUC__
    #define likely(x)       (((typeof(x))__builtin_expect(static_cast<long>(x), ~0l)))
    #define unlikely(x)     (((typeof(x))__builtin_expect(static_cast<long>(x), 0l)))
#else
    #define likely(x) x
    #define unlikely(x) x
#endif

#if defined(_WIN32) && defined(UNICODE)
#       define FPlatformDependedAPICall(x) _w##x
#       define FPlatformDependedSTDCall(x) std::w##x
#else
#   define FPlatformDependedAPICall(x) x
#   define FPlatformDependedSTDCall(x) std::x
#   define TEXT(x) x
#endif



