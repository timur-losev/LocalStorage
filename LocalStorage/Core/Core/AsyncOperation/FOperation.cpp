#include "CorePrecompiled.h"
#include "FOperation.h"

FOperation::FOperation()
{
    m_canceled = false;
}

FOperation::FOperation(const FVoidBlock_t& ex)
{
    m_canceled = false;
    execute(ex);
}

FOperation::~FOperation()
{

}

void FOperation::execute(const FVoidBlock_t & executeBlock)
{
    m_executeBlock = executeBlock;
}

void FOperation::execute()
{
    for (FOperationPtr& dep: m_dependencies)
    {
        if (m_canceled)
        {
            break;
        }

        dep->execute();
    }

    if (!m_canceled)
    {
        if (m_executeBlock)
        {
            m_executeBlock();
        }

        if (m_completeBlock)
        {
            m_completeBlock();
        }
    }
}

void FOperation::onComplete(const FVoidBlock_t& block)
{
    m_completeBlock = block;
}

void FOperation::cancel()
{
    m_canceled = true;
}

void FOperation::addDependency(const FOperationPtr& op)
{
    m_dependencies.push_back(op);
}
