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
// STree_orig.h
// $Revision: $
// $Date: $
//
// An impl. of van Emde Boas trees following the original description.
// ============================================================================

// Attn: this header file is order dependent, has to come before LEDA!!


#ifndef MAP32_STREE_ORIG_H
#define MAP32_STREE_ORIG_H

#ifdef GCC3
#include <ext/hash_map>
#else
#include <hash_map>
#endif

#include "allocator.h"

template <class Alloc = std::allocator<int> >
class STree_orig {
    int logDeg,logDeg1;
    int size;
    int mi;
    int ma;
    STree_orig* top;
#ifdef GCC3
    typedef __gnu_cxx::hash_map < int, STree_orig *> hash_table_type;
#else
    typedef std::hash_map < int, STree_orig *> hash_table_type;
#endif	
    hash_table_type * bot;

#ifdef USE_STD_ALLOCATOR
    typedef typename Alloc::template rebind<STree_orig> alloc_rebind;
    typedef typename alloc_rebind::other      local_allocator;
    static local_allocator m_alloc;
#endif
	
    STree_orig() {};
    inline int low(int x);
    inline int high(int x);
public:
    STree_orig(int x);
    STree_orig(int k,int x);
    virtual ~STree_orig();

    int locate(int x);
    int locate_down(int x);
    void insert(int x);
    void del(int x);
    int succ(int x);
    int pred(int x);
	
    bool defined(int x);
    int getSize(){ return size;};
    int min() {return mi;};
    int max() {return ma;};
#ifdef LEDA_USE_MEMORY_STD 
    LEDA_MEMORY(STree_orig);
#endif
};

#ifdef USE_STD_ALLOCATOR
// Definition of the static allocator variable
template<class Alloc>
typename STree_orig<Alloc>::local_allocator STree_orig<Alloc>::m_alloc;
#endif


template <class Alloc>    // x mod sqrt(N)
int STree_orig<Alloc>::low(int x) { return x & ((1<<logDeg)-1); } 

template <class Alloc>    // x div sqrt(N)
int STree_orig<Alloc>::high(int x) { return x >> logDeg; }         


template <class Alloc>
STree_orig<Alloc>::STree_orig(int x) // create single element k-bit tree for {x}
{
    const int k = 32;
    top = NULL;
    bot = NULL;
    logDeg = k/2;
    logDeg1 = ((logDeg)*2 == k)? logDeg: (logDeg+1); 
    size = 1;
    mi = ma = x;
}

template <class Alloc>
STree_orig<Alloc>::STree_orig(int k,int x)
{
    top = NULL;
    bot = NULL;
    logDeg = k/2;
    logDeg1 = ((logDeg)*2 == k)? logDeg: (logDeg+1);
    size = 1;
    mi = ma = x;
}

template <class Alloc>
STree_orig<Alloc>::~STree_orig()
{
    if (size > 2)
    {
#ifdef USE_STD_ALLOCATOR
        m_alloc.destroy(top);
        m_alloc.deallocate(top,1);
#else
        delete top;
#endif
        delete bot;
    }
}

template <class Alloc>
int STree_orig<Alloc>::locate(int x) // x <= ma
{
    if(!size) return -1;

    if(x <= mi) return mi; // covers case size == 1

    if(size > 2)
    {
        typename hash_table_type::iterator it = bot->find(high(x));
        STree_orig<Alloc> * b;
 
        if (it != bot->end() && low(x) <= (b=it->second)->ma )
        {
            return (high(x)<<logDeg) + b->locate(low(x)); // recursion
        }
        else
        {
            int z = top->locate(high(x) + 1);  // recursion
            return (z<<logDeg) + (bot->find(z)->second->mi);// O(1) hash table access
        }
    }
    return ma;
}
 
 
template <class Alloc>
int STree_orig<Alloc>::locate_down(int x) // x <= ma
{
    if(!size) return -1;

    if (x >= ma) return ma; // covers case size == 1

    if(size > 2)
    {
        typename hash_table_type::iterator it = bot->find(high(x));
        STree_orig<Alloc> * b;
        if (it != bot->end() && low(x) >= (b=it->second)->mi )
        {
            return (high(x)<<logDeg) + b->locate_down(low(x)); // recursion
        }
        else
        {
            int z = top->locate_down(high(x)-1);  // recursion
            return (z<<logDeg) + bot->find(z)->second->ma;// O(1) hash table access
        }
    }
    else
    {
        return (x < mi)? -1 : mi;
    }
}


template <class Alloc> 
int STree_orig<Alloc>::succ(int x) 
{
    if((!defined(x))|| size<=1 )
        return -1;

    return locate(x+1);
}

template <class Alloc>
int STree_orig<Alloc>::pred(int x) // x <= ma
{
    if((!defined(x))|| size<=1 )
        return -1;

    return locate_down(x-1);
}


