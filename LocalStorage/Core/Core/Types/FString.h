#include <memory>
#include <string.h>
#include <assert.h>
#include <functional>
#include <iosfwd>

#ifdef _WIN32
#include <cvt/wstring>
#endif

#include <locale>
#include <codecvt>

template<typename CharType>
class FString
{
public:
    template<typename CharTypeT>
    uint32_t strlen(const CharTypeT *str)
    {
        uint32_t retval = 0;

        for (; str && *str; ++retval, ++str);

        return retval;
    }

public:
    typedef CharType CharType_t;
private:
    std::unique_ptr<CharType_t[]> m_buffer;
    uint32_t m_strLen;
    uint32_t m_capacity;
    uint32_t m_hash;
private:
    template<typename CharTypeT>
    void _construct(const CharTypeT *str)
    {
        static_assert(sizeof(CharType_t) >= sizeof(CharTypeT), "Only types that lower or equal than nominal");

        m_capacity = _calcCapacity(m_strLen);
        m_buffer.reset(new CharType_t[m_capacity]);

        if (m_strLen != 0 && str)
        {
            //memcpy(m_buffer.get(), str, m_strLen * sizeof(CharTypeT));

            for (uint32_t i = 0; i < m_strLen; ++i)
            {
                m_buffer.get()[i] = static_cast<CharType_t>(str[i]);
            }

            if (str[m_strLen - 1] != 0)
            {
                m_buffer.get()[m_strLen] = 0;
            }

            m_hash = _calcHash(m_buffer.get());
        }
    }

    inline uint32_t _calcCapacity(uint32_t base) const
    {
        return static_cast<uint32_t>(base / 3) + base + 1;
    }

    uint32_t _calcHash(const CharType_t *str)
    {
        uint32_t hash = 0;
        uint32_t len = m_strLen;
        while (*str && len)
        {
            hash = hash * 101 + *str++;
            len--;
        }

        return hash;
    }

    void _copyBase(const FString &str)
    {
        m_strLen = str.m_strLen;
        m_hash = str.m_hash;
        m_capacity = str.m_capacity;
    }

public:
    FString() : m_strLen(0)
    {
        CharType_t *str = nullptr;
        _construct(str);
    }

    FString(std::string &&str)
    {
        m_strLen = str.size();
        _construct(str.c_str());
    }

    FString(const std::string &str)
    {
        m_strLen = str.size();
        _construct(str.c_str());
    }

    template<typename CharTypeT>
    FString(const CharTypeT *str)
    {
        m_strLen = strlen(str);
        _construct(str);
    }

    template<typename CharTypeT>
    FString(const CharTypeT *str, uint32_t len) : m_strLen(len)
    {
        _construct(str);
    }

    FString(const FString &oth)
    {
        _copyBase(oth);
        m_buffer.reset(new CharType_t[m_capacity]);
        memcpy(m_buffer.get(), oth.m_buffer.get(), sizeof(CharType_t) * m_strLen);
    }

    FString(FString &&oth)
    {
        _copyBase(oth);
        m_buffer = std::move(oth.m_buffer);
    }

    FString &operator=(const FString &oth)
    {
        _copyBase(oth);
        m_buffer.reset(new CharType_t[m_capacity]);
        memcpy(m_buffer.get(), oth.m_buffer.get(), sizeof(CharType_t) * m_strLen);

        return *this;
    }

    FString &operator=(FString &&oth)
    {
        _copyBase(oth);
        m_buffer = std::move(oth.m_buffer);
        oth.clear(); //! This will prevent troubles in the future

        return *this;
    }

    void assign(const CharType_t *str, uint32_t size)
    {
        if (m_capacity >= m_strLen + size)
        {
            memcpy(m_buffer.get() + m_strLen, str, sizeof(CharType_t) * size);
            m_strLen += size;
        }
        else
        {
            uint32_t newStrLen = m_strLen + size;
            m_capacity = _calcCapacity(newStrLen);
            CharType_t *newBuf = new CharType_t[m_capacity];
            memcpy(newBuf, m_buffer.get(), sizeof(CharType_t) * m_strLen);
            memcpy(newBuf + m_strLen, str, sizeof(CharType_t) * size);
            m_buffer.reset(newBuf);
            m_strLen = newStrLen;
        }

        m_hash = _calcHash(buffer()); //! What about h1 ^ (h2 << 1) instead ?
    }

