//
//  FMallocBinned.h
//  PetBuddies
//
//  Created by void on 8/17/15.
//  Copyright (c) 2015 Catalyst Apps. All rights reserved.
//

#pragma once

#include "FMemoryBase.h"
#include "Core/Math/FGenericMath.h"


#define MEM_TIME(st)

//#define USE_LOCKFREE_DELETE
#define USE_INTERNAL_LOCKS
#define CACHE_FREED_OS_ALLOCS

#ifdef USE_INTERNAL_LOCKS
//#	define USE_COARSE_GRAIN_LOCKS
#endif

#if defined USE_LOCKFREE_DELETE
#	define USE_INTERNAL_LOCKS
#	define USE_COARSE_GRAIN_LOCKS
#endif

#if defined CACHE_FREED_OS_ALLOCS
#	define MAX_CACHED_OS_FREES (32)
#	define MAX_CACHED_OS_FREES_BYTE_LIMIT (4*1024*1024)
#endif

#if defined USE_INTERNAL_LOCKS && !defined USE_COARSE_GRAIN_LOCKS
#	define USE_FINE_GRAIN_LOCKS
#endif

#define STATS 1

#if STATS
#define STAT(x) x
#else
#define STAT(x)
#endif

class FMallocBinned : public FMalloc
{
private:
    // Counts.
    enum { PoolCount = 42 };

    /** Maximum allocation for the pooled allocator */
    enum { extened_page_pool_allocation_count = 2 };
    enum { max_pooled_allocation_size   = 32768+1 };
    enum { page_size_limit = 65536 };
    // binned_alloc_pool_size can be increased beyond 64k to cause binned malloc to allocate
    // the small size bins in bigger chunks. if os allocation is slow, increasing
    // this number *may* help performance but ymmv.
    enum { binned_alloc_pool_size = 65536 };

    // Forward declares.
    struct FFreeMem;
    struct FPoolTable;

    // Memory pool info. 32 bytes.
    struct FPoolInfo
    {
        /** Number of allocated elements in this pool, when counts down to zero can free the entire pool. */
        uint16_t			taken;		// 2
        /** Index of pool. Index into MemSizeToPoolTable[]. Valid when < max_pooled_allocation_size, max_pooled_allocation_size is OsTable.
         When AllocSize is 0, this is the number of pages to step back to find the base address of an allocation. See FindPoolInfoInternal()
         */
        uint16_t			tableIndex; // 4
        /** Number of bytes allocated */
        uint32_t			allocSize;	// 8
        /** Pointer to first free memory in this pool or the OS Allocation Size in bytes if this allocation is not binned*/
        FFreeMem*		firstMem;   // 12/16
        FPoolInfo*		next;		// 16/24
        FPoolInfo**		prevLink;	// 20/32
#if LS_PLATFORM_32BITS
        /** Explicit padding for 32 bit builds */
        uint8_t padding[12]; // 32
#endif

        void setAllocationSizes( uint32_t inBytes, UIntPtr_t inOsBytes, uint32_t inTableIndex, uint32_t smallAllocLimt )
        {
            tableIndex = inTableIndex;
            allocSize = inBytes;

            if (tableIndex == smallAllocLimt)
            {
                firstMem = (FFreeMem*) inOsBytes;
            }
        }

        uint32_t getBytes() const
        {
            return allocSize;
        }

        UIntPtr_t getOsBytes( uint32_t inPageSize, uint32_t smallAllocLimt ) const
        {
            if (tableIndex == smallAllocLimt)
            {
                return (UIntPtr_t)firstMem;
            }
            else
            {
                return FAlign(allocSize, inPageSize);
            }
        }
        
        void link( FPoolInfo*& before )
        {
            if( before )
            {
                before->prevLink = &next;
            }

            next     = before;
            prevLink = &before;
            before   = this;
        }
        
        void unlink()
        {
            if( next )
            {
                next->prevLink = prevLink;
            }
            *prevLink = next;
        }
    };

    static_assert(sizeof(FPoolInfo) == 32, "If you get there, this probably means that FPoolInfo is not 32 bit length somehow");

    /** Information about a piece of free memory. 8 bytes */
    struct FFreeMem
    {
        /** Next or MemLastPool[], always in order by pool. */
        FFreeMem*	next;
        /** Number of consecutive free blocks here, at least 1. */
        uint32_t	numFreeBlocks;
    };

    /** Default alignment for binned allocator */
    enum { default_binned_allocator_alignment = sizeof(FFreeMem) };

#ifdef CACHE_FREED_OS_ALLOCS
    /**  */
    struct FFreePageBlock
    {
        void*				ptr = nullptr;
        size_t				byteSize = 0;
    };
#endif
    /** Pool table. */
    struct FPoolTable
    {
        FPoolInfo*			firstPool = nullptr;
        FPoolInfo*			exhaustedPool = nullptr;
        uint32_t			blockSize = 0;
#ifdef USE_FINE_GRAIN_LOCKS
        std::mutex          mutex;
#endif

#if STATS
        /** Number of currently active pools */
        uint32_t			numActivePools = 0;

