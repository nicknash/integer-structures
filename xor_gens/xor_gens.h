/*
 * These are prototypes for Richard Brent's 
 * XOR random number generator implementations.
 *
 * See xor_gens.c for more information.
 *
 * Nicholas Nash, Dept. of Computer Science, 
 * Trinity College Dublin.
 * 
 * nashn@cs.tcd.ie
 *
 * May 2007
 */
#ifndef __XOR_GENS_H

#define __XOR_GENS_H

//#ifdef __cplusplus
//extern "C" {
//#endif

unsigned long xor4096s(unsigned long seed = 0);
float xor4096f(unsigned long seed = 0);

unsigned long long xor4096l(unsigned long long seed = 0);
double xor4096d(unsigned long long seed = 0);

//#ifdef __cplusplus
//}
//#endif


#endif
