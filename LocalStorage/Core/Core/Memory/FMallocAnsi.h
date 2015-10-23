//
//  FMallocAnsi.h
//  PetBuddies
//
//  Created by void on 8/17/15.
//  Copyright (c) 2015 Catalyst Apps. All rights reserved.
//

#pragma once

#include "FMemoryBase.h"

class FMallocAnsi : public FMalloc
{
public:
    _AttrWarnUnusedResult
    virtual void* malloc(size_t count, uint32_t alignment = DefaultAlignment) override
    {
        //TODO: record malloc
        alignment = (EMemAlignment) getAlignement(count, alignment);
        void* result = _aligned_malloc(count, (uint32_t)alignment);

        //TODO: check result
        return result;
    }

    _AttrWarnUnusedResult
    virtual void* realloc(void* origin, size_t newCount, uint32_t alignment = DefaultAlignment) override
    {
        //TODO: record realloc
        alignment = (EMemAlignment) getAlignement(newCount, alignment);

        void* result = nullptr;
        if (origin && newCount)
        {
            result = _aligned_realloc(origin, newCount, alignment);
        }
        else if (!origin)
        {
            result = _aligned_malloc(newCount, alignment);
        }
        else
        {
            _aligned_free(result);
        }

        //TODO: check result
        return result;
    }

    virtual void free(void* origin) override
    {
        _aligned_free(origin);
    }
    
};