        /** Largest number of pools simultaneously active */
        uint32_t			maxActivePools = 0;

        /** Number of requests currently active */
        uint32_t			activeRequests = 0;

        /** High watermark of requests simultaneously active */
        uint32_t			maxActiveRequests = 0;

        /** Minimum request size (in bytes) */
        uint32_t			minRequest = 0;

        /** Maximum request size (in bytes) */
        uint32_t			maxRequest = 0;

        /** Total number of requests ever */
        uint64_t			totalRequests = 0;

        /** Total waste from all allocs in this table */
        uint64_t			totalWaste = 0;
#endif
    };

    /** Hash table struct for retrieving allocation book keeping information */
    struct PoolHashBucket
    {
        UIntPtr_t		key = 0;
        FPoolInfo*		firstPool = nullptr;
        PoolHashBucket* prev = nullptr;
        PoolHashBucket* next = nullptr;

        PoolHashBucket()
        {
            prev = this;
            next = this;
        }

        void link( PoolHashBucket* after )
        {
            link(after, prev, this);
        }

        static void link( PoolHashBucket* node, PoolHashBucket* before, PoolHashBucket* after )
        {
            node->prev = before;
            node->next = after;
            before->next=node;
            after->prev=node;
        }
        
        void unlink()
        {
            next->prev = prev;
            prev->next = next;
            prev=this;
            next=this;
        }
    };

    uint64_t tableAddressLimit;

#ifdef USE_LOCKFREE_DELETE
    /** We can't call the constructor to TLockFreePointerList in the BinnedMalloc constructor
     * as it attempts to allocate memory. We push this back and initialize it later but we
     * set aside the memory before hand
     */
    uint8							PendingFreeListMemory[sizeof(TLockFreePointerList<void>)];
    TLockFreePointerList<void>*		PendingFreeList;
    TArray<void*>					FlushedFrees;
    bool							bFlushingFrees;
    bool							bDoneFreeListInit;
#endif

    std::mutex                      accessGuard;

    // PageSize dependent constants
    uint64_t maxHashBuckets;
    uint64_t maxHashBucketBits;
    uint64_t maxHashBucketWaste;
    uint64_t maxBookKeepingOverhead;
    /** Shift to get the reference from the indirect tables */
    uint64_t poolBitShift;
    uint64_t indirectPoolBitShift;
    uint64_t indirectPoolBlockSize;
    /** Shift required to get required hash table key. */
    uint64_t hashKeyShift;
    /** Used to mask off the bits that have been used to lookup the indirect table */
    uint64_t poolMask;
    uint64_t binnedSizeLimit;
    uint64_t binnedOSTableIndex;

    // Variables.
    FPoolTable  poolTable[PoolCount];
    FPoolTable	osTable;
    FPoolTable	pagePoolTable[extened_page_pool_allocation_count];
    FPoolTable* memSizeToPoolTable[max_pooled_allocation_size + extened_page_pool_allocation_count];

    PoolHashBucket* hashBuckets;
    PoolHashBucket* hashBucketFreeList;
    
    uint32_t		pageSize;

#ifdef CACHE_FREED_OS_ALLOCS
    FFreePageBlock	freedPageBlocks[MAX_CACHED_OS_FREES];
    uint32_t		freedPageBlocksNum;
    uint32_t		cachedTotal;
#endif

#if STATS
    size_t		osCurrent;
    size_t		osPeak;
    size_t		wasteCurrent;
    size_t		wastePeak;
    size_t		usedCurrent;
    size_t		usedPeak;
    size_t		currentAllocs;
    size_t		totalAllocs;
    /** osCurrent - wasteCurrent - usedCurrent. */
    size_t		slackCurrent;
    double		memTime;
#endif

    //_AttrNoReturn
    void outOfMemory(uint64_t size, uint32_t alignment=0)
    {
        // this is expected not to return
    }

    _AttrAlwaysInline
    void trackStats(FPoolTable* table, uint32_t size)
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

