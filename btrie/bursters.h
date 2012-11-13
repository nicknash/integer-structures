#if !defined __BURSTERS_H

#define __BURSTERS_H

#include <key_utils/key_utils.h>
#include <bucket_structs/bucket_structs.h>
#include <count_alloc/count_alloc.h>

template <class KeyType, class ValueType, class LevelPathCompTrie, class Bucket, bool count_mem = false> class LevelPathCompTrieBurst
{
    typedef typename LevelPathCompTrie::INode INode;
    typedef typename LevelPathCompTrie::Leaf Leaf;
    typedef typename LevelPathCompTrie::ChildIdx ChildIdx;

    typedef KeyTypeInfo<KeyType> KeyInfo;
    typedef typename KeyInfo::BitIdx BitIdx;
    
    ValueType value;
    int min_children_bits;
    Bucket*& first_bucket;
public:
    LevelPathCompTrieBurst(const ValueType& value, int min_children_bits, Bucket*& first_bucket) : value(value), min_children_bits(min_children_bits), first_bucket(first_bucket) {}       
    inline void operator()(INode* parent, const KeyType& key, BitIdx shift)
    {
        ChildIdx leaf_idx = KeyInfo::extract_bits(key, shift, parent->num_children_bits);
        Leaf* leaf = parent->leaves[leaf_idx];
        Bucket* b = leaf->value;
        
        if(b->insert(key, value) == BucketData::INSERT_FILLED)
        {
            // Here we are bursting a bucket.
            // Before inserting the key we have
            //
            // PPPPPPPPPP
            //     |
            //     | S
            //     |
            //[LLL -> BBB] 
            //
            // Here P is the parent node, L is the leaf with
            // (implicit) path compression string S, which maps
            // a key to a bucket, all of whose keys have S as
            // a prefix.
            //
            // After bursting, we have the structure:
            //
            // PPPPPPPP
            //    |
            //    | S'
            //    |
            // SPLITTER
            // | | | |
            // L L L Lk
            //
            // Where L are the new leaves resulting from bursting the
            // bucket. Lk is the bucket containing the new key.
            //
            // S' is the longest common prefix of S and the new key.
            //
            // N.B. S is the longest common prefix of all the keys
            // in the bucket, excluding their common prefix before 
            // node P.
            INode* splitter = new INode(min_children_bits);

            update_mem_counter<count_mem,INode>(MemCounter::NEW, splitter);
            // Make the splitter the child of the parent, 
            // and mark the splitter as an internal node.

            // No need to update the in-node data
            // structure when adding the splitter, since
            // a leaf was already at this index.
            //parent->add_inode(splitter, leaf_idx);
            parent->inodes[leaf_idx] = splitter;
            parent->is_internal[leaf_idx] = true;

            // Compute the length of the longest common prefix
            // 
            BitIdx lcp_len = 0;
            BitIdx s = shift - min_children_bits;//parent->num_children_bits;
            while(s >= 0)
            {
                ChildIdx bits = KeyInfo::extract_bits(key, s, min_children_bits);
                if(!b->all_bits_match(bits, s, min_children_bits)) break;
                lcp_len += min_children_bits;
                s -= min_children_bits;
            }
//            splitter->skipped_bits = KeyInfo::extract_bits(key, shift - parent->num_children_bits - lcp_len, lcp_len);
            splitter->skipped_bits = KeyInfo::extract_bits(key, shift - lcp_len, lcp_len);
            splitter->num_skipped = lcp_len;
            
            if(!lcp_len)
            {
                parent->num_empty_internal++;
            }
            //
            // We now create all the new buckets
            //
            Bucket* z = b->burst_into(splitter, leaf, shift - min_children_bits - lcp_len, min_children_bits);

            if(b == first_bucket)
            {
                first_bucket = z;
            }

            splitter->node_struct->rebuild();
            update_mem_counter<count_mem,Bucket>(MemCounter::DELETE, b);
            update_mem_counter<count_mem,Leaf>(MemCounter::DELETE, leaf);
            delete b;
            delete leaf;
        }
        return;
    }    
    inline void connect(INode* parent, INode* node, ChildIdx idx, BitIdx shift)
    {
        Leaf* l = node->leaves[idx];
        Bucket* b = l->value;
        
        Bucket* z = b->burst_into(parent, l, shift, parent->num_children_bits);
        
        if(b == first_bucket)
        {
            first_bucket = z;
        }

        update_mem_counter<count_mem,Bucket>(MemCounter::DELETE, b);
        update_mem_counter<count_mem,Leaf>(MemCounter::DELETE, l);
        delete b;
        delete l;
        return;
    }
};

#endif
