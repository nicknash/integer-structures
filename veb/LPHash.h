#ifndef LPHash_H
#define LPHash_H

#include <iostream>
#include <cassert>
#include <algorithm>
#include "allocator.h"

// The hash function is a static 256 bytes array initialized automatically
// once at program startup.
static unsigned char LPHash_function[256];

struct Auto_init_hash_function {
    Auto_init_hash_function() {
        for(int i = 0; i < 256; i++)
            LPHash_function[i] = i;
        std::random_shuffle( LPHash_function, LPHash_function + 256);
    }
};

// This static variable triggers the initialization of the hash function
static Auto_init_hash_function auto_init_hash_function;


template<class T, class Alloc = default_allocator<T> > 
class LPHash {
private:
    struct Elem{ // item in hash table
        T item[4];
        unsigned char key[4];
    }; 
    Elem *table; // the hash array

    int arraySize;            // size of hash array (faster than computing
                              // it from size, see below)
    unsigned char shiftVal;   // shifts hash function for smaller hash tables
                              // (faster than computing it from arraySize)
    int size;                 // number of elements stored


    typedef typename Alloc::template rebind< Elem> Elem_alloc_rebind;
    typedef typename Elem_alloc_rebind::other      Elem_allocator;
    static Elem_allocator m_alloc; // allocator used for hash table

    // not assignable
    LPHash& operator=( const LPHash&);

public:
    // not copy constructable, only applicable for empty hash table
    LPHash( const LPHash& h) : table(0) {
        if ( h.table != 0) {
            std::cerr << "Copy construction of LPHash attamptet." << std::endl;
            std::abort();
        }
    }

    LPHash() : table(0) {}

    bool isInitialized() const { return table != 0; }
    int  getArraySize()  const { return arraySize; }

    void init();            // intialize hash table to arraySize == 4
    void destroy();	    // deletes hash table

    void doubleSize(); 
    void halfSize();
    void insert( T, unsigned char);
    void remove( unsigned char);	
    T*   find( unsigned char);	

    void printDebug( std::ostream& out = std::cerr);
  
#ifdef USE_LEDA_MEMORY
    LEDA_MEMORY(LPHash);
#endif

};

// Definition of the static allocator variable
template<class T, class Alloc> 
typename LPHash<T,Alloc>::Elem_allocator LPHash<T,Alloc>::m_alloc;


// Intialize hash table to arraySize == 4
template <class T, class Alloc> inline 
void LPHash<T,Alloc>::init() {
    // we assume here silently that we have a POD and skip m_alloc.construct()
    table = m_alloc.allocate( 1);
    std::memset( table, 0, sizeof( Elem));
    shiftVal  = 6; 
    arraySize = 4;
    size      = 0;
}  


// Delete hash table
template <class T, class Alloc> inline 
void LPHash<T,Alloc>::destroy() { 
    // we assume here silently that we have a POD and skip m_alloc.destroy()
    m_alloc.deallocate( table, arraySize >> 2);
    table = 0;
}

// Double the hash table size and insert old values in the new hash table
template<class T, class Alloc> inline 
void LPHash<T,Alloc>::doubleSize() {
    assert( arraySize <= 128);
    arraySize=arraySize<<1;
    shiftVal--;
    Elem *oldTable;
    oldTable = table; 
    // we assume here silently that we have a POD and skip m_alloc.construct()
    table = m_alloc.allocate( arraySize >> 2);
    std::memset( table, 0, sizeof( Elem) * (arraySize >> 2));
    size = 0;
    for ( int i=0; i<(arraySize>>1); i++) { // copy old to new table
        T tmp = oldTable[i>>2].item[i&3];
        if ( tmp !=T())
            insert( tmp, oldTable[i>>2].key[i&3]);
    }
    // we assume here silently that we have a POD and skip m_alloc.destroy()
    m_alloc.deallocate( oldTable, (arraySize>>3));
}

// Half the hash table size and insert old values in the new hash table
template<class T, class Alloc> inline
void LPHash<T,Alloc>::halfSize() {
    assert( arraySize >= 4);
    arraySize=arraySize>>1;
    shiftVal++;
    Elem *oldTable;
    oldTable = table; 
    // we assume here silently that we have a POD and skip m_alloc.construct()
    table = m_alloc.allocate( arraySize>>2);
    std::memset( table, 0, sizeof( Elem) * (arraySize >> 2));
    size = 0;
    for ( int i=0; i<(arraySize<<1); i++) { // copy old to new table
        T tmp = oldTable[i>>2].item[i&3];
        if ( tmp !=T())
            insert( tmp, oldTable[i>>2].key[i&3]);
    }
    // we assume here silently that we have a POD and skip m_alloc.destroy()
    m_alloc.deallocate( oldTable, (arraySize>>1));
}

