#pragma once

class FLogLevel
{
public:
    enum Enum
    {
        Debug,
        Normal,
        Notification,
        Warning,
        Error,
        Critical
    };

    _AttrAlwaysInline
    static std::string toString(Enum e)
    {
        switch (e)
        {
            case Debug:
                return "Debug";
            case Normal:
                return "Normal";
            case Notification:
                return "Notification";
            case Warning:
                return "Warning";
            case Error:
                return "Error";
            case Critical:
                return "Critical";
            default:
                assert(false);
                return "Unknown";
        }
    }
};
