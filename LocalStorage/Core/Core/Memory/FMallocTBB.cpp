//
//  FMemoryTBB.cpp
//  PetBuddies
//
//  Created by void on 8/17/15.
//  Copyright (c) 2015 Catalyst Apps. All rights reserved.
//

#include "Precompiled.h"
#include "FMallocTBB.h"

#include <tbb/scalable_allocator.h>

#define FreedMarker (0xdd)
#define NewMarker (0xcd)


void *FMallocTBB::malloc(size_t count, uint32_t alignment)
{
    void* retVal = nullptr;

    //TODO: record malloc
    if (alignment != DefaultAlignment)
    {
        alignment = getAlignement(count, alignment);
        retVal = scalable_aligned_malloc(count, alignment);
    }
    else
    {
        retVal = scalable_malloc(count);
    }

    //TODO: check ptr

#if LS_DEBUG_MODE
    if (count)
    {
        FMemory::memset(retVal, NewMarker, count);
    }
#endif

    return retVal;
}

void *FMallocTBB::realloc(void *origin, size_t newCount, uint32_t alignment)
{
#if LS_DEBUG_MODE
    size_t oldCount = 0;
    if (likely(origin != nullptr))
    {
        oldCount = scalable_msize(origin);
        if (newCount < oldCount)
        {
            FMemory::memset((uint8_t*)origin + newCount, NewMarker, oldCount - newCount);
        }
    }
#endif //LS_DEBUG_MODE

    //TODO: record realloc

    void* retval = nullptr;

    if (alignment != DefaultAlignment)
    {
        alignment = (EMemAlignment) getAlignement(newCount, alignment);
        retval = scalable_aligned_realloc(origin, newCount, (size_t)alignment);
    }
    else
    {
        retval = scalable_realloc(origin, newCount);
    }

#if LS_DEBUG_MODE
    if (retval && newCount > oldCount)
    {
        FMemory::memset((uint8_t*)retval + oldCount, NewMarker, newCount - oldCount);
    }
#endif //LS_DEBUG_MODE

    //TODO: check retval

    return retval;
}

void FMallocTBB::free(void *origin)
{
    if (unlikely(!origin))
    {
        return;
    }

#if LS_DEBUG_MODE
    FMemory::memset(origin, FreedMarker, scalable_msize(origin));
#endif //LS_DEBUG_MODE

    scalable_free(origin);
}