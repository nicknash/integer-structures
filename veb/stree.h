#if !defined __STREE_H

#define __STREE_H

#define GCC3
#define USE_STD_ALLOCATOR

//#if defined VEB_COMPILE   
#   define NDEBUG    
//#endif

#include <veb/LVL1Tree.h>
#include <iostream>

template <class KeyType, class ValueType> class STree
{
    LVL1Tree* l1t;
    ValueType val;
public:
    STree()
    {
        if(sizeof(KeyType) != 4)
        {
            cerr << "FATAL ERROR: STree only works on 32-bit keys!" << endl;
            l1t = 0;
        }
        else
        {
            l1t = new LVL1Tree;
        }
    }
    void insert(const KeyType& key, const KeyType& value)
    {
        l1t->insert(value, key);
        return;
    }
    ValueType* locate(const KeyType& key)
    {
        l1t->locateNode(key);
        return 0;
        //return &val;
    }
    void remove(const KeyType& key)
    {
        l1t->del(key);
        return;
    }
    ~STree()
    {
        delete l1t;
    }
};

#endif
