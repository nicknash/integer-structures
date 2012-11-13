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
// Top1.h
// $Revision: $
// $Date: $
//
// The top structure in level one. A stratified tree for 16 bits.
// ============================================================================

#ifndef MAP32_TOP1_H
#define MAP32_TOP1_H

#include <iostream>

class Top1 {	
    unsigned int hi1;      // left  of tree top layer; most significant bits
    unsigned int hi2;      // right of tree top layer; least significant bits
    unsigned int mid[64];  // tree middle layer
    unsigned int lo[2048]; // tree lower layer

    unsigned char msTable[65536]; // Table for ms_one

    unsigned int ms_one(unsigned int); 
    unsigned int ms_oneT(unsigned int); // table based version
    unsigned int ls_one(unsigned int);

    unsigned int findHi(unsigned int); 
    unsigned int findMid(unsigned int, unsigned int); 
    unsigned int findLo(unsigned int, unsigned int); 
public:
    Top1(); 
    bool isEmpty() { return (hi1|hi2) == 0; }
    bool isElement(unsigned int);  

    void insert(unsigned int); 
    void del(unsigned int);
    unsigned int findN(unsigned int);
    unsigned int maxMin();

    void printDebugList(  std::ostream& out = std::cerr);
};

inline unsigned int Top1::ms_one(unsigned int mry) {
    struct dint{unsigned int lo,hi;};
    union hack{double x; dint y;}; 
    register hack mr;
    mr.x = mry;
    return (mr.y.hi>>20)-1023;
}

// version with 16 bit lookup table
inline unsigned int Top1::ms_oneT(unsigned int x) {
    if(x>65535)
        return msTable[x>>16]+16;
    else
        return msTable[x];    
}

inline unsigned int Top1::ls_one(unsigned int x) { // position least sign. one
    union fe_cvt { float f; int   i; };
    register fe_cvt fe;
    fe.f=(float) ( (~(x-1))&x );
    return (fe.i>>23) - 0x7f;
}

// Find position in high level
inline unsigned int Top1::findHi(unsigned int apos) {	
    if (apos > 31) { 	
        unsigned int cut = apos & 31;
        if (cut == 0) {  // start at first bit
            apos = 63 - ms_oneT(hi2); // negative if not existent
        } else {
            unsigned int tmp = hi2&(0xffffffffu >> cut); 
            apos = 63 - ms_oneT(tmp); // negative if not existent
        }
    } else {
        if ( hi1 == 0) {
            apos = 63 - ms_oneT(hi2); // negative if not existent
        } else {
            if ( apos == 0) { // test first bit of hi1
                apos = 31 - ms_oneT(hi1); // negative if not existent
            } else { // the bit we are looking for lies inbetween
                unsigned int tmp = hi1&(0xffffffffu >> apos); 
                if ( tmp !=0)
                    apos = 31 - ms_oneT(tmp);
                else
                    apos = 63 - ms_oneT(hi2);
            }
        }
    }        
    return apos;           
}

// Find position in middle level
inline unsigned int Top1::findMid(unsigned int elem, unsigned int apos) {
    unsigned int tmp; 
    tmp = mid[apos]&(0xffffffffu>>elem);
    if(tmp>65535)
        tmp = msTable[tmp>>16]+16;        // ms_one replacement
    else
        tmp = msTable[tmp];   
    apos = (apos<<5) + 31 - tmp;
    return apos;
}

// Find position in low level
inline unsigned int Top1::findLo(unsigned int elem, unsigned int apos) {
    unsigned int tmp = lo[apos]&(0xffffffffu >> elem);
    if ( tmp>65535)
        tmp = msTable[tmp>>16]+16; // ms_one replacement
    else tmp = msTable[tmp];   
    return (apos<<5) + 31 - tmp;
    // } 
} 

inline Top1::Top1() {
    for(int i=0;i<2048;i++) // replace with memset later
        lo[i] = 0;
    for(int i=0;i<64;i++)
        mid[i] = 0;
    hi1 = 0;
    hi2 = 0;
    // init table for ms_one
    for(int e=0;e<65536;e++)
        msTable[e] = ms_one(e);
}

// Check if x is an element
bool Top1::isElement(unsigned int x) {
    unsigned int elem = x;
    unsigned int apos = elem>>5;                    // position in lo-array
    unsigned int ipos = (2147483648u) >> (31&elem); 
    return (lo[apos] == (lo[apos]|ipos));      
}

