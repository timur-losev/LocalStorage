#include "CorePrecompiled.h"
#include "FMemoryBase.h"
#include "FAllocationsTracker.h"

#define CHECK_AND_CREATE_GLOBAL_MEMORY_ALLOCATOR_IF_NECESSARY if(unlikely(!GMalloc)) CreateGlobalMemoryAllocator();

FMalloc* GMalloc = nullptr;

_AttrAlwaysInline
void CreateGlobalMemoryAllocator()
{
    GMalloc = FPlatformMemory_t::getAllocator();
}

void * FMemory::malloc(size_t count, EMemAlignment alignment)
{
    CHECK_AND_CREATE_GLOBAL_MEMORY_ALLOCATOR_IF_NECESSARY
    
    void * mem = GMalloc->malloc(count, alignment);

#if ENABLE_FINE_MEM_TRACKING
    GMalloc->getAllocationsTracker().recordAlloc(mem, count);
#endif

    return mem;
}

void * FMemory::mallocUntracked(size_t count, EMemAlignment alignment)
{
   CHECK_AND_CREATE_GLOBAL_MEMORY_ALLOCATOR_IF_NECESSARY
    
    return GMalloc->malloc(count, alignment);
}

void * FMemory::realloc(void *origin, size_t newCount, EMemAlignment alignment)
{
    CHECK_AND_CREATE_GLOBAL_MEMORY_ALLOCATOR_IF_NECESSARY

#if ENABLE_FINE_MEM_TRACKING
    GMalloc->getAllocationsTracker().recordDealloc(origin);
#endif

    void * mem = GMalloc->realloc(origin, newCount, alignment);

#if ENABLE_FINE_MEM_TRACKING
    GMalloc->getAllocationsTracker().recordAlloc(mem, newCount);
#endif

    return mem;
}

void FMemory::free(void *origin)
{
    CHECK_AND_CREATE_GLOBAL_MEMORY_ALLOCATOR_IF_NECESSARY

#if ENABLE_FINE_MEM_TRACKING
    GMalloc->getAllocationsTracker().recordDealloc(origin);
#endif

    GMalloc->free(origin);
}

void FMemory::freeUntracked(void *origin)
{
    CHECK_AND_CREATE_GLOBAL_MEMORY_ALLOCATOR_IF_NECESSARY
    
    GMalloc->free(origin);
}

void FMemory::dumpMemoryLeaks()
{
#if ENABLE_FINE_MEM_TRACKING
    CHECK_AND_CREATE_GLOBAL_MEMORY_ALLOCATOR_IF_NECESSARY
    
    return GMalloc->getAllocationsTracker().collectLeaks();
#endif
}

void FMemory::pushMemTag( const FMemTag * memTag)
{
#if ENABLE_FINE_MEM_TRACKING
    FAssert(memTag!=0);
    
    if(memTag)
    {
        CHECK_AND_CREATE_GLOBAL_MEMORY_ALLOCATOR_IF_NECESSARY
        GMalloc->getAllocationsTracker().pushTag( memTag);
    }
#endif
}

void FMemory::popMemTag( const FMemTag * memTag)
{
#if ENABLE_FINE_MEM_TRACKING
    FAssert(memTag!=0);
    
    if(memTag)
    {
        CHECK_AND_CREATE_GLOBAL_MEMORY_ALLOCATOR_IF_NECESSARY
        GMalloc->getAllocationsTracker().popTag( memTag);
    }
#endif
}

void * operator new[](std::size_t s) throw(std::bad_alloc)
{
    return FMemory::malloc( s );
}

void * operator new(std::size_t n) throw(std::bad_alloc)
{
    return FMemory::malloc( n );
}

void operator delete[](void * p) throw()
{
    FMemory::free( p );
}

void operator delete(void * p) throw()
{
    FMemory::free( p );
}