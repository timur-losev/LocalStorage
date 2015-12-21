#include <boost/container/list.hpp>
#include "FTLAllocator.h"

template<typename T, typename A = FTLAllocator<T, FDefaultDeleter> >
using FList =
#if LS_USE_BOOST_CONTAINERS
    boost::container::list<T, A>;
#else
    std::list<T, A>;
#endif