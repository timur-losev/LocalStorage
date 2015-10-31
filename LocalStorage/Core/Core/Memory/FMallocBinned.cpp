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

_AttrAlwaysInline
void FMallocBinned::trackStats(FPoolTable* table, uint32_t size)
{
#if STATS
    // keep track of memory lost to padding
    table->totalWaste += table->blockSize - size;
    table->totalRequests++;
    table->activeRequests++;
    table->maxActiveRequests = std::max(table->maxActiveRequests, table->activeRequests);
    table->maxRequest = size > table->maxRequest ? size : table->maxRequest;
    table->minRequest = size < table->minRequest ? size : table->minRequest;
#endif
}

FMallocBinned::FPoolInfo* FMallocBinned::createIndirect()
{
    FAssert(indirectPoolBlockSize * sizeof(FPoolInfo) <= pageSize);

    FPoolInfo* indirect = (FPoolInfo*)FPlatformMemory_t::binnedAllocFromOS(indirectPoolBlockSize * sizeof(FPoolInfo));
    if (unlikely(!indirect))
    {
        outOfMemory(indirectPoolBlockSize * sizeof(FPoolInfo));
    }

    FMemory::memset(indirect, 0, indirectPoolBlockSize*sizeof(FPoolInfo));

#if STATS
    osPeak = std::max(osPeak, osCurrent += FAlign(indirectPoolBlockSize * sizeof(FPoolInfo), pageSize));
    wastePeak = std::max(wastePeak, wasteCurrent += FAlign(indirectPoolBlockSize * sizeof(FPoolInfo), pageSize));
#endif
    return indirect;
}

_AttrAlwaysInline
FMallocBinned::FPoolInfo* FMallocBinned::getPoolInfo(UIntPtr_t ptr)
{
    if (!hashBuckets)
    {
        // Init tables.
        hashBuckets = (PoolHashBucket*)FPlatformMemory_t::binnedAllocFromOS(FAlign(maxHashBuckets * sizeof(PoolHashBucket), pageSize));

        for (uint32_t i = 0; i < maxHashBuckets; ++i)
        {
            new (hashBuckets + i) PoolHashBucket();
        }
    }

    UIntPtr_t key = ptr >> hashKeyShift;
    UIntPtr_t hash = key & (maxHashBuckets - 1);
    UIntPtr_t poolIndex = ((UIntPtr_t)ptr >> poolBitShift) & poolMask;

    PoolHashBucket* collision = &hashBuckets[hash];
    do
    {
        if (collision->key == key || !collision->firstPool)
        {
            if (!collision->firstPool)
            {
                collision->key = key;
                initializeHashBucket(collision);
                FAssert(collision->firstPool != nullptr);
            }
            return &collision->firstPool[poolIndex];
        }

        collision = collision->next;

    } while (collision != &hashBuckets[hash]);

    //Create a new hash bucket entry
    PoolHashBucket* newBucket = createHashBucket();
    newBucket->key = key;
    hashBuckets[hash].link(newBucket);
    return &newBucket->firstPool[poolIndex];
}

_AttrAlwaysInline
FMallocBinned::FPoolInfo* FMallocBinned::findPoolInfo(UIntPtr_t ptr1, UIntPtr_t& allocationBase)
{
    uint16_t nextStep = 0;

    UIntPtr_t ptr = ptr1 &~((UIntPtr_t)pageSize - 1);

    for (uint32_t i = 0, n = (binned_alloc_pool_size / pageSize) + 1; i < n; ++i)
    {
        FPoolInfo* pool = findPoolInfoInternal(ptr, nextStep);
        if (pool)
        {
            allocationBase = ptr;
            //checkSlow(Ptr1 >= AllocationBase && Ptr1 < AllocationBase+Pool->GetBytes());
            return pool;
        }

        ptr = ((ptr - (pageSize * nextStep)) - 1) &~((UIntPtr_t)pageSize - 1);
    }
    allocationBase = 0;
    return nullptr;
}

