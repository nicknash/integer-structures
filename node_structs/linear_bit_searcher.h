#if !defined __LINEAR_BIT_SEARCHER_H

#define __LINEAR_BIT_SEARCHER_H

#include <cstring>

template <bool count_mem = false> class LinearBitSearcher
{
    int min_idx;
    int max_idx;
    int size;
    int num_set_bits;
    void** bits;
public:
    static const int NOT_FOUND = -1;
    LinearBitSearcher(void** ptrs, int size_bits) : bits(ptrs), num_set_bits(0)
    {
        size = 1 << size_bits;
//        bits = new bool[size];
//        memset(bits, 0, size * sizeof(*bits));
        min_idx = size - 1;
        max_idx = 0;
        return;
    }
    inline void set_bit(int idx)
    {
 //       bits[idx] = true;
        num_set_bits++;
        if(idx < min_idx) 
        {
            min_idx = idx;
        }
        if(idx > max_idx)
        {
            max_idx = idx;
        }
        return;
    }
    inline void unset_bit(int idx)
    {
        // This has a bug.
   //     bits[idx] = false;
        num_set_bits--;
        if(idx == min_idx)
        {
            int i = idx + 1;
            if(i < max_idx)
            {
                while(!bits[i])
                {
                    i++;
                }
            }
            min_idx = i;
        }
        if(idx == max_idx)
        {
            int i = idx - 1;
            if(i > min_idx)
            {
                while(!bits[i])
                {
                    i--;
                }
                max_idx = i;
            }
        }
        return;
    }
    int pred(int idx)
    {
        if(idx <= min_idx)
        {
            return NOT_FOUND;
        }
        int i = idx - 1;
        while(!bits[i])
        {
            i--;
        }
        return i;
    }
    int succ(int idx)
    {
        if(idx >= max_idx)
        {
            return NOT_FOUND;
        }
        int i = idx + 1;
        while(!bits[i])
        {
            i++;
        }
        return i;
    }
/*    void build_from_other(LinearBitSearcher* other, int offset)
    {
        bits = other->bits + offset;
        max_idx = size - 1;
        if(bits[0]) min_idx = 0;
        else min_idx = succ(0);

        if(bits[size - 1]) max_idx = size - 1;
        else max_idx = pred(size - 1);
        
        //int start = std::max(offset, other->min_idx);
        //int j = start - offset;
        //for(int i = start; i < length; i++)
        //{
        //    bits[j] = other->bits[i + offset];
        //   j++;
        //}
        return;
    }*/
/*
    template <class INode> void build_from_node(INode* node)
    {
        int i = 0;
        while(i < size && !bits[i])
        {
            i++;
        }
        min_idx = i;
        i = size - 1;
        while(i >= 0 && !bits[i])
        {
            i--;
        }
        max_idx = i;
        num_set_bits = 2;
        for(i = min_idx + 1; i < max_idx; i++)
        {
            if(bits[i]) 
            {
                num_set_bits++;
            }
        }
        return;
    }*/
    void rebuild()
    {
        return;
    }
    
    inline bool is_empty() { return max_idx < min_idx; }
    bool has_pred(int idx) { return idx > min_idx; }
    bool has_succ(int idx) { return idx < max_idx; }
    int get_min_idx() { return min_idx; }
    int get_max_idx() { return max_idx; }
    int get_num_set_bits() { return num_set_bits; } 
    ~LinearBitSearcher()
    {
     //   delete [] bits;
        return;
    }
};


#endif
