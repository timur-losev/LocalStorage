//
//  FBSONElement.cpp
//  bson
//
//  Created by void on 9/4/15.
//  Copyright (c) 2015 Timur Losev. All rights reserved.
//
#include "Precompiled.h"
#include "FBSONelement.h"
#include "FBSONobj.h"

namespace _bson {
    FBSONElement FBSONElement::operator[](const std::string &field) const
    {
        FBSONObj o = Obj();
        return o[field];
    }
}