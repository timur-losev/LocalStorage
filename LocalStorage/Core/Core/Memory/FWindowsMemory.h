#pragma once

#include "FGenericMemory.h"

class FWindowsMemory : public FGenericMemory
{
public:
    static FMalloc* getAllocator();
    static void* binnedAllocFromOS(size_t size);
    static void binnedFreeToOS(void* ptr);
};

typedef /*FWindowsMemory */ FGenericMemory FPlatformMemory_t;