#include "Precompiled.h"
//#include "DB/BTree/BTree.h"

#if 0
struct BTreeNode
{
    int data[5] = { 0 };
    std::vector<BTreeNode*> child;
    bool leaf;
    int n;

    BTreeNode()
    {
        child.resize(6);
        leaf = true;
        n = 0;

        for (int i = 0; i < 6; ++i)
        {
            child[i] = nullptr;
        }
    }
};

BTreeNode* root = nullptr;
//BTreeNode* currentNode = nullptr;

int splitChild(BTreeNode* node, int i)
{
    BTreeNode* newNode1 = nullptr;
    BTreeNode* newNode3 = new BTreeNode();

    newNode3->leaf = true;

    int mid = 0;

    if (i == -1)
    {
        mid = node->data[2];
        node->data[2] = 0;
        node->n--;

        newNode1 = new BTreeNode();
        newNode1->leaf = false;
        node->leaf = true;

        for (int j = 3; j < 5; ++j)
        {
            newNode3->data[j - 3] = node->data[j];
            newNode3->child[j - 3] = node->child[j];
            newNode3->n++;
            node->data[j] = 0;
            node->n--;
        }

        for (int j = 0; j < 6; ++j)
        {
            node->child[j] = nullptr;
        }

        newNode1->data[0] = mid;
        newNode1->child[newNode1->n] = node;
        newNode1->child[newNode1->n + 1] = newNode3;
        newNode1->n++;

        root = newNode1;
    }
    else
    {

    }

    return mid;
}

void sort(BTreeNode* node)
{
    int i, j, temp;

    int* p = node->data;

    for (i = 0; i < node->n; i++)
    {
        for (j = i; j <= node->n; j++)
        {
            if (p[i] > p[j])
            {
                temp = p[i];
                p[i] = p[j];
                p[j] = temp;
            }
        }
    }
}

void insert(int a)
{
    if (!root)
    {
        root = new BTreeNode();
    }

    BTreeNode* currentNode = root;

    int i = 0;

//     for (; i < currentNode->n; ++i)
//     {
// 
//     }

    if (currentNode->leaf == true && currentNode->n == 5)
    {
        splitChild(currentNode, -1);

        currentNode = root;

        i = 0;
        for (; i < currentNode->n; ++i)
        {
            if (a > currentNode->data[i] && a < currentNode->data[i + 1])
            {
                ++i;
                break;
            }
            else if(a < currentNode->data[0])
            {
                break;
            }
            else
            {
                continue;
            }
        }

        currentNode = currentNode->child[i];
    }

    currentNode->data[currentNode->n] = a;
    sort(currentNode);
    currentNode->n++;
}

int main()
{
//     FComparisonFunction_t func = [](const void* k1, const void* k2)->int
//     {
//         uint32_t const *p1;
//         uint32_t const *p2;
//         p1 = static_cast<uint32_t const*>(k1);
//         p2 = static_cast<uint32_t const*>(k2);
//         return (*p1 == *p2) ? CREqual : (*p1 > *p2) ? CRGreater : CRLower;
//     };
// 
//     FTreeDescription info;
//     void* handle;
//     FErrType rc;
//     uint32_t key;
// 
//     info.name = TEXT("t1.dat");
//     info.keySize = sizeof(uint32_t);
//     info.dupKeys = false;
//     info.sectorSize = 4096;
//     info.comp = func;
//     FBTreePlusPlus tree;
//     if ((rc = tree.open(info, &handle)) != ErrOk)
//     {
//         printf("line %d: rc = %d\n", __LINE__, rc);
//         exit(0);
//     }

    insert(12);
    insert(16);
    insert(7);
    insert(1);
    insert(8);
    insert(11);
    insert(2);

    return 0;
}

#else
/*
* this file is divided into sections:
*   stuff you'll probably want to place in a .h file...
*     implementation dependent
*       - you'll probably have to change something here
*     implementation independent
*       - types and function prototypes that typically go in a .h file
*     function prototypes
*       - prototypes for user functions
*   internals
*     - local functions
*     - user functions
*   main()
*/

/****************************
* implementation dependent *
****************************/
typedef long eAdrType;          /* record address for external record */
typedef long bAdrType;          /* record address for btree node */

#define CC_EQ           0
#define CC_GT           1
#define CC_LT          -1

                                /* compare two keys and return:
                                *    CC_LT     key1 < key2
                                *    CC_GT     key1 > key2
                                *    CC_EQ     key1 = key2
                                */
typedef int(*bCompType)(const void *key1, const void *key2);

/******************************
* implementation independent *
******************************/

/* statistics */
int maxHeight;          /* maximum height attained */
int nNodesIns;          /* number of nodes inserted */
int nNodesDel;          /* number of nodes deleted */
int nKeysIns;           /* number of keys inserted */
int nKeysDel;           /* number of keys deleted */
int nDiskReads;         /* number of disk reads */
int nDiskWrites;        /* number of disk writes */

                        /* line number for last IO or memory error */
int bErrLineNo;

enum bErrType
{
    bErrOk,
    bErrKeyNotFound,
    bErrDupKeys,
    bErrSectorSize,
    bErrFileNotOpen,
    bErrFileExists,
    bErrIO,
    bErrMemory
};

typedef void *bHandleType;

struct bOpenType 
{                               /* info for bOpen() */
    char *iName;                /* name of index file */
    int keySize;                /* length, in bytes, of key */
    bool dupKeys;               /* true if duplicate keys allowed */
    int sectorSize;             /* size of sector on disk */
    bCompType comp;             /* pointer to compare function */
};

/***********************
* function prototypes *
***********************/
bErrType bOpen(const bOpenType& info, bHandleType *handle);
/*
* input:
*   info                   info for open
* output:
*   handle                 handle to btree, used in subsequent calls
* returns:
*   bErrOk                 open was successful
*   bErrMemory             insufficient memory
*   bErrSectorSize         sector size too small or not 0 mod 4
*   bErrFileNotOpen        unable to open index file
*/