    /**
     * Create a 64k page of FPoolInfo structures for tracking allocations
     */
    FPoolInfo* createIndirect()
    {
        FAssert(indirectPoolBlockSize * sizeof(FPoolInfo) <= pageSize);

        FPoolInfo* indirect = (FPoolInfo*)FPlatformMemory_t::binnedAllocFromOS(indirectPoolBlockSize * sizeof(FPoolInfo));
        if(unlikely(!indirect))
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

    /**
     * Gets the FPoolInfo for a memory address. If no valid info exists one is created.
     * NOTE: This function requires a mutex across threads, but its is the callers responsibility to
     * acquire the mutex before calling
     */
    _AttrAlwaysInline
    FPoolInfo* getPoolInfo( UIntPtr_t ptr )
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

        PoolHashBucket* collision= &hashBuckets[hash];
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
    FPoolInfo* findPoolInfo(UIntPtr_t ptr1, UIntPtr_t& allocationBase)
    {
        uint16_t nextStep = 0;

        UIntPtr_t ptr = ptr1 &~ ((UIntPtr_t)pageSize-1);

        for (uint32_t i=0, n=(binned_alloc_pool_size/pageSize) + 1; i < n; ++i)
        {
            FPoolInfo* pool = findPoolInfoInternal(ptr, nextStep);
            if (pool)
            {
                allocationBase = ptr;
                //checkSlow(Ptr1 >= AllocationBase && Ptr1 < AllocationBase+Pool->GetBytes());
                return pool;
            }

            ptr = ((ptr - (pageSize * nextStep)) - 1) &~ ((UIntPtr_t)pageSize-1);
        }
        allocationBase=0;
        return nullptr;
    }

    _AttrAlwaysInline
    FPoolInfo* findPoolInfoInternal(UIntPtr_t ptr, uint16_t& jumpOffset)
    {
        FAssert(!!hashBuckets);

        UIntPtr_t key = ptr >> hashKeyShift;
        UIntPtr_t hash = key & (maxHashBuckets-1);
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
            collision=collision->next;

        } while (collision!=&hashBuckets[hash]);
        
        return nullptr;
    }

    /**
     *	Returns a newly created and initialized PoolHashBucket for use.
     */
    _AttrAlwaysInline
    PoolHashBucket* createHashBucket()
    {
        PoolHashBucket* bucket = allocateHashBucket();
        initializeHashBucket(bucket);
        return bucket;
    }

    /**
     *	Initializes bucket with valid parameters
     *	@param bucket pointer to be initialized
     */
    _AttrAlwaysInline
    void initializeHashBucket(PoolHashBucket* bucket)
    {
        if (!bucket->firstPool)
        {
            bucket->firstPool = createIndirect();
        }
    }

    /**
     * Allocates a hash bucket from the free list of hash buckets
     */
    PoolHashBucket* allocateHashBucket()
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

        PoolHashBucket* nextFree        = hashBucketFreeList->next;
        PoolHashBucket* free            = hashBucketFreeList;

        free->unlink();

        if (nextFree == free)
        {
            nextFree = nullptr;
        }

        hashBucketFreeList = nextFree;
        return free;
    }

    _AttrAlwaysInline
    FPoolInfo* allocatePoolMemory(FPoolTable* table, uint32_t poolSize, uint16_t tableIndex)
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
        free = (FFreeMem*) osAlloc(osBytes, actualPoolSize);

        FAssert(!((UIntPtr_t)free & (pageSize-1)));

        if( !free )
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

            for (UIntPtr_t i = (UIntPtr_t)pageSize, offset=0; i < osBytes; i += pageSize, ++offset)
            {
                FPoolInfo* trailingPool = getPoolInfo(((UIntPtr_t)free) + i);

                FAssert(!!trailingPool);

                //Set trailing pools to point back to first pool
                trailingPool->setAllocationSizes(0, 0, (uint32_t)offset, (uint32_t)binnedOSTableIndex);
            }
        }

        // Init pool.
        pool->link( table->firstPool );
        pool->setAllocationSizes(bytes, (uint32_t)osBytes, tableIndex, (uint32_t)binnedOSTableIndex);

#if STATS
        osPeak = std::max(osPeak, osCurrent += osBytes);
        wastePeak = std::max(wastePeak, wasteCurrent += osBytes - bytes);
        pool->taken		 = 0;
        pool->firstMem   = free;

        table->numActivePools++;
        table->maxActivePools = std::max(table->maxActivePools, table->numActivePools);
