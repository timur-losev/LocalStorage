//
//  FIOSMemory.cpp
//  PetBuddies
//
//  Created by void on 8/17/15.
//  Copyright (c) 2015 Catalyst Apps. All rights reserved.
//

#include "Precompiled.h"
#include "FIOSMemory.h"
#include "FMallocBinned.h"
#include "FGenericMath.h"

#include "FMallocAnsi.h"

#include <mach/mach.h>

void FIOSMemory::init()
{

}

FMalloc* FIOSMemory::getAllocator()
{
    // get free memory
#if 1
    vm_size_t pageSize;
    host_page_size(mach_host_self(), &pageSize);

    vm_statistics stats;
    mach_msg_type_number_t statsSize = sizeof(stats);
    host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&stats, &statsSize);
    // 1 << FGenericMath::ceilLogTwo(MemoryConstants.TotalPhysical) should really be FMath::RoundUpToPowerOfTwo,
    // but that overflows to 0 when MemoryConstants.TotalPhysical is close to 4GB, since CeilLogTwo returns 32
    // this then causes the MemoryLimit to be 0 and crashing the app
    uint32_t limitOne = FGenericMath::ceilLogTwo(uint32_t(stats.free_count * pageSize));

    uint64_t memoryLimit = std::min<uint64_t>( uint64_t(1) << limitOne, 0x100000000);
    
    return new FMallocBinned((uint32_t)pageSize, memoryLimit);
#else
    return new FMallocAnsi();
#endif
}

void* FIOSMemory::binnedAllocFromOS( size_t size )
{
    return valloc(size);
}

void FIOSMemory::binnedFreeToOS( void* ptr )
{
    free(ptr);
}


