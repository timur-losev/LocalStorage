//
//  FMallocBinned.cpp
//  PetBuddies
//
//  Created by void on 8/17/15.
//  Copyright (c) 2015 Catalyst Apps. All rights reserved.
//

#include "CorePrecompiled.h"
#include "FMallocBinned.h"


FMallocBinned::FMallocBinned(uint32_t inPageSize, uint64_t addressLimit)
:	tableAddressLimit(addressLimit)
#ifdef USE_LOCKFREE_DELETE
,	PendingFreeList(nullptr)
,	bFlushingFrees(false)
,	bDoneFreeListInit(false)
#endif
,	hashBuckets(nullptr)
,	hashBucketFreeList(nullptr)
,	pageSize		(inPageSize)
#ifdef CACHE_FREED_OS_ALLOCS
,	freedPageBlocksNum(0)
,	cachedTotal(0)
#endif
#if STATS
,	osCurrent		( 0 )
,	osPeak			( 0 )
,	wasteCurrent	( 0 )
,	wastePeak		( 0 )
,	usedCurrent		( 0 )
,	usedPeak		( 0 )
,	currentAllocs	( 0 )
,	totalAllocs		( 0 )
,	slackCurrent	( 0 )
,	memTime			( 0.0 )
#endif
{
    FAssert(!(pageSize & (pageSize - 1)));
    FAssert(!(addressLimit & (addressLimit - 1)));

    FAssert(pageSize <= 65536); // There is internal limit on page size of 64k
    FAssert(addressLimit > pageSize); // Check to catch 32 bit overflow in AddressLimit

    /** Shift to get the reference from the indirect tables */
    poolBitShift = FGenericMath::ceilLogTwo(pageSize);
    indirectPoolBitShift = FGenericMath::ceilLogTwo(pageSize / sizeof(FPoolInfo));
    indirectPoolBlockSize = pageSize/sizeof(FPoolInfo);

    maxHashBuckets = addressLimit >> (indirectPoolBitShift + poolBitShift);
    maxHashBucketBits = FGenericMath::ceilLogTwo((uint32_t)maxHashBuckets);
    maxHashBucketWaste = (maxHashBuckets * sizeof(PoolHashBucket)) / 1024;
    maxBookKeepingOverhead = ((addressLimit / pageSize) * sizeof(PoolHashBucket)) / (1024*1024);

    /**
     * Shift required to get required hash table key.
     */
    hashKeyShift = poolBitShift + indirectPoolBitShift;
    /** Used to mask off the bits that have been used to lookup the indirect table */
    poolMask =  ( ( 1 << ( hashKeyShift - poolBitShift ) ) - 1 );
    binnedSizeLimit = page_size_limit / 2;
    binnedOSTableIndex = binnedSizeLimit + extened_page_pool_allocation_count;

    FAssert((binnedSizeLimit & (binnedSizeLimit - 1)) == 0);


    // Init tables.
    osTable.firstPool       = nullptr;
    osTable.exhaustedPool   = nullptr;
    osTable.blockSize = 0;

    /** The following options are not valid for page sizes less than 64k. They are here to reduce waste*/
    pagePoolTable[0].firstPool = nullptr;
    pagePoolTable[0].exhaustedPool = nullptr;
    pagePoolTable[0].blockSize = pageSize == page_size_limit ? (uint32_t)(binnedSizeLimit + (binnedSizeLimit / 2)) : 0;

    pagePoolTable[1].firstPool = nullptr;
    pagePoolTable[1].exhaustedPool = nullptr;
    pagePoolTable[1].blockSize = pageSize == page_size_limit ? pageSize + (uint32_t)binnedSizeLimit : 0;

    // Block sizes are based around getting the maximum amount of allocations per pool, with as little alignment waste as possible.
    // Block sizes should be close to even divisors of the POOL_SIZE, and well distributed. They must be 16-byte aligned as well.
    static const uint32_t blockSizes[PoolCount] =
    {
        8,		16,		32,		48,		64,		80,		96,		112,
        128,	160,	192,	224,	256,	288,	320,	384,
        448,	512,	576,	640,	704,	768,	896,	1024,
        1168,	1360,	1632,	2048,	2336,	2720,	3264,	4096,
        4672,	5456,	6544,	8192,	9360,	10912,	13104,	16384,
        21840,	32768
    };

    for( uint32_t i = 0; i < PoolCount; i++ )
    {
        poolTable[i].firstPool = nullptr;
        poolTable[i].exhaustedPool = nullptr;
        poolTable[i].blockSize = blockSizes[i];
#if STATS
        poolTable[i].minRequest = poolTable[i].blockSize;
#endif
    }

    for( uint32_t i = 0; i < max_pooled_allocation_size; i++ )
    {
        uint32_t index = 0;
        while( poolTable[index].blockSize < i )
        {
            ++index;
        }

        FAssert(index < PoolCount);
        memSizeToPoolTable[i] = &poolTable[index];
    }

    memSizeToPoolTable[binnedSizeLimit] = &pagePoolTable[0];
    memSizeToPoolTable[binnedSizeLimit + 1] = &pagePoolTable[1];
    
    FAssert(max_pooled_allocation_size - 1 == poolTable[PoolCount - 1].blockSize);
}

