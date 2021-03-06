//
//  PBIO.h
//  PetBuddies
//
//  Created by void on 8/14/15.
//  Copyright (c) 2015 Catalyst Apps. All rights reserved.
//

#pragma once

namespace PBIO = boost::asio;
typedef PBIO::ip::tcp FTcp_t;
typedef FTcp_t::socket FSocket_t;
typedef PBIO::io_service FIOContext_t;
typedef FIOContext_t::work FIOContextRetainer_t;
typedef boost::system::error_code FErrorCode_t;
typedef THREAD_PROVIDER::thread  FThread_t;
typedef THREAD_PROVIDER::mutex FMutex_t;
typedef THREAD_PROVIDER::recursive_mutex FRecursiveMutex_t;
typedef THREAD_PROVIDER::condition_variable FConditionVariable_t;
namespace FThisThread = THREAD_PROVIDER::this_thread;

// in std since 2017. if we really require it, we probably have to use boost::thread
typedef boost::shared_mutex FSharedMutex_t;
typedef boost::unique_lock<FSharedMutex_t> FWriteLock_t;
typedef boost::shared_lock<FSharedMutex_t> FReadLock_t;

typedef THREAD_PROVIDER::unique_lock<FMutex_t> FUniqueLock_t;
typedef THREAD_PROVIDER::lock_guard<FMutex_t> FLockGuard_t;
typedef THREAD_PROVIDER::lock_guard<FRecursiveMutex_t> FRecursiveLockGuard_t;

typedef FSharedPtr<FThread_t> FThreadPtr_t;
typedef boost::asio::deadline_timer FDeadlineTimer_t;

namespace FFileSystem = boost::filesystem;
typedef FFileSystem::path FPath_t;

extern bool FCheckIOError(const FErrorCode_t& ec);