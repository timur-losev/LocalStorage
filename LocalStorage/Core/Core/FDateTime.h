//
//  FDateTime.h
//  PetBuddies
//
//  Created by void on 8/26/15.
//  Copyright (c) 2015 Catalyst Apps. All rights reserved.
//

#pragma once

namespace FPosixTime = boost::posix_time;

typedef FPosixTime::ptime FTime_t;
typedef FPosixTime::time_duration FTimeDuration_t;
typedef FPosixTime::microsec_clock FMicrosecClock_t;
namespace FChrono = CHRONO_PROVIDER::chrono;
