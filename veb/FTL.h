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
// FTL.h
// $Revision: $
// $Date: $
//
// Test driver class: creates the test instances and measures runtime.
// ============================================================================

#ifndef MAP32_FTL_H
#define MAP32_FTL_H

#include <iostream>
#include <list>
#include <set>
#include <queue>
#include "STree_orig.h" // Attn: order dependent, has to come before LEDA!!
//#include <LEDA/_sortseq.h> 
//#include <LEDA/impl/ab_tree.h>
#include <map>

#include "Dlist.h"
#include "LVL1Tree.h"
#include "timer.h"
//#include "eb_tree.h"

//#define TEST_ORIG_STREE
//#define TEST_STREE
//#define TEST_STLMAP
//#define TEST_SORTSEQ
//#define TEST_LEDA_STREE
//#define TEST_LEDA_AB_TREE

// Use dynamic perfect hashing
#ifdef TEST_LEDA_STREE_DP
#define TEST_LEDA_STREE
#endif


//namespace fake_leda {
//    GenPtr nullGenPtr(0);
//}

#define REPEAT_BEGIN for(unsigned int count__=0;count__ < TIMES; count__++) {
#define REPEAT_END }


typedef int Type; 

inline unsigned my_rand(void) {
    static unsigned rand;
    return (rand = rand * 3141592621UL + 663896637UL);
}

typedef std::map<int,int> stl_map;
//typedef _sortseq<int,int,ab_tree> leda_ab_tree_;

#define DECLARE_DICT \
        LVL1Tree test; \
	std::map<int,int> paar; \
	leda_sortseq<int,int> l_sortseq; \
	STree_orig<> orig_stree(0); \
	leda_ab_tree_ l_dict;

// 	leda_stree l_stree(32);

class FTL { 
//    typedef leda_sortseq<int,int>::item sortseq_item_t;
 //   typedef leda_ab_tree_::item         dict_item_t;
///	typedef fake_leda::eb_tree leda_stree;
  
public:
  
    FTL() {}

    // perform multiple locates
    void locTest(unsigned int, unsigned int, unsigned int);
    // fill tree with random numbers
    void insTest(int,int);
    void delTest(int,int);
    void insDelTest(int,int);    
    void memoryTest();
    // insert element and its immediate neighbor
    void consecutiveElements(int,int);          
    void print(double time, unsigned int elements);
};

void FTL::print(double time, unsigned int elements) {
    std::cout << (time/elements) << " ";
}

// A hash table in level 3 is only needed if two elements fall in that 
// subtree. We insert a random element plus its immediate successor, so
// only with probability 1/256 we won't have them in the same level 3 tree.
void FTL::consecutiveElements(int elements, int shift) { 
    std::cout << elements << " ";
    unsigned const int temp = elements/2; // two elements per step
    unsigned int * testArray = new unsigned int[temp];
    for(unsigned int i=0;i<temp;i++)
        testArray[i]=(unsigned int) (my_rand()>>shift);
    timer Timer;
    
#ifdef TEST_STLMAP
    /* Fuelle Map  */ 
    REPEAT_BEGIN
        map<int,int> s_map;
        Timer.start();
        for(unsigned int i=0;i<temp;i++) 
            s_map.insert(pair<int,int>(testArray[i],testArray[i]));
        for(unsigned int i=0;i<temp;i++)
            s_map.insert(pair<int,int>(testArray[i]+1,testArray[i]+1));
        Timer.stop();
    REPEAT_END
    print(Timer.useconds(),elements);
    Timer.reset();
#endif
    /* Map filled */

#ifdef TEST_STREE
    /* Fuelle STree  */    
    REPEAT_BEGIN
        LVL1Tree stree;
        Timer.start();
        for(unsigned int i=0;i<temp;i++) 
            stree.insert(testArray[i],testArray[i]);           
        for(unsigned int i=0;i<temp;i++)
            stree.insert(testArray[i]+1,testArray[i]+1);    
        Timer.stop();
    REPEAT_END
    print(Timer.useconds(),elements);
    Timer.reset();
#endif
    /* STree filled */

#ifdef TEST_SORTSEQ
    /* Fill leda_sortseq */
    REPEAT_BEGIN
        leda_sortseq<int,int> l_sortseq;
        Timer.start();
        for(unsigned int i=0;i<temp;i++)
            l_sortseq.insert(testArray[i],testArray[i]);
        for(unsigned int i=0;i<temp;i++)
            l_sortseq.insert(testArray[i]+1,testArray[i]+1);
        Timer.stop();
    REPEAT_END
    print(Timer.useconds(),elements);
    Timer.reset();
#endif
#ifdef TEST_ORIG_STREE
		
    /* Fill orig_stree */
    REPEAT_BEGIN
        STree_orig<> orig_stree(0);
        Timer.start();
        for(unsigned int i=0;i<temp;i++)
            orig_stree.insert(testArray[i]);
        for(unsigned int i=0;i<temp;i++)
            orig_stree.insert(testArray[i]+1);
        Timer.stop();
    REPEAT_END
    print(Timer.useconds(),elements);
    Timer.reset();
#endif

#ifdef TEST_LEDA_AB_TREE
    /* Fill leda dict */
    REPEAT_BEGIN
        leda_ab_tree_ l_dict;
        Timer.start();
        for(unsigned int i=0;i<temp;i++)
            l_dict.insert(testArray[i],testArray[i]);
        for(unsigned int i=0;i<temp;i++)
            l_dict.insert(testArray[i]+1,testArray[i]+1);
        Timer.stop();
    REPEAT_END
    print(Timer.useconds(),elements);
#endif
		
#ifdef TEST_LEDA_STREE
    REPEAT_BEGIN
        leda_stree l_stree(32);
        Timer.start();
        for(unsigned int i=0;i<temp;i++)
            l_stree.insert((testArray[i]));
        for(unsigned int i=0;i<temp;i++)
            l_stree.insert((testArray[i]+1));
        Timer.stop();
    REPEAT_END
    print(Timer.useconds(),elements);
#endif
    std::cout << std::endl;
    delete [] testArray;
}


