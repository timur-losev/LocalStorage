//
//  FMemoryTBB.h
//  PetBuddies
//
//  Created by void on 8/17/15.
//  Copyright (c) 2015 Catalyst Apps. All rights reserved.
//

#pragma once

#include "FMemoryBase.h"

class FMallocTBB : public FMalloc
{
public:

    _AttrWarnUnusedResult
    virtual void *malloc(size_t count, uint32_t alignment = DefaultAlignment) override;

    _AttrWarnUnusedResult
    virtual void *realloc(void *origin, size_t newCount, uint32_t alignment = DefaultAlignment) override;

    virtual void free(void *origin) override;
};