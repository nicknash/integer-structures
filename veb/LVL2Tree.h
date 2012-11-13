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
// LVL2Tree.h
// $Revision: $
// $Date: $
//
// The second level data structure and its handle class. The handle class 
// manages that we store a direct pointer to the list entry if only one 
// element is stored in the second level structure. Only if more than one
// element is stored we allocate the second level structure.
// ============================================================================

#ifndef MAP32_LVL2TREE_H
#define MAP32_LVL2TREE_H

#include <iostream>

#include "LVL3Tree.h"
#include "LPHash.h"
#include "Top23.h"
#include "Dlist.h"
#include "allocator.h"

typedef int Type; // Element type stored in doubly-linked list
                  // will be templatized later

class LVL2Tree {
public:
    Dnode *maxx;           // direct list pointer for min and max
    Dnode *minx;
    unsigned int minKeyx;
    unsigned int maxKeyx;
    LPHash< LVL3Handle > bot;  // manage LVL3Trees in a hash table
    Top23 top; 		   // pointer to the top structure
   

    LVL2Tree( Dnode* node);
    void insert_2nd(Type,unsigned int, Dlist&);
    void insert(Type,unsigned int, Dlist&);
    bool del(unsigned int, Dlist&);
    Dnode* locateNode(unsigned int);

    void printDebugHash( std::ostream& out = std::cerr);

#ifdef USE_LEDA_MEMORY
    LEDA_MEMORY(LVL2Tree);
#endif
};

inline LVL2Tree::LVL2Tree( Dnode *node) :
    maxx( node),
    minx( node),
    minKeyx( node->getinfo()),
    maxKeyx( minKeyx)
{}


// Adds second key after the constructor has been called
// Precond: the LVL2Tree contains exactly one element
inline void LVL2Tree::insert_2nd(Type listItem,unsigned int key, Dlist& D) {
    assert( minKeyx!=key); // key differs from the one element stored
    unsigned char midKey = key     >> 8; // key for the LVL2 hash table
    unsigned char minKey = minKeyx >> 8;        
    unsigned char maxKey = maxKeyx >> 8;
    minKey = minKeyx >> 8; 
    if(midKey == minKey){  // insert both in LVL3Tree
        LVL3Handle max = LVL3Handle(minKeyx,D,maxx); // re-link original
        max.insert(listItem,key,D);  // insert new element
        maxx = max.max();            // update min/max
        minx = max.min();
        maxKeyx = (key & 0xffffff00) | (max.maxKey());
        minKeyx = (key & 0xffffff00) | (max.minKey());
        bot.init();                  // create hash table and top
        top.insert(maxKey);
        bot.insert(max,maxKey);
    } else {
        LVL3Handle max;            
        LVL3Handle min;
        if ( key < minKeyx) { // new minx
            max = LVL3Handle(minKeyx,D,maxx); // re-link original
            // insert new element to the left
            min = LVL3Handle(listItem,key,D,maxx);
            minx = maxx->getleft();
            minKeyx = key;
            minKey = midKey;
        } else { // new maxx
            Dnode *next = maxx->getright();
            max = LVL3Handle(listItem,key,D,next);
            min = LVL3Handle(key,D,minx);
            maxx = minx->getright();
            maxKeyx = key;
            maxKey = midKey;
        }
        bot.init();   // create hash table and top
        top.insert(minKey);
        top.insert(maxKey);
        bot.insert(min,minKey);
        bot.insert(max,maxKey);
    }
}

