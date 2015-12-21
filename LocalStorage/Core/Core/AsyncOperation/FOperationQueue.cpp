#include "CorePrecompiled.h"
#include "FOperationQueue.h"
#include "FOperation.h"

const std::string FOperationQueue::k_defaultBackgroundThreadName = "default";

FOperationQueue::FOperationQueue()
{

}

FOperationQueue::~FOperationQueue()
{

}

void FOperationQueue::createDefaultContexts()
{
    static std::once_flag once;

    std::call_once(once, [this]
    {
#if PB_DEBUG_MODE
        m_creationThreadId = FThisThread::get_id();
#endif

        m_mainContextRetainer = FMakeShared<FIOContextRetainer_t>(m_mainContext);
        addNewContext(k_defaultBackgroundThreadName);
    });
}

void FOperationQueue::addOperation(const FOperationPtr& operation)
{
    FLockGuard_t lock(m_mutex);

    m_backgroundContexts[k_defaultBackgroundThreadName]->addOperation(operation);
}

void FOperationQueue::addContext(const std::string& name, const FBackgroundContextPtr& context)
{
    FLockGuard_t lock(m_mutex);

    auto it = m_backgroundContexts.insert(std::make_pair(name, context));

    FAssert(!!it.second);
}

void FOperationQueue::removeContext(const std::string& name)
{
    FLockGuard_t lock(m_mutex);

    auto it = m_backgroundContexts.find(name);
    FAssert(it != m_backgroundContexts.end());

    m_backgroundContexts.erase(it);
}

void FOperationQueue::dispatchAsync(const FVoidBlock_t & executionBlock)
{
    dispatchAsync(k_defaultBackgroundThreadName, executionBlock);
}

void FOperationQueue::dispatchAsync(const std::string& contextName, const FVoidBlock_t& executionBlock)
{
    FLockGuard_t lock(m_mutex);

    auto it = m_backgroundContexts.find(contextName);
    FAssert(it != m_backgroundContexts.end());

    it->second->dispatchAsync(executionBlock);
}

void FOperationQueue::dispatchMain(const FVoidBlock_t& executionBlock)
{
    m_mainContext.post(executionBlock);
}

void FOperationQueue::pollMain()
{
    m_mainContext.poll_one();
}

FBackgroundContextPtr FOperationQueue::addNewContext(const std::string& name)
{
    FLockGuard_t lock(m_mutex);

#if PB_DEBUG_MODE
    FAssert(m_backgroundContexts.find(name) == m_backgroundContexts.end());
#endif

    FBackgroundContextPtr newContext = FMakeShared<FBackgroundContext>();
    m_backgroundContexts[name] = newContext;

    return newContext;
}

FBackgroundContextPtr FOperationQueue::getContext(const std::string& name) const
{
    FLockGuard_t lock(m_mutex);

    auto it = m_backgroundContexts.find(name);
    if (it != m_backgroundContexts.end())
    {
        return it->second;
    }
    else
    {
        return nullptr;
    }
}

FBackgroundContext::FBackgroundContext()
{
    create();
}

FBackgroundContext::~FBackgroundContext()
{
    destroy();
}

void FBackgroundContext::dispatchAsync(const FVoidBlock_t& executionBlock)
{
    m_backgroundContext.post(executionBlock);
}

void FBackgroundContext::addOperation(const FOperationPtr& op)
{
    dispatchAsync([op]
    {
        op->execute();
    });
}

void FBackgroundContext::create()
{
    m_backgroundContextRetainer = FMakeShared<FIOContextRetainer_t>(m_backgroundContext);
    m_backgroundThread = FMakeShared<FThread_t>([this]
    {
        m_backgroundContext.run();
    });
}

void FBackgroundContext::destroy()
{
    m_backgroundContext.stop();

    if (m_backgroundThread && m_backgroundThread->joinable())
    {
        m_backgroundThread->join();
    }
}
