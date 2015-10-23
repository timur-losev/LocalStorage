//
//  FDebug.h
//  PetBuddies
//
//  Created by void on 8/31/15.
//  Copyright (c) 2015 Catalyst Apps. All rights reserved.
//

#pragma once

class FDebug
{
private:
    static void bug(size_t ling, long val);
public:

    static void abort(size_t line, long val);
};