bErrType bClose(bHandleType handle);
/*
* input:
*   handle                 handle returned by bOpen
* returns:
*   bErrOk                 file closed, resources deleted
*/

bErrType bInsertKey(bHandleType handle, void *key, eAdrType rec);
/*
* input:
*   handle                 handle returned by bOpen
*   key                    key to insert
*   rec                    record address
* returns:
*   bErrOk                 operation successful
*   bErrDupKeys            duplicate keys (and info.dupKeys = false)
* notes:
*   If dupKeys is false, then all records inserted must have a
*   unique key.  If dupkeys is true, then duplicate keys are
*   allowed, but they must all have unique record addresses.
*   In this case, record addresses are included in internal
*   nodes to generate a "unique" key.
*/

bErrType bDeleteKey(bHandleType handle, void *key, eAdrType *rec);
/*
* input:
*   handle                 handle returned by bOpen
*   key                    key to delete
*   rec                    record address of key to delete
* output:
*   rec                    record address deleted
* returns:
*   bErrOk                 operation successful
*   bErrKeyNotFound        key not found
* notes:
*   If dupKeys is false, all keys are unique, and rec is not used
*   to determine which key to delete.  If dupKeys is true, then
*   rec is used to determine which key to delete.
*/

bErrType bFindKey(bHandleType handle, void *key, eAdrType *rec);
/*
* input:
*   handle                 handle returned by bOpen
*   key                    key to find
* output:
*   rec                    record address
* returns:
*   bErrOk                 operation successful
*   bErrKeyNotFound        key not found
*/

bErrType bFindFirstKey(bHandleType handle, void *key, eAdrType *rec);
/*
* input:
*   handle                 handle returned by bOpen
* output:
*   key                    first key in sequential set
*   rec                    record address
* returns:
*   bErrOk                 operation successful
*   bErrKeyNotFound        key not found
*/

bErrType bFindLastKey(bHandleType handle, void *key, eAdrType *rec);
/*
* input:
*   handle                 handle returned by bOpen
* output:
*   key                    last key in sequential set
*   rec                    record address
* returns:
*   bErrOk                 operation successful
*   bErrKeyNotFound        key not found
*/

bErrType bFindNextKey(bHandleType handle, void *key, eAdrType *rec);
/*
* input:
*   handle                 handle returned by bOpen
* output:
*   key                    key found
*   rec                    record address
* returns:
*   bErrOk                 operation successful
*   bErrKeyNotFound        key not found
*/

bErrType bFindPrevKey(bHandleType handle, void *key, eAdrType *rec);
/*
* input:
*   handle                 handle returned by bOpen
* output:
*   key                    key found
*   rec                    record address
* returns:
*   bErrOk                 operation successful
*   bErrKeyNotFound        key not found
*/

/*************
* internals *
*************/

/*
*  algorithm:
*    A B+tree implementation, with keys stored in internal nodes,
*    and keys/record addresses stored in leaf nodes.  Each node is
*    one sector in length, except the root node whose length is
*    3 sectors.  When traversing the tree to insert a key, full
*    children are adjusted to make room for possible new entries.
*    Similarly, on deletion, half-full nodes are adjusted to allow for
*    possible deleted entries.  Adjustments are first done by
*    examining 2 nearest neighbors at the same level, and redistibuting
*    the keys if possible.  If redistribution won't solve the problem,
*    nodes are split/joined as needed.  Typically, a node is 3/4 full.
*    On insertion, if 3 nodes are full, they are split into 4 nodes,
*    each 3/4 full.  On deletion, if 3 nodes are 1/2 full, they are
*    joined to create 2 nodes 3/4 full.
*
*    A LRR (least-recently-read) buffering scheme for nodes is used to
*    simplify storage management, and, assuming some locality of reference,
*    improve performance.
*
*    To simplify matters, both internal nodes and leafs contain the
*    same fields.
*
*/

/* macros for addressing fields */

/* primitives */
#define bAdr(p) *(bAdrType *)(p)
#define eAdr(p) *(eAdrType *)(p)

/* based on k = &[key,rec,childGE] */
#define childLT(k) bAdr((char *)k - sizeof(bAdrType))
#define key(k) (k)
#define rec(k) eAdr((char *)(k) + h->keySize)
#define childGE(k) bAdr((char *)(k) + h->keySize + sizeof(eAdrType))

/* based on b = &bufType */
#define leaf(b) b->locationInMemory->leaf
#define ct(b) b->locationInMemory->ct
#define next(b) b->locationInMemory->next
#define prev(b) b->locationInMemory->prev
#define fkey(b) &b->locationInMemory->fkey
#define lkey(b) (fkey(b) + ks((ct(b) - 1)))
#define p(b) (char *)(b->locationInMemory)

/* shortcuts */
#define ks(ct) ((ct) * h->sizeOfKeyEntry)

typedef char keyType;           /* keys entries are treated as char arrays */

struct nodeType 
{
    uint32_t leaf : 1;        /* first bit = 1 if leaf */
    uint32_t ct : 15;         /* count of keys present */
    bAdrType prev;              /* prev node in sequence (leaf) */
    bAdrType next;              /* next node in sequence (leaf) */
    bAdrType childLT;           /* child LT first key */
                                /* ct occurrences of [key,rec,childGE] */
    keyType fkey;               /* first occurrence */
};

struct bufType
{     /* location of node */
    struct bufType *next = nullptr;    /* next */
    struct bufType *prev = nullptr;    /* previous */
    bAdrType adr = 0;               /* on disk */
    nodeType *locationInMemory = nullptr;                /* in memory */
    bool valid = false;                 /* true if buffer contents valid */
    bool modified = false;              /* true if buffer modified */
};

