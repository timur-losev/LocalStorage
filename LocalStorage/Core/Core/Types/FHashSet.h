#pragma once

#include <boost/unordered_set.hpp>
#include "FTLAllocator.h"

#if 0

template<
typename T,
typename Hasher = std::hash<T>,
typename Eq = std::equal_to<T>,
typename A = FTLAllocator<T, FDefaultDeleter > >
using FHashSet =
#if LS_USE_BOOST_CONTAINERS
    boost::unordered::unordered_set<T, Hasher, Eq, A>;
#else
    std::unordered_set<T, Hasher, Eq, T>;
#endif

#else

#if LS_USE_BOOST_CONTAINERS 
    #define FHashSet boost::unordered::unordered_set 
#else
    #define FHashSet std::unordered_set 
#endif


#endif