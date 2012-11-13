#if !defined __KEY_UTILS_H

#define __KEY_UTILS_H

#include <limits>

template <class KeyType> class KeyTypeInfo 
{
public:
    static const int NUM_BITS = std::numeric_limits<KeyType>::digits;
    typedef char BitIdx; // It's handy to have this as a signed type for looping conditions (i.e. >= 0 etc)

    static inline KeyType extract_bits(const KeyType& key, BitIdx shift, BitIdx num_bits)
    {
        return (key >> shift) & ((1 << num_bits) - 1);
    }    
    static inline BitIdx get_match_len(BitIdx skip, BitIdx chunk_size, const KeyType& k1, const KeyType& k2)
    {
        // Ignoring the first skip most significant bits of k1 and k2, 
        // how long a common prefix do k1 and k2 have, in chunk_size chunks?
        // The answer is computed by this function and stored in len.
        // 
        // E.g. skip = 16, k1 = 0xFFAABBCC
        //                 k2 = 0xFFAABBDD
        //
        // len = 8 in this case, since the "BB"s match, the "FFAA" is ignored due to skip
        // (Assuming chunk_size = 8).
        //
        BitIdx shift = NUM_BITS - chunk_size - skip; 
        BitIdx len = 0;
        while(shift >= 0 && extract_bits(k1, shift, chunk_size) == extract_bits(k2, shift, chunk_size))
        {
            len += chunk_size;
            shift -= chunk_size;
        }
        return len;
    }
};

#endif