/* one node for each open handle */
struct hNode
{
    struct hNode *prev;      /* previous node */
    struct hNode *next;      /* next node */
    FILE *fp;                   /* idx file */
    int keySize;                /* key length */
    bool dupKeys;               /* true if duplicate keys */
    int sectorSize;             /* block size for idx records */
    bCompType comp;             /* pointer to compare routine */
    bufType root;               /* root of b-tree, room for 3 sets */
    bufType bufList;            /* head of buf list */
    FArray<bufType*> bufLists;   /* malloc'd resources */
    void *malloc2;              /* malloc'd resources */
    bufType gatherBuffer;               /* gather buffer, room for 3 sets */
    bufType *curBuf;            /* current location */
    keyType *curKey;            /* current key in current node */
    unsigned int maximumNumberOfKeysInANode;         
    int sizeOfKeyEntry;                     /* sizeof key entry */
    bAdrType nextFreeAdr;       /* next free b-tree record address */
};

static hNode hList;             /* list of hNodes */
static hNode *h;                /* current hNode */

#define error(rc) lineError(__LINE__, rc)

static bErrType lineError(int lineno, bErrType rc)
{
    if (rc == bErrIO || rc == bErrMemory)
        if (!bErrLineNo)
            bErrLineNo = lineno;
    return rc;
}

static bAdrType allocAdr(void)
{
    bAdrType adr;
    adr = h->nextFreeAdr;
    h->nextFreeAdr += h->sectorSize;
    return adr;
}

static bErrType flush(bufType *buf)
{
    int len;            /* number of bytes to write */

                        /* flush buffer to disk */
    len = h->sectorSize;

    if (buf->adr == 0)
        len *= 3;        /* root */

    if (fseek(h->fp, buf->adr, SEEK_SET))
        return error(bErrIO);
    if (fwrite(buf->locationInMemory, len, 1, h->fp) != 1)
        return error(bErrIO);

    buf->modified = false;
    nDiskWrites++;
    return bErrOk;
}

static bErrType flushAll(void)
{
    bErrType rc;                /* return code */
    bufType *buf;               /* buffer */

    if (h->root.modified)
        if ((rc = flush(&h->root)) != 0) 
            return rc;

    buf = h->bufList.next;

    while (buf != &h->bufList)
    {
        if (buf->modified)
            if ((rc = flush(buf)) != 0)
                return rc;
        buf = buf->next;
    }

    return bErrOk;
}

static bErrType assignBuf(bAdrType adr, bufType **b)
{
    /* assign buf to adr */
    bufType *buf;               /* buffer */
    bErrType rc;                /* return code */

    if (adr == 0)
    {
        *b = &h->root;
        return bErrOk;
    }

    /* search for buf with matching adr */
    buf = h->bufList.next;
    while (buf->next != &h->bufList)
    {
        if (buf->valid && buf->adr == adr) 
            break;
        buf = buf->next;
    }

    /* either buf points to a match, or it's last one in list (LRR) */
    if (buf->valid)
    {
        if (buf->adr != adr)
        {
            if (buf->modified)
            {
                if ((rc = flush(buf)) != 0) 
                    return rc;
            }

            buf->adr = adr;
            buf->valid = false;
        }
    }
    else
    {
        buf->adr = adr;
    }

    /* remove from current position and place at front of list */
    buf->next->prev = buf->prev;
    buf->prev->next = buf->next;
    buf->next = h->bufList.next;
    buf->prev = &h->bufList;
    buf->next->prev = buf;
    buf->prev->next = buf;
    *b = buf;
    return bErrOk;
}

static bErrType writeDisk(bufType *buf)
{
    /* write buf to disk */
    buf->valid = true;
    buf->modified = true;
    return bErrOk;
}

static bErrType readDisk(bAdrType adr, bufType **b) 
{
    /* read data into buf */
    int len;
    bufType *buf;               /* buffer */
    bErrType rc;                /* return code */

    if ((rc = assignBuf(adr, &buf)) != 0)
        return rc;
    if (!buf->valid)
    {
        len = h->sectorSize;
        if (adr == 0) 
            len *= 3;         /* root */

        if (fseek(h->fp, adr, SEEK_SET)) 
            return error(bErrIO);

        if (fread(buf->locationInMemory, len, 1, h->fp) != 1)
            return error(bErrIO);

        buf->modified = false;
        buf->valid = true;
        nDiskReads++;
    }
    *b = buf;
    return bErrOk;
}

typedef enum { MODE_FIRST, MODE_MATCH } modeEnum;

static int search(bufType *buf, void *key, eAdrType rec, keyType **mkey, modeEnum mode)
{
    /*
    * input:
    *   p                      pointer to node
    *   key                    key to find
    *   rec                    record address (dupkey only)
    * output:
    *   k                      pointer to keyType info
    * returns:
    *   CC_EQ                  key = mkey
    *   CC_LT                  key < mkey
    *   CC_GT                  key > mkey
    */
    int cc;                     /* condition code */
    int m;                      /* midpoint of search */
    int lb;                     /* lower-bound of binary search */
    int ub;                     /* upper-bound of binary search */
    bool foundDup;              /* true if found a duplicate key */

                                /* scan current node for key using binary search */
    foundDup = false;
    lb = 0;
    ub = ct(buf) - 1;
    while (lb <= ub)
    {
        m = (lb + ub) / 2;
        *mkey = fkey(buf) + ks(m);
        cc = h->comp(key, key(*mkey));

        if (cc < 0)
            /* key less than key[m] */
            ub = m - 1;
        else if (cc > 0)
            /* key greater than key[m] */
            lb = m + 1;
        else 
        {
            /* keys match */
            if (h->dupKeys)
            {
                switch (mode)
                {
                case MODE_FIRST:
                    /* backtrack to first key */
                    ub = m - 1;
                    foundDup = true;
                    break;
                case MODE_MATCH:
                    /* rec's must also match */
                    if (rec < rec(*mkey))
                    {
                        ub = m - 1;
                        cc = CC_LT;
                    }
                    else if (rec > rec(*mkey))
                    {
                        lb = m + 1;
                        cc = CC_GT;
                    }
                    else
                    {
                        return CC_EQ;
                    }
                    break;
                }
            }
            else
            {
                return cc;
            }
        }
    }
    if (ct(buf) == 0)
    {
        /* empty list */
        *mkey = fkey(buf);
        return CC_LT;
    }
    if (h->dupKeys && (mode == MODE_FIRST) && foundDup)
    {
        /* next key is first key in set of duplicates */
        *mkey += ks(1);
        return CC_EQ;
    }
    /* didn't find key */
    return cc;
}

