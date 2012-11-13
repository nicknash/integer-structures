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
// Top23.h
// $Revision: $
// $Date: $
//
// The top structure in level two and three. A stratified tree for 8 bits.
// ============================================================================

#ifndef MAP32_TOP23_H
#define MAP32_TOP23_H

#include <iostream>

class Top23 {	
    unsigned char hi;
    unsigned int lo[8];

    unsigned int ms_one(unsigned int);
    unsigned int ls_one(unsigned int);
    unsigned int findLo(unsigned int, unsigned int);
    unsigned char hiPos(unsigned char);
    unsigned char loPos(unsigned char);
public:
    Top23();

    bool isEmpty() { return hi == 0; }

    unsigned int findNext(unsigned char); 
    unsigned int findN(unsigned char);   // better FindNext
    unsigned int maxMin();

    void insert(unsigned int); 
    void del(unsigned int);

    void printDebugList(  std::ostream& out = std::cerr);
};

inline unsigned int Top23::ms_one(unsigned int mry) {
    struct dint{unsigned int lo,hi;};
    union hack{double x; dint y;}; 
    register hack mr;
    mr.x = mry;
    return (mr.y.hi>>20)-1023;
}

inline unsigned int Top23::ls_one(unsigned int x) { // position least sign. one
    union fe_cvt { float f; int   i; };
    register fe_cvt fe;
    fe.f=(float) ( (~(x-1))&x );
    return (fe.i>>23) - 0x7f;
}

// Find position in low level
inline unsigned int Top23::findLo(unsigned int elem, unsigned int apos) {
    unsigned int tmp = 0xffffffffu >>elem;
    tmp = lo[apos]&tmp;
    if(tmp==0)
        return 1000;
    return (apos<<5) + 31 - ms_one(tmp);
}

// Returns the first position in a byte
inline unsigned char Top23::hiPos(unsigned char x) {
    unsigned char z=x|(x>>1);	
    z = z|(z>>2);
    z = z|(z>>4); 
    z = (0x55 & (z>>1)) + (0x55 & z); 
    z = (0x33 & (z>>2)) + (0x33 & z); 
    return 9-(0xf & ((z>>4)+z));
}

/* Gibt die hinterste position in einem byte zurueck */
inline unsigned char Top23::loPos(unsigned char x) {
    unsigned char z=x|(x<<1);	
    z = z|(z<<2);
    z = z|(z<<4); 
    z = (0x55 & (z>>1)) + (0x55 & z); 
    z = (0x33 & (z>>2)) + (0x33 & z); 
    return 0xf & ((z>>4)+z);
}

inline Top23::Top23() {
    lo[0]=0;lo[1]=0;lo[2]=0;lo[3]=0;
    lo[4]=0;lo[5]=0;lo[6]=0;lo[7]=0;
    hi=0;
}

// better FindNext
inline unsigned int Top23::findN(unsigned char next){
    unsigned int loPos = next>>5;
    unsigned int iPos = next&31; 
    unsigned int pos = findLo(iPos,loPos);  // search in low level
    if ( pos<1000) { 
        return pos;
    } else {
        unsigned int tmp = hi & (0xffu >> (loPos+1));
        loPos = ms_one(tmp);
        if(loPos<1000){ 
            loPos = 7 - loPos;
            return  (loPos<<5) + 31 - ms_one(lo[loPos]);
        } else
            return 1000;
    }  
}

inline unsigned int Top23::findNext(unsigned char elem) {
    unsigned int apos = elem>>5;
    unsigned int ipos = 2147483648u>>(31 & elem); 
    unsigned char shift = 31&elem;
    if((lo[apos]|ipos)==lo[apos]){
        return elem;
    }
    if ((lo[apos]<<shift)== 0) { // next element is not in position apos
        unsigned char tmp = hi%(128>>apos); // are there still elements
        if ( tmp==0) { // nothing to find
            return 1000;
        } else { // look for the next element in hi			
            apos = hiPos(tmp)-1; // new search position
            ipos = lo[apos];
        }
    } else {	
        ipos = lo[apos]%(2147483648u>>shift); 
    }
    if ((0xffff&ipos)==ipos) {
        if ((0xff&ipos)==ipos)		
            ipos=24+hiPos(ipos);		
        else
            ipos=16+hiPos(ipos>>8);
    } else {		
        if ((0x00ffffff&ipos)==ipos)
            ipos=8+hiPos(ipos>>16);
        else 
            ipos=hiPos(ipos>>24);				
    }
    return (ipos+(apos << 5) - 1);
}

// find new min and max
inline unsigned int Top23::maxMin(){
    unsigned int mi =7 - ms_one(hi);
    unsigned int ma =7 - ls_one(hi);
    mi = (mi<<5) + 31 - ms_one(lo[mi]);
    ma = (ma<<5) + 31 - ls_one(lo[ma]);
    return (ma<<8)|mi; // reduce pointer access
}

// Insert new elem in bit-array
inline void Top23::insert(unsigned int elem) {
    unsigned char apos = elem>>5; // position in array
    // shift a one into the position (bit%32)+1 counting from the beginning
    unsigned int ipos = 2147483648u >>(31&elem);   
    lo[apos] = lo[apos]|ipos; // store element in the low level
    hi=hi|(128>>apos); // set the high level
}

// Remove element
inline void Top23::del(unsigned int elem) { // room for optimization?
    unsigned char apos = elem>>5; // position in array
    // shift a one into the position (bit%32)+1 counting from the beginning
    unsigned int ipos = 2147483648u >> (31&elem);
    lo[apos] = (lo[apos]|ipos)^ipos;  // remove element in the low level
    if (lo[apos]==0)
        hi = (hi|(128>>apos))^(128>>apos); // update high level
}

inline void Top23::printDebugList( std::ostream& out) {
    out << "Hi-lvl:" << ((int) hi) << " Lo-lvl:";
    for ( int i=0;i<8;i++) {
        out << "("<< lo[i] << ")" ;
    }
    out << std::endl;
}


#endif // MAP32_TOP23_H