void* FMallocBinned::malloc(size_t size, uint32_t alignment)
{
#ifdef USE_COARSE_GRAIN_LOCKS
    FScopeLock ScopedLock(&AccessGuard);
#endif

    flushPendingFrees();

    // Handle DEFAULT_ALIGNMENT for binned allocator.
    if (alignment == DefaultAlignment)
    {
        alignment = default_binned_allocator_alignment;
    }

    FAssert(alignment <= pageSize);

    alignment = std::max<uint32_t>(alignment, default_binned_allocator_alignment);

    size = std::max<size_t>(alignment, FAlign(size, alignment));

    MEM_TIME(MemTime -= FPlatformTime::Seconds());

    STAT(currentAllocs++);
    STAT(totalAllocs++);

    FFreeMem* free = nullptr;

    if( size < binnedSizeLimit )
    {
        // Allocate from pool.
        FPoolTable* table = memSizeToPoolTable[size];
#ifdef USE_FINE_GRAIN_LOCKS
        std::lock_guard<std::mutex> tableLock(table->mutex);
#endif
        FAssert(size <= table->blockSize);

        trackStats(table, (uint32_t)size);

        FPoolInfo* pool = table->firstPool;
        if( !pool )
        {
            pool = allocatePoolMemory(table, binned_alloc_pool_size/*PageSize*/, size);
        }

        free = allocateBlockFromPool(table, pool);
    }
    else if ( ((size >= binnedSizeLimit && size <= pagePoolTable[0].blockSize) ||
               (size > pageSize && size <= pagePoolTable[1].blockSize))
             && alignment == default_binned_allocator_alignment )
    {
        // Bucket in a pool of 3*PageSize or 6*PageSize
        uint32_t binType = size < pageSize ? 0 : 1;
        uint32_t pageCount = 3 * binType + 3;

        FPoolTable* table = &pagePoolTable[binType];
#ifdef USE_FINE_GRAIN_LOCKS
        std::lock_guard<std::mutex> tableLock(table->mutex);
#endif
        FAssert(size <= table->blockSize);

        trackStats(table, (uint32_t)size);

        FPoolInfo* pool = table->firstPool;
        if( !pool )
        {
            pool = allocatePoolMemory(table, pageCount * pageSize, binnedSizeLimit + binType);
        }

        free = allocateBlockFromPool(table, pool);
    }
    else
    {
        // Use OS for large allocations.
        UIntPtr_t alignedSize = FAlign(size, pageSize);

        size_t actualPoolSize; //TODO: use this to reduce waste?
        free = (FFreeMem*)osAlloc(alignedSize, actualPoolSize);
        if( !free )
        {
            outOfMemory(alignedSize);
        }

        FAssert(!((size_t)free & (pageSize - 1)));

        // Create indirect.
        FPoolInfo* pool;
        {
#ifdef USE_FINE_GRAIN_LOCKS
            std::lock_guard<std::mutex> poolInfoLock(accessGuard);
#endif
            pool = getPoolInfo((UIntPtr_t)free);
        }

        pool->setAllocationSizes((uint32_t)size, alignedSize, (uint32_t)binnedOSTableIndex, (uint32_t)binnedOSTableIndex);

        STAT(osPeak = std::max(osPeak, osCurrent += alignedSize));
        STAT(usedPeak = std::max(usedPeak, usedCurrent += size));
        STAT(wastePeak = std::max(wastePeak, wasteCurrent += alignedSize - size));
    }
    
    MEM_TIME(MemTime += FPlatformTime::Seconds());
    return free;

}

