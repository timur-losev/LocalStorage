#include "CorePrecompiled.h"
#include "FAllocationsTracker.h"

static boost::thread_specific_ptr<bool> sRecursiveCallGuard;

_AttrAlwaysInline
bool FAllocationsTracker::canRecord()
{
    if (unlikely(!sRecursiveCallGuard.get()))
    {
        if( m_recursiveGuardMutex.try_lock())
        {
            bool * ptr = static_cast<bool*>(FMemory::mallocUntracked(sizeof(bool)));
            *ptr = false;
            sRecursiveCallGuard.reset(ptr);
            
            m_recursiveGuardMutex.unlock();
        }
    }
    return m_recordEnabled && sRecursiveCallGuard.get() && !(*sRecursiveCallGuard.get());
}

void FAllocationsTracker::recordAlloc(void* ptr, size_t sz, const char* file, size_t ln, const char* func)
{
    if (canRecord())
    {
        ThreadGuard recursiveLock(sRecursiveCallGuard.get());
        FLockGuard_t lock(m_mutex);
        
        if(m_allocations.find(ptr) != m_allocations.end())
        {
            UString message = (UFormat("Unable to locate allocation unit - this probably means you have a mismatched allocation / deallocation style. [tag]: %s") % getTagStackString()).str().c_str();
            m_warningMessages.push_back(message);
        }
        m_allocations[ptr] = Block(sz, file, ln, func, getTagStackString());
    }
}

void FAllocationsTracker::recordDealloc(void *ptr)
{
    if(canRecord())
    {
        ThreadGuard recursiveLock(sRecursiveCallGuard.get());
        FLockGuard_t lock(m_mutex);
        
        // deal cleanly with null pointers
        if(likely(!!ptr))
        {
            AllocationMap_t::iterator i = m_allocations.find(ptr);
            if(i == m_allocations.end())
            {
                UString message = (UFormat("Unable to locate allocation unit - this probably means you have a mismatched allocation / deallocation style. [tag]: %s") % getTagStackString()).str().c_str();
                
                m_warningMessages.push_back(message);
            }
            else
            {
                m_allocations.erase(i);
            }
        }
    }
}

size_t FAllocationsTracker::getTotalAllocs(const std::string& tag)
{    
    if(likely(m_recordEnabled))
    {
        UString tagToCheck = tag.c_str();
        size_t memoryAllocated = 0;
        if (!likely(m_allocations.empty()))
        {
            for (const auto& pair : m_allocations)
            {
                const Block& block = pair.second;
                if (tag.empty() || tagToCheck == block.tag)
                {
                    memoryAllocated += block.size;
                }
            }
        }

        return memoryAllocated;
    }

    return 0;
}

void FAllocationsTracker::collectLeaks()
{
    if(canRecord())
    {
        ThreadGuard recursiveLock(sRecursiveCallGuard.get());
        FLockGuard_t lock(m_mutex);
        
        UOstringStream_t ss;
        size_t totalAllocs = getTotalAllocs();

        if (likely(totalAllocs == 0))
        {
            ss << UString("AllocationsTracker: No memory leaks").c_str() << std::endl;
        }
        else
        {
            UVectorTmpl<std::pair< void*, Block> > orderedVector(m_allocations.begin(),m_allocations.end());
            
            sort_by_mem memsort;
            std::sort(orderedVector.begin(), orderedVector.end(), memsort);
            
            ss << "AllocationsTracker: Detected memory leaks" << std::endl;
            ss << UFormat("AllocatinsTracker: (%d) Allocation(s) with total %d bytes.") % m_allocations.size() % totalAllocs;
            ss << std::endl << "Allocations dump (first 50 entries):" << std::endl << std::endl;

            ss << " +-------+---------------------------------------------------------------------------------------------------" << std::endl;
            ss << " | BYTES |                                          TAG                                                      " << std::endl;
            ss << " +-------+---------------------------------------------------------------------------------------------------" << std::endl;
            
            int entryCounter = 50;
            for(const auto& pair : orderedVector)
            {
                const Block& block = pair.second;
                
                ss << " | " << UFormat("%-6d| %s") % block.size % block.tag.c_str() << std::endl;
                
                entryCounter--;
                if( entryCounter == 0) break;
            }
            ss << " +-------+---------------------------------------------------------------------------------------------------" << std::endl;
            ss << std::endl;
            ss << UFormat("Warnings: %d") % m_warningMessages.size() << std::endl;
            for(const auto& message : m_warningMessages)
            {
                ss << message << std::endl;
            }

            ss << std::endl;
        }

        FLogNormal((ss.str().c_str()));
    }
}

void FAllocationsTracker::pushTag(const FMemTag * memTag)
{
    if(canRecord())
    {
        ThreadGuard recursiveLock(sRecursiveCallGuard.get());
        FLockGuard_t lock(m_mutex);
    
        FAssert(memTag!=0);
    
        if(memTag)
        {
            std::thread::id this_id = std::this_thread::get_id();
            UString tagName = memTag->m_tagName.c_str();
            m_tagStack[this_id].push_back(tagName);
        }
    }
}

void FAllocationsTracker::popTag(const FMemTag *memTag)
{
    if(canRecord())
    {
        ThreadGuard recursiveLock(sRecursiveCallGuard.get());
        FLockGuard_t lock(m_mutex);

        FAssert(memTag!=0);
        FAssert(!m_tagStack.empty());
    
        if(memTag && !m_tagStack.empty())
        {
            std::thread::id this_id = std::this_thread::get_id();
            if( m_tagStack.find(this_id) != m_tagStack.end())
            {
                UString tagName = m_tagStack[this_id].back();
                UString tagToCompare = memTag->m_tagName.c_str();
                //m_warningMessages.push_back((UFormat("%s %s %d") % tagName.c_str() % tagToCompare.c_str() % this_id).str().c_str());
                FAssert(tagName.compare(tagToCompare) == 0);
                m_tagStack[this_id].pop_back();
            }
        }
    }
}

FAllocationsTracker::UString FAllocationsTracker::getTagStackString()
{
    UString outString = "";
    std::thread::id this_id = std::this_thread::get_id();
    for(const auto & tagName : m_tagStack[this_id])
    {
        outString += "\\" + tagName;
    }
    
    return outString;
}

FMemTag::FMemTag(const char * tagName, int line, const char * file, const char * function)
{
    m_tagName = tagName ? tagName : "";
    m_function = function ? function : "";
    m_fileName = file ? file : "";
    m_line = line;
    
    FMemory::pushMemTag(this);
}

FMemTag::~FMemTag()
{
    FMemory::popMemTag(this);
}







