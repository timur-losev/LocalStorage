//
//  FMacMemory.h
//  PetBuddies
//
//  Created by void on 8/17/15.
//  Copyright (c) 2015 Catalyst Apps. All rights reserved.
//

#pragma once

#include "FGenericMemory.h"

class FMacMemory : public FGenericMemory
{
public:
    FMacMemory();

    ~FMacMemory();

public:

    static void init();
    static FMalloc* getAllocator();

    static void* binnedAllocFromOS( size_t size );
    static void binnedFreeToOS( void* ptr );

};

typedef FMacMemory FPlatformMemory_t;
