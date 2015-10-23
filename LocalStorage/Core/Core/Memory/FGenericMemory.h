//
//  FGenericMemory.h
//  PetBuddies
//
//  Created by void on 8/17/15.
//  Copyright (c) 2015 Catalyst Apps. All rights reserved.
//

#pragma once

class FMalloc;

class FGenericMemory
{
protected:
public:

    FGenericMemory();

    virtual ~FGenericMemory();

    static void init();
    static FMalloc* getAllocator();

    _AttrWarnUnusedResult _AttrNonNull1 _AttrNonNull2
        static _AttrAlwaysInline void* memmove(void* dest, const void* src, size_t count)
    {
        return ::memmove(dest, src, count);
    }

    _AttrNonNull1 _AttrNonNull2
        static _AttrAlwaysInline int32_t memcmp(const void* in1, const void* in2, size_t count)
    {
        return ::memcmp(in1, in2, count);
    }

    _AttrNonNull1
        static _AttrAlwaysInline void* memset(void* dest, uint8_t val, size_t count)
    {
        return ::memset(dest, val, count);
    }

    _AttrNonNull1
        static _AttrAlwaysInline void* memzero(void* dest, size_t count)
    {
        return ::memset(dest, 0, count);
    }

    _AttrNonNull1 _AttrNonNull2
        static _AttrAlwaysInline void* memcpy(void* dest, const void* src, size_t count)
    {
        return ::memcpy(dest, src, count);
    }

    _AttrNonNull1 _AttrNonNull2
        static _AttrAlwaysInline void* memcpy_large(void* dest, const void* src, size_t count)
    {
        return ::memcpy(dest, src, count);
    }

    _AttrAlwaysInline
    static void* binnedAllocFromOS(size_t size)
    {
        return nullptr;
    }

    _AttrAlwaysInline
    static void binnedFreeToOS(void* ptr)
    {

    }

};