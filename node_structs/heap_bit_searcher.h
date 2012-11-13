#if !defined __OR_TREE_H

#define __OR_TREE_H

#include <count_alloc/count_alloc.h>

template <bool count_mem = false> class HeapBitSearcher
{
/*
    // This is an O(log w) (w is word length in bits)
    // algorithm to convert a breadth-first index to a van Emde Boas index.
    // This is for experimenting with cache oblivious layouts.
    unsigned int BFStoVEB(unsigned int idx, unsigned int depth, unsigned int height)
    {       
        if(height <= 15)
        {
            return idx;
        }
        unsigned int bottom_height = height >> 1;
        unsigned int top_height = height - bottom_height;
        unsigned int result;
        if(depth <= top_height)
        {
            result = BFStoVEB(idx, depth, top_height);
        }
        else
        {
            unsigned int top_size = 1 << top_height;
            unsigned int depth_in_subtree = depth - top_height;
            unsigned int subtree_number = (idx >> (depth_in_subtree - 1)) - top_size;
            unsigned int subtree_root = (top_size - 1) + subtree_number * ((1 << bottom_height) - 1);
            unsigned int idx_in_subtree = (1 << (depth_in_subtree - 1)) | (idx & ((1 << (depth_in_subtree - 1)) - 1));
            result = subtree_root + BFStoVEB(idx_in_subtree, depth_in_subtree, bottom_height);
        }
        return result;
    }
*/    
    inline unsigned int left_child(unsigned int x)  { return x << 1; }
    inline unsigned int right_child(unsigned int x) { return 1 + (x << 1); }
    inline unsigned int parent(unsigned int x)      { return x >> 1; }

    inline bool get_heap_bit(unsigned int idx)
    {
        // 1-based indexing
        // The heap has num_bits - 1 nodes
        // at indices 1, ..., num_bits - 1
        if(idx < num_bits)
        {
            return or_heap[idx];
        }
        return ptrs[idx - num_bits] != 0;
    }
    unsigned int num_bits;
    unsigned int heap_height;
    void** ptrs;
    bool* or_heap;
public:
    static const unsigned int NO_PRED = 0xFFFFFFFF;
    static const unsigned int NO_SUCC = 0x7FFFFFFF;

    unsigned int min_idx;
    unsigned int max_idx;
    unsigned int num_set_bits;
    HeapBitSearcher(void** ptrs, unsigned int radix) : num_bits(1 << radix), heap_height(radix + 1), ptrs(ptrs), min_idx(num_bits), max_idx(0), num_set_bits(0)
    {
        unsigned int heap_size = num_bits; // Not num_bits - 1, since we use 1-based indexing
        or_heap = new bool[heap_size];

        update_mem_counter<count_mem,bool>(MemCounter::NEW, or_heap, heap_size);
        memset(or_heap, 0, heap_size * sizeof(*or_heap));
        return;
    }
    inline void set_bit(unsigned int bit_idx)
    {
        unsigned int idx = parent(num_bits + bit_idx);
        while(idx)
        {            
            or_heap[idx] = 1;
            idx = parent(idx);
        }
        if(bit_idx < min_idx) 
        {
            min_idx = bit_idx;
        }
        if(bit_idx > max_idx)
        {
            max_idx = bit_idx;
        }
        return;
    }   
    inline void unset_bit(unsigned int bit_idx)
    {
        unsigned int idx = parent(num_bits + bit_idx);
        while(idx)
        {            
            or_heap[idx] = 0;
            idx = parent(idx);
        }
        if(bit_idx == min_idx)
        {
            min_idx = succ(min_idx);
        }
        if(bit_idx == max_idx)
        {
            max_idx = pred(max_idx);
        }
        return;
    }   
    unsigned int succ(unsigned int bit_idx)
    {
        unsigned int idx = num_bits + bit_idx;
        // First find the lowest ancestor
        // of idx with non-zero right child, 
        // whose left child is also an ancestor of idx.    
        unsigned int last = idx;
        idx = parent(idx);
        // While we didn't come up from the left, or our right child
        // is zero, loop:
        while(last != left_child(idx) || !get_heap_bit(right_child(idx)))
        {
            if(idx == 1)
            {
                // We're at the root, so there's
                // no successor.
                return NO_SUCC;
            }
            last = idx;
            idx = parent(idx);
        }    
        // At this point, we came up from the left and our
        // right child is non-zero.

        // Now we find the left-most non-zero leaf descending
        // from right child of the just located ancestor.
        while(1)
        {
            idx = right_child(idx);
            while(idx < num_bits && get_heap_bit(left_child(idx))) 
            {
                idx = left_child(idx);
            }
            if(idx >= num_bits) break;
        }
        return idx - num_bits;
    }
    unsigned int pred(unsigned int bit_idx)
    {
        unsigned int idx = num_bits + bit_idx;
        // First find the lowest ancestor
        // of idx with non-zero left child, 
        // whose right child is also an ancestor of idx.    
        unsigned int last = idx;
        idx = parent(idx);
        // While we didn't come up from the right, or our left child
        // is zero, loop:
        while(last != right_child(idx) || !get_heap_bit(left_child(idx)))
        {
            //if(idx == 1)
            if(idx <= 1)
            {
                // We're at the root, so there's
                // no predecessor.
                return NO_PRED;
            }
            last = idx;
            idx = parent(idx);
        }    
        // At this point, we came up from the right and our
        // left child is non-zero.

        // Now we find the right-most non-zero leaf descending
        // from left child of the just located ancestor.
        while(1)
        {
            idx = left_child(idx);
            while(idx < num_bits && get_heap_bit(right_child(idx))) 
            {
                idx = right_child(idx);
            }
            if(idx >= num_bits) break;
        }
        return idx - num_bits;
    }
    void rebuild()
    {
        unsigned int length = num_bits >> 1;
        unsigned int skip = num_bits;

        // Peel off first iteration
       

        num_set_bits = 0;
        for(unsigned int i = 0; i < length; i++)
        {
            unsigned char bit1 = (ptrs[i << 1] != 0);
            unsigned char bit2 = (ptrs[(i << 1) + 1] != 0);
            or_heap[skip - length + i] = bit1 | bit2;
            num_set_bits += bit1 + bit2;
        }         
        skip -= length;
        length >>= 1;
        while(length)
        {
            for(unsigned int i = 0; i < length; i++)
            {
                or_heap[skip - length + i] = or_heap[skip + (i << 1)] | 
                                             or_heap[skip + (i << 1) + 1];
            }            
            skip -= length;
            length >>= 1;
        }
        if(ptrs[0]) min_idx = 0;
        else min_idx = succ(0);

        if(ptrs[num_bits - 1]) max_idx = num_bits - 1;
        else max_idx = pred(num_bits - 1);
        return;
    }
    inline bool is_empty() { return max_idx < min_idx; }
    bool has_pred(unsigned int idx) { return idx > min_idx; }
    bool has_succ(unsigned int idx) { return idx < max_idx; }
    unsigned int get_min_idx() { return min_idx; }
    unsigned int get_max_idx() { return max_idx; }
    unsigned int get_num_set_bits() { return num_set_bits; }


    ~HeapBitSearcher()
    {
        update_mem_counter<count_mem,bool>(MemCounter::DELETE, or_heap);
        delete[] or_heap;
        return;
    }
};

#endif
