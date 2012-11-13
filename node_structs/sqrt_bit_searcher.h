#if !defined __SQRT_BIT_SEARCHER_H

#define __SQRT_BIT_SEARCHER_H

#include <cstring>
#include <count_alloc/count_alloc.h>

template <bool count_mem = false> class SqrtBitSearcher
{
public:   
    
    // The following two values are chosen because I never
    // expect to have a SqrtBitArray over enough bits
    // that they could be valid.
    static const unsigned int NO_PRED = 0xFFFFFFFF;
    static const unsigned int NO_SUCC = 0x7FFFFFFF;
    
    void** bits;
    unsigned int min_idx;
    unsigned int max_idx;
    unsigned int shift;
    unsigned int num_set_bits;
    unsigned int num_counters, size;
    unsigned short* counters;
//public:
    SqrtBitSearcher(void** ptrs, int size_bits) : bits(ptrs), num_set_bits(0)
    {        
        size = 1 << size_bits;

        shift = size_bits >> 1;
        num_counters = 1 << shift;
        
        counters = new unsigned short[num_counters];
        update_mem_counter<count_mem,unsigned short>(MemCounter::NEW, counters, num_counters); 

        memset(counters, 0, num_counters * sizeof(*counters));
        min_idx = size - 1;
        max_idx = 0;
        return;
    }
    inline void set_bit(unsigned int idx)
    {
        //if(!bits[idx])
        {
            num_set_bits++;
            counters[idx >> shift]++;
            if(idx < min_idx) 
            {
                min_idx = idx;
            }
            if(idx > max_idx)
            {
                max_idx = idx;
            }

        }
        return;
    }
    inline void unset_bit(unsigned int idx)
    {
       // if(bits[idx])
        {            
            num_set_bits--;
            counters[idx >> shift]--;
            if(idx == min_idx)
            {
                min_idx = succ(min_idx);
            }
            if(idx == max_idx)
            {
                max_idx = pred(max_idx);
            }
        }
        return;
    }
    unsigned int pred(unsigned int idx)
    {
        int i = idx >> shift;

        if(counters[i] >= 1)
        {
            int j = idx - 1;
            int end;
            if(!i)
            {
                end = 0;
            }
            else
            {
                end = (i - 1) << shift;
            }        
            while(j >= end) 
            {
                if(bits[j])
                {
                    return j;
                }
                j--;
            }
        }    
        while(--i >= 0 && !counters[i]) ;    
        if(i < 0) return NO_PRED; // Not found
        i++;
        i <<= shift;
        while(!bits[--i]) ;    
        return i;

    }
    unsigned int succ(unsigned int idx)
    {
        unsigned int i = idx >> shift;

        if(counters[i] >= 1)
        {
            int j = idx + 1;
            int end = (i + 1) << shift;
            while(j < end) 
            {
                if(bits[j])
                {
                    return j;
                }
                j++;
            }
        }    
        while(++i < num_counters && !counters[i]) ;    
        if(i == num_counters) return NO_SUCC; // Not found
        i <<= shift;
        while(!bits[i]) i++;    
        return i;

    }
    void rebuild()
    {
        unsigned int i = 0;
        while(i < size && !bits[i]) i++;
        min_idx = i;
        if(min_idx == size)
        {
            num_set_bits = 0;
        }
        while(i < size)
        {
            if(bits[i])
            {
                counters[i >> shift]++;
                num_set_bits++;
                max_idx = i;
            }
            i++;
        }        
        return;
    }
    inline bool is_empty() { return max_idx < min_idx; }
    bool has_pred(unsigned int idx) { return idx > min_idx; }
    bool has_succ(unsigned int idx) { return idx < max_idx; }
    unsigned int get_min_idx() { return min_idx; }
    unsigned int get_max_idx() { return max_idx; }
    unsigned int get_num_set_bits() { return num_set_bits; }
    ~SqrtBitSearcher()
    {
        delete [] counters;
        //used_memory -= size;
        update_mem_counter<count_mem,unsigned short>(MemCounter::DELETE, counters);
        return;
    }
};


#endif
