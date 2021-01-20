#if !defined __SORTED_BUCKET_H

#define __SORTED_BUCKET_H

#include <bucket_structs/common.h>
#include <key_utils/key_utils.h>
#include <count_alloc/count_alloc.h>
#include <algorithm>

template <class KeyType, class ValueType, bool count_mem = false> class SortedBucket
{
public:
    typedef /*unsigned short*/unsigned int ChildIdx;

    int num_elems, capacity, max_capacity;
    SortedBucket* prev;
    SortedBucket* next;
    KeyType* keys;
    ValueType* values;

    static const int GROWTH_FACTOR        = 2;
    static const int INITIAL_CAPACITY     = 2;

    SortedBucket(int capacity, int max_capacity) : num_elems(0), capacity(capacity), max_capacity(max_capacity), prev(0), next(0)
    {         
        keys = new KeyType[capacity];
        values = new ValueType[capacity];

        update_mem_counter<count_mem,KeyType>(MemCounter::NEW, keys, capacity);
        update_mem_counter<count_mem,ValueType>(MemCounter::NEW, values, capacity);
        return;
    }
    SortedBucket(const KeyType& key, const ValueType& value, int capacity, int max_capacity) : num_elems(1), capacity(capacity), max_capacity(max_capacity), prev(0), next(0)
    {
        keys = new KeyType[capacity];
        values = new ValueType[capacity];

        update_mem_counter<count_mem,KeyType>(MemCounter::NEW, keys, capacity);
        update_mem_counter<count_mem,ValueType>(MemCounter::NEW, values, capacity);
        
        keys[0] = key;
        values[0] = value;
        return;
    }
    void check_grow()
    {
        if(num_elems == capacity)
        {
            capacity *= GROWTH_FACTOR;
//            capacity += (capacity >> 1);
            KeyType* new_keys = new KeyType[capacity];
            ValueType* new_values = new ValueType[capacity];

            update_mem_counter<count_mem,KeyType>(MemCounter::NEW, new_keys, capacity);
            update_mem_counter<count_mem,ValueType>(MemCounter::NEW, new_values, capacity);
    
            memcpy(new_keys, keys, num_elems * sizeof(KeyType));
            memcpy(new_values, values, num_elems * sizeof(ValueType));

            update_mem_counter<count_mem,KeyType>(MemCounter::DELETE, keys);
            update_mem_counter<count_mem,ValueType>(MemCounter::DELETE, values);
            
            delete[] keys;
            delete[] values;
            keys = new_keys;
            values = new_values;
        }
        return;
    }
    void check_shrink()
    {
        if(num_elems <= (capacity / GROWTH_FACTOR) && capacity > INITIAL_CAPACITY)
        {
            capacity /= GROWTH_FACTOR;
//            capacity += (capacity >> 1);
            KeyType* new_keys = new KeyType[capacity];
            ValueType* new_values = new ValueType[capacity];

            update_mem_counter<count_mem,KeyType>(MemCounter::NEW, new_keys, capacity);
            update_mem_counter<count_mem,ValueType>(MemCounter::NEW, new_values, capacity);
    
            memcpy(new_keys, keys, num_elems * sizeof(KeyType));
            memcpy(new_values, values, num_elems * sizeof(ValueType));

            update_mem_counter<count_mem,KeyType>(MemCounter::DELETE, keys);
            update_mem_counter<count_mem,ValueType>(MemCounter::DELETE, values);
            
            delete[] keys;
            delete[] values;
            keys = new_keys;
            values = new_values;
        }
        return;
    }

    
    void unchecked_insert(const KeyType& key, const ValueType& value)
    {
        check_grow();
        keys[num_elems] = key;
        values[num_elems] = value;
        num_elems++;
        return;
    }
    BucketData::INSERT_RESULT insert(const KeyType& key, const ValueType& value)
    {
        using namespace BucketData;
        INSERT_RESULT result;

        KeyType* p = keys;
        KeyType* q = keys + num_elems;

        while(p < q && *p < key)
        { 
            p++;
        }
        size_t diff = p - keys;
        if(p < q && *p == key)
        {            
            *(values + diff) = value;
            result = INSERT_UPDATED;
        }
        else if(num_elems == max_capacity)
        {
            result = INSERT_FAILED;
        }
        else 
        {
            check_grow();
            if(num_elems == max_capacity - 1)
            {
                result = INSERT_FILLED;
            }
            else 
            {
                result = INSERT_CREATED;
            }
            // If here we know keys[i - 1] < key < keys[i]
            // So we need to shift keys[i], .., keys[s-1] into keys[i + 1], .., keys[s]
            // and similarly with the values
            ValueType* r = values + num_elems;
            q = keys + num_elems;
            p = keys + diff;
            while(q > p)
            {
                *r = *(r - 1);
                *q = *(q - 1);
                r--;
                q--;
            }
            *p = key;
            *(values + (p - keys)) = value;
            num_elems++;
        }
        return result;
    }
    ValueType* remove(const KeyType& key)
    {
        KeyType* p = keys;
        KeyType* q = keys + num_elems;

        while(p < q && *p < key)
        { 
            p++;
        }
        size_t diff = p - keys;
        ValueType* result = 0;
        if(p < q && *p == key)
        {            
            result = values + diff;
            ValueType* r = values + diff;
            q--;
            while(p < q)
            {
                *r = *(r + 1);
                *p = *(p + 1);
                r++;
                p++;
            }
            num_elems--;
            check_shrink();
        }
        return result;
    }
    void sort()
    {
        return; // Do nothing.
    }
    SortedBucket* split()
    {        
        if(num_elems != max_capacity)
        {
            // Can only split full buckets.
            return 0;
        }
        num_elems /= 2;
        SortedBucket* b = new SortedBucket<KeyType,ValueType,count_mem>(num_elems, max_capacity);
        update_mem_counter<count_mem,SortedBucket>(MemCounter::NEW, b);
        for(int i = 0; i < num_elems; i++)
        {
            b->keys[i] = keys[i + num_elems];
        }
        for(int i = 0; i < num_elems; i++)
        {
            b->values[i] = values[i + num_elems];
        }
        b->num_elems = num_elems;
        return b;
    }
    template <class INode, class Leaf> SortedBucket* burst_into(INode* node, Leaf*, int shift, int length)
    {
        const KeyType& k = keys[0];
        const KeyType& v = values[0];
        int idx = (ChildIdx)KeyTypeInfo<KeyType>::extract_bits(k, shift, length);
        SortedBucket* first_new = new SortedBucket<KeyType,ValueType,count_mem>(k, v, INITIAL_CAPACITY, max_capacity);
        update_mem_counter<count_mem,SortedBucket>(MemCounter::NEW, first_new);
        if(prev)
        {
            first_new->prev = prev;
            prev->next = first_new;
        }

        node->leaves[idx] = new Leaf(k, first_new);
        update_mem_counter<count_mem,Leaf>(MemCounter::NEW, node->leaves[idx]);

        SortedBucket* b = first_new;
        for(int i = 1; i < num_elems; i++)
        {
            const KeyType& k = keys[i];
            const KeyType& v = values[i];
            int idx = (ChildIdx)KeyTypeInfo<KeyType>::extract_bits(k, shift, length);
            if(!node->leaves[idx])
            {
                SortedBucket* b_new = new SortedBucket<KeyType,ValueType,count_mem>(k, v, INITIAL_CAPACITY, max_capacity);
                update_mem_counter<count_mem,SortedBucket>(MemCounter::NEW, b_new);

                b_new->prev = b;
                b->next = b_new;
                
                node->leaves[idx] = new Leaf(k, b_new);
                update_mem_counter<count_mem,Leaf>(MemCounter::NEW, node->leaves[idx]);
                
                b = b_new;
            }
            else
            {
                node->leaves[idx]->value->unchecked_insert(k, v);
            }
        }
        if(next)
        {
            b->next = next;
            next->prev = b;
        }
        return first_new;
    }
    bool all_bits_match(const KeyType& bits, int shift, int length)
    {
        for(int i = 0; i < num_elems; i++)
        {
            if(bits != KeyTypeInfo<KeyType>::extract_bits(keys[i], shift, length))
            {
                return false;
            }
        }
        return true;
    }
    ValueType* search(const KeyType& key)
    {
        using namespace std;
        /*KeyType* it = lower_bound(keys, keys + num_elems, key);
          if(it != keys + num_elems)
          {
          value = *(values + (it - keys));
          return true;
          }*/
        static const int BSEARCH_THRESHOLD = 10;
        int first = 0, last = num_elems - 1;
        KeyType *p = keys;
        while(first - last >= BSEARCH_THRESHOLD) 
        {
            int mid = ((unsigned int) (first + last)) >> 1;
            KeyType *q = p + mid;
            if(key > *q)
            {
                first = mid + 1;
            }
            else if(key < *q)
            {
                last = mid - 1;
            }
            else
            {
                return values + mid;        
            }
        }
        KeyType *q = p + last;
        p += first;
        while(*p < key)
        {
            p++;
            if(p > q) 
            {
                return 0;
            }
        }
        if(*p == key)
        {
            return values + (p - keys);
        }
        return 0;
    }
    ValueType* locate_with_list(const KeyType& key)
    {
        if(key < keys[0]) {
            return prev->locate_with_list(key);
        } else { 
            auto it = std::upper_bound(keys, keys + num_elems, key);
            return (--it);
        }
    }
    inline ValueType* get_max_value_ptr()
    {
        return values + num_elems - 1;
    }
    inline const KeyType& get_min_key()
    {
        return keys[0];
    }
    inline const KeyType& get_key(int idx)
    {
        return keys[idx];
    }
    inline const ValueType& get_value(int idx)
    {
        return values[idx];
    }
    inline void set_key(const KeyType& key, int idx)
    {
        keys[idx] = key;
        return;
    }
    inline void set_value(const ValueType& value, int idx)
    {
        values[idx] = value;
        return;
    }
    ~SortedBucket()
    {
        update_mem_counter<count_mem,KeyType>(MemCounter::DELETE, keys);
        update_mem_counter<count_mem,ValueType>(MemCounter::DELETE, values);
        delete[] keys;
        delete[] values;
        return;
    }
};

#endif
