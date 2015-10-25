#include "Precompiled.h"
#include "DB/BTree/BTree.h"

int main()
{
    FComparisonFunction_t func = [](const void* k1, const void* k2)->int
    {
        uint32_t const *p1;
        uint32_t const *p2;
        p1 = static_cast<uint32_t const*>(k1);
        p2 = static_cast<uint32_t const*>(k2);
        return (*p1 == *p2) ? CREqual : (*p1 > *p2) ? CRGreater : CRLower;
    };

    FTreeDescription info;
    void* handle;
    FErrType rc;
    uint32_t key;

    info.name = TEXT("t1.dat");
    info.keySize = sizeof(uint32_t);
    info.dupKeys = false;
    info.sectorSize = 4096;
    info.comp = func;
    FBTreePlusPlus tree;
    if ((rc = tree.open(info, &handle)) != ErrOk)
    {
        printf("line %d: rc = %d\n", __LINE__, rc);
        exit(0);
    }

    return 0;
}