_AttrAlwaysInline
FMallocBinned::FPoolInfo* FMallocBinned::findPoolInfoInternal(UIntPtr_t ptr, uint16_t& jumpOffset)
{
    FAssert(!!hashBuckets);

    UIntPtr_t key = ptr >> hashKeyShift;
    UIntPtr_t hash = key & (maxHashBuckets - 1);
    UIntPtr_t poolIndex = ((UIntPtr_t)ptr >> poolBitShift) & poolMask;
    jumpOffset = 0;

    PoolHashBucket* collision = &hashBuckets[hash];
    do
    {
        if (collision->key == key)
        {
            if (!collision->firstPool[poolIndex].allocSize)
            {
                jumpOffset = collision->firstPool[poolIndex].tableIndex;
                return nullptr;
            }
            return &collision->firstPool[poolIndex];
        }
        collision = collision->next;

    } while (collision != &hashBuckets[hash]);

    return nullptr;
}

_AttrAlwaysInline
FMallocBinned::PoolHashBucket* FMallocBinned::createHashBucket()
{
    PoolHashBucket* bucket = allocateHashBucket();
    initializeHashBucket(bucket);
    return bucket;
}

_AttrAlwaysInline
void FMallocBinned::initializeHashBucket(PoolHashBucket* bucket)
{
    if (!bucket->firstPool)
    {
        bucket->firstPool = createIndirect();
    }
}

FMallocBinned::PoolHashBucket* FMallocBinned::allocateHashBucket()
{
    if (!hashBucketFreeList)
    {
        hashBucketFreeList = (PoolHashBucket*)FPlatformMemory_t::binnedAllocFromOS(pageSize);

#if STATS
        osPeak = std::max(osPeak, osCurrent += pageSize);
        wastePeak = std::max(wastePeak, wasteCurrent += pageSize);
#endif
        for (UIntPtr_t i = 0, n = (pageSize / sizeof(PoolHashBucket)); i < n; ++i)
        {
            hashBucketFreeList->link(new (hashBucketFreeList + i) PoolHashBucket());
        }
    }

    PoolHashBucket* nextFree = hashBucketFreeList->next;
    PoolHashBucket* free = hashBucketFreeList;

    free->unlink();

    if (nextFree == free)
    {
        nextFree = nullptr;
    }

    hashBucketFreeList = nextFree;
    return free;
}

_AttrAlwaysInline
FMallocBinned::FPoolInfo* FMallocBinned::allocatePoolMemory(FPoolTable* table, uint32_t poolSize, uint16_t tableIndex)
{
    // Must create a new pool.
    uint32_t blocks = poolSize / table->blockSize;
    uint32_t bytes = blocks * table->blockSize;

    UIntPtr_t osBytes = FAlign(bytes, pageSize);

    FAssert(blocks >= 1);
    FAssert(blocks * table->blockSize <= bytes && poolSize >= bytes);

    // Allocate memory.
    FFreeMem* free = nullptr;

    size_t actualPoolSize; //TODO: use this to reduce waste?
    free = (FFreeMem*)osAlloc(osBytes, actualPoolSize);

    FAssert(!((UIntPtr_t)free & (pageSize - 1)));

    if (!free)
    {
        outOfMemory(osBytes);
    }

    // Create pool in the indirect table.
    FPoolInfo* pool;
    {
#ifdef USE_FINE_GRAIN_LOCKS
        std::lock_guard<std::mutex> poolInfoLock(accessGuard);
#endif
        pool = getPoolInfo((UIntPtr_t)free);

        for (UIntPtr_t i = (UIntPtr_t)pageSize, offset = 0; i < osBytes; i += pageSize, ++offset)
        {
            FPoolInfo* trailingPool = getPoolInfo(((UIntPtr_t)free) + i);

            FAssert(!!trailingPool);

            //Set trailing pools to point back to first pool
            trailingPool->setAllocationSizes(0, 0, (uint32_t)offset, (uint32_t)binnedOSTableIndex);
        }
    }

    // Init pool.
    pool->link(table->firstPool);
    pool->setAllocationSizes(bytes, (uint32_t)osBytes, tableIndex, (uint32_t)binnedOSTableIndex);

#if STATS
    osPeak = std::max(osPeak, osCurrent += osBytes);
    wastePeak = std::max(wastePeak, wasteCurrent += osBytes - bytes);
    pool->taken = 0;
    pool->firstMem = free;

    table->numActivePools++;
    table->maxActivePools = std::max(table->maxActivePools, table->numActivePools);
#endif
    // Create first free item.
    free->numFreeBlocks = blocks;
    free->next = nullptr;

    return pool;
}

