//
//  PBIO.h
//  PetBuddies
//
//  Created by void on 8/14/15.
//  Copyright (c) 2015 Catalyst Apps. All rights reserved.
//

#pragma once

namespace FIO = boost::asio;
typedef FIO::ip::tcp FTcp_t;
typedef FTcp_t::socket FSocket_t;
typedef FIO::io_service FIOContext_t;
typedef FIOContext_t::work FIOContextRetainer_t;
typedef boost::system::error_code FErrorCode_t;
typedef THREAD_PROVIDER::thread  FThread_t;
typedef THREAD_PROVIDER::mutex FMutex_t;
namespace FThisThread = THREAD_PROVIDER::this_thread;

// typedef std::shared_mutex FSharedMutex; //since 2017. if we really require it, we should use boost::thread
// typedef std::unique_lock<FSharedMutex> FWriteLock;
// typedef std::shared_lock<FSharedMutex> FReadLock;

typedef THREAD_PROVIDER::lock_guard<FMutex_t> FLockGuard_t;

typedef FSharedPtr<FThread_t> FThreadPtr_t;
typedef boost::asio::deadline_timer FDeadlineTimer_t;

namespace FFileSystem = boost::filesystem;
typedef FFileSystem::path FPath_t;

bool FCheckIOError(const FErrorCode_t& ec);