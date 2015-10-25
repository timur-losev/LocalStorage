#pragma once

enum FKeyComparisonResult
{
    CREqual = 0,
    CRGreater = 1,
    CRLower = -1
};

typedef std::function<int(const void* key1, const void* key2)> FComparisonFunction_t;

struct FTreeDescription
{
    FPath_t      name;              /* name of index file */
    uint32_t     keySize;           /* length, in bytes, of key */
    bool         dupKeys;                   /* true if duplicate keys allowed */
    uint32_t     sectorSize;                 /* size of sector on disk */
    FComparisonFunction_t comp;             /* pointer to compare function */

    FTreeDescription()
    {

    }

    ~FTreeDescription()
    {

    }

    FTreeDescription(const FTreeDescription& oth) :
        name(oth.name),
        keySize(oth.keySize),
        dupKeys(oth.dupKeys),
        sectorSize(oth.sectorSize),
        comp(oth.comp)
    {

    }

    FTreeDescription& operator=(const FTreeDescription& oth)
    {
        name = oth.name;
        keySize = oth.keySize;
        dupKeys = oth.dupKeys;
        sectorSize = oth.sectorSize;
        comp = oth.comp;
    }
};