_AttrAlwaysInline
FMallocBinned::FFreeMem* FMallocBinned::allocateBlockFromPool(FPoolTable* table, FPoolInfo* pool)
{
    // Pick first available block and unlink it.
    pool->taken++;

    FAssert(pool->tableIndex < binnedOSTableIndex); // if this is false, FirstMem is actually a size not a pointer
    FAssert(!!pool->firstMem);
    FAssert(pool->firstMem->numFreeBlocks > 0);
    FAssert(pool->firstMem->numFreeBlocks < page_size_limit);

    FFreeMem* free = (FFreeMem*)((uint8_t*)pool->firstMem + --pool->firstMem->numFreeBlocks * table->blockSize);

    if (!pool->firstMem->numFreeBlocks)
    {
        pool->firstMem = pool->firstMem->next;
        if (!pool->firstMem)
        {
            // Move to exhausted list.
            pool->unlink();
            pool->link(table->exhaustedPool);
        }
    }
#if STATS
    usedPeak = std::max(usedPeak, usedCurrent += table->blockSize);
#endif
    return free;
}

void FMallocBinned::freeInternal(void* ptr)
{
    MEM_TIME(memTime -= FPlatformTime::Seconds());
    STAT(currentAllocs--);

    UIntPtr_t basePtr;
    FPoolInfo* pool = findPoolInfo((UIntPtr_t)ptr, basePtr);

#if LS_TARGET_PLATFORM == LS_PLATFORM_IOS
    if (pool == nullptr)
    {
        FLogWarning(("Memory : Attempting to free a pointer we didn't allocate"));
        return;
    }
#endif
    FAssert(!!pool);
    FAssert(pool->getBytes() != 0);

    if (pool->tableIndex < binnedOSTableIndex)
    {
        FPoolTable* table = memSizeToPoolTable[pool->tableIndex];
#ifdef USE_FINE_GRAIN_LOCKS
        std::lock_guard<std::mutex> tableLock(table->mutex);
#endif

        STAT(table->activeRequests--);

        // If this pool was exhausted, move to available list.
        if (!pool->firstMem)
        {
            pool->unlink();
            pool->link(table->firstPool);
        }

        // Free a pooled allocation.
        FFreeMem* free = (FFreeMem*)ptr;
        free->numFreeBlocks = 1;
        free->next = pool->firstMem;
        pool->firstMem = free;

        STAT(usedCurrent -= table->blockSize);

        // Free this pool.
        FAssert(pool->taken >= 1);

        if (--pool->taken == 0)
        {
#if STATS
            table->numActivePools--;
#endif
            // Free the OS memory.
            size_t osBytes = pool->getOsBytes(pageSize, (uint32_t)binnedOSTableIndex);

            STAT(osCurrent -= osBytes);
            STAT(wasteCurrent -= osBytes - pool->getBytes());

            pool->unlink();
            pool->setAllocationSizes(0, 0, 0, (uint32_t)binnedOSTableIndex);

            osFree((void*)basePtr, osBytes);
        }
    }
    else
    {
        // Free an OS allocation.
        FAssert(!((UIntPtr_t)ptr & (pageSize - 1)));

        size_t osBytes = pool->getOsBytes(pageSize, (uint32_t)binnedOSTableIndex);

        STAT(usedCurrent -= pool->getBytes());
        STAT(osCurrent -= osBytes);
        STAT(wasteCurrent -= osBytes - pool->getBytes());

        osFree(ptr, osBytes);
    }

    MEM_TIME(MemTime += FPlatformTime::Seconds());
}

void FMallocBinned::pushFreeLockless(void* origin)
{
#ifdef USE_LOCKFREE_DELETE
    PendingFreeList->Push(Ptr);
#else
#ifdef USE_COARSE_GRAIN_LOCKS
    FScopeLock ScopedLock(&AccessGuard);
#endif
    freeInternal(origin);
#endif
}

