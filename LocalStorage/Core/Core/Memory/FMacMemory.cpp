//
//  FMacMemory.cpp
//  PetBuddies
//
//  Created by void on 8/17/15.
//  Copyright (c) 2015 Catalyst Apps. All rights reserved.
//

#include "Precompiled.h"
#include "FMacMemory.h"

#include "FMallocTBB.h"

FMacMemory::FMacMemory()
{

}

FMacMemory::~FMacMemory()
{

}

void FMacMemory::init()
{

}

FMalloc* FMacMemory::getAllocator()
{
    return new FMallocTBB();
}

void* FMacMemory::binnedAllocFromOS( size_t size )
{
    return valloc(size);
}

void FMacMemory::binnedFreeToOS( void* ptr )
{
    free(ptr);
}
