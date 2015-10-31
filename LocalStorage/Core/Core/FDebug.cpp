//
//  FDebug.cpp
//  PetBuddies
//
//  Created by void on 8/31/15.
//  Copyright (c) 2015 Catalyst Apps. All rights reserved.
//

#include "CorePrecompiled.h"
#include "FDebug.h"

void FDebug::abort(size_t line, long val)
{
    bug(line, val);
    ::abort();
}

void FDebug::bug(size_t line, long val)
{
    static void* lastSeen = nullptr;

    void* retAddr = FReturnAddress(0);

    if (lastSeen != retAddr)
    {
        lastSeen = retAddr;

        FLogCritical(("BUG : %1% - %2%") % line % val);
    }
}