void* FMallocBinned::realloc(void* origin, size_t newSize, uint32_t alignment)
{
    // Handle DefaultAlignment for binned allocator.
    if (alignment == DefaultAlignment)
    {
        alignment = default_binned_allocator_alignment;
    }

    FAssert(alignment <= pageSize);

    alignment = std::max<uint32_t>(alignment, default_binned_allocator_alignment);

    if (newSize)
    {
        newSize = std::max<size_t>(alignment, FAlign(newSize, alignment));
    }

    MEM_TIME(MemTime -= FPlatformTime::Seconds());

    UIntPtr_t basePtr;

    void* newPtr = origin;

    if( origin && newSize )
    {
        FPoolInfo* pool = findPoolInfo((UIntPtr_t)origin, basePtr);

        if( pool->tableIndex < binnedOSTableIndex )
        {
            // Allocated from pool, so grow or shrink if necessary.
            FAssert(pool->tableIndex > 0); // it isn't possible to allocate a size of 0, Malloc will increase the size to default_binned_allocator_alignment

            const uint32_t thisTableBlockSize = memSizeToPoolTable[pool->tableIndex]->blockSize;

            if( newSize > thisTableBlockSize || newSize <= memSizeToPoolTable[pool->tableIndex - 1]->blockSize )
            {
                newPtr = this->malloc( newSize, alignment );
                FMemory::memcpy( newPtr, origin, std::min<size_t>( newSize, thisTableBlockSize ) );
                this->free( origin );
            }
        }
        else
        {
            // Allocated from OS.
            FAssert(!((UIntPtr_t)origin & (pageSize-1)));

            if( newSize > pool->getOsBytes(pageSize, (int32_t)binnedOSTableIndex)
               || newSize * 3 < pool->getOsBytes(pageSize, (uint32_t)binnedOSTableIndex) * 2 )
            {
                // Grow or shrink.
                newPtr = this->malloc( newSize, alignment );
                FMemory::memcpy( newPtr, origin, std::min<size_t>(newSize, pool->getBytes()) );
                this->free( origin );
            }
            else
            {
                // Keep as-is, reallocation isn't worth the overhead.
                STAT(usedCurrent += newSize - pool->getBytes());
                STAT(usedPeak = std::max(usedPeak, usedCurrent));
                STAT(wasteCurrent += pool->getBytes() - newSize);
                pool->setAllocationSizes((uint32_t)newSize, pool->getOsBytes(pageSize, (uint32_t)binnedOSTableIndex), (uint32_t)binnedOSTableIndex, (uint32_t)binnedOSTableIndex);
            }
        }
    }
    else if( origin == nullptr )
    {
        newPtr = this->malloc( newSize, alignment );
    }
    else
    {
        this->free( origin );
        newPtr = nullptr;
    }
    
    MEM_TIME(MemTime += FPlatformTime::Seconds());
    return newPtr;
}

void FMallocBinned::free(void* origin)
{
    if( !origin )
    {
        return;
    }

    pushFreeLockless(origin);
}

bool FMallocBinned::getAllocationSize(void *origin, size_t &sizeOut)
{
    if (!origin)
    {
        return false;
    }

    UIntPtr_t basePtr;
    FPoolInfo* pool = findPoolInfo((UIntPtr_t)origin, basePtr);

    sizeOut = pool->tableIndex < binnedSizeLimit ? memSizeToPoolTable[pool->tableIndex]->blockSize : pool->getBytes();
    return true;
}

bool FMallocBinned::validateHeap()
{
#ifdef USE_COARSE_GRAIN_LOCKS
    FScopeLock ScopedLock(&AccessGuard);
#endif

    for( int32_t i = 0; i < PoolCount; i++ )
    {
        FPoolTable* table = &poolTable[i];
#ifdef USE_FINE_GRAIN_LOCKS
        std::lock_guard<std::mutex> tableLock(table->mutex);
#endif
        for( FPoolInfo** poolPtr = &table->firstPool; *poolPtr; poolPtr = &(*poolPtr)->next )
        {
            FPoolInfo* pool = *poolPtr;
            FAssert(pool->prevLink == poolPtr);
            FAssert(!!pool->firstMem);
            for( FFreeMem* free = pool->firstMem; free; free = free->next )
            {
                FAssert(free->numFreeBlocks > 0);
            }
        }

        for( FPoolInfo** poolPtr = &table->exhaustedPool; *poolPtr; poolPtr = &(*poolPtr)->next )
        {
            FPoolInfo* pool = *poolPtr;
            FAssert(pool->prevLink == poolPtr);
            FAssert(!pool->firstMem);
        }
    }

    return true;
}







