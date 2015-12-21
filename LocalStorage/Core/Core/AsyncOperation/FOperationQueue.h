#pragma once

#include "Core/IO/FIO.h"

forward_this(FOperation);
forward_this(FBackgroundContext);

class FBackgroundContext : boost::noncopyable
{
private:
    FIOContext_t                     m_backgroundContext;
    FSharedPtr<FIOContextRetainer_t> m_backgroundContextRetainer;
    FThreadPtr_t                     m_backgroundThread;

    void create();
    void destroy();
public:

    FBackgroundContext();
    ~FBackgroundContext();

    void dispatchAsync(const FVoidBlock_t& executionBlock);
    void addOperation(const FOperationPtr& op);
};

class FOperationQueue : boost::noncopyable
{
private:
    static const std::string k_defaultBackgroundThreadName;

public:
private:
    FIOContext_t                            m_mainContext;
    FSharedPtr<FIOContextRetainer_t>        m_mainContextRetainer;

    FThread_t::id                           m_creationThreadId;
    mutable FMutex_t                        m_mutex;

    FHashTable<std::string, FBackgroundContextPtr> m_backgroundContexts;

public:

    FOperationQueue();
    ~FOperationQueue();

    void createDefaultContexts();
    void addOperation(const FOperationPtr& operation);

    void addContext(const std::string& name, const FBackgroundContextPtr& context);
    
    FBackgroundContextPtr addNewContext(const std::string& name);
    FBackgroundContextPtr getContext(const std::string& name) const;

    void removeContext(const std::string& name);

    void dispatchAsync(const FVoidBlock_t& executionBlock);
    void dispatchAsync(const std::string& contextName, const FVoidBlock_t& executionBlock);
    void dispatchMain(const FVoidBlock_t& executionBlock);

    void pollMain();

    // Returns main's thread id for the primary FOperationQueue hosted in PBCore
    _AttrAlwaysInline
        FThread_t::id getSpawnThread() const
    {
        return m_creationThreadId;
    }
};
