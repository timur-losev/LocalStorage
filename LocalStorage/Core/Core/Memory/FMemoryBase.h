//
//  FMemoryBase.h
//  PetBuddies
//
//  Created by void on 8/17/15.
//  Copyright (c) 2015 Catalyst Apps. All rights reserved.
//

#pragma once

#include "FMemory.h"
#include "FAllocationsTracker.h"

class FMalloc: public FUseSysMalloc
{
protected:

    static _AttrAlwaysInline uint32_t getAlignement(size_t count, uint32_t alignment)
    {
        return std::max(count >= 16 ? (uint32_t)16 : (uint32_t)8, alignment);
    }
    
    FAllocationsTracker m_allocationsTracker;

public:
 
    virtual void* malloc(size_t count, uint32_t alignment = DefaultAlignment) = 0;
    virtual void* realloc(void* origin, size_t newCount, uint32_t alignment = DefaultAlignment) = 0;
    virtual void free(void* origin) = 0;

    virtual bool getAllocationSize(void *origin, size_t& sizeOut)
    {
        sizeOut = 0;
        return false;
    }

    virtual bool validateHeap()
    {
        return true;
    }
    
    FAllocationsTracker& getAllocationsTracker()
    {
        return m_allocationsTracker;
    }
};

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_MAC
// Malloc is aligned on Apple platforms:
// https://developer.apple.com/library/mac/documentation/Darwin/Reference/ManPages/man3/malloc.3.html

#define _aligned_malloc(__size, __align) ::malloc(__size)
#define _aligned_realloc(__ptr, __size, __align) ::realloc(__ptr, __size)
#define _aligned_free(__ptr) ::free(__ptr)
#define _aligned_msize(__ptr, __align, __offset) ::malloc_size(__ptr)

#define MallocIsAligned 1
#elif CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID

//XXX: is malloc aligned on Android?
#define _aligned_malloc(__size, __align) ::malloc(__size)
#define _aligned_realloc(__ptr, __size, __align) ::realloc(__ptr, __size)
#define _aligned_free(__ptr) ::free(__ptr)

#define MallocIsAligned 1
#endif