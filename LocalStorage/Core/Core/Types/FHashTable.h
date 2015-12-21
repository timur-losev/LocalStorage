#pragma once

#include <boost/unordered_map.hpp>
#include "FTLAllocator.h"

#if 1

template<
        typename Key,
        typename T,
        typename Hasher = std::hash<Key>,
        typename Eq = std::equal_to<Key>,
        typename A = FTLAllocator<std::pair<Key, T>, FDefaultDeleter > >
using FHashTable =
#if LS_USE_BOOST_CONTAINERS
boost::unordered::unordered_map<Key, T, Hasher, Eq, A>;
#else
std::unordered_map<Key, T, Hasher, Eq, FTLAllocator<
                                            std::pair<const Key, T>, FDefaultDeleter >
                    >;
#endif

#else

#if LS_USE_BOOST_CONTAINERS 
#define FHashTable boost::unordered::unordered_map 
#else
#define FHashTable std::unordered_map
#endif

#endif
