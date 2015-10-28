#include "Precompiled.h"
//#include "DB/BTree/BTree.h"

struct BTreeNode
{
    int* data;
    BTreeNode** child;
    bool leaf;
    int n;

    BTreeNode()
    {
        data = new int[5];
        child = new BTreeNode*[6];
        leaf = true;
        n = 0;

        for (int i = 0; i < 6; ++i)
        {
            child[i] = nullptr;
        }
    }
};

BTreeNode* root = nullptr;
BTreeNode* currentNode = nullptr;


int splitChild(BTreeNode* node, int i)
{
    BTreeNode* newNode1 = nullptr;
    BTreeNode* newNode3 = new BTreeNode();

    newNode3->leaf = true;

    int mid = 0;

    if (i == -1)
    {
        mid = node->data[2];
        node->n--;

        newNode1 = new BTreeNode();
        newNode1->leaf = false;
        node->leaf = true;

        for (int j = 3; j < 5; ++i)
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

void insert(int a)
{
    if (!root)
    {
        root = new BTreeNode();
        currentNode = root;
    }


    if (currentNode->leaf == true && currentNode->n == 5)
    {
        int temp = splitChild(currentNode, -1);

        currentNode = root;

        int i = 0;

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

    return 0;
}