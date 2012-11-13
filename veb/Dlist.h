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
// Dlist.h
// $Revision: $
// $Date: $
//
// A simple doubly-linked list. It's main advantage for our work 
// is that its interface works on pointers to nodes, so we can 
// use 0 as a special value.
// ============================================================================

#ifndef MAP32_DLIST_H
#define MAP32_DLIST_H

#include <cassert>
#include <iostream>

#include "allocator.h"

typedef int Type; // Element type stored in doubly-linked list
                 // will be templatized later

//typedef unsigned long Type;

class Dnode {
    Type   info;
    Dnode *left;
    Dnode *right;
public:
    Dnode()          : info(0), left(0), right(0) {}
    Dnode(Type x)    : info(x), left(0), right(0) {}
    Type   getinfo()  { return info; }
    Dnode* getleft()  { return left; }
    Dnode* getright() { return right; }
    void   setinfo(  Type   x) { info  = x; }
    void   setleft(  Dnode* n) { left  = n; }
    void   setright( Dnode* n) { right = n; }
#ifdef USE_LEDA_MEMORY
    LEDA_MEMORY(Dnode);
#endif  
};

class Dlist {
    Dnode *leftend;
    Dnode *rightend;
public:
    Dlist() : leftend(0), rightend(0) {};
    ~Dlist();

    int    isempty()  { return leftend == 0;}
    Dnode *firstnode(){ return leftend; }
    Dnode *lastnode() { return rightend; }

    void insertfirst(Type x);
    void insertlast(Type x);
    void insertright(Type x, Dnode *p);
    void insertleft(Type x, Dnode *p);
    Type removefirst();
    Type removelast();
    Type removeright(Dnode *p);
    Type removeleft(Dnode *p);
    Type remove(Dnode *p);
    int find(Type x);
    void printDebug( std::ostream& out = std::cerr);

private:
#ifdef USE_LEDA_MEMORY
    LEDA_MEMORY(Dlist);
#endif
    static default_allocator<Dnode> node_alloc;
};

// Hack: definition of static allocator variable in header, trust its included 
// only once in the application here. Will be fixed later to change list 
// to a template design.
default_allocator<Dnode> Dlist::node_alloc;


inline void Dlist::insertfirst(Type x) {
#ifndef USE_LEDA_MEMORY
    Dnode *p = node_alloc.allocate(1);
    node_alloc.construct(p, Dnode(x));
#else
    Dnode *p = new Dnode (x);
#endif
    assert(p);
    p->setright(leftend);
    if (leftend)
        leftend->setleft(p);
    else
        rightend = p;
    leftend = p;
}

inline void Dlist::insertlast(Type x) {
#ifndef USE_LEDA_MEMORY
    Dnode *p = node_alloc.allocate(1);
    node_alloc.construct(p, Dnode(x));
#else
    Dnode *p = new Dnode (x);
#endif
    assert(p);
    p->setleft(rightend);
    if (rightend)
        rightend->setright(p);
    else
        leftend = p;
    rightend = p;
}

inline void Dlist::insertleft(Type x, Dnode *p) {
    assert (p);
#ifndef USE_LEDA_MEMORY
    Dnode *q = node_alloc.allocate(1);
    node_alloc.construct(q, Dnode(x));
#else
    Dnode *q = new Dnode(x);
#endif
    assert (q);
    Dnode *r = p->getleft();
    p->setleft(q);
    q->setleft(r);
    q->setright(p);
    if (r)
        r->setright(q);
    else
        leftend = q;
}

inline void Dlist::insertright(Type x, Dnode *p) {
    assert (p);
#ifndef USE_LEDA_MEMORY
    Dnode *q = node_alloc.allocate(1);
    node_alloc.construct(q, Dnode(x));
#else
    Dnode *q = new Dnode(x);
#endif
    assert (q);
    Dnode *r = p->getright();
    p->setright(q);
    q->setright(r);
    q->setleft(p);
    if (r)
        r->setleft(q);
    else
        rightend = q;
}

inline Type Dlist::removefirst() {
    assert(!isempty());
    Dnode *p = leftend;
    Type x = p->getinfo();
    leftend = p->getright();
    if (leftend)
        leftend->setleft(NULL);
    else
        rightend = NULL;
#ifndef USE_LEDA_MEMORY
    node_alloc.destroy(p);
    node_alloc.deallocate(p,1);
#else
    delete p;
#endif
    return x;
}

inline Type Dlist::removelast() {
    assert(!isempty());
    Dnode *p = rightend;
    Type x = p->getinfo();
    rightend = p->getleft();
    if (rightend)
        rightend->setright(NULL);
    else
        leftend = NULL;
#ifndef USE_LEDA_MEMORY
    node_alloc.destroy(p);
    node_alloc.deallocate(p,1);
#else
    delete p;
#endif
    return x;
}

inline Type Dlist::removeleft(Dnode *p) {
    assert(p);
    Dnode *q = p->getleft();
    assert(q);
    Dnode *r = q->getleft();
    Type x = q->getinfo();
    p->setleft(r);
    if (r)
        r->setright(p);
    else
        leftend = p;
#ifndef USE_LEDA_MEMORY
    node_alloc.destroy(q);
    node_alloc.deallocate(q,1);
#else
    delete q;
#endif
    return x;
}

inline Type Dlist::removeright(Dnode *p) {
    assert(p);
    Dnode *q = p->getright();
    assert(q);
    Dnode *r = q->getright();
    Type x = q->getinfo();
    p->setright(r);
    if (r)
        r->setleft(p);
    else
        rightend = p;
#ifndef USE_LEDA_MEMORY
    node_alloc.destroy(q);
    node_alloc.deallocate(q,1);
#else
    delete q;
#endif
    return x;
}

inline Type Dlist::remove(Dnode *p) {
    assert(p);
    Type x = p->getinfo();
    Dnode *q = p->getleft();
    Dnode *r = p->getright();
    if (q)
        q->setright(r);
    else
        leftend = r;
    if (r)
        r->setleft(q);
    else
        rightend = q;
#ifndef USE_LEDA_MEMORY
    node_alloc.destroy(p);
    node_alloc.deallocate(p,1);
#else
    delete (p);
#endif
    return x;
}

inline void Dlist::printDebug( std::ostream& out) {
    Dnode *p = leftend;
    while (p){
        out << p-> getinfo()<<' ';
        p = p->getright();
    }
    out << std::endl;
}

inline Dlist::~Dlist() {
    Dnode *p = leftend;
    while (p) {
        Dnode *q = p;
        p = p->getright();
#ifndef USE_LEDA_MEMORY
        node_alloc.destroy(q);
        node_alloc.deallocate(q,1);
#else
        delete q;
#endif
    }
}

inline int Dlist::find(Type x) {
    Dnode *p = leftend;
    while (p) {
        if (p->getinfo()==x)
            return 1;
        p = p->getright();
    }
    return 0;
}
#endif // MAP32_DLIST_H