template <class Alloc>
bool STree_orig<Alloc>::defined(int x)
{
    if(x > ma || x< mi || (!size)) return false;
    if(x == ma || x == mi) return true;

    if(size > 2)
    {
        typename hash_table_type::iterator b = bot->find(high(x));
        if(b!=bot->end())
            return b->second->defined(low(x));
    }

    return false;	
}

template <class Alloc>
void STree_orig<Alloc>::insert(int x) // require !x in D
{
    if(defined(x))
        return;

    if(!size)
    {
        mi=ma=x;
        size=1;
        return;
    }

    STree_orig<Alloc> *b;

    switch(size)
    {
    case 1:
        if (x > ma) ma = x;
        else if (x < mi) mi = x;
        size++;
        return;
    case 2:
#ifdef USE_STD_ALLOCATOR
        top = m_alloc.allocate(1);
        m_alloc.construct(top,STree_orig<Alloc>(logDeg1,high(mi)));
        b = m_alloc.allocate(1);
        m_alloc.construct(b,STree_orig<Alloc>(logDeg,low(mi)));
#else
        top = new STree_orig<Alloc>(logDeg1,high(mi));
        b = new STree_orig<Alloc>(logDeg,low(mi));
#endif
        bot = new hash_table_type((1<<logDeg)>>3);
			
        bot->insert(hash_table_type::value_type(high(mi),b));

        if(high(mi) == high(ma))
        {
            b->insert(low(ma));
        }
        else
        {
            top->insert(high(ma));
#ifdef USE_STD_ALLOCATOR
            b = m_alloc.allocate(1);
            m_alloc.construct(b,STree_orig<Alloc>(logDeg,low(ma)));
#else
            b = new STree_orig<Alloc>(logDeg,low(ma));
#endif
            bot->insert(hash_table_type::value_type(high(ma),b));
        }
    default:
        typename hash_table_type::iterator it = bot->find(high(x));
        if(it == bot->end())
        {
#ifdef USE_STD_ALLOCATOR
            b = m_alloc.allocate(1);
            m_alloc.construct(b,STree_orig<Alloc>(logDeg,low(x)));
#else
            b = new STree_orig<Alloc>(logDeg,low(x));
#endif
            top->insert(high(x));
            bot->insert(hash_table_type::value_type(high(x),b));	
        }
        else
        {
            it->second->insert(low(x));
        }
        break;
    }
    if (x > ma) ma = x;
    else if (x < mi) mi = x;

    size++;

}

template <class Alloc>
void STree_orig<Alloc>::del(int x) // require x in D && size >= 2
{
    if(!defined(x))
        return;

    if(size <2)
    {
        size=0;
    } else if (size == 2 ) { 
        // back to trivial tree
        size = 1;
    		
        if (x == mi) 
            mi = ma; 
        else 
            ma = mi;
    		
    }
    else
    {
        STree_orig<Alloc> * b = bot->find(high(x))->second; // O(1) hash table access
        if (b->size == 1)
        { 
            if(x==ma)
                ma = pred(x);
            if(x==mi)
                mi = succ(x);

#ifdef USE_STD_ALLOCATOR
            m_alloc.destroy(b);
            m_alloc.deallocate(b,1);
#else
            delete b;
#endif
            bot->erase(high(x)); // O(1) hash table access
            top->del(high(x)); // recursion

        }
        else
        {
            b->del(low(x));  // recursion
            // reconstruct mi and ma
            ma = ((top->ma)<<logDeg) + bot->find(top->ma)->second->ma; // O(1) hash table access
            mi = ((top->mi)<<logDeg) + bot->find(top->mi)->second->mi; // O(1) hash table access
        }
    		
        if(size == 3)
        {
            if(high(mi) == high(ma))
            {
                b = bot->find(high(ma))->second;
                bot->erase(high(ma));
#ifdef USE_STD_ALLOCATOR
                m_alloc.destroy(b);
                m_alloc.destroy(top);
                m_alloc.deallocate(b,1);
                m_alloc.deallocate(top,1);
#else
                delete b;
                delete top;
#endif
                delete bot;
            }
            else
            {
                b = bot->find(high(ma))->second;
                bot->erase(high(ma));
#ifdef USE_STD_ALLOCATOR
                m_alloc.destroy(b);
                m_alloc.deallocate(b,1);
#else
                delete b;
#endif
                b = bot->find(high(mi))->second;
                bot->erase(high(mi));
#ifdef USE_STD_ALLOCATOR
                m_alloc.destroy(b);
                m_alloc.destroy(top);
                m_alloc.deallocate(b,1);
                m_alloc.deallocate(top,1);
#else
                delete b;
                delete top;
#endif
                delete bot;
                top = NULL;
            }
        }	
        size--;
    }
}

#endif // MAP32_STREE_ORIG_H