static bErrType scatterRoot(void)
{
    bufType *gbuf;
    bufType *root;

    /* scatter gbuf to root */

    root = &h->root;
    gbuf = &h->gatherBuffer;
    FMemory::memcpy(fkey(root), fkey(gbuf), ks(ct(gbuf)));
    childLT(fkey(root)) = childLT(fkey(gbuf));
    ct(root) = ct(gbuf);
    leaf(root) = leaf(gbuf);
    return bErrOk;
}

static bErrType scatter(bufType *pbuf, keyType *pkey, int is, bufType **tmp)
{
    bufType *gbuf;              /* gather buf */
    keyType *gkey;              /* gather buf key */
    bErrType rc;                /* return code */
    int iu;                     /* number of tmp's used */
    int k0Min;                  /* min #keys that can be mapped to tmp[0] */
    int knMin;                  /* min #keys that can be mapped to tmp[1..3] */
    int k0Max;                  /* max #keys that can be mapped to tmp[0] */
    int knMax;                  /* max #keys that can be mapped to tmp[1..3] */
    int sw;                     /* shift width */
    int len;                    /* length of remainder of buf */
    int base;                   /* base count distributed to tmps */
    int extra;                  /* extra counts */
    int ct;
    int i;

    /*
    * input:
    *   pbuf                   parent buffer of gathered keys
    *   pkey                   where we insert a key if needed in parent
    *   is                     number of supplied tmps
    *   tmp                    array of tmp's to be used for scattering
    * output:
    *   tmp                    array of tmp's used for scattering
    */

    /* scatter gbuf to tmps, placing 3/4 max in each tmp */

    gbuf = &h->gatherBuffer;
    gkey = fkey(gbuf);
    ct = ct(gbuf);

    /****************************************
    * determine number of tmps to use (iu) *
    ****************************************/
    iu = is;

    /* determine limits */
    if (leaf(gbuf))
    {
        /* minus 1 to allow for insertion */
        k0Max = h->maximumNumberOfKeysInANode - 1;
        knMax = h->maximumNumberOfKeysInANode - 1;
        /* plus 1 to allow for deletion */
        k0Min = (h->maximumNumberOfKeysInANode / 2) + 1;
        knMin = (h->maximumNumberOfKeysInANode / 2) + 1;
    }
    else 
    {
        /* can hold an extra gbuf key as it's translated to a LT pointer */
        k0Max = h->maximumNumberOfKeysInANode - 1;
        knMax = h->maximumNumberOfKeysInANode;
        k0Min = (h->maximumNumberOfKeysInANode / 2) + 1;
        knMin = ((h->maximumNumberOfKeysInANode + 1) / 2) + 1;
    }

    /* calculate iu, number of tmps to use */
    while (1)
    {
        if (iu == 0 || ct > (k0Max + (iu - 1)*knMax))
        {
            /* add a buffer */
            if ((rc = assignBuf(allocAdr(), &tmp[iu])) != 0)
                return rc;
            /* update sequential links */
            if (leaf(gbuf)) 
            {
                /* adjust sequential links */
                if (iu == 0)
                {
                    /* no tmps supplied when splitting root for first time */
                    prev(tmp[0]) = 0;
                    next(tmp[0]) = 0;
                }
                else
                {
                    prev(tmp[iu]) = tmp[iu - 1]->adr;
                    next(tmp[iu]) = next(tmp[iu - 1]);
                    next(tmp[iu - 1]) = tmp[iu]->adr;
                }
            }
            iu++;
            nNodesIns++;
        }
        else if (iu > 1 && ct < (k0Min + (iu - 1)*knMin))
        {
            /* del a buffer */
            iu--;
            /* adjust sequential links */
            if (leaf(gbuf) && tmp[iu - 1]->adr) {
                next(tmp[iu - 1]) = next(tmp[iu]);
            }
            next(tmp[iu - 1]) = next(tmp[iu]);
            nNodesDel++;
        }
        else
        {
            break;
        }
    }

    /* establish count for each tmp used */
    base = ct / iu;
    extra = ct % iu;
    for (i = 0; i < iu; i++)
    {
        int n;

        n = base;
        /* distribute extras, one at a time */
        /* don't do to 1st node, as it may be internal and can't hold it */
        if (i && extra) 
        {
            n++;
            extra--;
        }
        ct(tmp[i]) = n;
    }


    /**************************************
    * update sequential links and parent *
    **************************************/
    if (iu != is)
    {
        /* link last node to next */
        if (leaf(gbuf) && next(tmp[iu - 1])) 
        {
            bufType *buf;
            if ((rc = readDisk(next(tmp[iu - 1]), &buf)) != 0) return rc;
            prev(buf) = tmp[iu - 1]->adr;
            if ((rc = writeDisk(buf)) != 0) return rc;
        }
        /* shift keys in parent */
        sw = ks(iu - is);
        if (sw < 0)
        {
            len = ks(ct(pbuf)) - (pkey - fkey(pbuf)) + sw;
            FMemory::memmove(pkey, pkey - sw, len);
        }
        else
        {
            len = ks(ct(pbuf)) - (pkey - fkey(pbuf));
            FMemory::memmove(pkey + sw, pkey, len);
        }
        /* don't count LT buffer for empty parent */
        if (ct(pbuf))
            ct(pbuf) += iu - is;
        else
            ct(pbuf) += iu - is - 1;
    }

    /*******************************
    * distribute keys to children *
    *******************************/
    for (i = 0; i < iu; i++)
    {
        /* update LT pointer and parent nodes */
        if (leaf(gbuf))
        {
            /* update LT, tmp[i] */
            childLT(fkey(tmp[i])) = 0;

            /* update parent */
            if (i == 0)
            {
                childLT(pkey) = tmp[i]->adr;
            }
            else
            {
                FMemory::memcpy(pkey, gkey, ks(1));
                childGE(pkey) = tmp[i]->adr;
                pkey += ks(1);
            }
        }
        else
        {
            if (i == 0)
            {
                /* update LT, tmp[0] */
                childLT(fkey(tmp[i])) = childLT(gkey);
                /* update LT, parent */
                childLT(pkey) = tmp[i]->adr;
            }
            else
            {
                /* update LT, tmp[i] */
                childLT(fkey(tmp[i])) = childGE(gkey);
                /* update parent key */
                FMemory::memcpy(pkey, gkey, ks(1));
                childGE(pkey) = tmp[i]->adr;
                gkey += ks(1);
                pkey += ks(1);
                ct(tmp[i])--;
            }
        }

        /* install keys, tmp[i] */
        FMemory::memcpy(fkey(tmp[i]), gkey, ks(ct(tmp[i])));
        leaf(tmp[i]) = leaf(gbuf);

        gkey += ks(ct(tmp[i]));
    }
    leaf(pbuf) = false;

    /************************
    * write modified nodes *
    ************************/
    if ((rc = writeDisk(pbuf)) != 0)
        return rc;
    for (i = 0; i < iu; i++)
        if ((rc = writeDisk(tmp[i])) != 0)
            return rc;

    return bErrOk;
}

