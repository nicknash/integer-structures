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
// LVL1Tree.h
// $Revision: $
// $Date: $
//
// The top level data structure, i.e., the public interface of our sorted
// list data structure.
// ============================================================================

#ifndef MAP32_LVL1TREE_H
#define MAP32_LVL1TREE_H

#include <cstring>
#include <iostream>

#include "Dlist.h"
#include "LVL2Tree.h"
#include "Top1.h"
#include "allocator.h"


typedef int Type;


class LVL1Tree {
public:     
    unsigned int minKey;        
    unsigned int maxKey;
 
    LVL2Handle max; 			      
    LVL2Handle min;						
    Top1 top;
    LVL2Handle bot[65536];	
    Dlist D;


    LVL1Tree();

    void insert(Type,unsigned int);
    void del(unsigned int);
    Dnode* locateNode(unsigned int);

    void printDebugTable( std::ostream& out = std::cerr);
    void printDebugList(  std::ostream& out = std::cerr);
};

inline LVL1Tree::LVL1Tree() {           
    D.insertfirst(0); 
    D.insertlast(0);             
    minKey = 0xffffffff;                
    maxKey = 0xffffffff;
    // Handle has a constructor now
    // max = 0;  
    // min = 0; 
    // faster than:   for(int i=0;i<65536;i++) bot[i]=0;        
    // std::memset( bot, 0, 65536 * sizeof(LVL2Handle));
}

// Insert in level 1
/*inline*/ void LVL1Tree::insert(Type listItem,unsigned int key) {
    unsigned int aPos = key >> 16;
    if ( bot[aPos].isNull()) { // new LVL2Tree
        if(!top.isEmpty()) { // max and min are set
            unsigned int next = top.findN(aPos);                       
            // Notice: bot[aPos]=0, i.e., aPos!=maxKey
            if ( aPos<maxKey) { // successor found with findN
                if ( aPos<minKey) {
                    bot[aPos] = LVL2Handle(listItem,key,D,min.minx());
                    min = bot[aPos];
                    minKey = aPos;	
                } else {
                    if ( next < 0xffffffff)
                        bot[aPos] = LVL2Handle(listItem,key,D,
                                               bot[next].minx());
                    else
                        bot[aPos] = LVL2Handle(listItem,key,D,D.lastnode());
                }
            } else { // no successor found with findN
                Dnode *tmp = D.lastnode();      
                bot[aPos] = LVL2Handle(listItem,key,D,tmp);
                max = bot[aPos]; // new max
                maxKey = aPos;
            }    
        } else { // empty tree
            max = LVL2Handle(listItem,key,D,D.lastnode());             
            min = max; 
            bot[aPos]= max;    
            minKey = key>>16;                
            maxKey = minKey;  
        }
    } else {   // subtree exists already
        bot[aPos].insert(listItem,key,D);  
    }
    assert(aPos<=maxKey);
    assert(aPos>=minKey);
    top.insert(aPos);  // update top structure  
}

// Delete in level 1
inline void LVL1Tree::del(unsigned int key) {
    unsigned int aPos = key >> 16;
    if ( bot[aPos].isTree()) { // nothing to do if there is no subtree
        bot[aPos].del(key,D);    
        if(bot[aPos].isNull()) { // has the subtree been removed?
            top.del(aPos);         
        }
        if(!top.isEmpty()) {
            unsigned int tmp = top.maxMin();  
            minKey = tmp & 0x0000ffff;
            maxKey = tmp >> 16;
            min = bot[minKey];
            max = bot[maxKey];
        } else { // empty tree
            minKey = 0xffffffff;   
            maxKey = 0xffffffff;
            min = LVL2Handle();
            max = LVL2Handle();
        }
    }
}
#include <iostream>
using namespace std;

// search in level 1
/*inline*/ Dnode* LVL1Tree::locateNode(unsigned int key) {
    unsigned int aPos = key >> 16;
    if((aPos<maxKey)&(maxKey!=0xffffffff)){     // sure find
        //cout << "In here1\n";
        unsigned int next = top.findN(aPos);
        if(next==aPos)                            // Next > aPos ?
            return bot[next].locateNode(key);   // recursion
        else
            return bot[next].minx();                 //min->min;
    } else {
        if((aPos==maxKey)&(maxKey!=0xffffffff)){ // difficult case
        //cout << "In here2: aPos = " << aPos << ", maxKey = " << maxKey << endl;

            // reconstruct largest key
            unsigned int globalMax = max.maxKeyx();
            //cout << "globalMax = " << globalMax << endl;
            if(key<=globalMax){
                unsigned int next = top.findN(aPos);
                return bot[next].locateNode(key);
            }	
        }
    }
    //cout << "Returning NULL!\n";

    return 0;
}

// debug printout of the table
void LVL1Tree::printDebugTable( std::ostream& out) {
    for ( int i=0;i<65536;i++){
        LVL2Handle tm = bot[i]; 
        if ( tm.isTree()){
            out << "LVL1-Entry at:" << i << std::endl;
            tm.printDebugHash( out);
        }
    }
    out << std::endl;
}

// debug printout of the Dlist
void LVL1Tree::printDebugList( std::ostream& out) {
    out << std::endl;
    Dnode *tmp=D.firstnode();
    while ( tmp!=D.lastnode()){
        tmp = tmp->getright();
        out << ";" << (unsigned int)tmp->getinfo() << std::endl;
    }    
    out << std::endl;
}

#endif // MAP32_LVL1TREE_H

