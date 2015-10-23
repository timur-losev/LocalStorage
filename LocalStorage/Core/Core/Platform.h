//
//  Platform.h
//  Core
//
//  Created by void on 10/23/15.
//  Copyright © 2015 Timur Losev. All rights reserved.
//

#pragma once


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

#define PB_PLATFORM_32BITS (!PB_PLATFORM_64BITS)

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
#       ifndef TEXT
#           define TEXT(x) L##x
#       endif
#else
#   define FPlatformDependedAPICall(x) x
#   define FPlatformDependedSTDCall(x) std::x
#   define TEXT(x) x
#endif



