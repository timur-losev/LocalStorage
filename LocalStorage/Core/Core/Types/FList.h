#include <boost/container/list.hpp>
#include "FTLAllocator.h"

template<typename T, typename A = FTLAllocator<T, FDefaultDeleter> >
using FList =
#ifdef LS_USE_BOOST_CONTAINERS
    boost::container::list<T, A>;
#else
    std::list<T, A>;
#endif