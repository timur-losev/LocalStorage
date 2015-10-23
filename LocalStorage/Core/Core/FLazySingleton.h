#pragma once

template<class T>
class FLazySingleton
{
protected:
    static T*   m_inst;
public:

    _AttrAlwaysInline
    static T& getRef()
    {
        assert(m_inst);

        return *m_inst;
    }

    _AttrAlwaysInline
    static T* getPtr()
    {
        return m_inst;
    }
};

#    define SINGLETON_SETUP(_class_) template<> _class_* FLazySingleton<_class_>::m_inst = 0
#    define SINGLETON_ENABLE_THIS m_inst = this