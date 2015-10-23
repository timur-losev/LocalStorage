#include "CorePrecompiled.h"
#include "FIO.h"

bool FCheckIOError(const FErrorCode_t& ec)
{
    if (unlikely(!!ec))
    {
        std::string message = ec.message();
        FLogError(("IO Error: [%2%] [%1%]") % message % ec.value());
        FAssert("IO Error" && false);

        return false;
    }

    return true;
}