// Compare between lookup und lower_bound.
// Input: elements: number of elements in the test tree
//        iter: number of repetitions
//        shift: shifts the random keys to the right to reduce the key size
// Output: comparison data
void FTL::locTest(unsigned int elements,unsigned int iter, unsigned int shift){
    std::cout << elements << " ";
    const int iterationen = iter;
    unsigned const int temp = elements;       
    unsigned int * testArray = new unsigned int[temp];

    //DECLARE_DICT
  
    for(unsigned int i=0;i<temp;i++) { 
        testArray[i]=(unsigned int) (my_rand())>>shift;
    }

    /* Fill trees */
    for(unsigned int i=0;i<temp;i++) {
#ifdef TEST_STREE
        test.insert(testArray[i],testArray[i]);    
#endif
#ifdef TEST_STLMAP
        paar.insert(pair<int,int>(testArray[i],testArray[i]));
#endif
#ifdef TEST_SORTSEQ
        l_sortseq.insert(testArray[i],testArray[i]);
#endif
#ifdef TEST_ORIG_STREE
        orig_stree.insert(testArray[i]);
#endif 
#ifdef TEST_LEDA_AB_TREE
        l_dict.insert(testArray[i],testArray[i]);
#endif
#ifdef TEST_LEDA_STREE
        l_stree.insert(testArray[i]);
#endif
    }

    // locate results
    Dnode*                 map32_item;    
    //map<int,int>::iterator mapIter;
    //sortseq_item_t         leda_sortseq_item;
    //dict_item_t            leda_dict_item;
    int tmp = 0;
    tmp = tmp + 1; // to get rid of warning
	
    // Pre-run before measuring

#ifdef TEST_STLMAP
    for(int e=0;e<iterationen;e++) { // pre-run
        unsigned int zu3 = (unsigned int) (my_rand())>>shift;   
        mapIter = paar.lower_bound(zu3);
    }
#endif
#ifdef TEST_STREE
    for(int e=0;e<iterationen;e++) { // pre-run
        unsigned int zu2 = (unsigned int) (my_rand())>>shift; 
        map32_item = test.locateNode(zu2);  
    }
#endif
#ifdef TEST_SORTSEQ
    for(int e=0;e<iterationen;e++) { // pre-run
        unsigned int zu2 = (unsigned int) (my_rand())>>shift;
        leda_sortseq_item = l_sortseq.locate(zu2);
    }
#endif
#ifdef TEST_ORIG_STREE
    for(int e=0;e<iterationen;e++) {
        unsigned int zu2 = (unsigned int) (my_rand())>>shift;
        tmp = orig_stree.locate(zu2);
    }
#endif
#ifdef TEST_LEDA_AB_TREE
    for(int e=0;e<iterationen;e++) {
        unsigned int zu2 = (unsigned int) (my_rand())>>shift;
        leda_dict_item = l_dict.locate(zu2);
    }
#endif	
#ifdef TEST_LEDA_STREE
    for(int e=0;e<iterationen;e++) {
        unsigned int zu2 = (unsigned int) (my_rand())>>shift;
        tmp = l_stree.succ(zu2);
    }
#endif	

    // Run for the measurements
    timer Timer;
  
#ifdef TEST_STLMAP
	
    Timer.start();
    for(int e=0;e<iterationen;e++) {
        unsigned int zu3 = (unsigned int) (my_rand())>>shift;    
        mapIter = paar.lower_bound(zu3);              
    }
    Timer.stop();
    print(Timer.useconds(),elements);
    Timer.reset();
#endif

    /******************** STree *************/
#ifdef TEST_STREE
    Timer.start();
    for(int e=0;e<iterationen;e++) { 
        unsigned int zu2 = (unsigned int) (my_rand())>>shift; 
        map32_item = test.locateNode(zu2);  
    }
    Timer.stop();
    print(Timer.useconds(),elements);
    Timer.reset();
#endif
    /******************************************/

    /* LEDA sortseq */
#ifdef TEST_SORTSEQ
    Timer.start();
    for(int e=0;e<iterationen;e++) {
        unsigned int zu2 = (unsigned int) (my_rand())>>shift;
        leda_sortseq_item = l_sortseq.locate(zu2);
    }
    Timer.stop();
    print(Timer.useconds(),elements);
#endif

    /* original Stree */
#ifdef TEST_ORIG_STREE
    Timer.start();
    for(int e=0;e<iterationen;e++) {
        unsigned int zu2 = (unsigned int) (my_rand())>>shift;
        tmp = orig_stree.locate(zu2);
    } 
    Timer.stop();
    print(Timer.useconds(),elements);
    Timer.reset();
#endif

#ifdef TEST_LEDA_AB_TREE
    Timer.start();
    for(int e=0;e<iterationen;e++) {
        unsigned int zu2 = (unsigned int) (my_rand())>>shift;
        leda_dict_item = l_dict.locate(zu2);
    } 
    Timer.stop();
    print(Timer.useconds(),elements);
    Timer.reset();
#endif
	
#ifdef TEST_LEDA_STREE
    Timer.start();
    for(int e=0;e<iterationen;e++) {
        unsigned int zu2 = (unsigned int) (my_rand())>>shift;
        tmp = l_stree.succ(zu2);
    } 
    Timer.stop();
    print(Timer.useconds(),elements);
#endif

    std::cout << std::endl;
    delete [] testArray;
}


