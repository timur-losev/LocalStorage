#pragma once

#include <boost/array.hpp>

template <class T, size_t N>
using FStaticArray =
#if LS_USE_BOOST_CONTAINERS
boost::array<T, N>;
#else
std::array<T,N>;
#endif