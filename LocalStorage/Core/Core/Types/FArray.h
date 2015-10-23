#pragma once

#include <boost/container/vector.hpp>
#include "FTLAllocator.h"

template <typename T, typename A = FTLAllocator<T, FDefaultDeleter>>
using FArray =
#ifdef LS_USE_BOOST_CONTAINERS
    boost::container::vector<T, A>;
#else
    std::vector<T, A>;
#endif
