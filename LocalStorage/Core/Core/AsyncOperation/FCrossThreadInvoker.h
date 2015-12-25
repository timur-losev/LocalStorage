#pragma once


class FCrossThreadInvoker
{
public:
    typedef std::function<void(void)> Signature_t;
private:
    FDeque<Signature_t> m_stack;
    bool m_created = false;

protected:
    FSharedPtr<FThread_t>     m_thread;
    FSharedPtr<FThread_t::id> m_threadId;
    FSharedPtr<FMutex_t>      m_mutex;
    FConditionVariable_t      m_predicate;
protected:

    FCrossThreadInvoker();
    ~FCrossThreadInvoker();
    
public:
    static void                             performCrossThreadCall(Signature_t f, FCrossThreadInvoker* invobj, bool waitUntilDone);


    bool                                    needInvoke() const;
    void                                    beginInvoke(Signature_t f);
    void                                    updateInvoker();

public:
    void                                    createInvoker(const FSharedPtr<FThread_t>& t);
    void                                    createInvoker();

    _AttrAlwaysInline
    bool                                    isInvokerCreated() const { return m_created; }


    FSharedPtr<FThread_t>                   getThread() { return m_thread; }
    FSharedPtr<FMutex_t>                    getMutex() { return m_mutex; }
};