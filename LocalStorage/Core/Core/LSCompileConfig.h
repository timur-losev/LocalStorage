//
//  LSCompileConfig.h
//  Core
//
//  Created by void on 10/23/15.
//  Copyright © 2015 Timur Losev. All rights reserved.
//

#pragma once

#define THREAD_PROVIDER_BOOST   1
#define THREAD_PROVIDER_STD     2

#define SHARED_POINTER_PROVIDER_BOOST   1
#define SHARED_POINTER_PROVIDER_STD     2

#define CURRENT_THREAD_PROVIDER THREAD_PROVIDER_STD
#define CURRENT_SHARED_POINTER_PROVIDER SHARED_POINTER_PROVIDER_STD

#if CURRENT_THREAD_PROVIDER == THREAD_PROVIDER_STD
    #define THREAD_PROVIDER std
#else
    #define THREAD_PROVIDER boost
#endif

#if CURRENT_SHARED_POINTER_PROVIDER == SHARED_POINTER_PROVIDER_STD
    #define SHARED_POINTER_PROVIDER std
#else
    #define SHARED_POINTER_PROVIDER boost
#endif

#define ENABLE_FINE_MEM_TRACKING 0

#define LS_DEBUG_MODE 1
#define LS_USE_BOOST_CONTAINERS 0