// Find next element
inline unsigned int Top1::findN(unsigned int next) {
    // hiPos (bit position in top level) is not yet needed
    // midPos = bit position in middle level
    // loPos  = bit position in lo[midPos] (32 bit)
    unsigned int midPos = next>>5;
    unsigned int loPos = next&31; 

    //Delete all bits before our search position
    unsigned int pos = lo[midPos]&(0xffffffffu >> loPos); 

    // if there are still bits left in pos then result is in same lo[] field
    if (pos!=0){   
        if(pos>65535)
            pos = msTable[pos>>16]+16;      
        else
            pos = msTable[pos];   
        return (midPos<<5) + 31 - pos;
    } else {          
        // We have to get one level up in the tree since the current integer
        // in lo[] was empty. It also implies that we can incr. midPos once.
        midPos++;
        unsigned int hiPos = midPos>>5;
        pos = mid[hiPos]&(0xffffffffu>>(midPos&31));
        if(pos!=0){ // Found non-zero position in mid[]
            if(pos>65535)
                pos = msTable[pos>>16]+16; // ms_one replacement
            else
                pos = msTable[pos];
            pos = (hiPos<<5) + 31 - pos ;  
            return findLo(0,pos);  		
        } else { // mid[] was zero, we continue in the high level.
            pos = findHi(hiPos+1);
            if (pos<64) {  // found it
                return findLo(0,findMid(0,pos));
            } else {
                return 0xffffffff;
            }
        }
    }
}

// Find current min and max values
inline unsigned int Top1::maxMin() {
    // High level
    unsigned int mi,ma;
    if(hi1 == 0){ // first 32 bits are 0
        if(hi2 == 0){ // both 32 bit top level entries are 0
            return 0xffffffff;
        }
        mi = 63 - ms_one(hi2);
        ma = 63 - ls_one(hi2);
    } else {
        if(hi2==0) { // Last 32 bits are 0
            ma = 31 - ls_one(hi1);
        } else { 
            ma = 63 - ls_one(hi2);
        }
        mi = 31 - ms_one(hi1);
    }
    // Middle level
    mi = (mi<<5) + 31 - ms_one(mid[mi]);
    ma = (ma<<5) + 31 - ls_one(mid[ma]);
    // Lower level
    mi = (mi<<5) + 31 - ms_one(lo[mi]);
    ma = (ma<<5) + 31 - ls_one(lo[ma]);
    return (ma<<16)|mi;
}

// Inserts the element x into the top structure
inline void Top1::insert(unsigned int x) {
    unsigned int elem = x;
    unsigned int apos = elem>>5;                    // position in lo-array
    unsigned int ipos = (2147483648u) >> (31&elem); 
    lo[apos] = lo[apos]|ipos;      
    elem = elem>>5;
    apos = elem>>5;                                 // position in mid-array
    ipos = (2147483648u) >> (31&elem); 
    mid[apos] = mid[apos]|ipos;
    if ( apos > 31) {
        ipos = (2147483648u) >> (apos-32);
        hi2 = hi2|ipos;
    } else {
        ipos = (2147483648u) >> apos;
        hi1 = hi1|ipos;
    }
} 

// Deletes the element x in the top structure
inline void Top1::del(unsigned int x) {
    unsigned int elem = x;
    unsigned int apos = elem>>5;                      // position in lo-array
    unsigned int ipos = (2147483648u) >> (31&elem);
    lo[apos] = (lo[apos]|ipos)^ipos;                  // delete bit ipos
    if(lo[apos]==0){
        elem = elem >>5;
        apos = elem>>5;               		      // position in mid-array
        ipos = (2147483648u) >>(31&elem); 
        mid[apos] = (mid[apos]|ipos)^ipos;            // delete bit ipos
        if ( mid[apos] == 0) {
            if ( apos > 31) {
                hi2 = (hi2|((2147483648u)>>apos))^(((2147483648u)>>apos));
            } else {
                hi1 = (hi1|((2147483648u)>>apos))^(((2147483648u)>>apos));
            }
        }
    }
}

inline void Top1::printDebugList( std::ostream& out) {
    out << "lvl1 hi1:" << hi1 << " hi2:" << hi2 << std::endl;
    out << "lvl2:" << std::endl;
    for ( int i=0; i<64;i++)
        out << mid[i] << ";";
    out << std::endl << "lvl3:" << std::endl;
    for ( int i=0; i<2048;i++)
        out << lo[i] << ";";
    out << std::endl;
}

#endif // MAP32_TOP1_H
