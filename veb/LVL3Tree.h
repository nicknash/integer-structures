// ============================================================================
// (c) Copyright 2003 Roman Dementiev, Lutz Kettner, Jens Mehnert, and Peter
// Sanders, MPI Informatik, Stuhlsatzenhausweg 85, 66123 Saarbrucken, Germany.
//
// This source code is described in the paper: 
// Engineering a Sorted List Data Structure for 32 Bit Keys.
// By Roman Dementiev, Lutz Kettner, Jens Mehnert and Peter Sanders. 
// In: Algorithm Engineering and Experiments (ALENEX'04), New Orleans, Jan 2004
// ----------------------------------------------------------------------------
//
// LVL3Tree.h
// $Revision: $
// $Date: $
//
// The third level data structure and its handle class. The handle class 
// manages that we store a direct pointer to the list entry if only one 
// element is stored in the third level structure. Only if more than one
// element is stored we allocate the third level structure.
// ============================================================================

#ifndef MAP32_LVL3TREE_H
#define MAP32_LVL3TREE_H

#include <iostream>

#include "LPHash.h"
#include "Top23.h"
#include "Dlist.h"
#include "allocator.h"

typedef int Type; // Element type stored in doubly-linked list
                  // will be templatized later

class LVL3Tree {  
public:
    unsigned char minKey; // Keys fuer min und max
    unsigned char maxKey;
    Dnode *max;           // Minimum und maximumpointer
    Dnode *min;           
    Top23  top;           // Pointer auf die top-datenstrucktur
    LPHash< Dnode* > bot; // verwalte die listiteratoren in eine hashtabelle

    LVL3Tree( Dnode *node);
    void insert_2nd(Type,unsigned int, Dlist&);
    void insert(Type,unsigned int, Dlist&);
    bool del(unsigned char, Dlist& D);
    Dnode* locateNode(unsigned char);

    void printDebugHash( std::ostream& out = std::cerr);

#ifdef USE_LEDA_MEMORY
    LEDA_MEMORY(LVL3Tree);
#endif
};

/* Case analysis:
 * - the list cannot be empty since we are in level 3
 * - we insert at the end (different constructure)
 * - we insert at the beginning or inbetween
 */

inline LVL3Tree::LVL3Tree( Dnode *node) :
    minKey( node->getinfo()),
    maxKey( minKey),
    max( node),
    min( node)
{}

// Adds second key after the constructor has been called
// Precond: the LVL3Tree contains exactly one element
inline void LVL3Tree::insert_2nd(Type listItem,unsigned int elem, Dlist& D) {
    unsigned char midKey = elem;
    assert( midKey!=minKey); // new key
    bot.init(); // init hash table
    top.insert(minKey); // update top structure
    top.insert(midKey);
    if(minKey < midKey){
        Dnode *tmp = min;
        tmp = tmp->getright();
        D.insertleft(listItem,tmp); // insert elem in list
        max = min; // new max
        max =max->getright();   
        bot.insert(min,minKey);
        maxKey = midKey;  
        bot.insert(max,maxKey); 
    } else {
        D.insertleft(listItem,min); // insert elem in list
        min =min->getleft(); // new min
        bot.insert(max,minKey);
        maxKey = minKey;
        minKey = midKey;
        bot.insert(min,minKey);     
    }
    assert(midKey<=maxKey);
    assert(midKey>=minKey);
}

/* Fuegt ein weiteres element in die strucktur ein */
// Precond: the LVL3Tree contains already two elements
inline void LVL3Tree::insert(Type listItem,unsigned int elem, Dlist& D) {
    unsigned char midKey = elem;
    assert(bot.isInitialized());
    unsigned int key = top.findNext(midKey);
    if ( key!=((unsigned int)midKey)) { // new key
        if ( key==1000) { // we have identified a new maximum
            Dnode* tmp=max;
            tmp = tmp->getright();
            D.insertleft(listItem,tmp); // insert elem in list
            tmp = tmp->getleft();
            max = tmp;
            maxKey=midKey;
            bot.insert(tmp,maxKey);
        } else {
            if ( minKey>midKey) { // found new minimum
                D.insertleft(listItem,min); // insert elem in list
                min = min->getleft(); // new min in bottom stucture
                minKey = midKey;        
                bot.insert(min,minKey); 
            } else {
                Dnode *tmp3 = *(bot.find(key)); // hash table access
                // store element before next neighbor
                D.insertleft(listItem,tmp3);
                tmp3= tmp3->getleft();
                bot.insert(tmp3,midKey);// update bot
            }
        }
        top.insert(midKey); // update top
    }    
    assert(midKey<=maxKey);
    assert(midKey>=minKey);
}

// Removes the elment 'elem' from the level 3 tree
// Precond: at least two elements in LVL2Tree.
// Returns true if only one element is left in this LVL3Tree.
inline bool LVL3Tree::del(unsigned char elem, Dlist& D) {
    assert(bot.isInitialized());
    Dnode **tmp = bot.find(elem); // find pointer to list element
    if ( tmp == 0) // no entry found for key, nothing happens
        return false;

    top.del(elem);                    // because of maxMin()
    unsigned int iTmp = top.maxMin(); // update min- and maxkey
    minKey = iTmp & 255;
    maxKey = iTmp >> 8;
    max = *(bot.find(maxKey));        // later in else branch
    min = *(bot.find(minKey));
    D.remove(*tmp);                   // remove elem from list
    if (maxKey == minKey){            // do we end up with one element only?
        bot.destroy();                // clear bot structure
        return true;                  // trigger reduction in the handle class
    }
    bot.remove(elem);              // remove pointer from bot
    return false;
}