    void assign(const FString &oth)
    {
        assign(oth.buffer(), oth.lenght());
    }

    FString operator+(const FString &oth)
    {
        FString<CharType_t> retv(*this);
        retv.assign(oth);

        return std::move(retv);
    }

    FString operator+(const CharType_t *str)
    {
        FString<CharType_t> retv(*this);
        retv.assign(str, strlen(str));

        return std::move(retv);
    }

    FString &operator+=(const FString &oth)
    {
        assign(oth);

        return *this;
    }

    FString &operator+=(const CharType_t *str)
    {
        assign(str, strlen(str));

        return *this;
    }

    FString &operator+=(CharType_t ch)
    {
        assign(&ch, 1);

        return *this;
    }

    void initFromBuffer(CharType_t *buffer, uint32_t len)
    {
        if (buffer && *buffer)
        {
            m_strLen = len;
            m_capacity = _calcCapacity(len);
            m_buffer.reset(buffer);
            m_hash = _calcHash(buffer);
        }
    }

    bool hashCompare(const FString &oth) const
    {
        return m_hash == oth.m_hash;
    }

    bool isEqual(const CharType_t *oth) const
    {
        return isEqual(oth, strlen(oth));
    }

    bool isEqual(const CharType_t *oth, uint32_t len) const
    {
        if (m_strLen != len)
            return false;

        CharType_t *c0 = m_buffer.get();
        CharType_t *c1 = oth

                assert(c0 && c1);

        uint32_t i = 0;

        for (; i < m_strLen && *c0 == *c1; ++c0, ++c1, ++i);

        return i == m_strLen;
    }

    bool isEqual(const FString &oth) const
    {
        return isEqual(oth.m_buffer.get(), oth.m_strLen);
    }

    bool operator==(const CharType_t *oth) const
    {

    }

    //! Only hash is compared here. Use isEqual method for strict comparison
    bool operator==(const FString &oth) const
    {
        return hashCompare(oth);
    }

    bool operator!=(const FString &oth) const
    {
        return !(operator==(oth));
    }

    void clear()
    {
        m_buffer.reset();
        m_strLen = 0;
        m_hash = 0;
        m_capacity = 0;
    }

    bool empty() const
    {
        return m_strLen == 0;
    }

    uint32_t getHash() const
    {
        return m_hash;
    }

    const CharType_t *buffer() const
    {
        return m_buffer.get();
    }

    // I don't like to see it here but wanna more compatibility
    const CharType_t *c_str() const
    {
        return m_buffer.get();
    }

    uint32_t lenght() const
    {
        return m_strLen;
    }

    uint32_t size() const
    {
        return m_strLen;
    }

};

inline std::ostream &operator<<(std::ostream &o, const FString<char> &str)
{
    o.write(str.buffer(), static_cast<std::streamsize>(str.lenght()));

    return o;
}

typedef FString<char> FString8_t;
typedef FString<char16_t> FString16_t;
typedef FString<char32_t> FString32_t;
typedef FString<wchar_t> FStringW_t;

inline FString8_t FStringConvertFrom16to8(const FString16_t &str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    FString8_t retval(convert.to_bytes(str.buffer()).c_str());

    return retval;
}

inline std::string FStringConvertFrom16to8STD(const FString16_t &str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    std::string retval = convert.to_bytes(str.buffer());

    return retval;
}

inline FString16_t FStringConvertFrom8to16(const FString8_t &str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    FString16_t retval(convert.from_bytes(str.buffer()).c_str());

    return retval;
}

template<class T>
struct FStringHaser
{
    size_t operator()(const T &str) const
    {
        return static_cast<size_t>(str.getHash());
    }
};