#endif
        // Create first free item.
        free->numFreeBlocks = blocks;
        free->next          = nullptr;
        
        return pool;
    }

    _AttrAlwaysInline
    FFreeMem* allocateBlockFromPool(FPoolTable* table, FPoolInfo* pool)
    {
        // Pick first available block and unlink it.
        pool->taken++;

        FAssert(pool->tableIndex < binnedOSTableIndex); // if this is false, FirstMem is actually a size not a pointer
        FAssert(!!pool->firstMem);
        FAssert(pool->firstMem->numFreeBlocks > 0);
        FAssert(pool->firstMem->numFreeBlocks < page_size_limit);

        FFreeMem* free = (FFreeMem*)((uint8_t*)pool->firstMem + --pool->firstMem->numFreeBlocks * table->blockSize);

        if( !pool->firstMem->numFreeBlocks )
        {
            pool->firstMem = pool->firstMem->next;
            if( !pool->firstMem )
            {
                // Move to exhausted list.
                pool->unlink();
                pool->link( table->exhaustedPool );
            }
        }
#if STATS
        usedPeak = std::max(usedPeak, usedCurrent += table->blockSize);
#endif
        return free;
    }

    /**
     * Releases memory back to the system. This is not protected from multi-threaded access and it's
     * the callers responsibility to Lock AccessGuard before calling this.
     */
    void freeInternal( void* ptr )
    {
        MEM_TIME(memTime -= FPlatformTime::Seconds());
        STAT(currentAllocs--);

        UIntPtr_t basePtr;
        FPoolInfo* pool = findPoolInfo((UIntPtr_t)ptr, basePtr);
        
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
        if (pool == nullptr)
        {
            FLogWarning(("Memory : Attempting to free a pointer we didn't allocate"));
            return;
        }
#endif
        FAssert(!!pool);
        FAssert(pool->getBytes() != 0);

        if( pool->tableIndex < binnedOSTableIndex )
        {
            FPoolTable* table = memSizeToPoolTable[pool->tableIndex];
#ifdef USE_FINE_GRAIN_LOCKS
            std::lock_guard<std::mutex> tableLock(table->mutex);
#endif

            STAT(table->activeRequests--);

            // If this pool was exhausted, move to available list.
            if( !pool->firstMem )
            {
                pool->unlink();
                pool->link( table->firstPool );
            }

            // Free a pooled allocation.
            FFreeMem* free		= (FFreeMem*)ptr;
            free->numFreeBlocks	= 1;
            free->next			= pool->firstMem;
            pool->firstMem		= free;

            STAT(usedCurrent -= table->blockSize);

            // Free this pool.
            FAssert(pool->taken >= 1);

            if( --pool->taken == 0 )
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
            FAssert(!((UIntPtr_t)ptr & (pageSize-1)));

            size_t osBytes = pool->getOsBytes(pageSize, (uint32_t)binnedOSTableIndex);

            STAT(usedCurrent -= pool->getBytes());
            STAT(osCurrent -= osBytes);
            STAT(wasteCurrent -= osBytes - pool->getBytes());

            osFree(ptr, osBytes);
        }
        
        MEM_TIME(MemTime += FPlatformTime::Seconds());
    }

    void pushFreeLockless(void* origin)
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

    /**
     * Clear and Process the list of frees to be deallocated. It's the callers
     * responsibility to Lock AccessGuard before calling this
     */
    void flushPendingFrees()
    {
#ifdef USE_LOCKFREE_DELETE
        if (!PendingFreeList && !bDoneFreeListInit)
        {
            bDoneFreeListInit=true;
            PendingFreeList = new ((void*)PendingFreeListMemory) TLockFreePointerList<void>();
        }
        // Because a lockless list and TArray calls new/malloc internally, need to guard against re-entry
        if (bFlushingFrees || !PendingFreeList)
        {
            return;
        }
        bFlushingFrees=true;
        PendingFreeList->PopAll(FlushedFrees);
        for (uint32 i=0, n=FlushedFrees.Num(); i<n; ++i)
        {
            FreeInternal(FlushedFrees[i]);
        }
        FlushedFrees.Reset();
        bFlushingFrees=false;
#endif
    }

    _AttrAlwaysInline
    void osFree(void* ptr, size_t size)
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
    void* osAlloc(size_t newSize, size_t& outActualSize)
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

#ifdef CACHE_FREED_OS_ALLOCS
    void flushAllocCache()
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
#endif

public:

    // FMalloc interface.
    // inPageSize - First parameter is page size, all allocs from BinnedAllocFromOS() MUST be aligned to this size
    // addressLimit - Second parameter is estimate of the range of addresses expected to be returns by BinnedAllocFromOS(). Binned
    // Malloc will adjust it's internal structures to make look ups for memory allocations O(1) for this range.
    // It's is ok to go outside this range, look ups will just be a little slower
    FMallocBinned(uint32_t inPageSize, uint64_t addressLimit);

    virtual ~FMallocBinned() {}

    _AttrWarnUnusedResult
    virtual void* malloc(size_t count, uint32_t alignment = DefaultAlignment) override;

    _AttrWarnUnusedResult
    virtual void* realloc(void* origin, size_t newCount, uint32_t alignment = DefaultAlignment) override;

    virtual void free(void* origin) override;

    /**
     * If possible determine the size of the memory allocated at the given address
     *
     * @param Original - Pointer to memory we are checking the size of
     * @param SizeOut - If possible, this value is set to the size of the passed in pointer
     * @return true if succeeded
     */
    virtual bool getAllocationSize(void *origin, size_t& sizeOut) override;

    /**
     * Validates the allocator's heap
     */
    virtual bool validateHeap() override;
};