/* Fuegt neuen LVL3Tree ein, oder erweitert bestehenden */
// Precond: the LVL2Tree contains already two elements
inline void LVL2Tree::insert(Type listItem,unsigned int key, Dlist& D) {
    unsigned char midKey = key >> 8;    // Key for the LVL2 hash table
    unsigned char minKey = minKeyx>>8;        
    unsigned char maxKey = maxKeyx>>8;
    assert(bot.isInitialized());
    unsigned int nextKey = top.findNext(midKey);
    if ( nextKey == 1000) { // new local maximum found
        Dnode *next = maxx->getright();  // since inserting left of next!
        LVL3Handle max = LVL3Handle(listItem,key,D,next);
        maxx = next->getleft(); 
        maxKeyx = key;
        maxKey = midKey;
        bot.insert(max,midKey);          // update bot!!!
        top.insert(midKey);              // update top
    } else if (((unsigned char)nextKey) == midKey){  // no new tree
        // search correct subtree and insert element
        bot.find(midKey)->insert(listItem,key,D);
    } else if ( midKey <minKey) { // new min found
        LVL3Handle min = LVL3Handle(listItem,key,D,minx);
        minx = minx->getleft();
        minKeyx = key;	  
        minKey = midKey;
        bot.insert(min,midKey);          // update bot!!!
        top.insert(midKey);              // update top
    } else { // new LVL3Tree between min and max
        Dnode *next = bot.find(nextKey)->min();
        LVL3Handle tmp(listItem,key,D,next);
        bot.insert(tmp,midKey);          // update bot
        top.insert(midKey);              // update top
    }	
    assert(midKey<=maxKey);
    assert(midKey>=minKey);
}

// Removes the element 'key' from the tree
// Precond: at least two elements in LVL2Tree.
// Returns true if only one element is left in this LVL2Tree.
inline bool LVL2Tree::del(unsigned int key, Dlist& D) {
    assert(bot.isInitialized());
    unsigned char midKey = key >> 8;
    // make sure that aliasing works with hash table: ops on tmp 
    // have to affect the entry in the hash table, except if
    // handle would become the 0 pointer
    LVL3Handle* tmp = bot.find(midKey);
    if ( tmp == 0) // no entry found for key, nothing happens
        return false;

    if ( tmp->isSingular()) { // avoid having *tmp become the 0 pointer
        LVL3Handle tmp2 = *tmp;
        tmp2.del(key,D); // recursive deletion, no side affect on hash table
        if (tmp2.isNull()) { // LVL3Tree is empty
            bot.remove(midKey);
            top.del(midKey);     // remove from top structure
        }	
    } else {
        tmp->del(key,D); // recursive deletion, with side effect on hash table
    }
    // one LVL3Tree must remain
    assert(!(top.isEmpty()));
    unsigned int  iTmp   = top.maxMin();  // update min- and maxkey
    unsigned char minKey = iTmp & 255;
    unsigned char maxKey = iTmp >> 8;
    if ( maxKey == minKey) { // one LVL3Tree left, maybe needs to collapse
        LVL3Handle max = *(bot.find(maxKey));
        minKeyx = (key & 0xffffff00)|(max.minKey());
        minx = max.min();
        maxKeyx = (key & 0xffffff00)|(max.maxKey());
        maxx = max.max();
        if ( minKeyx == maxKeyx) { // one element left in LVL3Tree, collapse it
            max.del( maxKey, D); // replaces max.free(); 
            bot.destroy();
            return true;
        }
    } else {
        if ( key==minKeyx) { // Update min-max-nodes. Move? later ...
            LVL3Handle min = *(bot.find(minKey));
            // assemble new key
            minKeyx = (key & 0xffffff00)|(min.minKey());
            minx = minx->getright();
        }
        if ( key==maxKeyx) {
            LVL3Handle max = *(bot.find(maxKey));
            maxKeyx = (key & 0xffffff00)|(max.maxKey());
            maxx = maxx->getleft();
        }
    }
    return false;
}


// Find list node of the next element >= key 
inline Dnode* LVL2Tree::locateNode(unsigned int key) {
    unsigned char midKey = key >> 8;
    unsigned int tmp = top.findN(midKey);
    unsigned char maxKey = maxKeyx >> 8;
    if ( midKey <= maxKey) {
        LVL3Handle tmp2 = *(bot.find(tmp));
        if (((unsigned char)tmp)==midKey)
            return tmp2.locateNode(key);
        return tmp2.min();
    }
    // midKey > maxKey 
    return maxx->getright(); // max->max->getright();
}

