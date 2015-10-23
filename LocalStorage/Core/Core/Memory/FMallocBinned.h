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

    static_assert(sizeof(FPoolInfo) == 32, "If you get there, this probably means that FPoolInfo is not 32 bit lenght somehow");

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
    void trackStats(FPoolTable* table, uint32_t size);

    /**
     * Create a 64k page of FPoolInfo structures for tracking allocations
     */
    FPoolInfo* createIndirect();

    /**
     * Gets the FPoolInfo for a memory address. If no valid info exists one is created.
     * NOTE: This function requires a mutex across threads, but its is the callers responsibility to
     * acquire the mutex before calling
     */
    _AttrAlwaysInline
    FPoolInfo* getPoolInfo( UIntPtr_t ptr );

    _AttrAlwaysInline
    FPoolInfo* findPoolInfo(UIntPtr_t ptr1, UIntPtr_t& allocationBase);

    _AttrAlwaysInline
    FPoolInfo* findPoolInfoInternal(UIntPtr_t ptr, uint16_t& jumpOffset);

    /**
     *	Returns a newly created and initialized PoolHashBucket for use.
     */
    _AttrAlwaysInline
    PoolHashBucket* createHashBucket();

    /**
     *	Initializes bucket with valid parameters
     *	@param bucket pointer to be initialized
     */
    _AttrAlwaysInline
    void initializeHashBucket(PoolHashBucket* bucket);

    /**
     * Allocates a hash bucket from the free list of hash buckets
     */
    PoolHashBucket* allocateHashBucket();

    _AttrAlwaysInline
    FPoolInfo* allocatePoolMemory(FPoolTable* table, uint32_t poolSize, uint16_t tableIndex);

    _AttrAlwaysInline
    FFreeMem* allocateBlockFromPool(FPoolTable* table, FPoolInfo* pool);

    /**
     * Releases memory back to the system. This is not protected from multi-threaded access and it's
     * the callers responsibility to Lock AccessGuard before calling this.
     */
    void freeInternal( void* ptr );

    void pushFreeLockless(void* origin);

    /**
     * Clear and Process the list of frees to be deallocated. It's the callers
     * responsibility to Lock AccessGuard before calling this
     */
    void flushPendingFrees();

    _AttrAlwaysInline
    void osFree(void* ptr, size_t size);

    _AttrAlwaysInline
    void* osAlloc(size_t newSize, size_t& outActualSize);

#ifdef CACHE_FREED_OS_ALLOCS
    void flushAllocCache();
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
