#if !defined __LPCQTRIE_H

#define __LPCQTRIE_H

#include <lpctrie/lpctrie.h>
#include <bucket_structs/bucket_structs.h>
#include <node_structs/node_structs.h>
#include <qtrie/qtrie.h>

template <class KeyType, class ValueType, bool count_mem = false> class LPCQTrie
{
    static const int MAX_BUCKET_SIZE = 128;

    typedef SortedBucket<KeyType, ValueType, count_mem> Bucket; 
/*
    typedef LPCTrie<KeyType, Bucket*, SqrtBitSearcher<count_mem > > LPCTrie_sqrt;
    typedef QTrie<KeyType, ValueType, LPCTrie_sqrt, Bucket, count_mem> LPCQTrie_internal;
    
    LPCQTrie_internal* lpcqtrie;
    LPCTrie_sqrt* lpctrie;
*/    
    typedef LPCTrie<KeyType, Bucket*, HeapBitSearcher<count_mem > > LPCTrie_heap;
    typedef QTrie<KeyType, ValueType, LPCTrie_heap, Bucket, count_mem> LPCQTrie_internal;
    
    LPCQTrie_internal* lpcqtrie;
    LPCTrie_heap* lpctrie;
public:
    LPCQTrie()
    {
        lpctrie = new LPCTrie_heap(4, 20, 0.75f, 0.25f);
        lpcqtrie = new LPCQTrie_internal(*lpctrie, MAX_BUCKET_SIZE);
        return;
    }
    void insert(const KeyType& key, const ValueType& value)
    {
        lpcqtrie->insert(key, value);        
        return;
    }
    ValueType* locate(const KeyType& key)
    {
        return lpcqtrie->locate(key);
    }
    void remove(const KeyType& key)
    {
        lpcqtrie->remove(key);
        return;
    }
    ~LPCQTrie()
    {
        delete lpctrie;
        delete lpcqtrie;
        return;
    }
};

#endif