// Inserts and element 'key' in the hash table.
// Precond: the 'key' is not in the hash table. This has to be taken care of
// in the top level structure.
template<class T, class Alloc> inline 
void LPHash<T,Alloc>::insert( T element, unsigned char key) {
    // double hash table size if table if filled to 3/4
    if ( size < (arraySize-(arraySize>>2)) | (arraySize==256)) {
        unsigned char n = (LPHash_function[key])>>shiftVal;
        while ( table[n>>2].item[n&3] != T()) {
            n = (n + 1) & (arraySize-1); // faster than: n = (n+1) % arraySize;
        }
        table[n>>2].item[n&3] = element;
        table[n>>2].key[n&3]  = key;
        size++;
    } else {
        doubleSize();
        insert(element,key);
    }
}

// finds the 'key' in the hash table.
// Returns a pointer 'T*' to the value of type 'T' stored with the 'key' 
// or 0 if key is not stored.
template<class T, class Alloc> inline 
T* LPHash<T,Alloc>::find(unsigned char key) {
    unsigned char n = (LPHash_function[key])>>shiftVal;
    unsigned char m = n >> 2;
    unsigned char i = n & 3;
    while(table[m].item[i] != T()){
        if ( table[m].key[i] == key)
            return &(table[m].item[i]);
        n = (n + 1) & (arraySize-1); // faster than: n = (n + 1) % arraySize;
        m = n >> 2;
        i = n & 3;
    }
    return 0;
}

// removes the element 'key' from the hash table.
// Uses remove with marking (if necessary)
/* Benutze loeschen durch markieren, falls noetig
 * Markiert wird, wenn das element in keiner suchkette mehr vorkommen kann
 * Eine neuordnung erfolgt beim vergroessern oder verkleinern der tabelle
 */
template<class T, class Alloc> inline
void LPHash<T,Alloc>::remove(unsigned char key) {
    if((size>(arraySize>>2))|(arraySize==4)){
        unsigned char n = (LPHash_function[key])>>shiftVal;
        unsigned char firstHole = n;

        if (size==256)
            firstHole=(n+255)%256;
        else
            while (table[firstHole>>2].item[firstHole&3] != T())
                firstHole=(firstHole+1)%arraySize;
        while (table[n>>2].item[n&3] != T()){
            if ( table[n>>2].key[n&3]==key){
                // remove item, check invariant to the right
                table[n>>2].item[n&3]=T();
                size--;
                unsigned char empty = n;    // possible insertion place
                n = (n + 1) & (arraySize-1); // faster than: (n+1) % arraySize;
                while ( table[n>>2].item[n&3]!=T()){
                    unsigned char pos =LPHash_function[table[n>>2]
                                                      .key[n&3]]>>shiftVal;
                     // Must become more efficient ||
                    if(((pos>firstHole)&(empty<firstHole))
                       |((empty<firstHole)&(pos<=empty))
                       |((pos>firstHole)&(pos<=empty))) {
                        // copy item to empty place
                        table[empty>>2].key[empty&3]=table[n>>2].key[n&3];
                        table[empty>>2].item[empty&3]=table[n>>2].item[n&3];
                        table[n>>2].item[n&3]=T(); // remove the copied item
                        empty=n; // set new empty position
                    }
                    n = (n + 1) & (arraySize-1);
                }	
                break; // main loop
            } else {
                n = (n + 1) & (arraySize-1);
            }
        }
    } else { // shrink table and remove element thereafter
        halfSize();
        remove(key);
    }
}

// debug printout of all records in the hash table
template<class T, class Alloc>
void LPHash<T,Alloc>::printDebug( std::ostream& out) {
    if(table!=0){
        out << "Element-Key:" << std::endl;
        int count = 0;
        for( int i = 0; i < arraySize; i++) {
            int x = -1;
            if ( table[i>>2].item[i&3]!=T()){ 
                x =* (table[i>>2].item[i&3]);
                count++;
            }
            out << "(" << x << "," << ((int)table[i>>2].key[i&3]) << ")"; 
        }
        if( count != size){ // error condition
            out << "Error starting from:"<< std::endl;
            size = 1000;
        }
        out << std::endl << " element size " << size <<" and table size "
            << arraySize << std::endl;
    }
}

#endif // LPHash_H

