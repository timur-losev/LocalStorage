#pragma once

#include <boost/container/map.hpp>
#include "FTLAllocator.h"

#if LS_USE_BOOST_CONTAINERS
template < class Key, class T, class Compare = std::less<Key>
        , class Allocator = FTLAllocator< std::pair< const Key, T>, FDefaultDeleter >,
        class MapOptions = boost::container::tree_assoc_defaults >

using FMap = boost::container::map<Key, T, Compare, Allocator, MapOptions>;

template < class Key, class T>
using FUnorderedMap = boost::container::unordered_map<Key, T>;

#else
template < class Key, class T, class Compare = std::less<Key>
        , class Allocator = FTLAllocator< std::pair< const Key, T>, FDefaultDeleter > >
        using FMap = std::map<Key, T, Compare, Allocator>;

template < class Key, class T>
using FUnorderedMap = std::unordered_map<Key, T>;
#endif
