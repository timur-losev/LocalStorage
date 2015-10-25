#pragma once

#include "TreeError.h"
#include "TreeDescription.h"

typedef long eAdrType;          /* record address for external record */
typedef long bAdrType;          /* record address for btree node */

// /* statistics */
// int maxHeight;          /* maximum height attained */
// int nNodesIns;          /* number of nodes inserted */
// int nNodesDel;          /* number of nodes deleted */
// int nKeysIns;           /* number of keys inserted */
// int nKeysDel;           /* number of keys deleted */
// int nDiskReads;         /* number of disk reads */
// int nDiskWrites;        /* number of disk writes */
// 
//                         /* line number for last IO or memory error */
// int bErrLineNo;



typedef void *FHandleType;

typedef char KeyType_t;             /* keys entries are treated as char arrays */

struct FNode
{
    uint32_t leaf : 1;              /* first bit = 1 if leaf */
    uint32_t countOfKeysPresent : 15;       /* count of keys present */
    void* prev;                     /* prev node in sequence (leaf) */
    void* next;                     /* next node in sequence (leaf) */
    void* childLT;                  /* child LT first key */
                                    /* ct occurrences of [key,rec,childGE] */
    KeyType_t fkey;                 /* first occurrence */
};

struct FNodeLocation
{
    FNodeLocation *next;    /* next */
    FNodeLocation *prev;    /* previous */
    void* adr;               /* on disk */
    FNode *p;                   /* in memory */
    bool valid;                 /* true if buffer contents valid */
    bool modified;              /* true if buffer modified */
};

/* one node for each open handle */
struct FNodeTag 
{
    struct FNodeTag *prev;      /* previous node */
    struct FNodeTag *next;      /* next node */
    FILE *fp;                   /* idx file */
//     int keySize;                /* key length */
//     bool dupKeys;               /* true if duplicate keys */
//     int sectorSize;             /* block size for idx records */
//     FComparisonFunction_t comp;             /* pointer to compare routine */
    FTreeDescription desc;
    FNodeLocation root;               /* root of b-tree, room for 3 sets */
    FNodeLocation bufList;            /* head of buf list */
    void *malloc1;              /* malloc'd resources */
    void *malloc2;              /* malloc'd resources */
    FNodeLocation gbuf;               /* gather buffer, room for 3 sets */
    FNodeLocation *curBuf;            /* current location */
    FNodeLocation *curKey;            /* current key in current node */
    uint32_t maxKeyCount;
    uint32_t sizeOfKeyEntry;                     /* sizeof key entry */
    bAdrType nextFreeAdr;          /* next free b-tree record address */
};

static FNodeTag hList;             /* list of hNodes */
static FNodeTag *currentNode;      /* current hNode */

class FBTreePlusPlus
{
public:
    FErrType open(const FTreeDescription& info, FHandleType *handle);
};

