#pragma once

template <typename T>
struct FRemovePointer { typedef T Type; };

template <typename T>
struct FRemovePointer<T*> { typedef T Type; };


template <typename T>
struct FRemoveReference { typedef T Type; };

template <typename T>
struct FRemoveReference<T&> { typedef T Type; };

template <typename T>
struct FRemoveReference<T&&> { typedef T Type; } ;

template <typename T>
_AttrAlwaysInline
typename FRemoveReference<T>::Type&&
FMove(T&& obj) // <-- implicit cast
{
    return (typename FRemoveReference<T>::Type&&)obj;
}

template <typename T>
_AttrAlwaysInline
T&&
FForward(typename FRemoveReference<T>::Type& obj)
{
    return (T&&)obj;
}

template <typename T>
_AttrAlwaysInline
T&&
FForward(typename FRemoveReference<T>::Type&& obj)
{
    return (T&&)obj;
}

template<typename T32BITS, typename T64BITS, int PointerSize>
struct FSelectIntPointerType
{
    // nothing here are is it an error if the partial specializations fail
};

template<typename T32BITS, typename T64BITS>
struct FSelectIntPointerType<T32BITS, T64BITS, 8>
{
    typedef T64BITS IntPointer; // select the 64 bit type
};

template<typename T32BITS, typename T64BITS>
struct FSelectIntPointerType<T32BITS, T64BITS, 4>
{
    typedef T32BITS IntPointer; // select the 64 bit type
};

typedef FSelectIntPointerType<uint32_t, uint64_t, sizeof(void*)>::IntPointer UIntPtr_t;	// unsigned int the same size as a pointer
typedef FSelectIntPointerType<int32_t, int64_t, sizeof(void*)>::IntPointer IntPtr_t;		// signed int the same size as a pointer

/**
 * Aligns a value to the nearest higher multiple of 'Alignment', which must be a power of two.
 *
 * @param Ptr			Value to align
 * @param Alignment		Alignment, must be a power of two
 * @return				Aligned value
 */
template< class T >
_AttrAlwaysInline
T FAlign( const T value, int32_t alignment )
{
    return (T)(((IntPtr_t)value + alignment - 1) & ~(alignment-1));
}


template <class T, template<typename> class Deleter = FDefaultDeleter, class ..._Args>
_AttrAlwaysInline
FSharedPtr<T> FMakeShared(_Args&&... args)
{
    FTLAllocator<T, Deleter> alloc;
    return FAllocateShared<T>(alloc, FForward<_Args>(args)...);
}



