#undef check
#undef verify

#if __GNUC__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif

#if !defined(FUSION_MAX_VECTOR_SIZE)
#define FUSION_MAX_VECTOR_SIZE 20
#endif
#if !defined(FUSION_MAX_MAP_SIZE)
#define FUSION_MAX_MAP_SIZE 20
#endif

#define BOOST_THREAD_VERSION 4
#define BOOST_THREAD_USES_DATETIME

#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/signals2.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/noncopyable.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
//#include <boost/thread/thread_guard.hpp>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/export.hpp>

#include <boost/archive/detail/common_oarchive.hpp>
#include <boost/archive/detail/common_iarchive.hpp>

#include <boost/type_traits.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/mpl/assert.hpp>

#include <boost/utility/enable_if.hpp>

#include <boost/fusion/container/map.hpp>
#include <boost/fusion/include/has_key.hpp>
#include <boost/fusion/include/at_key.hpp>
#include <boost/fusion/include/value_at_key.hpp>

// #include <boost/archive/basic_archive.hpp>
// #include <boost/archive/detail/register_archive.hpp>
// #include <boost/serialization/item_version_type.hpp>
// #include <boost/serialization/collection_size_type.hpp>
// #include <boost/serialization/extended_type_info_no_rtti.hpp>
// #include <boost/serialization/type_info_implementation.hpp>
// #include <boost/serialization/strong_typedef.hpp>

#include <boost/filesystem.hpp>

#if __GNUC__
#pragma clang diagnostic pop
#endif

typedef boost::format FFormat;
typedef boost::noncopyable FNoncopyable;

namespace FSignals = boost::signals2;
