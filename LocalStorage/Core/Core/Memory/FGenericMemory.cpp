//
//  FGenericMemory.cpp
//  PetBuddies
//
//  Created by void on 8/17/15.
//  Copyright (c) 2015 Catalyst Apps. All rights reserved.
//

#include "CorePrecompiled.h"
#include "FGenericMemory.h"
#include "FMallocAnsi.h"
#include "FAllocationsTracker.h"

FMalloc* FGenericMemory::getAllocator()
{
    return new FMallocAnsi();
}

FGenericMemory::FGenericMemory()
{

}

FGenericMemory::~FGenericMemory()
{

}

void FGenericMemory::init()
{
    
}
