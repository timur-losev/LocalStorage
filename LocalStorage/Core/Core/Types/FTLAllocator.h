#pragma once

#include "Core/Memory/FMemory.h"

template <class T>
class FDefaultDeleter
{
public:
    typedef size_t              size_type;

    static
    _AttrAlwaysInline
    void deallocate(T* ptr, size_type n = 0)
    {
        FMemory::free(ptr);
    }

    static
    _AttrAlwaysInline
    void destroy(T* ptr)
    {
        ptr->~T();
    }
};

template <class T>
class FEmptyDeleter
{
public:
    typedef size_t              size_type;

    _AttrAlwaysInline
    static
    void deallocate(T* ptr, size_type n = 0)
    {

    }

    _AttrAlwaysInline
    static
    void destroy(T* ptr)
    {
        
    }
};

template <class T>
class FUntrackedDeleter
{
public:
    typedef size_t              size_type;
    
    _AttrAlwaysInline
    static
    void deallocate(T* ptr, size_type n = 0)
    {
        FMemory::systemFree(ptr);
    }
    
    _AttrAlwaysInline
    static
    void destroy(T* ptr)
    {
        ptr->~T();
    }
    
};

template <class T>
class FDefaultAllocator
{
public:
    typedef size_t              size_type;
    
    _AttrAlwaysInline
    static
    T* allocate(size_type cnt, const void* hint = 0)
    {
        return reinterpret_cast<T*>(FMemory::malloc(cnt * sizeof(T), DefaultAlignment));
    }
};

template <class T>
class FUntrackedAllocator
{
public:
    typedef size_t              size_type;
    
    _AttrAlwaysInline
    static
    T* allocate(size_type cnt, const void* hint = 0)
    {
        return reinterpret_cast<T*>(FMemory::systemMalloc(cnt * sizeof(T)));
    }
};

template <typename T, template<typename> class DeletePolicy, template<typename> class AllocatePolicy = FDefaultAllocator, EMemAlignment MemAlign = DefaultAlignment>
class FTLAllocator
{
public:

    typedef size_t              size_type;
    typedef std::ptrdiff_t      difference_type;
    typedef T*                  pointer;
    typedef const T*            const_pointer;
    typedef T&                  reference;
    typedef const T&            const_reference;
    typedef T                   value_type;
    typedef DeletePolicy<T>     delete_policy;
    typedef AllocatePolicy<T>   allocate_policy;

    template <typename U>
    struct rebind
    {
        typedef FTLAllocator<U, DeletePolicy, AllocatePolicy, MemAlign> other;
    };

    FTLAllocator() = default;

    template <typename U1>
    FTLAllocator(const FTLAllocator<U1, DeletePolicy, AllocatePolicy, MemAlign>&)
    {

    }

    pointer address(reference r) const
    {
        return &r;
    }
    
    T* allocate (size_t cnt, const void* hint = 0)
    {
        return allocate_policy::allocate(cnt, hint);
    }

    size_type max_size() const
    {
        return size_type(-1) / sizeof(T);
    }

    void deallocate(T* ptr, size_type n = 0)
    {
        delete_policy::deallocate(ptr, n);
    }

    void destroy(T* ptr)
    {
        delete_policy::destroy(ptr);
    }
#if 1
    template <typename... Args>
    void construct(T* ptr, Args&&... args)
    {
        new ((void*)ptr) T(std::forward<Args>(args)...);
    }
#endif
    bool operator == (const FTLAllocator& ) const
    {
        return true;
    }

    bool operator != (const FTLAllocator& ) const
    {
        return false;
    }
};