static bErrType gatherRoot(void) 
{
    bufType *gbuf;
    bufType *root;

    /* gather root to gbuf */
    root = &h->root;
    gbuf = &h->gatherBuffer;
    FMemory::memcpy(p(gbuf), root->locationInMemory, 3 * h->sectorSize);
    leaf(gbuf) = leaf(root);
    ct(root) = 0;
    return bErrOk;
}

static bErrType gather(bufType *pbuf, keyType **pkey, bufType **tmp)
{
    bErrType rc;                /* return code */
    bufType *gbuf;
    keyType *gkey;

    /*
    * input:
    *   pbuf                   parent buffer
    *   pkey                   pointer to match key in parent
    * output:
    *   tmp                    buffers to use for scatter
    *   pkey                   pointer to match key in parent
    * returns:
    *   bErrOk                 operation successful
    * notes:
    *   Gather 3 buffers to gbuf.  Setup for subsequent scatter by
    *   doing the following:
    *     - setup tmp buffer array for scattered buffers
    *     - adjust pkey to point to first key of 3 buffers
    */

    /* find 3 adjacent buffers */
    if (*pkey == lkey(pbuf))
        *pkey -= ks(1);
    if ((rc = readDisk(childLT(*pkey), &tmp[0])) != 0) 
        return rc;

    if ((rc = readDisk(childGE(*pkey), &tmp[1])) != 0) 
        return rc;

    if ((rc = readDisk(childGE(*pkey + ks(1)), &tmp[2])) != 0) 
        return rc;

    /* gather nodes to gbuf */
    gbuf = &h->gatherBuffer;
    gkey = fkey(gbuf);

    /* tmp[0] */
    childLT(gkey) = childLT(fkey(tmp[0]));
    FMemory::memcpy(gkey, fkey(tmp[0]), ks(ct(tmp[0])));
    gkey += ks(ct(tmp[0]));
    ct(gbuf) = ct(tmp[0]);

    /* tmp[1] */
    if (!leaf(tmp[1]))
    {
        FMemory::memcpy(gkey, *pkey, ks(1));
        childGE(gkey) = childLT(fkey(tmp[1]));
        ct(gbuf)++;
        gkey += ks(1);
    }

    FMemory::memcpy(gkey, fkey(tmp[1]), ks(ct(tmp[1])));
    gkey += ks(ct(tmp[1]));
    ct(gbuf) += ct(tmp[1]);

    /* tmp[2] */
    if (!leaf(tmp[2]))
    {
        FMemory::memcpy(gkey, *pkey + ks(1), ks(1));
        childGE(gkey) = childLT(fkey(tmp[2]));
        ct(gbuf)++;
        gkey += ks(1);
    }

    FMemory::memcpy(gkey, fkey(tmp[2]), ks(ct(tmp[2])));
    ct(gbuf) += ct(tmp[2]);

    leaf(gbuf) = leaf(tmp[0]);

    return bErrOk;
}

