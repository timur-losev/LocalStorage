#pragma once

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
#define leaf(b) b->p->leaf
#define ct(b) b->p->ct
#define next(b) b->p->next
#define prev(b) b->p->prev
#define fkey(b) &b->p->fkey
#define lkey(b) (fkey(b) + sizeOfKeyEntry((ct(b) - 1)))
#define p(b) (char *)(b->p)

/* shortcuts */
#define sizeOfKeyEntr(ct) ((ct) * h->sizeOfKeyEntry)
