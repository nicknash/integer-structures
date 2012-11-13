#if !defined __QTRIE_H

#define __QTRIE_H

#include <bucket_structs/bucket_structs.h>
#include <count_alloc/count_alloc.h>

template <class KeyType, class ValueType, class TopStruct, class Bucket, bool mem_count = false> class QTrie
{
    static const int INITIAL_BUCKET_SIZE = 2;
    TopStruct& top_struct;
    Bucket* min_bucket;
public:
    QTrie(TopStruct& top_struct, int max_bucket_size) : top_struct(top_struct)
    {
        min_bucket = new Bucket(INITIAL_BUCKET_SIZE, max_bucket_size);
        update_mem_counter<mem_count,Bucket>(MemCounter::NEW, min_bucket);
        return;
    }
    bool insert(const KeyType& key, const ValueType& value)
    {
        using namespace BucketData;
        Bucket* pred_bucket = 0;
        KeyType pred_key;
        if(!top_struct.find_predecessor(key, pred_key, pred_bucket))
        {
            pred_bucket = min_bucket;
        }
        if(pred_bucket->insert(key, value) == INSERT_FILLED)
        {
            // Add a new representative key and split the pred_bucket
            pred_bucket->sort();
            Bucket* b = pred_bucket->split();
            const KeyType& k = b->get_min_key();
            top_struct.insert(k, b);

            // Insert b into the linked list of buckets:
            //
            b->prev = pred_bucket;
            if(pred_bucket->next)
            {
                pred_bucket->next->prev = b;
                b->next = pred_bucket->next;
            }
            pred_bucket->next = b;            
        } 
        return false;
    }  
    void remove(const KeyType& key)
    {
        Bucket* pred_bucket = 0;
        KeyType pred_key;
        if(!top_struct.find_predecessor(key, pred_key, pred_bucket))
        {
            if(min_bucket->remove(key) && !min_bucket->num_elems && min_bucket->next)
            {            
                Bucket* next = min_bucket->next;
                update_mem_counter<mem_count,Bucket>(MemCounter::DELETE, min_bucket);
                delete min_bucket;
                top_struct.remove(next->get_min_key());
                next->prev = 0;
                min_bucket = next;
            }
        }
        else if(pred_bucket->remove(key) && !pred_bucket->num_elems)
        {
            top_struct.remove(pred_key);
            pred_bucket->prev->next = pred_bucket->next;
            if(pred_bucket->next)
            {
                pred_bucket->next->prev = pred_bucket->prev;
            }
            delete pred_bucket;
        }
        return;
    }
    ValueType* search(const KeyType& key)
    {
        Bucket* b;
        KeyType k;
        if(top_struct.find_predecessor(key, k, b))
        {
            return b->search(key);
        }
        return min_bucket->search(key);
    }

    ValueType* locate(const KeyType& key)
    {
        Bucket* b;
        KeyType k;
        if(top_struct.find_predecessor(key, k, b))
        {
            ValueType* v = b->search(key);
            if(!v)
            {
                return b->prev->values + b->num_elems - 1;
            }
            return v;
        }        
       return min_bucket->search(key);
    }
    ~QTrie()
    {
        Bucket* b = min_bucket;
        while(b)
        {
            Bucket* n = b->next;
            update_mem_counter<mem_count,Bucket>(MemCounter::DELETE, b);
            delete b;
            b = n;
        }
        return;
    }
};

#endif
