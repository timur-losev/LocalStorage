#include "CorePrecompiled.h"
#include "FLoggerGeneric.h"

#if __GNUC__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif

#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>

#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include <boost/log/attributes/timer.hpp>
#include <boost/log/attributes/named_scope.hpp>

#include <boost/log/sources/logger.hpp>

#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/sinks.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/log/utility/record_ordering.hpp>

#if __GNUC__
#pragma clang diagnostic pop
#endif

// #include "Managers/PathManager/PBPathManagerConstants.h"
// 
// extern FPath_t getGamePathForType(GamePathType gamePathType);

namespace logging       = boost::log;
namespace attrs         = boost::log::attributes;
namespace src           = boost::log::sources;
namespace sinks         = boost::log::sinks;
namespace expr          = boost::log::expressions;
namespace keywords      = boost::log::keywords;

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(pblg, src::severity_logger_mt<FLogLevel::Enum>);

SINGLETON_SETUP(FLogger);

FLogger::FLogger()
{
    SINGLETON_ENABLE_THIS;
}

FLogger::~FLogger()
{

}

bool FLogger::setupLogger(const FPath_t &logFileName, FLogLevel::Enum fileLogFilter)
{
    FPath_t fullPath = logFileName;//getGamePathForType(GamePathType::Cache) / logFileName;

    boost::shared_ptr<FPlatformDependedSTDCall(ostream)> 
        logStrm(new FPlatformDependedSTDCall(ofstream(fullPath.c_str(), std::ios_base::out | std::ios_base::app)));

    logging::add_console_log(FPlatformDependedSTDCall(cout), boost::log::keywords::format = TEXT(">> %Message%"));

    if (!logStrm->good())
        return false;
#if LS_TARGET_PLATFORM == LS_PLATFORM_WIN32
    typedef sinks::wtext_ostream_backend backend_t;
#else
    typedef sinks::text_ostream_backend backend_t;
#endif

    typedef sinks::asynchronous_sink<
        backend_t,
        sinks::unbounded_ordering_queue<
                logging::attribute_value_ordering<uint32_t, std::less<uint32_t>
                >
            >
        > sink_t;

    boost::shared_ptr<sink_t> sink(new sink_t(
            boost::make_shared<backend_t>(),
            keywords::file_name = fullPath,
            //keywords::rotation_size = 5 * 1024 * 1024,
            keywords::order = logging::make_attr_ordering("RecordID", std::less<uint32_t>()),
            keywords::filter = expr::attr< FLogLevel::Enum >("Severity") >= fileLogFilter,
            //keywords::open_mode = (std::ios::out | std::ios::app),
            keywords::auto_flush = true)
    );

    sink->locked_backend()->add_stream(logStrm);

    //sink->locked_backend()->scan_for_files();

    sink->set_formatter
            (
                    expr::format(TEXT("%1%: [%2%] [%3%] - %4%"))
                            % expr::attr< unsigned int >("RecordID")
                            % expr::attr< boost::posix_time::ptime >("TimeStamp")
                            % expr::attr< boost::thread::id >("ThreadID")
                            % expr::smessage
            );


    logging::core::get()->add_sink(sink);

    logging::core::get()->add_global_attribute("TimeStamp", attrs::local_clock());
    logging::core::get()->add_global_attribute("RecordID", attrs::counter< uint32_t >());

    return true;
}

void FLogger::trace(FLogLevel::Enum sev, const std::string &line)
{
    BOOST_LOG_SCOPED_THREAD_TAG("ThreadID", boost::this_thread::get_id());
    BOOST_LOG_SEV(pblg::get(), sev) << line;
    logging::core::get()->flush();
}