// Compare insertion speed
void FTL::insTest(int elements, int shift) { 
    std::cout << elements << " ";  
    unsigned const int temp = elements;        
    unsigned int *testArray = new unsigned int[temp];
    for(unsigned int i=0;i<temp;i++) { 
        testArray[i]=(unsigned int) (my_rand()>>shift);
    }
 
    // Fill map
    timer Timer;
	
#ifdef TEST_STLMAP
    REPEAT_BEGIN
        map<int,int> s_map;
        Timer.start();
        for(unsigned int i=0;i<temp;i++)
            s_map.insert(pair<int,int>(testArray[i],testArray[i]));
        Timer.stop();
    REPEAT_END
    print(Timer.useconds(),elements);
    Timer.reset();
#endif

    // Fill LVL1Tree
#ifdef TEST_STREE
    REPEAT_BEGIN
        LVL1Tree stree;
        Timer.start();
        for(unsigned int i=0;i<temp;i++)
            stree.insert(testArray[i],testArray[i]);    
        Timer.stop();
    REPEAT_END 
    print(Timer.useconds(),elements);
    Timer.reset();
#endif
  
    // LEDA sort seq
#ifdef TEST_SORTSEQ
    REPEAT_BEGIN
        leda_sortseq<int,int> l_sortseq;
        Timer.start();
        for(unsigned int i=0;i<temp;i++)
            l_sortseq.insert(testArray[i],testArray[i]);
        Timer.stop();
    REPEAT_END
    print(Timer.useconds(),elements);
    Timer.reset();
#endif

    /* original stree */
#ifdef TEST_ORIG_STREE
    REPEAT_BEGIN
        STree_orig<> orig_stree(0);
        Timer.start();
        for(unsigned int i=0;i<temp;i++)
            orig_stree.insert(testArray[i]);
        Timer.stop();
    REPEAT_END
    print(Timer.useconds(),elements);
    Timer.reset();
#endif

#ifdef TEST_LEDA_AB_TREE
    REPEAT_BEGIN
        leda_ab_tree_ l_dict;
        Timer.start();
        for(unsigned int i=0;i<temp;i++)
            l_dict.insert(testArray[i],testArray[i]);
        Timer.stop();
    REPEAT_END
    print(Timer.useconds(),elements);
    Timer.reset();
#endif
		
#ifdef TEST_LEDA_STREE
    REPEAT_BEGIN
        leda_stree l_stree(32);
        Timer.start();
        for(unsigned int i=0;i<temp;i++)
            l_stree.insert(testArray[i]);
        Timer.stop();
    REPEAT_END
    print(Timer.useconds(),elements);
    Timer.reset();
#endif
		
    std::cout << std::endl;
    delete [] testArray;
}


