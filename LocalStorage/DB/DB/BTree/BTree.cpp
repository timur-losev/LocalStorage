#include "DBPrecompiled.h"
#include "BTree.h"

FErrType FBTreePlusPlus::open(const FTreeDescription& info, FHandleType *handle)
{
    FErrType rc;                /* return code */
    int numberOfTMPBuffers;                  /* number of tmp buffers */
    FNodeLocation *buf;         /* buffer */
    
    FNodeLocation *root;
    int i;
    FNode *p;

    if ((info.sectorSize < sizeof(FNodeTag)) || (info.sectorSize % 4))
    {
        return ErrSectorSize;
    }

    /* determine sizes and offsets */
    /* leaf/n, prev, next, [childLT,key,rec]... childGE */
    /* ensure that there are at least 3 children/parent for gather/scatter */

    uint32_t maxNumberOfKeysInANode = info.sectorSize - (sizeof(FNode) - sizeof(KeyType_t));
    maxNumberOfKeysInANode /= sizeof(bAdrType) + info.keySize + sizeof(eAdrType);

    if (maxNumberOfKeysInANode < 6)
        return ErrSectorSize;

    /* copy params to hNode */
    if ((currentNode = (FNodeTag*)FMemory::malloc(sizeof(FNodeTag))) == nullptr)
        return ErrMemory;

    FMemory::memset(currentNode, 0, sizeof(FNodeTag));
    currentNode->desc = info;

    /* childLT, key, rec */
    currentNode->sizeOfKeyEntry = sizeof(bAdrType) + currentNode->desc.keySize + sizeof(eAdrType);
    currentNode->maxKeyCount = maxNumberOfKeysInANode ;

    /* Allocate buflist.
    * During insert/delete, need simultaneous access to 7 buffers:
    *  - 4 adjacent child bufs
    *  - 1 parent buf
    *  - 1 next sequential link
    *  - 1 lastGE
    */
    numberOfTMPBuffers = 7;
    if ((currentNode->malloc1 = FMemory::malloc(numberOfTMPBuffers * sizeof(FNodeLocation))) == nullptr)
        return ErrMemory;
    buf = (FNodeLocation *)currentNode->malloc1;

    /*
    * Allocate bufs.
    * We need space for the following:
    *  - numberOfTMPBuffers buffers, of size sectorSize
    *  - 1 buffer for root, of size 3*sectorSize
    *  - 1 buffer for gbuf, size 3*sectorsize + 2 extra keys
    *    to allow for LT pointers in last 2 nodes when gathering 3 full nodes
    */
    if ((currentNode->malloc2 = FMemory::malloc((numberOfTMPBuffers + 6) * 
        currentNode->desc.sectorSize + 2 * currentNode->sizeOfKeyEntry)) == nullptr)
        return ErrMemory;

    p = (FNode*)currentNode->malloc2;

    /* initialize buflist */
    currentNode->bufList.next = buf;
    currentNode->bufList.prev = buf + (numberOfTMPBuffers - 1);

    for (i = 0; i < numberOfTMPBuffers; i++)
    {
        buf->next = buf + 1;
        buf->prev = buf - 1;
        buf->modified = false;
        buf->valid = false;
        buf->p = p;
        p = (FNode *)((char *)p + currentNode->desc.sectorSize);
        buf++;
    }

    currentNode->bufList.next->prev = &currentNode->bufList;
    currentNode->bufList.prev->next = &currentNode->bufList;

    /* initialize root */
    root = &currentNode->root;
    root->p = p;
    p = (FNode *)((char *)p + 3 * currentNode->desc.sectorSize);
    currentNode->gbuf.p = p;      /* done last to include extra 2 keys */

    currentNode->curBuf = nullptr;
    currentNode->curKey = nullptr;

    /* initialize root */
#if 0
    if ((h->fp = FPlatformDependedAPICall(fopen(info.name.c_str(), "r+b")) != nullptr)
    {
        /* open an existing database */
        if ((rc = readDisk(0, &root)) != 0) return rc;
        if (fseek(h->fp, 0, SEEK_END)) return error(bErrIO);
        if ((h->nextFreeAdr = ftell(h->fp)) == -1) return error(bErrIO);
    }
    else if ((h->fp = fopen(info.iName, "w+b")) != nullptr)
    {
        /* initialize root */
        memset(root->p, 0, 3 * h->sectorSize);
        leaf(root) = 1;
        h->nextFreeAdr = 3 * h->sectorSize;
    }
    else
    {
        /* something's wrong */
        free(h);
        return bErrFileNotOpen;
    }

    /* append node to hList */
    if (hList.next)
    {
        h->prev = hList.next;
        h->next = &hList;
        h->prev->next = h;
        h->next->prev = h;
    }
    else
    {
        /* first item in hList */
        h->prev = h->next = &hList;
        hList.next = hList.prev = h;
    }
#endif
    *handle = currentNode;
    return ErrOk;
}