void FMallocBinned::flushPendingFrees()
{
#ifdef USE_LOCKFREE_DELETE
    if (!PendingFreeList && !bDoneFreeListInit)
    {
        bDoneFreeListInit = true;
        PendingFreeList = new ((void*)PendingFreeListMemory) TLockFreePointerList<void>();
    }
    // Because a lockless list and TArray calls new/malloc internally, need to guard against re-entry
    if (bFlushingFrees || !PendingFreeList)
    {
        return;
    }
    bFlushingFrees = true;
    PendingFreeList->PopAll(FlushedFrees);
    for (uint32 i = 0, n = FlushedFrees.Num(); i < n; ++i)
    {
        FreeInternal(FlushedFrees[i]);
    }
    FlushedFrees.Reset();
    bFlushingFrees = false;
#endif
}

_AttrAlwaysInline
void FMallocBinned::osFree(void* ptr, size_t size)
{
#ifdef CACHE_FREED_OS_ALLOCS
#ifdef USE_FINE_GRAIN_LOCKS
    std::lock_guard<std::mutex> mainLock(accessGuard);
#endif
    if ((cachedTotal + size > MAX_CACHED_OS_FREES_BYTE_LIMIT) || (size > binned_alloc_pool_size))
    {
        FPlatformMemory_t::binnedFreeToOS(ptr);
        return;
    }

    if (freedPageBlocksNum >= MAX_CACHED_OS_FREES)
    {
        //Remove the oldest one
        void* freePtr = freedPageBlocks[freedPageBlocksNum - 1].ptr;
        cachedTotal -= freedPageBlocks[freedPageBlocksNum - 1].byteSize;
        --freedPageBlocksNum;
        FPlatformMemory_t::binnedFreeToOS(freePtr);
    }

    freedPageBlocks[freedPageBlocksNum].ptr = ptr;
    freedPageBlocks[freedPageBlocksNum].byteSize = size;

    cachedTotal += size;
    ++freedPageBlocksNum;
#else
    (void)size;
    FPlatformMemory_t::binnedFreeToOS(ptr);
#endif
}

_AttrAlwaysInline
void* FMallocBinned::osAlloc(size_t newSize, size_t& outActualSize)
{
#ifdef CACHE_FREED_OS_ALLOCS
{
#ifdef USE_FINE_GRAIN_LOCKS
    // We want to hold the lock a little as possible so release it
    // before the big call to the OS
    std::lock_guard<std::mutex> mainLock(accessGuard);
#endif
    for (uint32_t i = 0; i < freedPageBlocksNum; ++i)
    {
        // is it possible (and worth i.e. <25% overhead) to use this block
        if (freedPageBlocks[i].byteSize >= newSize && freedPageBlocks[i].byteSize * 3 <= newSize * 4)
        {
            void* ret = freedPageBlocks[i].ptr;

            outActualSize = freedPageBlocks[i].byteSize;

            cachedTotal -= freedPageBlocks[i].byteSize;

            freedPageBlocks[i] = freedPageBlocks[--freedPageBlocksNum];
            return ret;
        }
    };
}

outActualSize = newSize;

void* ptr = FPlatformMemory_t::binnedAllocFromOS(newSize);

if (!ptr)
{
    //Are we holding on to much mem? Release it all.
    flushAllocCache();
    ptr = FPlatformMemory_t::binnedAllocFromOS(newSize);
}

return ptr;
#else
    (void)outActualSize;
    return FPlatformMemory_t::binnedAllocFromOS(NewSize);
#endif
}

void FMallocBinned::flushAllocCache()
{
#ifdef USE_FINE_GRAIN_LOCKS
    std::lock_guard<std::mutex> mainLock(accessGuard);
#endif
    for (int i = 0, n = freedPageBlocksNum; i < n; ++i)
    {
        //Remove allocs
        FPlatformMemory_t::binnedFreeToOS(freedPageBlocks[i].ptr);
        freedPageBlocks[i].ptr = nullptr;
        freedPageBlocks[i].byteSize = 0;
    }

    freedPageBlocksNum = 0;
    cachedTotal = 0;
}

