#pragma once

#include <boost/container/deque.hpp>
#include "FTLAllocator.h"


template <typename T, typename Allocator = FTLAllocator<T, FDefaultDeleter>>
using FDeque =
#if LS_USE_BOOST_CONTAINERS
    boost::container::deque<T, Allocator>;
#else
    std::deque<T, Allocator>;
#endif