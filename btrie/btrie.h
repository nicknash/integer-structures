#if !defined __BTRIE_H

#define __BTRIE_H

#include <key_utils/key_utils.h>
#include <btrie/bursters.h>
#include <count_alloc/count_alloc.h>


template <class KeyType, class ValueType, class TopStruct, class UpdateLeafBucket, class Bucket, bool count_mem = false> class BTrie
{
    typedef KeyTypeInfo<KeyType> KeyInfo;
    typedef typename KeyInfo::BitIdx BitIdx;
    typedef typename TopStruct::Leaf Leaf;
    typedef typename TopStruct::INode INode;
    typedef /*unsigned short*/unsigned int ChildIdx;

    TopStruct& top_struct;
    int bucket_size;
    Bucket* first_bucket;

    static const int INITIAL_BUCKET_SIZE = 2;

    class CreateLeafBucket
    {
        ValueType value;
        int max_bucket_size;
        /*const*/ TopStruct& ts;
        Bucket*& fb;
    public:
        CreateLeafBucket(Bucket*& fb, /*const*/ TopStruct& ts, const ValueType& value, int max_bucket_size) : value(value), max_bucket_size(max_bucket_size), ts(ts), fb(fb) {}
        inline void operator()(INode* parent, const KeyType& leaf_idx, const KeyType& key)
        {
            Bucket* b = new Bucket(key, value, INITIAL_BUCKET_SIZE, max_bucket_size);
            
            KeyType pred_key;
            Bucket* pred_bucket;

            // Link in b to the linked list of buckets
            if(ts.find_predecessor(key, pred_key, pred_bucket))
            {
                b->prev = pred_bucket;
                if(pred_bucket->next)
                {
                    b->next = pred_bucket->next;
                    pred_bucket->next->prev = b;
                }
                pred_bucket->next = b;
            }
            else
            {
                if(fb)
                {
                    b->next = fb;
                    fb->prev = b;
                }
                fb = b;
            }
            Leaf* l = new Leaf(key, b);
            parent->add_leaf(l, (ChildIdx)leaf_idx);

            update_mem_counter<count_mem,Bucket>(MemCounter::NEW, b);
            update_mem_counter<count_mem,Leaf>(MemCounter::NEW, l);
            return;
        }
    };
    class MatchTester
    {
    public:
        inline bool operator()(const KeyType&, const KeyType&)
        {
            return true;
        } 
    };
    class RemovePred
    {
        const KeyType& key;
        Bucket*& fb;
    public:
        RemovePred(const KeyType& key, Bucket*& fb) : key(key), fb(fb) { }
        inline bool operator()(Leaf* l)
        {
            Bucket* b = l->value;
            if(b->remove(key) && !b->num_elems)
            {
                if(b->prev) b->prev->next = b->next;
                if(b->next) b->next->prev = b->prev;
                if(b == fb) fb = b->next;
                update_mem_counter<count_mem,Bucket>(MemCounter::DELETE, b);
                delete b;
                return true;
            }
            return false;
        }
    };
public:
    BTrie(TopStruct& top_struct, int bucket_size) : top_struct(top_struct), bucket_size(bucket_size), first_bucket(0)
    {
        return;
    }
    bool insert(const KeyType& key, const ValueType& value)
    {
        top_struct.insert(key, MatchTester(), CreateLeafBucket(first_bucket, top_struct, value, bucket_size), UpdateLeafBucket(value, top_struct.get_min_children_bits(), first_bucket));
        return false;
    }
    void remove(const KeyType& key)
    {
        top_struct.remove_if(key, MatchTester(), RemovePred(key, first_bucket));
        return;
    }
    ValueType* search(const KeyType& key)
    {
        using namespace std;
        Bucket** b;
        if(b = top_struct.search(key, MatchTester()))
        {
            return (*b)->search(key);
        }
        return 0;
    }
    ValueType* locate(const KeyType& key)
    {
        using namespace std;
        Bucket** b;

        typename TopStruct::SearchStatus status;
        b = top_struct.general_search(key, status);

        if(!b)
        {
            return 0;
        }
        if(status == TopStruct::FOUND_KEY) // Found a bucket
        {
            return (*b)->locate_with_list(key);
        }
        else if(status == TopStruct::FOUND_PRED) // Found predecessor
        {
            return (*b)->get_max_value_ptr();
        }
        // Found successor
        Bucket* p = (*b)->prev;
        if(!p)
        {
            return 0;
        }
        return p->get_max_value_ptr();
    }
    void print(std::ostream& out)
    {
        top_struct.print(out);
        return;
    }
    ~BTrie()
    {
        Bucket* b = first_bucket;
        while(b)
        {
            Bucket* n = b->next;            
            update_mem_counter<count_mem,Bucket>(MemCounter::DELETE, b);
            delete b;
            b = n;
        }
        return;
    }
};

#endif
