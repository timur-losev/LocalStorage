#pragma once

forward_this(FOperation);

class FOperation
{
    friend class FBackgroundContext;
public:
    typedef FList<FOperationPtr> Dependencies_t;
protected:
    Dependencies_t m_dependencies;
    FVoidBlock_t   m_executeBlock;
    FVoidBlock_t   m_completeBlock;
    std::atomic_bool m_canceled;

    void execute();
public:

    FOperation(const FVoidBlock_t& exec);
    FOperation();
    ~FOperation();

    void execute(const FVoidBlock_t &);
    void onComplete(const FVoidBlock_t&);

    void cancel();

    void addDependency(const FOperationPtr& op);
};