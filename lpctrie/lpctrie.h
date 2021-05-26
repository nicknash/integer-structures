#if !defined __LPCTRIE_H

#define __LPCTRIE_H

#include <iostream>
#include <cstring>
#include <list>

#include <key_utils/key_utils.h>
#include <node_structs/node_structs.h>
#include <count_alloc/count_alloc.h>

template <class KeyType, class ValueType, class NodeStruct = LinearBitSearcher<false>, bool count_mem = false> class LPCTrie
{    
    typedef KeyTypeInfo<KeyType> KeyInfo;
    typedef typename KeyInfo::BitIdx BitIdx;
public:  
    class INode;
    class Leaf;

    static const BitIdx NUM_KEY_BITS = KeyInfo::NUM_BITS;
    typedef /*unsigned short*/unsigned int ChildIdx;
private:
    INode* root;
    int min_children_bits, max_children_bits;
    float expand_threshold, contract_threshold;
public:
    class DefaultMatchTester
    {
    public:
        inline bool operator()(const KeyType& key1, const KeyType& key2)
        {
            return key1 == key2;
        }
    };
    class DefaultCreateLeaf
    {
        ValueType value;
    public:
        DefaultCreateLeaf(ValueType value) : value(value) {}
        inline void operator()(INode* parent, const KeyType& leaf_idx, const KeyType& key)
        {
            Leaf *l = new Leaf(key, value);
            parent->add_leaf(l, (ChildIdx)leaf_idx);
            update_mem_counter<count_mem,Leaf>(MemCounter::NEW, l);           
            return;
        }
    };
    class DefaultUpdateLeaf
    {
        ValueType value;
    public:
        DefaultUpdateLeaf(ValueType value) : value(value) {}
        inline void operator()(INode* n, const KeyType& key, BitIdx shift)
        {
            ChildIdx idx = (ChildIdx)KeyInfo::extract_bits(key, shift, n->num_children_bits);
            n->leaves[idx]->value = value;
            return;
        }
        inline void connect(INode* parent, INode* node, ChildIdx idx, BitIdx shift)
        {
            const KeyType& k = node->leaves[idx]->key;
            int pidx = (ChildIdx)KeyInfo::extract_bits(k, shift, parent->num_children_bits);
            parent->leaves[pidx] = node->leaves[idx];
            return;
        }
    };
    class DefaultRemovePred
    {
    public:
        inline bool operator()(Leaf* l)
        {
            return true;
        }
    };
    class Leaf
    {
    public:
        ValueType value;
        KeyType key;
        Leaf() {}
        Leaf(const KeyType& key, const ValueType& value) : value(value), key(key) {}
    };
    class INode // An Internal trie Node.
    {
    public:    
        BitIdx num_children_bits;
        NodeStruct* node_struct;        
        union
        {
            INode** inodes;
            Leaf** leaves;
        };
        bool* is_internal; // is_internal[i] == true only if i-th branch is to a non-null, non-leaf node.
        BitIdx num_skipped;
        KeyType skipped_bits;
        ChildIdx num_empty_internal;

        INode(int num_children_bits) : num_children_bits(num_children_bits),
                                       num_skipped(0), skipped_bits(0), num_empty_internal(0)
        {
            unsigned int num_children = 1 << num_children_bits;
            inodes = new INode*[num_children];
            update_mem_counter<count_mem,INode*>(MemCounter::NEW, inodes, num_children);

            node_struct = new NodeStruct((void**) inodes, num_children_bits);
            update_mem_counter<count_mem,NodeStruct>(MemCounter::NEW, node_struct);

            is_internal = new bool[num_children];
            update_mem_counter<count_mem,bool>(MemCounter::NEW, is_internal, num_children);
            
            memset(inodes, 0, num_children * sizeof(*inodes));
            memset(is_internal, 0, num_children * sizeof(*is_internal));
            return;
        }
        bool is_full_enough(float expand_threshold)
        {
            return num_empty_internal >= expand_threshold * (1 << num_children_bits);
        }
        bool is_empty_enough(float contract_threshold)
        {
            return node_struct->get_num_set_bits() < 0.5f + contract_threshold * (1 << num_children_bits);
        }
        void update_node_struct()
        {
            node_struct->rebuild();
            return;
        }
        void add_inode(INode* n, ChildIdx idx)
        {
            inodes[idx] = n;
            is_internal[idx] = true;
            node_struct->set_bit(idx);
            return;
        }
        void add_leaf(Leaf* l, ChildIdx idx)
        {
            leaves[idx] = l;
            node_struct->set_bit(idx);
            return;
        }
        void remove_leaf(ChildIdx idx)
        {
            update_mem_counter<count_mem,Leaf>(MemCounter::DELETE, leaves[idx]);
            
            node_struct->unset_bit(idx);
            delete leaves[idx];
            leaves[idx] = 0;
            return;
        }

        bool has_branch_before(ChildIdx idx) { return node_struct->has_pred(idx); }
        bool has_branch_after(ChildIdx idx)  { return node_struct->has_succ(idx); }
        ChildIdx closest_branch_before(ChildIdx idx) { return node_struct->pred(idx); }
        ChildIdx closest_branch_after(ChildIdx idx)  { return node_struct->succ(idx); }
        ChildIdx last_branch()  { return node_struct->get_max_idx(); }
        ChildIdx first_branch() { return node_struct->get_min_idx(); }
        
        void destroy()
        {
            update_mem_counter<count_mem,bool>(MemCounter::DELETE, is_internal);
            delete[] is_internal;

            update_mem_counter<count_mem,INode*>(MemCounter::DELETE, inodes);
            delete[] inodes;

            update_mem_counter<count_mem,NodeStruct>(MemCounter::DELETE, node_struct);
            delete node_struct;
            return;
        }
    };
    LPCTrie(int min_children_bits, int max_children_bits, float expand_threshold, float contract_threshold) : min_children_bits(min_children_bits), 
                                                                                                              max_children_bits(max_children_bits), 
                                                                                                              expand_threshold(expand_threshold), 
                                                                                                              contract_threshold(contract_threshold)
    {
        root = new INode(min_children_bits); 
        update_mem_counter<count_mem,INode>(MemCounter::NEW, root);
    }
    // Add the mapping key -> value to the trie.
    // Return true only if we update rather than create the mapping.
    // N.B. the true/false thing isn't working at the moment w.r.t the
    // burst trie
    bool insert(const KeyType& key, const ValueType& value)
    {
        return insert(key, DefaultMatchTester(), DefaultCreateLeaf(value), DefaultUpdateLeaf(value));
    }
    template <class MatchTester, class CreateLeaf, class UpdateLeaf>
    inline bool insert(const KeyType& key,            
            MatchTester match_tester, 
            CreateLeaf create_leaf, 
            UpdateLeaf update_leaf)
    {        
        BitIdx shift = NUM_KEY_BITS - root->num_children_bits;
        ChildIdx idx = (ChildIdx)KeyInfo::extract_bits(key, shift, root->num_children_bits);

        ChildIdx parent_idx = 0;
        INode* parent = 0;
        INode* node = root;
        INode* child = root->inodes[idx];
        
        // Loop until we are at the bottom of the trie 
        // (i.e. when shift == 0 or we're at a leaf) or
        // when the path compression bits don't match the key's bits.
        while(shift > 0 && node->is_internal[idx] && child->skipped_bits == KeyInfo::extract_bits(key, shift - child->num_skipped, child->num_skipped))
        {
            shift -= child->num_children_bits + child->num_skipped;
            parent_idx = idx; // node is found at parent_idx in parent
            idx = (ChildIdx)KeyInfo::extract_bits(key, shift, child->num_children_bits);
            parent = node;
            node = child;
            child = node->inodes[idx];
        }
        using namespace std;
        bool found = false;
        if(!child)
        {
            using namespace std;
           // This is the simplest case. 
            // There is no child coming from node, so just
            // create a leaf.
            create_leaf(node, idx, key);                   
            //
        }
        else if(!node->is_internal[idx])
        {
            // In this case, we are at a leaf, so we might
            // need to split its path compression string (which
            // is not actually stored, but we deduce it by the depth
            // we are at in the trie).
            Leaf* leaf = node->leaves[idx];
            if(match_tester(leaf->key, key))
            {
                // The leaf's key matches the key we are inserting
                // so just update the value.
                //
                update_leaf(node, key, shift);
                found = true;
                check_expand(parent, parent_idx, shift, node, update_leaf);
            }
            else
            {
                // If the leaf's key doesn't match the key to be inserted
                // we need to split the leaf's path compression string.
                
                // Before the insertion we have something like this:
                //
                // NNNNNN
                //   |
                //   | S
                //   |
                //  LLL
                //
                // N is the node with the leaf L as a child, having
                // path compression string S. We deduce S from the key
                // in the leaf, using the depth of the leaf. 
                //
                // To add the new key, we must change this structure to
                //
                // NNNNNNNNN
                //     | 
                //     | S'
                //     |
                //  SPLITTER
                //   |   |  
                //  LLL LLL
                //
                // Where S' is the longest prefix shared by the key in the
                // original leaf, and the new key.
                //
                INode* splitter = new INode(min_children_bits);
                update_mem_counter<count_mem,INode>(MemCounter::NEW, splitter);
                // Make the splitter a child of the node at idx, which
                // is where we found this leaf.
                
                node->is_internal[idx] = true;
                node->inodes[idx] = splitter;
                
                // Now we want to determine the longest prefix shared by
                // key and leaf->key, but we need to exclude all bits up
                // to those that lead us to the leaf. This prefix
                // is the path compression string for the splitter.
                //
                // len is the length of that prefix.
                BitIdx num_matched = NUM_KEY_BITS - shift;                       
                BitIdx len = KeyInfo::get_match_len(num_matched, min_children_bits, key, leaf->key);
                splitter->num_skipped = len;
                
                // Now we use tmp to work out how much to shift
                // the key to extract the prefix bits.
                BitIdx tmp = NUM_KEY_BITS - len - num_matched;
                splitter->skipped_bits = KeyInfo::extract_bits(key, tmp, len);

                // Finally, the next less significant splitter->num_children_bits chunk of bits after
                // the prefix bits are the ones we branch on, so add in the leaves to the 
                // splitter using them.
                tmp -= splitter->num_children_bits;

                create_leaf(splitter, KeyInfo::extract_bits(key, tmp, splitter->num_children_bits), key);
                splitter->add_leaf(leaf, (ChildIdx)KeyInfo::extract_bits(leaf->key, tmp, splitter->num_children_bits));

                if(!splitter->num_skipped)
                {
                    node->num_empty_internal++;           
                }
                check_expand(parent, parent_idx, shift, node, update_leaf);
            }
        }
        else
        {
            // If we're in here, some path compression string didn't match
            // the corresponding bits in the key to be inserted, so we need
            // a splitter node between two internal nodes.
            // 
            // That is, we begin with the structure
            //
            // NNNN1111
            //    |
            //    | S
            //    |
            // NNNN2222
            //    |
            //    .
            //    .
            //
            // Where S is the non-matching path compression string.
            // We modify the structure to
            //
            //  NNNNN11111
            //      |
            //      | S'
            //      |
            //   SPLITTER
            //   |      |
            //   | T    |
            //   |      |
            // NNN222  LLL
            //   |
            //   .
            //   .
            //
            // The inserted key resides in L, a new leaf.
            // S' is the longest prefix of the path compression string and the
            // corresponding key bits. T is the suffix of S, excluding the bits
            // that are now branched on in the splitter (i.e. S = S'T)
            //
            
            // Add the splitter as a child at idx of node, where the mismatch
            // occured.
            INode* splitter = new INode(min_children_bits);
            update_mem_counter<count_mem,INode>(MemCounter::NEW, splitter);
            // We don't call add_inode here because that would update
            // internal node data structures that don't require updating
            // in this case.
            node->inodes[idx] = splitter;
            
            // Now find the longest prefix of the key matching the path
            // compression string in len.
            BitIdx ns = child->num_skipped;
            BitIdx len = KeyInfo::get_match_len(NUM_KEY_BITS - ns, min_children_bits, KeyInfo::extract_bits(key, shift - ns, ns), child->skipped_bits);
                                     
            // Set up the path compression string of the splitter.
            splitter->num_skipped = len;
            splitter->skipped_bits = KeyInfo::extract_bits(child->skipped_bits, ns - len, len);
            
            // The new leaf contains the key to be inserted, so find the right chunk of bits to branch
            // on in the splitter and add the leaf there.
            create_leaf(splitter, KeyInfo::extract_bits(key, shift - len - splitter->num_children_bits, splitter->num_children_bits), key);

            // Now we add in the sub-trie that originally had the non-matching path
            // compression string. 
            splitter->add_inode(child, (ChildIdx)KeyInfo::extract_bits(child->skipped_bits, ns - len - splitter->num_children_bits, splitter->num_children_bits));

            // Now update the child with the suffix of its original path compression string.
            child->num_skipped = ns - len - splitter->num_children_bits;
            child->skipped_bits &= ((1 << child->num_skipped) - 1);
            if(!child->num_skipped)
            {
                splitter->num_empty_internal++;
            }
            if(!splitter->num_skipped)
            {
                node->num_empty_internal++;
            }
            check_expand(parent, parent_idx, shift, node, update_leaf);
        }
        return found;
    }
    void remove(const KeyType& key)
    {
        remove_if(key, DefaultMatchTester(), DefaultRemovePred());
        return;
    }
    
    template <class MatchTester, class RemovePred> void remove_if(const KeyType& key,
                                                            MatchTester match_tester,
                                                            RemovePred remove_pred)
    {
        BitIdx shift = NUM_KEY_BITS - root->num_children_bits;
        ChildIdx idx = (ChildIdx)KeyInfo::extract_bits(key, shift, root->num_children_bits);

        ChildIdx parent_idx = 0;
        ChildIdx parent_parent_idx = 0;
        INode* parent_parent = 0; // The parent of the parent of node
        INode* parent = 0;    // The parent of node
        INode* node = root;   // Well, this is just plain old node :)
        
        INode* child = root->inodes[idx];
        
        // Loop until we are at the bottom of the trie 
        // (i.e. when shift == 0 or we're at a leaf) or
        // when the path compression bits don't match the key's bits.
        
        using namespace std;
        while(shift > 0 && node->is_internal[idx])
        {
            shift -= child->num_children_bits + child->num_skipped;
            parent_parent_idx = parent_idx;
            parent_idx = idx; // node is found at parent_idx in parent
            idx = (ChildIdx)KeyInfo::extract_bits(key, shift, child->num_children_bits);
            parent_parent = parent;
            parent = node;
            node = child;
            child = node->inodes[idx];
        }
        if(!child)
        {
            return;
        }
        Leaf* leaf = node->leaves[idx];
        
        if(!match_tester(leaf->key, key) || !remove_pred(leaf))
        {
            return;
        }
        if(parent && node->node_struct->get_num_set_bits() == 2)
        {
            int other_idx;
            if(idx == node->first_branch())
            {
                other_idx = node->last_branch();
            }
            else
            {
                other_idx = node->first_branch();
            }
            // If in here we need to preserve path compression
            if(!node->num_skipped)
            {
                // We're going to remove an internal node (node) from parent.
                // If node had an empty path
                // compression string that will have been counted in parent, so
                // decrement num_empty_internal for the parent
                // (Since the replacement for node is either x, which
                // always has a non-empty path compression string or a leaf)
                parent->num_empty_internal--;
            }
            if(node->is_internal[other_idx])
            {
                INode* x = node->inodes[other_idx];
                parent->inodes[parent_idx] = x;                
                
                // Concatenate the path compression strings, and the node index.
                x->skipped_bits |= (node->skipped_bits << (x->num_skipped + node->num_children_bits)) | (idx << x->num_skipped);
                x->num_skipped += node->num_skipped + node->num_children_bits;
            }
            else
            {                
                // Don't need to update the node structure for parent,
                // since there was already a branch at parent_idx to node
                parent->is_internal[parent_idx] = false;
                parent->leaves[parent_idx] = node->leaves[other_idx];
            }
            node->destroy();
           
            update_mem_counter<count_mem,INode>(MemCounter::DELETE, node);            
            delete node;

            update_mem_counter<count_mem,Leaf>(MemCounter::DELETE, leaf);
            delete leaf;
        
            check_contract(parent_parent, parent_parent_idx, parent); 
        }
        else
        {
            node->remove_leaf(idx);
            check_contract(parent, parent_idx, node);
        }
        return; 
    }
    inline void divide_node(INode* node, INode* parent, ChildIdx parent_offset)
    {
        using namespace std;
        BitIdx num_bits = node->num_children_bits;

        // We Need 2^min_children_bits dividers
        // each of 2^(num_bits - min_children_bits) children
        BitIdx sbits = num_bits - min_children_bits;
        ChildIdx num_divider_children = 1 << sbits;
        ChildIdx end = 1 << num_bits;
        ChildIdx k = 0;
        while(k != NodeStruct::NO_SUCC && k < end)// k != -1
        {
            ChildIdx divider_start = k & (~(num_divider_children - 1)); 
            ChildIdx divider_end = divider_start + num_divider_children;
            ChildIdx first_branch = k;
            if(!node->inodes[k])
            {
                first_branch = node->closest_branch_after(k);
            }
            if(first_branch >= divider_end || first_branch == NodeStruct::NO_SUCC)
            {
                k = first_branch;
            }
            else
            {
                 // i is the index of this divider node in the parent.
                ChildIdx i = k >> sbits;
               
                ChildIdx next_branch = node->closest_branch_after(first_branch);
                if(next_branch >= divider_end || next_branch == NodeStruct::NO_SUCC)
                {
                    // To preserve path compression, don't create a divider
                    // node, just pull up the sub-trie at first_branch
                    k = next_branch;
                    
                    INode* n = node->inodes[first_branch];
                    parent->inodes[parent_offset + i] = n;
                    if(node->is_internal[first_branch])
                    {
                        parent->is_internal[parent_offset + i] = true;
                        n->skipped_bits |= ((first_branch - divider_start) & (num_divider_children - 1)) << n->num_skipped;
                        n->num_skipped += sbits;
                    }
                }
                else
                {
                    INode* divider = new INode(sbits);
                    update_mem_counter<count_mem,INode>(MemCounter::NEW, divider);

                    // Link in the divider to the parent
                    parent->inodes[parent_offset + i] = divider;
                    parent->num_empty_internal++;
                    parent->is_internal[parent_offset + i] = true;

                    // Finally link the appropriate children from the 
                    // node we are dividing's children into the divider
                    //
                    ChildIdx j = k - divider_start;
                    while(k < divider_end)
                    {
                        divider->inodes[j] = node->inodes[k];
                        if(node->is_internal[k])
                        {
                            divider->is_internal[j] = true;
                            if(!node->inodes[k]->num_skipped)
                            {
                                divider->num_empty_internal++;
                            }
                        }
                        k++;
                        j++;
                    }
                    divider->update_node_struct(); 
                }
            }
        }
        return;
    }
    template <class UpdateLeaf> void compress_into(INode* parent, INode* node, ChildIdx parent_offset, ChildIdx idx, BitIdx num_consumed, UpdateLeaf update_leaf)
    // Compress the children of node into parent
    {
        if(node->is_internal[idx])
        {
            INode* n = node->inodes[idx];
            BitIdx num_bits = n->num_children_bits;
            if(n->num_skipped)
            {
                ChildIdx pidx = parent_offset + (ChildIdx)KeyInfo::extract_bits(n->skipped_bits, n->num_skipped - min_children_bits, min_children_bits);
                parent->inodes[pidx] = n;
                parent->is_internal[pidx] = true;    
                n->num_skipped -= min_children_bits;
                n->skipped_bits &= ((1 << n->num_skipped) - 1);
                if(!n->num_skipped)
                {
                    parent->num_empty_internal++;
                }
            }
            else if(num_bits > min_children_bits)
            {
                // If the number of bits branched on in this child (n) of parent
                // is greater than the number of bits we're expanding by, then
                // we need to divide the node.
                //
                divide_node(n, parent, parent_offset);
                update_mem_counter<count_mem,INode>(MemCounter::DELETE, n);
                n->destroy(); 
                delete n;
            }
            else 
            {
                // We must have num_bits == min_children_bits here.
                // This is because we always expand by min_children_bits,
                // and each node branches on at least min_children_bits.
                //
                for(int i = 0; i < (1 << num_bits); i++)
                {                    
                    parent->inodes[parent_offset + i] = n->inodes[i];
                    if(n->is_internal[i])
                    {                     
                        parent->is_internal[parent_offset + i] = true;
                        parent->num_empty_internal++;
                    }
                }
                update_mem_counter<count_mem,INode>(MemCounter::DELETE, n);
                n->destroy();
                delete n;
            }
        }
        else if(node->leaves[idx])        
        {
            update_leaf.connect(parent, node, idx, NUM_KEY_BITS - num_consumed);
        }
        return;
    }
    template <class UpdateLeaf> void check_expand(INode* parent, ChildIdx parent_idx, BitIdx shift, INode* node, UpdateLeaf update_leaf)
    {       
        
        if(node->num_children_bits >= max_children_bits || !node->is_full_enough(expand_threshold))
        {
            return;
        }
        INode* new_node = new INode(node->num_children_bits + min_children_bits);            
        update_mem_counter<count_mem,INode>(MemCounter::NEW, new_node);


        if(parent)
        {
            parent->inodes[parent_idx] = new_node;

            new_node->num_skipped = node->num_skipped;
            new_node->skipped_bits = node->skipped_bits;

            if(!node->num_skipped)
            {
                parent->num_empty_internal++;
            }
        }
        else
        {
            root = new_node;
        }
        BitIdx num_bits = node->num_children_bits;
        for(int i = 0; i < (1 << num_bits); i++)
        {
            compress_into(new_node, node, 
                    i << min_children_bits, i, 
                    NUM_KEY_BITS - shift + min_children_bits, update_leaf);
        }
        new_node->update_node_struct();
        node->destroy();
        update_mem_counter<count_mem,INode>(MemCounter::DELETE, node);
        delete node;
        return;
    }
    void check_contract(INode* parent, ChildIdx parent_idx, INode* node)
    {
        if(node->num_children_bits <= min_children_bits || !node->is_empty_enough(contract_threshold))
        {
            return;
        }

        INode* new_node = new INode(min_children_bits);
        update_mem_counter<count_mem,INode>(MemCounter::NEW, new_node);
        
        divide_node(node, new_node, 0);
        new_node->update_node_struct();

        if(parent)
        {
            if(new_node->node_struct->get_num_set_bits() == 1)
            {
                ChildIdx idx = new_node->first_branch();
                INode* n = new_node->inodes[idx];
                parent->inodes[parent_idx] = n;
                n->skipped_bits = (node->skipped_bits << min_children_bits) | idx;
                n->num_skipped = node->num_skipped + min_children_bits;
                               
                new_node->destroy();
                update_mem_counter<count_mem,INode>(MemCounter::DELETE, new_node);
                delete new_node;
            }
            else
            {
                parent->inodes[parent_idx] = new_node;
                new_node->skipped_bits = node->skipped_bits;
                new_node->num_skipped = node->num_skipped;
            } 
        }
        else
        {
            if(new_node->node_struct->get_num_set_bits() == 1)
            {
                root = new_node->inodes[new_node->first_branch()];
                new_node->destroy();
                update_mem_counter<count_mem,INode>(MemCounter::DELETE, new_node);
                delete new_node;
            }
            else
            {
                root = new_node;
            }
        }

        node->destroy();
        update_mem_counter<count_mem,INode>(MemCounter::DELETE, node);
        delete node;        
        return;
    }

    ValueType* search(const KeyType& key)
    {
        return search(key, DefaultMatchTester());
    }
    template <class MatchTester> ValueType* search(const KeyType& key, MatchTester match_tester) const
    {
        BitIdx shift = NUM_KEY_BITS - root->num_children_bits;
        ChildIdx idx = (ChildIdx)KeyInfo::extract_bits(key, shift, root->num_children_bits);

        INode* node = root;
        INode* child = root->inodes[idx];
        while(shift > 0 && node->is_internal[idx])            
        {
            shift -= child->num_children_bits + child->num_skipped;
            idx = (ChildIdx)KeyInfo::extract_bits(key, shift, child->num_children_bits);
            node = child;
            child = node->inodes[idx];
        }    
        if(!child)
        {
            return 0;
        }
        Leaf* leaf = node->leaves[idx];
        if(match_tester(leaf->key, key))
        {
            return &(leaf->value);
        }
        return 0; 
    }
    typedef enum { FOUND_KEY = 0, FOUND_SUCC, FOUND_PRED } SearchStatus;
    ValueType* general_search(const KeyType& key, SearchStatus& status) const
    {
        BitIdx shift = NUM_KEY_BITS - root->num_children_bits;
        ChildIdx idx = (ChildIdx)KeyInfo::extract_bits(key, shift, root->num_children_bits);

        INode* node = root;
        INode* child = root->inodes[idx];
        while(shift > 0 && node->is_internal[idx])            
        {
            shift -= child->num_children_bits + child->num_skipped;
            idx = (ChildIdx)KeyInfo::extract_bits(key, shift, child->num_children_bits);
            node = child;
            child = node->inodes[idx];
        }    
        status = FOUND_KEY;
        if(!child)
        {
            ChildIdx i = node->closest_branch_before(idx);
            if(i > static_cast<ChildIdx>(1 << node->num_children_bits))
            {
                status = FOUND_SUCC;
                // Stay left.
                idx = node->closest_branch_after(idx);

                while(node->is_internal[idx])
                {
                    node = node->inodes[idx];
                    idx = node->first_branch();
                }
            }
            else
            {
                status = FOUND_PRED;
                // Stay right.
                idx = i;
                while(node->is_internal[idx])
                {
                    node = node->inodes[idx];
                    idx = node->first_branch();
                }
            }
        }
        return &(node->leaves[idx]->value); 
    }

    
    // This function returns the largest key less than or equal to the supplied key (key).
    bool find_predecessor(const KeyType& key, KeyType& pred_key, ValueType& pred_value) const
    {
        // First, find the deepest internal node that has a branch to a predecessor
        // of key.        
        INode* pred_ancestor = 0;
        ChildIdx idx_at_ancestor = 0;
        BitIdx shift = NUM_KEY_BITS - root->num_children_bits;
        ChildIdx idx = (ChildIdx)KeyInfo::extract_bits(key, shift, root->num_children_bits);
        
        KeyType key_bits = 0;
        // Loop until we are at the bottom of the trie 
        // (i.e. when shift == 0 or we're at a leaf) or
        // when the path compression bits don't match the key's bits.
        INode* node = root;
        INode* child = root->inodes[idx];
        while(shift > 0)
        {
            if(node->has_branch_before(idx))
            {
                pred_ancestor = node;
                idx_at_ancestor = idx;
            }
            if(!node->is_internal[idx])
            {
                break;
            }
            key_bits = KeyInfo::extract_bits(key, shift - child->num_skipped, child->num_skipped);
            if(key_bits != child->skipped_bits)
            {                
                break; 
            }
            shift -= child->num_children_bits + child->num_skipped;            
            idx = (ChildIdx)KeyInfo::extract_bits(key, shift, child->num_children_bits);
            node = child;
            child = node->inodes[idx];
        }
        if(!shift || !node->is_internal[idx])
        {
            // If we're in here then node has a leaf at idx
            // or a null-branch.
            //
            if(node->leaves[idx] && node->leaves[idx]->key <= key)
            {
                // The predecessor is sitting at the leaf,
                // and we're all done.
                pred_key = node->leaves[idx]->key;
                pred_value = node->leaves[idx]->value;
                return true;
            }
            else if(node->has_branch_before(idx))            
            {
                pred_ancestor = node;
                idx = node->closest_branch_before(idx);
            }
            else if(pred_ancestor)
            {
                idx = pred_ancestor->closest_branch_before(idx_at_ancestor);
            }
        }
        else if(key_bits > child->skipped_bits)
        {
            pred_ancestor = node; 
        }
        else if(key_bits < child->skipped_bits && pred_ancestor)
        {
            idx = pred_ancestor->closest_branch_before(idx_at_ancestor);
        }
        if(!pred_ancestor)
        {
            return false;
        }
        node = pred_ancestor;
        if(idx >= static_cast<ChildIdx>(1 << node->num_children_bits))
        {
            // Deal with the special case where the trie is just the
            // root with 0 or 1 children.
            return false;
        }
        while(node->is_internal[idx])
        {            
            node = node->inodes[idx];
            idx = node->last_branch();
        }
        // now we have the leaf, tidy up and we're done.
        Leaf* l = node->leaves[idx];
        pred_key = l->key;
        pred_value = l->value;
        return true; 
    }
    void print(std::ostream& out)
    {
        using namespace std;
        out << "digraph G { " << endl;
        list<INode*> worklist;
        worklist.push_front(root);
        while(!worklist.empty())
        {
            INode* n = worklist.front();
            worklist.pop_front();
            for(int i = 0; i < (1 << n->num_children_bits); i++)
            {
                if(n->is_internal[i])
                {
                    INode* child = n->inodes[i];
                    out << hex << "\"" << n << " (" << (int) n->num_children_bits << ")\" -> \"" << child << "\"[label=\"" << i << "(" << dec << (int) child->num_skipped << ", " << hex << (int) child->skipped_bits << ")\"];" << endl;
                    worklist.push_back(child);
                }
                else if(n->leaves[i])
                {
                    Leaf* l = n->leaves[i];
                    out << hex << "\"" << n << " (" << (int) n->num_children_bits << ")\" -> \"" << l->key << " -> " << l->value << "\"[label=\"" << i << "\"]" << endl;
                }
            }
        }
        out << "}";
        return;
    }
    void destroy()
    {
        using namespace std;
        list<INode*> worklist;
        worklist.push_front(root);
        while(!worklist.empty())
        {
            INode* n = worklist.front();
            worklist.pop_front();
            for(int i = 0; i < (1 << n->num_children_bits); i++)
            {
                if(n->is_internal[i])
                {
                    INode* child = n->inodes[i];
                    worklist.push_back(child);
                }
                else if(n->leaves[i])
                {
                    update_mem_counter<count_mem,Leaf>(MemCounter::DELETE, n->leaves[i]);
                    delete n->leaves[i];
                }
            }
            n->destroy();
            update_mem_counter<count_mem,INode>(MemCounter::DELETE, n);
            delete n;
        }
        return;
    }
    int get_min_children_bits() { return min_children_bits; }
    ~LPCTrie()
    {
        destroy();
        return;
    }
};

#endif

