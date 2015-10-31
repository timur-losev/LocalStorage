#pragma once


#include "FLogLevel.h"
#include "Core/FLazySingleton.h"

#include "Core/IO/FIO.h"

class FLogger : public FLazySingleton<FLogger>
{
private:

public:
    FLogger();
    ~FLogger();

    bool setupLogger(const FPath_t& logFileName, FLogLevel::Enum fileLogFilter = FLogLevel::Warning);

    void trace(FLogLevel::Enum, const std::string& line);
};

#define FLogDebug(x)           do { FLogger::getRef().trace(FLogLevel::Debug, (FFormat x).str()); } while(false)
#define FLogNormal(x)          do { FLogger::getRef().trace(FLogLevel::Normal, (FFormat x).str()); } while(false)
#define FLogNotification(x)    do { FLogger::getRef().trace(FLogLevel::Notification, (FFormat x).str()); } while(false)
#define FLogWarning(x)         do { FLogger::getRef().trace(FLogLevel::Warning, (FFormat x).str()); } while(false)
#define FLogError(x)           do { FLogger::getRef().trace(FLogLevel::Error, (FFormat x).str()); } while(false)
#define FLogCritical(x)        do { FLogger::getRef().trace(FLogLevel::Critical, (FFormat x).str()); } while(false)




