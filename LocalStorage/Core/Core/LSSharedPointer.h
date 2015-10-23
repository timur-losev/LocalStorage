#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

template <typename T>
using FSharedPtr = SHARED_POINTER_PROVIDER::shared_ptr<T>;

template <typename T>
using FWeakPtr = SHARED_POINTER_PROVIDER::weak_ptr<T>;


#define forward_this(__class__)\
class __class__;\
typedef FSharedPtr<__class__> __class__##Ptr;\
typedef FSharedPtr<__class__ const> __class__##ConstPtr

#define forward_this_s(__class__)\
struct __class__;\
typedef FSharedPtr<__class__> __class__##Ptr;\
typedef FSharedPtr<__class__ const> __class__##ConstPtr

#define forward_this_w(__class__)\
class __class__;\
typedef FWeakPtr<__class__> __class__##WeakPtr;\
typedef FWeakPtr<__class__ const> __class__##ConstWeakPtr

#define forward_this_ws(__class__)\
struct __class__;\
typedef FWeakPtr<__class__> __class__##WeakPtr;\
typedef FWeakPtr<__class__ const> __class__##ConstWeakPtr

#define FEnableSharedFromThis(__class__)\
    SHARED_POINTER_PROVIDER::enable_shared_from_this<__class__>

#define FStaticPointerCast SHARED_POINTER_PROVIDER::static_pointer_cast
#define FConstPointerCast SHARED_POINTER_PROVIDER::const_pointer_cast
#define FAllocateShared SHARED_POINTER_PROVIDER::allocate_shared