// Find node of the next element
// Precond: at least two elements in tree
inline Dnode* LVL3Tree::locateNode(unsigned char x) {
    unsigned char elem = x;    
    unsigned int tmp = top.findN(elem);
    if ( elem<=maxKey) // element in current tree
        return *(bot.find(tmp));
    // element not in current tree
    return max->getright();
}


void LVL3Tree::printDebugHash( std::ostream& out) {
    if ( bot.isInitialized()!=0){
        for ( int i=0;i<256;i++){      
            Dnode *tm = *(bot.find(i));
            if(tm!=0){
                out << "key:" << i  << "item: " << ((*tm).getinfo()) 
                    << " tablesize:" << bot.getArraySize()<< std::endl;;
            }     
            out << std::endl;
        }
    }
}

// ==========================================================================

// The LVL3Handle encapsulates a pointer to a LVL3Tree.
// It manages the optimized allocation and deallocation of these trees,
// e.g., that a single key does not create a tree.
// Note that this class allows aliasing pointers to LVL3Trees, the user
// is responsible for proper resources management.
class LVL3Handle {
    std::ptrdiff_t tree;
    static default_allocator<LVL3Tree> lvl3_alloc;

    // if bit is set, we have a Dnode* ptr instead of a LVL3Tree ptr
    bool      bit() const  { return BIT( tree); }
    LVL3Tree* ptr()  { assert( ! bit()); return PTR(LVL3Tree*,tree); }
    Dnode*    node() { assert(   bit()); return PTR(Dnode*,tree); }

    void set_ptr( LVL3Tree* p) { tree = reinterpret_cast< std::ptrdiff_t&>(p);}
    void set_node( Dnode* p) { tree = reinterpret_cast< std::ptrdiff_t&>(p)|1;}

public:
    bool isNull() const { return tree == 0;}
    bool isTree() const { return tree != 0;}
    bool isSingular() const { return bit(); }

    Dnode* max() {
        assert( isTree());
        if ( bit())
            return node();
        return ptr()->max;
    }
    Dnode* min() {
        assert( isTree());
        if ( bit())
            return node();
        return ptr()->min;
    }
    unsigned char minKey() {
        assert( isTree());
        if ( bit())
            return (unsigned char)(node()->getinfo());
        return ptr()->minKey;
    }
    unsigned char maxKey() {
        assert( isTree());
        if ( bit())
            return (unsigned char)(node()->getinfo());
        return ptr()->maxKey;
    }

    bool operator==( LVL3Handle p) const { return tree == p.tree; }
    bool operator!=( LVL3Handle p) const { return tree != p.tree; }

    void insert(Type  listItem,unsigned int key, Dlist& D) {
        assert( isTree());
        if ( bit()) {
            if ((unsigned char)(key) != (unsigned char)(node()->getinfo())) {
                // new key: deferred creation and insertion, do it now
#ifndef USE_LEDA_MEMORY
                LVL3Tree* p = lvl3_alloc.allocate(1);
                lvl3_alloc.construct( p, LVL3Tree( node()));
                assert(((std::ptrdiff_t)(p) & 1) == 0);
                set_ptr(p);
#else
                set_ptr( new LVL3Tree( node()));
#endif
                ptr()->insert_2nd( listItem, key, D);
            }
        } else {
            ptr()->insert( listItem, key, D);
        }
    }
    void del(unsigned int key, Dlist& D) {
        assert( isTree());
        if ( bit()) {
            if ( key == (unsigned int)(node()->getinfo())) {
                D.remove( node());
                tree = 0;
            }
        } else {
            if ( ptr()->del( key, D)) { // only one element left in tree
                assert( ptr()->minKey == ptr()->maxKey);
                Dnode* p = ptr()->max;
#ifndef USE_LEDA_MEMORY
                lvl3_alloc.destroy( ptr());
                lvl3_alloc.deallocate( ptr(),1);
#else
                delete ptr();
#endif
                set_node(p);
            }
        }
    }
    Dnode* locateNode(unsigned char x) {
        assert( isTree());
        if ( bit()) {
            Dnode* p = node();
            if (x <= (unsigned char)(p->getinfo()))
                return p; 
            return p->getright(); // max->getright();
        }
        return ptr()->locateNode(x);
    }
    LVL3Handle() : tree(0) {}
    LVL3Handle( Type listItem,unsigned int key,Dlist& D,Dnode *next) {
        D.insertleft(listItem,next);  // store element in the list
        // deferred creation and insertion, store the node pointer only
        set_node( (*next).getleft());
    }
    LVL3Handle( unsigned int key,Dlist& D,Dnode *next) {
        // deferred creation and insertion, store the node pointer only
        set_node( next);
    }
    void printDebugHash( std::ostream& o = std::cerr) {
        ptr()->printDebugHash(o);
    }
};

// Hack: definition of static allocator variable in header, trust its included 
// only once in the application here. Will be fixed later to change class 
// to a template design.
default_allocator<LVL3Tree> LVL3Handle::lvl3_alloc;

  
#endif // MAP32_LVL3TREE_H
