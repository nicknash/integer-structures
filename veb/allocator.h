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
// allocator.h
// $Revision: $
// $Date: $
//
// Selects the default allocator used in all allocations.
// Set one of USE_*_ALLOCATOR in the Makefile. In addition, when 
// USE_LEDA_MEMORY is set, some structures switch to the LEDA_MEMORY 
// macro and new/delete allocation.
// ============================================================================

#ifndef MAP32_ALLOCATOR_H
#define MAP32_ALLOCATOR_H

// Some support for the bit-in-pointer trick
#define PTR(T,val) ( reinterpret_cast<T>( (val) & (~ std::ptrdiff_t(1))))
#define BIT(val)   ( bool(((val) & 1) == 1))

// The allocators

#ifdef USE_STD_MT_ALLOCATOR
#include <memory>
#define default_allocator std::allocator
#endif

#ifdef USE_STD_ALLOCATOR
#include <memory>
#ifdef GCC3
//template <class T>
//struct default_allocator : public std::__allocator<T, std::__single_client_alloc> {};
template <class T> struct default_allocator : public std::allocator<T> {};

#else
template <class T>
struct default_allocator 
    : public std::__allocator<T, std::single_client_alloc> {};
#endif
#endif

#ifdef USE_LEDA_MEMORY
#define LEDA_USE_MEMORY_STD
#include <LEDA/memory.h>
#endif

#ifdef USE_LEDA_ALLOCATOR
#define LEDA_USE_MEMORY_STD
#include <LEDA/allocator.h>
#if __LEDA__ >= 440
#define default_allocator leda::leda_allocator
#else
#define default_allocator leda_allocator
#endif
#endif

#ifdef USE_LEDA_BIG_ALLOCATOR
#define LEDA_USE_MEMORY_STD
#include <LEDA/allocator.h>

// leda allocator that manages also objects of size up to 1024 bytes
#if __LEDA__ >= 440
extern leda::memory_manager memory_mgr;
#else
extern memory_manager memory_mgr;
#endif

// Hack: definition of variable in header, trust its included only once 
// in the application here. Will be fixed later to seperate C file.
#if __LEDA__ >= 440
leda::memory_manager memory_mgr( 1024);
#else
memory_manager memory_mgr( 1024);
#endif

template <class T>
class leda_big_allocator {
public:
    typedef size_t      size_type;
    typedef ptrdiff_t   difference_type;
    typedef T           value_type;
    typedef T*          pointer;
    typedef const T*    const_pointer;
    typedef T&          reference;
    typedef const T&    const_reference;

    template <class T1> class rebind { public:
      typedef leda_big_allocator<T1> other;
    };

    leda_big_allocator() {}
    leda_big_allocator(const leda_big_allocator<T>&) {}
    template <class TO> 
    leda_big_allocator(const leda_big_allocator<TO>&) {}
    ~leda_big_allocator() {}

    pointer allocate(size_type n, const_pointer = 0) {
        return 0 == n ? 0 : 
            (T*) memory_mgr.allocate_bytes( n * sizeof(T) );
    }
    void deallocate(pointer p, size_type n) { 
        memory_mgr.deallocate_bytes(p , n * sizeof(T));
    }
    void construct(pointer p, const_reference r) { new(p) value_type(r); }
    void destroy(pointer p)                      { p->~T(); }

    pointer       address(reference r)       { return &r; }
    const_pointer address(const_reference r) { return &r; }
    size_type     max_size() const { return memory_mgr.max_size(); }
};

#define default_allocator leda_big_allocator

#endif // USE_LEDA_BIG_ALLOCATOR

#ifdef USE_LOKI_ALLOCATOR
#include <SmallObj.h>

// leda allocator that manages also objects of size up to 1024 bytes
extern Loki::SmallObjAllocator memory_mgr;

// Hack: definition of variable in header, trust its included only once 
// in the application here. Will be fixed later to seperate C file.
Loki::SmallObjAllocator memory_mgr( 4096, 1024);

template <class T>
class loki_allocator {
public:
    typedef size_t      size_type;
    typedef ptrdiff_t   difference_type;
    typedef T           value_type;
    typedef T*          pointer;
    typedef const T*    const_pointer;
    typedef T&          reference;
    typedef const T&    const_reference;

    template <class T1> class rebind { public:
      typedef loki_allocator<T1> other;
    };

    loki_allocator() {}
    loki_allocator(const loki_allocator<T>&) {}
    template <class TO> 
    loki_allocator(const loki_allocator<TO>&) {}
    ~loki_allocator() {}

    pointer allocate(size_type n, const_pointer = 0) {
        return 0 == n ? 0 : 
            (T*) memory_mgr.Allocate( n * sizeof(T) );
    }
    void deallocate(pointer p, size_type n) { 
        memory_mgr.Deallocate(p , n * sizeof(T));
    }
    void construct(pointer p, const_reference r) { new(p) value_type(r); }
    void destroy(pointer p)                      { p->~T(); }

    pointer       address(reference r)       { return &r; }
    const_pointer address(const_reference r) { return &r; }
    size_type     max_size() const { size_type(-1); }
};

#define default_allocator loki_allocator

#endif // USE_LOKI_ALLOCATOR

#endif // MAP32_ALLOCATOR_H
