//
//  Header.h
//  PetBuddies
//
//  Created by void on 8/17/15.
//  Copyright (c) 2015 Catalyst Apps. All rights reserved.
//

#pragma once

#if LS_TARGET_PLATFORM == LS_PLATFORM_MAC
    #include "FMacMemory.h"
#elif LS_TARGET_PLATFORM == LS_PLATFORM_IOS
    #include "FIOSMemory.h"
#elif LS_TARGET_PLATFORM == LS_PLATFORM_WIN32
    #include "FWindowsMemory.h"
#elif LS_TARGET_PLATFORM == LS_PLATFORM_ANDROID
    #include "FAndroidMemory.h"
#endif

class FMemTag;

enum EMemAlignment
{
    // Default allocator alignment. If the default is specified, the allocator applies to engine rules.
    // Blocks >= 16 bytes will be 16-byte-aligned, Blocks < 16 will be 8-byte aligned. If the allocator does
    // not support allocation alignment, the alignment will be ignored
    DefaultAlignment = 0,

    // Minimum allocator alignment
    MinAlignment = 8,
};



class FMemory
{
public:
    _AttrWarnUnusedResult _AttrNoThrow _AttrMalloc
    static _AttrAlwaysInline void* systemMalloc(size_t count)
    {
        return ::malloc(count);
    }

    _AttrNonNull1 _AttrNoThrow
    static _AttrAlwaysInline void systemFree(void* ptr)
    {
        ::free(ptr);
    }

    _AttrNonNull1 _AttrNoThrow
    static _AttrAlwaysInline void* memmove(void* dest, const void* src, size_t count)
    {
        return FPlatformMemory_t::memmove(dest, src, count);
    }

    _AttrNonNull1 _AttrNonNull2 _AttrNoThrow
    static _AttrAlwaysInline int32_t memcmp(const void* in1, const void* in2, size_t count)
    {
        return FPlatformMemory_t::memcmp(in1, in2, count);
    }

    _AttrNonNull1
    static _AttrAlwaysInline void* memset(void* dest, uint8_t val, size_t count)
    {
        return FPlatformMemory_t::memset(dest, val, count);
    }

    _AttrNonNull1
    static _AttrAlwaysInline void* memzero(void* dest, size_t count)
    {
        return FPlatformMemory_t::memzero(dest, count);
    }

    _AttrNonNull1 _AttrNonNull2
    static _AttrAlwaysInline void* memcpy(void* dest, const void* src, size_t count)
    {
        return FPlatformMemory_t::memcpy(dest, src, count);
    }

    _AttrNonNull1 _AttrNonNull2
    static _AttrAlwaysInline void* memcpy_large(void* dest, const void* src, size_t count)
    {
        return FPlatformMemory_t::memcpy_large(dest, src, count);
    }

    _AttrWarnUnusedResult _AttrMalloc
    static void *malloc(size_t count, EMemAlignment alignment = DefaultAlignment);
    
    _AttrWarnUnusedResult _AttrMalloc
    static void *mallocUntracked(size_t count, EMemAlignment alignment = DefaultAlignment);

    _AttrWarnUnusedResult
    static void *realloc(void *origin, size_t newCount, EMemAlignment alignment = DefaultAlignment);

    static void free(void *origin);
    
    static void freeUntracked(void *origin);
    
    static void dumpMemoryLeaks();
    
    static void pushMemTag( const FMemTag * memTag);
    static void popMemTag( const FMemTag * memTag);

    template<typename T>
    class Deleter
    {
    public:
        void operator() (T* ptr) const
        {
            static_assert(0 < sizeof(T),
                          "can't delete an incomplete type");
            FMemory::free(ptr);
        }
    };

    template<typename T>
    class Deleter<T[]>
    {
    public:
        template<typename Oth>
        void operator()(Oth*) const = delete;

        void operator()(T* ptr) const
        {
            static_assert(0 < sizeof(T),
                          "can't delete an incomplete type");

            FMemory::free(ptr);
        }
    };
};

class FUseSysMalloc
{
public:
    void* operator new(size_t count)
    {
        return FMemory::systemMalloc(count);
    }
    
    void operator delete(void* ptr)
    {
        FMemory::systemFree(ptr);
    }
    
    void* operator new[](size_t count)
    {
        return FMemory::systemMalloc(count);
    }
    
    void operator delete[](void* ptr)
    {
        FMemory::systemFree(ptr);
    }
};


extern FMalloc* GMalloc;