// Compare removal speed
void FTL::delTest(int elements,int shift) {   
    std::cout << elements << " ";
    unsigned const int temp = elements;       
    unsigned int *testArray = new unsigned int[temp];
    for(unsigned int i=0;i<temp;i++)
        testArray[i]=(unsigned int) (my_rand() >> shift);
    timer Timer;

#ifdef TEST_STLMAP
    // removal in map
	
    REPEAT_BEGIN
        map<int,int> s_map;
        for(unsigned int i=0;i<temp;i++)
            s_map.insert(pair<int,int>(testArray[i],testArray[i]));
        Timer.start();
        for(unsigned int i=0;i<temp;i++)     
            s_map.erase(testArray[i]);     
        Timer.stop();
    REPEAT_END
    print(Timer.useconds(),elements);
    Timer.reset();
#endif
  
    /* STree loeschen */
#ifdef TEST_STREE
    REPEAT_BEGIN
        LVL1Tree stree;
        for(unsigned int i=0;i<temp;i++)
            stree.insert(testArray[i],testArray[i]);
        Timer.start();
        for(unsigned int i=0;i<temp;i++)
            stree.del(testArray[i]);
        Timer.stop();
    REPEAT_END
    print(Timer.useconds(),elements);
    Timer.reset();
#endif

    /* LEDA sortseq */
#ifdef TEST_SORTSEQ
    REPEAT_BEGIN
        leda_sortseq<int,int> l_sortseq;
        for(unsigned int i=0;i<temp;i++)
            l_sortseq.insert(testArray[i],testArray[i]);
        Timer.start();
        for(unsigned int i=0;i<temp;i++)
            l_sortseq.del(testArray[i]);
        Timer.stop();
    REPEAT_END
    print(Timer.useconds(),elements);
    Timer.reset();
#endif

    /* orig stree */
#ifdef TEST_ORIG_STREE
    REPEAT_BEGIN
        STree_orig<> orig_stree(0);
        for(unsigned int i=0;i<temp;i++)
            orig_stree.insert(testArray[i]);
        Timer.start();
        for(unsigned int i=0;i<temp;i++)
            orig_stree.del(testArray[i]);
        Timer.stop();
    REPEAT_END
    print(Timer.useconds(),elements);
    Timer.reset();
#endif

#ifdef TEST_LEDA_AB_TREE
    REPEAT_BEGIN
        leda_ab_tree_ l_dict;
        for(unsigned int i=0;i<temp;i++)
            l_dict.insert(testArray[i],testArray[i]);
        Timer.start();
        for(unsigned int i=0;i<temp;i++)
            l_dict.del(testArray[i]);
        Timer.stop();
    REPEAT_END
    print(Timer.useconds(),elements);
    Timer.reset();
#endif

#ifdef TEST_LEDA_STREE
    REPEAT_BEGIN
        leda_stree l_stree(32);
        for(unsigned int i=0;i<temp;i++)
            l_stree.insert(testArray[i]);
        Timer.start();
        for(unsigned int i=0;i<temp;i++)
            l_stree.del(testArray[i]);
        Timer.stop();
    REPEAT_END
    print(Timer.useconds(),elements);
    Timer.reset();
#endif

    std::cout << std::endl;
    delete [] testArray;
}

