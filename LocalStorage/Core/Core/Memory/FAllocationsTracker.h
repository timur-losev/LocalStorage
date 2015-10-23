#pragma once

#include "Core/IO/FIO.h"
#include "Core/Types/FTLAllocator.h"
#include "Core/Memory/FMemory.h"

class FMemTag;

#define COMBINE(Y,X) Y ## X
#define COMBINE1(Y,X) COMBINE(Y,X)
#if ENABLE_FINE_MEM_TRACKING
    #define MEMTAG(name) FMemTag __tag(name,COMBINE1( ,__LINE__),COMBINE1( ,__FILE__),COMBINE1( ,__FUNCTION__));
    #define MEMTAG_F MEMTAG(COMBINE1( ,__FUNCTION__));
    #define MEMTAG_CF MEMTAG((std::string(typeid(*this).name())+std::string("::")+std::string(COMBINE1( ,__FUNCTION__))).c_str());
#else
    #define MEMTAG(name)
    #define MEMTAG_F
    #define MEMTAG_CF
#endif

class ThreadGuard
{
private:
    bool * m_guard;
public:
    ThreadGuard(bool * guard):
    m_guard(guard)
    {
        *m_guard = true;
    }
    ~ThreadGuard()
    {
        *m_guard = false;
    }
};

class FAllocationsTracker: public FUseSysMalloc
{
    friend class FMemory;
private:
    typedef std::basic_string<char,std::char_traits<char>, FTLAllocator< char, FUntrackedDeleter, FUntrackedAllocator > > UString;
    
    class Block
    {
    public:
        size_t size = 0;
        size_t line = 0;
        const char* filename = nullptr;
        const char* function = nullptr;
        UString tag;

        Block() = default;
        Block(size_t sz, const char* file, size_t ln, const char* func, UString tagName): size(sz),
        line(ln), filename(file), function(func), tag(tagName) {};
    };

    //Types below use FUntrackedDeleter in order to prevent FAllocationsTracker sends requests to record itself.
    //Well, this is not always possible, so actual defence against recurcive allocations records is sRecursiveCallGuard
    //U as Untracked
    
    //std::unordered_map
    template<
        class Key,
        class T,
        class Value = std::pair<const Key, T>,
        class Hash = std::hash<Key>,
        class KeyEqual = std::equal_to<Key>,
        class A = FTLAllocator< Value , FUntrackedDeleter, FUntrackedAllocator > >
    using UUnorderedMapTmpl =  std::unordered_map<Key, T, Hash, KeyEqual, A>;
    
    //std::vector
    template <
        class T,
        class A = FTLAllocator< T , FUntrackedDeleter, FUntrackedAllocator > >
    using UVectorTmpl = std::vector<T,A>;
    
    //std::ostringstream
    template <
        class Traits = std::char_traits<char>,
        class A = FTLAllocator< char , FUntrackedDeleter, FUntrackedAllocator > >
    using UOStringStreamTmpl = std::basic_ostringstream<char,Traits,A>;
    
    //boost::format
    template <
        class A = FTLAllocator< char , FUntrackedDeleter, FUntrackedAllocator > >
    using UFormatTmpl =  boost::basic_format<char,::std::char_traits<char>,A>;
    typedef UFormatTmpl<>      UFormat;
    
    //std::list
    template<
        class T,
        class A = FTLAllocator< T, FUntrackedDeleter, FUntrackedAllocator > >
    using UList = std::list<T,A>;
    

    typedef UUnorderedMapTmpl<void*, Block> AllocationMap_t;
    typedef UUnorderedMapTmpl<std::thread::id, UList<UString> > TagStack_t;
    typedef UOStringStreamTmpl<> UOstringStream_t;
   
    UList<UString>      m_warningMessages;
    AllocationMap_t     m_allocations;
    TagStack_t          m_tagStack;
    FMutex_t            m_mutex;
    FMutex_t            m_recursiveGuardMutex;
    const bool          m_recordEnabled = ENABLE_FINE_MEM_TRACKING ? true : false;
    
    
    
    void pushTag(const FMemTag * memTag);
    void popTag(const FMemTag * memTag);
    UString getTagStackString();
    
    struct sort_by_mem
    {
        bool operator()(std::pair<void*, Block> a, std::pair<void*,Block> b)
        {
            return a.second.size > b.second.size;
        }
    };

public:
    
    FAllocationsTracker()
    {
    }
    
    void recordAlloc(void* ptr, size_t sz, const char* file = nullptr, size_t ln = 0, const char* func = nullptr);

    void recordDealloc(void* ptr);

    void collectLeaks();  

    size_t getTotalAllocs(const std::string& tag = "");
    
    _AttrAlwaysInline bool canRecord();
    
  };

class FMemTag
{
    friend class FAllocationsTracker;
private:
    std::string m_tagName;
    std::string m_fileName;
    std::string m_function;
    int m_line;
public:
    FMemTag(const char * tagName, int line = 0, const char * file = 0, const char * func = 0);
    ~FMemTag();    
};