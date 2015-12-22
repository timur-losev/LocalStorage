#pragma once

#include <boost/container/map.hpp>
#include "FTLAllocator.h"

#if LS_USE_BOOST_CONTAINERS
template < class Key, class T, class Compare = std::less<Key>
        , class Allocator = FTLAllocator< std::pair< const Key, T>, FDefaultDeleter >,
        class MapOptions = boost::container::tree_assoc_defaults >
using FMap = boost::container::map<Key, T, Compare, Allocator, MapOptions>;

#else
template < class Key, class T, class Compare = std::less<Key>
        , class Allocator = FTLAllocator< std::pair< const Key, T>, FDefaultDeleter > >
        using FMap = std::map<Key, T, Compare, Allocator>;
#endif