bErrType bOpen(const bOpenType& info, bHandleType *handle)
{
    bErrType rc;                /* return code */
    int numberOfTmpBuffers;                  /* number of tmp buffers */
    bufType *buf;               /* buffer */
    int maximumNumberOfKeysInANode;                  /* maximum number of keys in a node */
    nodeType *p;

    if ((info.sectorSize < sizeof(hNode)) || (info.sectorSize % 4))
        return bErrSectorSize;

    /* determine sizes and offsets */
    /* leaf/n, prev, next, [childLT,key,rec]... childGE */
    /* ensure that there are at least 3 children/parent for gather/scatter */
    maximumNumberOfKeysInANode = info.sectorSize - (sizeof(nodeType) - sizeof(keyType));
    maximumNumberOfKeysInANode /= sizeof(bAdrType) + info.keySize + sizeof(eAdrType);
    if (maximumNumberOfKeysInANode < 6) return bErrSectorSize;

    /* copy parms to hNode */
    if ((h = (hNode*)FMemory::malloc(sizeof(hNode))) == NULL)
        return error(bErrMemory);

    FMemory::memset(h, 0, sizeof(hNode));
    h->keySize = info.keySize;
    h->dupKeys = info.dupKeys;
    h->sectorSize = info.sectorSize;
    h->comp = info.comp;

    /* childLT, key, rec */
    h->sizeOfKeyEntry = sizeof(bAdrType) + h->keySize + sizeof(eAdrType);
    h->maximumNumberOfKeysInANode = maximumNumberOfKeysInANode;

    /* Allocate buflist.
    * During insert/delete, need simultaneous access to 7 buffers:
    *  - 4 adjacent child bufs
    *  - 1 parent buf
    *  - 1 next sequential link
    *  - 1 lastGE
    */
    numberOfTmpBuffers = 7;
    h->bufLists.resize(numberOfTmpBuffers);

    for (uint32_t i = 0; i < numberOfTmpBuffers; ++i)
    {
        h->bufLists[i] = new bufType();// static_cast<bufType*>(FMemory::malloc(sizeof(bufType)));
    }

    /*
    * Allocate bufs.
    * We need space for the following:
    *  - numberOfTmpBuffers buffers, of size sectorSize
    *  - 1 buffer for root, of size 3*sectorSize
    *  - 1 buffer for gatherBuffer, size 3*sectorsize + 2 extra keys
    *    to allow for LT pointers in last 2 nodes when gathering 3 full nodes
    */
    const size_t malloc2Size = (numberOfTmpBuffers + 6) * h->sectorSize + 2 * h->sizeOfKeyEntry;
    if ((h->malloc2 = FMemory::malloc(malloc2Size)) == NULL)
        return error(bErrMemory);
    p = (nodeType *)h->malloc2;

    /* initialize buflist */
    h->bufList.next = h->bufLists[0];
    h->bufList.prev = h->bufLists[h->bufLists.size() - 1];
    for (uint32_t i = 0; i < numberOfTmpBuffers; i++)
    {
        h->bufLists[i]->next = (i + 1) >= numberOfTmpBuffers ? nullptr : h->bufLists[i + 1];
        h->bufLists[i]->prev = i == 0 ? nullptr : h->bufLists[i - 1];
        h->bufLists[i]->modified = false;
        h->bufLists[i]->valid = false;
        h->bufLists[i]->locationInMemory = p;
        p = (nodeType *)((char *)p + h->sectorSize);
    }
    h->bufList.next->prev = &h->bufList;
    h->bufList.prev->next = &h->bufList;

    /* initialize root */
    bufType *root = &h->root;
    root->locationInMemory = p;
    p = (nodeType *)((char *)p + 3 * h->sectorSize);
    h->gatherBuffer.locationInMemory = p;      /* done last to include extra 2 keys */

    h->curBuf = NULL;
    h->curKey = NULL;

    /* initialize root */
    if ((h->fp = fopen(info.iName, "r+b")) != NULL)
    {
        /* open an existing database */
        if ((rc = readDisk(0, &root)) != 0) 
            return rc;
        if (fseek(h->fp, 0, SEEK_END))
            return error(bErrIO);

        if ((h->nextFreeAdr = ftell(h->fp)) == -1)
            return error(bErrIO);
    }
    else if ((h->fp = fopen(info.iName, "w+b")) != NULL)
    {
        /* initialize root */
        FMemory::memset(root->locationInMemory, 0, 3 * h->sectorSize);
        leaf(root) = 1;
        h->nextFreeAdr = 3 * h->sectorSize;
    }
    else 
    {
        /* something's wrong */
        FMemory::free(h);
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

    *handle = h;
    return bErrOk;
}


bErrType bClose(bHandleType handle)
{
    h = (hNode*)handle;

    if (h == NULL) 
        return bErrOk;

    /* remove from list */
    if (h->next)
    {
        h->next->prev = h->prev;
        h->prev->next = h->next;
    }

    /* flush idx */
    if (h->fp)
    {
        flushAll();
        fclose(h->fp);
    }

    if (h->malloc2) 
        FMemory::free(h->malloc2);

//     if (h->malloc1)
//         FMemory::free(h->malloc1);

    FMemory::free(h);
    return bErrOk;
}

bErrType bFindKey(bHandleType handle, void *key, eAdrType *rec)
{
    keyType *mkey;              /* matched key */
    bufType *buf;               /* buffer */
    bErrType rc;                /* return code */

    h = (hNode*)handle;
    buf = &h->root;

    /* find key, and return address */
    while (1)
    {
        if (leaf(buf))
        {
            if (search(buf, key, 0, &mkey, MODE_FIRST) == 0) 
            {
                *rec = rec(mkey);
                h->curBuf = buf; h->curKey = mkey;
                return bErrOk;
            }
            else
            {
                return bErrKeyNotFound;
            }
        }
        else
        {
            if (search(buf, key, 0, &mkey, MODE_FIRST) < 0)
            {
                if ((rc = readDisk(childLT(mkey), &buf)) != 0)
                    return rc;
            }
            else
            {
                if ((rc = readDisk(childGE(mkey), &buf)) != 0)
                    return rc;
            }
        }
    }
}

bErrType bInsertKey(bHandleType handle, void *key, eAdrType rec) 
{
    bErrType rc;                     /* return code */
    keyType *mkey;              /* match key */
    int len;                    /* length to shift */
    int cc;                     /* condition code */
    bufType *buf, *root;
    bufType *tmp[4];
    unsigned int keyOff;
    bool lastGEvalid;           /* true if GE branch taken */
    bool lastLTvalid;           /* true if LT branch taken after GE branch */
    bAdrType lastGE;            /* last childGE traversed */
    unsigned int lastGEkey;     /* last childGE key traversed */
    int height;                 /* height of tree */

    h = (hNode*)handle;
    root = &h->root;
    lastGEvalid = false;
    lastLTvalid = false;

    /* check for full root */
    if (ct(root) == 3 * h->maximumNumberOfKeysInANode) 
    {
        /* gather root and scatter to 4 bufs */
        /* this increases b-tree height by 1 */
        if ((rc = gatherRoot()) != 0) 
            return rc;
        if ((rc = scatter(root, fkey(root), 0, tmp)) != 0) 
            return rc;
    }

    buf = root;
    height = 0;
    while (1)
    {
        if (leaf(buf))
        {
            /* in leaf, and there' room guaranteed */

            if (height > maxHeight) maxHeight = height;

            /* set mkey to point to insertion point */
            switch (search(buf, key, rec, &mkey, MODE_MATCH))
            {
            case CC_LT:  /* key < mkey */
                if (!h->dupKeys && h->comp(key, mkey) == CC_EQ)
                    return bErrDupKeys;
                break;
            case CC_EQ:  /* key = mkey */
                return bErrDupKeys;
                break;
            case CC_GT:  /* key > mkey */
                if (!h->dupKeys && h->comp(key, mkey) == CC_EQ)
                    return bErrDupKeys;
                mkey += ks(1);
                break;
            }

            /* shift items GE key to right */
            keyOff = mkey - fkey(buf);
            len = ks(ct(buf)) - keyOff;
            if (len) 
                FMemory::memmove(mkey + ks(1), mkey, len);

            /* insert new key */
            FMemory::memcpy(key(mkey), key, h->keySize);
            
            rec(mkey) = rec;
            childGE(mkey) = 0;
            ct(buf)++;

            if ((rc = writeDisk(buf)) != 0) 
                return rc;

            /* if new key is first key, then fixup lastGE key */
            if (!keyOff && lastLTvalid)
            {
                bufType *tbuf;
                keyType *tkey;
                if ((rc = readDisk(lastGE, &tbuf)) != 0)
                    return rc;

                tkey = fkey(tbuf) + lastGEkey;
                FMemory::memcpy(key(tkey), key, h->keySize);
                rec(tkey) = rec;
                if ((rc = writeDisk(tbuf)) != 0) 
                    return rc;
            }
            nKeysIns++;
            break;
        }
        else 
        {
            /* internal node, descend to child */
            bufType *cbuf;      /* child buf */

            height++;

            /* read child */
            if ((cc = search(buf, key, rec, &mkey, MODE_MATCH)) < 0)
            {
                if ((rc = readDisk(childLT(mkey), &cbuf)) != 0) 
                    return rc;
            }
            else
            {
                if ((rc = readDisk(childGE(mkey), &cbuf)) != 0) 
                    return rc;
            }

            /* check for room in child */
            if (ct(cbuf) == h->maximumNumberOfKeysInANode) 
            {

                /* gather 3 bufs and scatter */
                if ((rc = gather(buf, &mkey, tmp)) != 0) 
                    return rc;
                if ((rc = scatter(buf, mkey, 3, tmp)) != 0) 
                    return rc;

                /* read child */
                if ((cc = search(buf, key, rec, &mkey, MODE_MATCH)) < 0) 
                {
                    if ((rc = readDisk(childLT(mkey), &cbuf)) != 0) 
                        return rc;
                }
                else 
                {
                    if ((rc = readDisk(childGE(mkey), &cbuf)) != 0) 
                        return rc;
                }
            }

            if (cc >= 0 || mkey != fkey(buf))
            {
                lastGEvalid = true;
                lastLTvalid = false;
                lastGE = buf->adr;
                lastGEkey = mkey - fkey(buf);
                if (cc < 0) lastGEkey -= ks(1);
            }
            else 
            {
                if (lastGEvalid) lastLTvalid = true;
            }
            buf = cbuf;
        }
    }

    return bErrOk;
}

bErrType bDeleteKey(bHandleType handle, void *key, eAdrType *rec)
{
    bErrType rc;                     /* return code */
    keyType *mkey;              /* match key */
    int len;                    /* length to shift */
    int cc;                     /* condition code */
    bufType *buf;               /* buffer */
    bufType *tmp[4];
    unsigned int keyOff;
    bool lastGEvalid;           /* true if GE branch taken */
    bool lastLTvalid;           /* true if LT branch taken after GE branch */
    bAdrType lastGE;            /* last childGE traversed */
    unsigned int lastGEkey;     /* last childGE key traversed */
    bufType *root;
    bufType *gbuf;

    h = (hNode*)handle;
    root = &h->root;
    gbuf = &h->gatherBuffer;
    lastGEvalid = false;
    lastLTvalid = false;

    buf = root;
    while (1)
    {
        if (leaf(buf))
        {

            /* set mkey to point to deletion point */
            if (search(buf, key, *rec, &mkey, MODE_MATCH) == 0)
                *rec = rec(mkey);
            else
                return bErrKeyNotFound;

            /* shift items GT key to left */
            keyOff = mkey - fkey(buf);
            len = ks(ct(buf) - 1) - keyOff;
            if (len)
                FMemory::memmove(mkey, mkey + ks(1), len);
            ct(buf)--;

            if ((rc = writeDisk(buf)) != 0) 
                return rc;

            /* if deleted key is first key, then fixup lastGE key */
            if (!keyOff && lastLTvalid)
            {
                bufType *tbuf;
                keyType *tkey;
                if ((rc = readDisk(lastGE, &tbuf)) != 0) 
                    return rc;
                tkey = fkey(tbuf) + lastGEkey;

                FMemory::memcpy(key(tkey), mkey, h->keySize);

                rec(tkey) = rec(mkey);

                if ((rc = writeDisk(tbuf)) != 0) 
                    return rc;
            }
            nKeysDel++;
            break;
        }
        else
        {
            /* internal node, descend to child */
            bufType *cbuf;      /* child buf */

                                /* read child */
            if ((cc = search(buf, key, *rec, &mkey, MODE_MATCH)) < 0)
            {
                if ((rc = readDisk(childLT(mkey), &cbuf)) != 0) 
                    return rc;
            }
            else
            {
                if ((rc = readDisk(childGE(mkey), &cbuf)) != 0) 
                    return rc;
            }

            /* check for room to delete */
            if (ct(cbuf) == h->maximumNumberOfKeysInANode / 2)
            {

                /* gather 3 bufs and scatter */
                if ((rc = gather(buf, &mkey, tmp)) != 0) 
                    return rc;

                /* if last 3 bufs in root, and count is low enough... */
                if (buf == root
                    && ct(root) == 2
                    && ct(gbuf) < (3 * (3 * h->maximumNumberOfKeysInANode)) / 4)
                {
                    /* collapse tree by one level */
                    scatterRoot();
                    nNodesDel += 3;
                    continue;
                }

                if ((rc = scatter(buf, mkey, 3, tmp)) != 0) 
                    return rc;

                /* read child */
                if ((cc = search(buf, key, *rec, &mkey, MODE_MATCH)) < 0)
                {
                    if ((rc = readDisk(childLT(mkey), &cbuf)) != 0) 
                        return rc;
                }
                else 
                {
                    if ((rc = readDisk(childGE(mkey), &cbuf)) != 0) return rc;
                }
            }
            if (cc >= 0 || mkey != fkey(buf))
            {
                lastGEvalid = true;
                lastLTvalid = false;
                lastGE = buf->adr;
                lastGEkey = mkey - fkey(buf);
                if (cc < 0) lastGEkey -= ks(1);
            }
            else
            {
                if (lastGEvalid) lastLTvalid = true;
            }
            buf = cbuf;
        }
    }

    return bErrOk;
}

bErrType bFindFirstKey(bHandleType handle, void *key, eAdrType *rec)
{
    bErrType rc;                /* return code */
    bufType *buf;               /* buffer */

    h = (hNode*)handle;
    buf = &h->root;
    while (!leaf(buf))
    {
        if ((rc = readDisk(childLT(fkey(buf)), &buf)) != 0) 
            return rc;
    }

    if (ct(buf) == 0) return bErrKeyNotFound;
    FMemory::memcpy(key, key(fkey(buf)), h->keySize);
    *rec = rec(fkey(buf));
    h->curBuf = buf;
    h->curKey = fkey(buf);

    return bErrOk;
}

bErrType bFindLastKey(bHandleType handle, void *key, eAdrType *rec)
{
    bErrType rc;                /* return code */
    bufType *buf;               /* buffer */

    h = (hNode*)handle;
    buf = &h->root;
    while (!leaf(buf)) 
    {
        if ((rc = readDisk(childGE(lkey(buf)), &buf)) != 0) 
            return rc;
    }

    if (ct(buf) == 0)
        return bErrKeyNotFound;

    FMemory::memcpy(key, key(lkey(buf)), h->keySize);
    *rec = rec(lkey(buf));
    h->curBuf = buf; 
    h->curKey = lkey(buf);

    return bErrOk;
}

bErrType bFindNextKey(bHandleType handle, void *key, eAdrType *rec)
{
    bErrType rc;                /* return code */
    keyType *nkey;              /* next key */
    bufType *buf;               /* buffer */

    h = (hNode*)handle;
    if ((buf = h->curBuf) == NULL) 
        return bErrKeyNotFound;

    if (h->curKey == lkey(buf))
    {
        /* current key is last key in leaf node */
        if (next(buf))
        {
            /* fetch next set */
            if ((rc = readDisk(next(buf), &buf)) != 0) return rc;
            nkey = fkey(buf);
        }
        else
        {
            /* no more sets */
            return bErrKeyNotFound;
        }
    }
    else
    {
        /* bump to next key */
        nkey = h->curKey + ks(1);
    }
    FMemory::memcpy(key, key(nkey), h->keySize);
    *rec = rec(nkey);
    h->curBuf = buf; h->curKey = nkey;
    return bErrOk;
}

bErrType bFindPrevKey(bHandleType handle, void *key, eAdrType *rec)
{
    bErrType rc;                /* return code */
    keyType *pkey;              /* previous key */
    keyType *fkey;              /* first key */
    bufType *buf;               /* buffer */

    h = (hNode*)handle;
    if ((buf = h->curBuf) == NULL) 
        return bErrKeyNotFound;

    fkey = fkey(buf);
    if (h->curKey == fkey)
    {
        /* current key is first key in leaf node */
        if (prev(buf))
        {
            /* fetch previous set */
            if ((rc = readDisk(prev(buf), &buf)) != 0) return rc;
            pkey = fkey(buf) + ks((ct(buf) - 1));
        }
        else 
        {
            /* no more sets */
            return bErrKeyNotFound;
        }
    }
    else
    {
        /* bump to previous key */
        pkey = h->curKey - ks(1);
    }

    FMemory::memcpy(key, key(pkey), h->keySize);
    *rec = rec(pkey);
    h->curBuf = buf; h->curKey = pkey;
    return bErrOk;
}

int comp(const void *key1, const void *key2)
{
    unsigned int const *p1;
    unsigned int const *p2;
    p1 = (const unsigned int*)key1;
    p2 = (const unsigned int*)key2;
    return (*p1 == *p2) ? CC_EQ : (*p1 > *p2) ? CC_GT : CC_LT;
}

int main(void)
{
    bOpenType info;
    bHandleType handle;
    bErrType rc;
    unsigned int key;

    remove("t1.dat");

    info.iName = "t1.dat";
    info.keySize = sizeof(int);
    info.dupKeys = false;
    info.sectorSize = 4096;
    info.comp = comp;
    if ((rc = bOpen(info, &handle)) != bErrOk)
    {
        printf("line %d: rc = %d\n", __LINE__, rc);
        exit(0);
    }

    key = 0x11;
    if ((rc = bInsertKey(handle, &key, 0x300)) != bErrOk)
    {
        printf("line %d: rc = %d\n", __LINE__, rc);
        exit(0);
    }

    bClose(handle);

    printf("statistics:\n");
    printf("    maximum height: %8d\n", maxHeight);
    printf("    nodes inserted: %8d\n", nNodesIns);
    printf("    nodes deleted:  %8d\n", nNodesDel);
    printf("    keys inserted:  %8d\n", nKeysIns);
    printf("    keys deleted:   %8d\n", nKeysDel);
    printf("    disk reads:     %8d\n", nDiskReads);
    printf("    disk writes:    %8d\n", nDiskWrites);

    return 0;
}


#endif