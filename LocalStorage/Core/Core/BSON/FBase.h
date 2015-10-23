#pragma once

#define NOINLINE_DECL

namespace _bson {

    class MsgAssertionException : public std::exception {
    public:
        MsgAssertionException(unsigned, std::string _s) : s(_s) {}
        ~MsgAssertionException() throw() { }
        const std::string s;
        virtual const char * what() const throw() { return s.c_str();  }
    };

    _AttrAlwaysInline void uasserted(unsigned, std::string) { assert(false); }
    _AttrAlwaysInline void uassert(unsigned a, const char *str, bool x) {
        assert(x);
    }
    _AttrAlwaysInline void uassert(unsigned, std::string, bool x) {
        assert(x);
    }
    _AttrAlwaysInline void msgasserted(unsigned x, std::string s) { throw MsgAssertionException(x, s); }
    _AttrAlwaysInline void massert(unsigned a, const char *b, bool x) {
        if (!x) msgasserted(a, std::string(b));
    }
    _AttrAlwaysInline void massert(unsigned a, std::string b, bool x) {
        if (!x) msgasserted(a, b);
    }
    _AttrAlwaysInline void verify(bool x) { assert(x); }

}