void LVL2Tree::printDebugHash( std::ostream& out) {
    if ( bot.isInitialized()){
        for ( int i=0;i<256;i++){
            LVL3Handle tmp = *(bot.find(i)); 
            if(tmp.isTree()){
                out << "Entry at:" << i << std::endl;
                tmp.printDebugHash( out);
            }
        }    
        out << std::endl;
    }
}


// ==========================================================================

// The LVL2Handle encapsulates a pointer to a LVL2Tree.
// It manages the optimized allocation and deallocation of these trees,
// e.g., that a single key does not create a tree.
// Note that this class allows aliasing pointers to LVL2Trees, the class
// user is responsible for proper resource management.
class LVL2Handle {
    std::ptrdiff_t tree;
    static default_allocator<LVL2Tree> lvl2_alloc;

    // if bit is set, we have a Dnode* ptr instead of a LVL2Tree ptr
    bool      bit()  { return BIT( tree); }
    LVL2Tree* ptr()  { assert( ! bit()); return PTR(LVL2Tree*,tree); }
    Dnode*    node() { assert(   bit()); return PTR(Dnode*,tree); }

    void set_ptr( LVL2Tree* p) { tree = reinterpret_cast< std::ptrdiff_t&>(p);}
    void set_node( Dnode* p) { tree = reinterpret_cast< std::ptrdiff_t&>(p)|1;}

public:
    bool isNull() const { return tree == 0;}
    bool isTree() const { return tree != 0;}

    Dnode*  maxx() {
        assert( isTree());
        if ( bit())
            return node();
        return ptr()->maxx;
    }
    Dnode*  minx() {
        assert( isTree());
        if ( bit())
            return node();
        return ptr()->minx;
    }
    unsigned int maxKeyx() {
        assert( isTree());
        if ( bit())
            return node()->getinfo();
        return ptr()->maxKeyx;
    }

    void insert(Type  listItem,unsigned int key, Dlist& D) {
        assert( isTree());
        if ( bit()) {
            if ( key != (unsigned int)(node()->getinfo())) {
                // new key: deferred creation and insertion, do it now
#ifndef USE_LEDA_MEMORY
                LVL2Tree* p = lvl2_alloc.allocate(1);
                lvl2_alloc.construct( p, LVL2Tree( node()));
                assert(((std::ptrdiff_t)(p) & 1) == 0);
                set_ptr(p);
#else
                set_ptr( new LVL2Tree( node()));
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
            if ( key == (unsigned int)(node()->getinfo())) { // ????
                D.remove( node());
                tree = 0;
            }
        } else {
            if ( ptr()->del( key, D)) { // only one element left in tree
                assert( ptr()->minKeyx == ptr()->maxKeyx); // ????
                Dnode* p = ptr()->maxx;
#ifndef USE_LEDA_MEMORY
                lvl2_alloc.destroy( ptr());
                lvl2_alloc.deallocate( ptr(),1);
#else
                delete ptr();
#endif
                set_node(p);
            }
        }
    }
    Dnode* locateNode(unsigned int key) {
        assert( isTree());
        if ( bit()) {
            Dnode* p = node();
            if (key <= (unsigned int)(p->getinfo()))
                return p; 
            return p->getright(); // max->max->getright();
        }
        return ptr()->locateNode(key);
    }
    LVL2Handle() : tree(0) {}
    LVL2Handle( Type listItem,unsigned int key,Dlist& D,Dnode *next) {
        D.insertleft(listItem,next);  // store element in the list
        // deferred creation and insertion, store the node pointer only
        set_node( (*next).getleft());
    }
    void printDebugHash( std::ostream& o = std::cerr) { 
        ptr()->printDebugHash(o);
    }
};

// Hack: definition of static allocator variable in header, trust its included 
// only once in the application here. Will be fixed later to change class 
// to a template design.
default_allocator<LVL2Tree> LVL2Handle::lvl2_alloc;

#endif //  MAP32_LVL2TREE_H
