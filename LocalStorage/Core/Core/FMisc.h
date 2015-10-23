//
//  PCMisc.h
//  PetBuddies
//
//  Created by void on 5/15/15.
//
//

#pragma once

#include "Core/FDebug.h"

#if CC_TARGET_PLATFORM == CC_PLATFORM_WIN32
#define __CompileTimeStub
#else
    #define __CompileTimeStub char __compile_time_assert__[(bool)(exp) ? 1 : -1] _AttrUnused;
#endif

//TODO: implement PBDebug class
#define FAssert(exp) do { \
        if (FBuiltInConstant((exp))) { \
            __CompileTimeStub \
        } else { _AttrUnused \
            typeof((exp)) _exp = likely((exp)); \
            if (LS_DEBUG_MODE && !(exp)) { FDebug::abort(__LINE__, (long)_exp); };\
        }\
    }while(0)

#define PBHalt() do { \
FDebug::abort(__LINE__, 0);\
}while(0)

template <int I>
class FInt2Type
{
public:
    enum { value = I };
};

// _AttrAlwaysInline int FRandom(int max)
// {
//     std::random_device rd;
//     std::mt19937 mt(rd());
//     std::uniform_real_distribution<double> dist(0, max);
// 
//     return dist(mt);
// }

//_AttrAlwaysInline
//std::string FStringConvertFromWCharto8STD(const wchar_t* str)
//{
//    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
//    std::string retval = convert.to_bytes(str);
//
//    return retval;
//}