#pragma once

#include "FGenericMemory.h"

class FAndroidMemory : public FGenericMemory
{
    static uint32_t pageSize;
public:
    static FMalloc* getAllocator();
    static void* binnedAllocFromOS(size_t size);
    static void binnedFreeToOS(void* ptr);
};

typedef FAndroidMemory FPlatformMemory_t;
