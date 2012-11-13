#if !defined __LPCBTRIE_H

#define __LPCBTRIE_H

#include <lpctrie/lpctrie.h>
#include <bucket_structs/bucket_structs.h>
#include <node_structs/node_structs.h>
#include <btrie/btrie.h>

template <class KeyType, class ValueType, bool count_mem = false> class LPCBTrie
{
    static const int MAX_BUCKET_SIZE = 128;
/*
    typedef SortedBucket<KeyType, ValueType, count_mem> Bucket; 
    typedef SqrtBitSearcher<count_mem> NodeStruct;
    typedef LPCTrie<KeyType, Bucket*, NodeStruct, count_mem > LPCTrie_sqrt;
    typedef LevelPathCompTrieBurst<KeyType, ValueType, LPCTrie_sqrt, Bucket, count_mem> LPCTrieBurst;    

    typedef BTrie<KeyType, ValueType, LPCTrie_sqrt, LPCTrieBurst, Bucket, count_mem> LPCBTrie_internal; 
 

    LPCBTrie_internal* lpcbtrie;
    LPCTrie_sqrt* lpctrie;


*/    
    typedef SortedBucket<KeyType, ValueType, count_mem> Bucket; 
    typedef HeapBitSearcher<count_mem> NodeStruct;
    typedef LPCTrie<KeyType, Bucket*, NodeStruct, count_mem > LPCTrie_heap;
    typedef LevelPathCompTrieBurst<KeyType, ValueType, LPCTrie_heap, Bucket, count_mem> LPCTrieBurst;    

    typedef BTrie<KeyType, ValueType, LPCTrie_heap, LPCTrieBurst, Bucket, count_mem> LPCBTrie_internal; 
 

    LPCBTrie_internal* lpcbtrie;
    LPCTrie_heap* lpctrie;

public:
    LPCBTrie()
    {
  /*
        lpctrie = new LPCTrie_sqrt(4, 24, 0.75f, 0.25f);
        lpcbtrie = new LPCBTrie_internal(*lpctrie, MAX_BUCKET_SIZE);
    */    
        lpctrie = new LPCTrie_heap(4, 24, 0.75f, 0.25f);
        lpcbtrie = new LPCBTrie_internal(*lpctrie, MAX_BUCKET_SIZE);
        return;
    }

    void insert(const KeyType& key, const ValueType& value)
    {
        lpcbtrie->insert(key, value);        
        return;
    }
    ValueType* locate(const KeyType& key)
    {
        return lpcbtrie->locate(key);
    }
    void remove(const KeyType& key)
    {
        lpcbtrie->remove(key);
        return;
    }
    ~LPCBTrie()
    {
        delete lpctrie;
        delete lpcbtrie;
        return;
    }
};

#endif