// Switch between removal and insertion of random keys
void FTL::insDelTest(int elements, int shift) {  
    std::cout << elements << " ";
    unsigned const int temp = elements;
    unsigned int * testArray = new unsigned int[temp];

    //DECLARE_DICT

    timer Timer;

#ifdef TEST_STLMAP
    /* Test MAP */
    for(unsigned int i=0;i<temp;i++) {
        const unsigned int tmp = my_rand()>>shift;
        testArray[i]=tmp;
        paar.insert(pair<int,int>(tmp,tmp));
    }
    Timer.start();
    REPEAT_BEGIN
        for(unsigned int i=0;i<temp;i++) {
            const unsigned int tmp = my_rand()>>shift;
            paar.erase(testArray[i]);
            paar.insert(pair<int,int>(tmp,tmp));
            testArray[i] = tmp;
        }
    REPEAT_END
    Timer.stop();
    print(Timer.useconds(),elements);
    Timer.reset();
#endif

    /* STree  */
#ifdef TEST_STREE
    for(unsigned int i=0;i<temp;i++) {
        const unsigned int tmp = my_rand()>>shift;
        testArray[i]=tmp;
        test.insert(tmp,tmp);
    }
    Timer.start();
    REPEAT_BEGIN
        for(unsigned int i=0;i<temp;i++) {
            const unsigned int tmp = my_rand()>>shift;
            test.del(testArray[i]);
            test.insert(tmp,tmp);
            testArray[i] = tmp;
        }
    REPEAT_END
    Timer.stop();
    print(Timer.useconds(),elements);
    Timer.reset();
    /* STree filled */
#endif

    /* LEDA sortseq */
#ifdef TEST_SORTSEQ
    for(unsigned int i=0;i<temp;i++) {
        const unsigned int tmp = my_rand()>>shift;
        testArray[i]=tmp;
        l_sortseq.insert(tmp,tmp);
    }
    Timer.start();
    REPEAT_BEGIN
        for(unsigned int i=0;i<temp;i++) {
            const unsigned int tmp = my_rand()>>shift;
            l_sortseq.del(testArray[i]);
            l_sortseq.insert(tmp,tmp);
            testArray[i] = tmp;
        }
    REPEAT_END
    Timer.stop();
    print(Timer.useconds(),elements);
    Timer.reset();
#endif

    /* Orig stree */
#ifdef TEST_ORIG_STREE
    for(unsigned int i=0;i<temp;i++) {
        const unsigned int tmp = my_rand()>>shift;
        testArray[i]=tmp;
        orig_stree.insert(tmp);
    }
    Timer.start();
    REPEAT_BEGIN
        for(unsigned int i=0;i<temp;i++) {
            const unsigned int tmp = my_rand()>>shift;
            orig_stree.del(testArray[i]);
            orig_stree.insert(tmp);
            testArray[i] = tmp;
        }
    REPEAT_END
    Timer.stop();
    print(Timer.useconds(),elements);
    Timer.reset();
#endif

#ifdef TEST_LEDA_AB_TREE
    for(unsigned int i=0;i<temp;i++) {
        const unsigned int tmp = my_rand()>>shift;
        testArray[i]=tmp;
        l_dict.insert(tmp,tmp);
    }
    Timer.start();
    REPEAT_BEGIN
        for(unsigned int i=0;i<temp;i++) {
            const unsigned int tmp = my_rand()>>shift;
            l_dict.del(testArray[i]);
            l_dict.insert(tmp,tmp);
            testArray[i] = tmp;
        }
    REPEAT_END
    Timer.stop();
    print(Timer.useconds(),elements);
    Timer.reset();
#endif

	
#ifdef TEST_LEDA_STREE
    for(unsigned int i=0;i<temp;i++) {
        const unsigned int tmp = my_rand()>>shift;
        testArray[i]=tmp;
        l_stree.insert(tmp);
    }
    Timer.start();
    REPEAT_BEGIN
        for(unsigned int i=0;i<temp;i++) {
            const unsigned int tmp = my_rand()>>shift;
            l_stree.del(testArray[i]);
            l_stree.insert(tmp);
            testArray[i] = tmp;
        }
    REPEAT_END
    Timer.stop();
    print(Timer.useconds(),elements);
#endif

    std::cout << std::endl;
    delete [] testArray;

}

#endif // MAP32_FTL_H
