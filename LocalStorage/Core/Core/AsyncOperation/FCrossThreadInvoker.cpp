#include "CorePrecompiled.h"
#include "FCrossThreadInvoker.h"

FCrossThreadInvoker::FCrossThreadInvoker()
{

}

FCrossThreadInvoker::~FCrossThreadInvoker()
{

}

void FCrossThreadInvoker::performCrossThreadCall(Signature_t f, FCrossThreadInvoker* invobj, bool waitUntilDone)
{
    FAssert(invobj->isInvokerCreated());

    if (!waitUntilDone)
    {
        invobj->beginInvoke(f);
    }
    else
    {
        FUniqueLock_t lock(*invobj->m_mutex);
        invobj->m_stack.push_front(f);
        invobj->m_predicate.wait(lock);
    }
}

bool FCrossThreadInvoker::needInvoke() const
{
    FAssert(!!m_created);

    if (m_thread)
        return FThisThread::get_id() != m_thread->get_id();
    else if (m_threadId)
        return *m_threadId != FThisThread::get_id();

    FAssert("There are no acceptable comparator for thread id" && false);

    return false;
}

void FCrossThreadInvoker::beginInvoke(Signature_t f)
{
    FAssert(!!m_created);
    FLockGuard_t lock(*m_mutex);

    m_stack.push_front(f);
}

void FCrossThreadInvoker::updateInvoker()
{
    if (m_created)
    {

        if (!m_stack.empty())
        {
            m_mutex->lock();
            FDeque<Signature_t> stack2 = FMove(m_stack);
            m_mutex->unlock();

            while (!stack2.empty())
            {
                stack2.front()();
                m_predicate.notify_one();
                stack2.pop_front();
            }
        }
    }
}

void FCrossThreadInvoker::createInvoker(const FSharedPtr<FThread_t>& t)
{
    FAssert(t != nullptr);

    m_mutex.reset(new FMutex_t());
    m_thread = t;
    m_created = true;
}

void FCrossThreadInvoker::createInvoker()
{
    m_thread.reset();
    m_mutex.reset(new FMutex_t());
    m_threadId.reset(new FThread_t::id(FThisThread::get_id()));
    m_created = true;
}
