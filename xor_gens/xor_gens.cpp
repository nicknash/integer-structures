/* xorgens.c - Some long-period random number generators
	       generalising Marsaglia's Xorshift RNGs
 
==========================================================================
|                                                                        |
|  Copyright (C) 2004 R. P. Brent.                                       |
|                                                                        | 
|  This program is free software; you can redistribute it and/or         |
|  modify it under the terms of the GNU General Public License,          |
|  version 2, June 1991, as published by the Free Software Foundation.   |
|  For details see http://www.gnu.org/copyleft/gpl.html .                |    
|                                                                        |
==========================================================================
 
Author:         Richard P. Brent (random@rpbrent.co.uk)

Version:	2.01

Date:           20040823

Contents:       xor4096s        "unsigned long" 32-bit integer RNG,
                                period (2^4096 - 1)*2^32.
                                
                xor4096f        "float" 32-bit real RNG (based on xor4096s).
                                
                xor4096l        "unsigned long long" 64-bit integer RNG,
                                period (2^4096 - 1)*2^64.

                xor4096d        "double" 64-bit real RNG (based on xor4096l).

Comments:       Some fast RNGs with very long periods, using minimal
		storage.  They use generalisations of (but not exactly the
		same as) the xorshift RNGs described by Marsaglia[4].  The
		output is combined with a "Weil generator" although
		this extra precaution is probably unnecessary.
                
                The generators are fast, although quality has not been
                sacrificed for speed.  To speed them up further, the code
                could easily be modified to return an array of random numbers
                instead of a single number, thus reducing function call
                overheads. For the sake of simplicity I have not done this.
                
                The generators are believed to be of high quality, and have
                passed Marsaglia's DIEHARD tests. Of course, users should
                apply their own tests.  Please inform the author if you
                discover any genuine problems.

Calling sequence:

                All four generators are functions with one argument.  

                The first call should be of the form
                
                        xor4096x(seed);
                        
                where "x" is one of "s","l","f","d", and
                "seed" is a nonzero seed of the appropriate type
                (unsigned long for xor4096s and ran4096f,
                 unsigned long long for ran4096l and ran4096d).
                 
                Subsequent calls should (usually) be of the form
                
                        result = ran4096x(0);
                        
                where "result" is of the appropriate type
                (long, long long, float or double).
                
                If the first call with nonzero argument is omitted,
                a default initialisation (seed = ~0) is used.
                
                The generator can be reinitialised as often as necessary by
                calling with nonzero argument.
                        
Other generators:                        

		Analogous generators xor2048x, xor1024x, ... are available
		from the author.  They use slightly less space and are
		slightly faster to initialise, but otherwise have no
		particular advantages over the generators included here.
		                        
References:	
		1. Richard P. Brent, "Note on Marsaglia's xorshift random 
		   number generators", J. Statistical Software 11, 5 (2004). 
		   http://www.jstatsoft.org .   Also available from http://
		   www.comlab.ox.ac.uk/oucl/work/richard.brent/pub/pub218.html
		
		2. Richard P. Brent, "Random number generation and
		   simulation on vector and parallel computers",
		   LNCS 1470, Springer-Verlag, Berlin, 1998, 1-20.
		   Available from http://www.comlab.ox.ac.uk/oucl/
		   work/richard.brent/pub/pub185.html

		3. Pierre L'Ecuyer, "Random Number Generation", in Handbook
		   of Computational Statistics, Chapter 2, J. E. Gentle, 
		   W.  Haerdle, and Y. Mori, eds., Springer-Verlag, 2004,
		   35-70.  Available from
		   http://www.iro.umontreal.ca/~lecuyer/papers.html
		
                4. George Marsaglia, "Xorshift RNGs", Journal of Statistical
                   Software 8, 14 (2003).  http://www.jstatsoft.org     
        
*/      
#include "xor_gens.h"

unsigned long xor4096s(unsigned long seed)

  {
  /* 32-bit random number generator with period at least 2^4096-1.
     
     Is is assumed that "unsigned long" is a 32-bit integer.

     The method is a generalisation of Marsaglia's xorgen generators,
     see G. Marsaglia, "Xorshift RNGs", JSS 8, 14 (2003), Sec. 3.1
     (available from http://www.jstatsoft.org/ ).
 
     The primary recurrence is x_k = x_{k-r}A + x_{k-s}B, where

     	   A = (I + L^a)(I + R^b), B = (I + L^c)(I + R^d)

     in the notation of Marsaglia [JSS 8,14 (2003)].
     
     We choose r > 1, 0 < s < r, n = 32*r, and (a, b, c, d) such that the 
     n times n matrix T defining the recurrence has minimal polynomial P
     which is of degree n and primitive over GF(2).  Thus the period is
     2^n-1 and all possible n-bit sequences (except all zeros) occur in the
     output over a full period.  Storage is (r + constant) 32-bit words.
     
     Our generalisation is:
 
     (1) The block matrix has r times r blocks, where r is a parameter.  
         To test primitivity we need to be able to factor 2^n-1.  Marsaglia
         considered some cases with small r.  In our implementation r is a
         power of 2, but this restriction is easily removed. We use a
         circular array, which makes the number of memory references
         independent of r.
         
     (2) Marsaglia's block (I + R^c) is replaced by (I + L^c)(I + R^d).
         
     (3) The block mentioned in (2) is in position (r+1-s,r)
         instead of position (r,r) of the block matrix. (Marsaglia
         considers the case s = 1.)    
         
     By introducing the additional parameters d and s we increase the
     set of candidate recurrences to compensate for the fact that
     primitive recurrences get harder to find as n increases.
         
     The output of the primary recurrence is combined with a Weil generator
     to avoid problems with the binary rank test and correlations related to
     the sparsity of T. This increases the period by a factor of 2^32.

     Should be called once with nonzero seed, thereafter with zero seed.
         
     The generator implemented here has parameters
     (wlen 32, r 128, s 95, a 17, b 12, c 13, d 15) with Weight(P) 251.
     Other parameters can be used by changing the definitions below.

     R. P. Brent, 20040720, last revised 20040802.
     
     */

#define wlen 32
#define r 128
#define s 95
#define a 17
#define b 12
#define c 13
#define d 15
 
  static unsigned long w, x[r], weil = 0x61c88647;
  unsigned long t, v;
  static int i = -1 ;		   /* i < 0 indicates first call */
  int k;
  if ((i < 0) || (seed != 0)) {	   /* Initialisation necessary */
  
    v = (seed!=0) ? seed: ~seed;   /* v must be nonzero */

    for (k = wlen; k > 0; k--) 	   /* Avoid correlations for close seeds */
      v^=(v^=(v^=v<<13)>>17)<<5;   /* This recurrence has period 2^32-1 */

    for (w = v, k = 0; k < r; k++) /* Initialise circular array */

      x[k] = (v^=(v^=(v^=v<<13)>>17)<<5) + (w+=weil);
      
    for (i = r-1, k = 4*r; k > 0; k--) 
      {  			   /* Discard first 4*r results (Gimeno) */
      t  = x[i = (i+1)&(r-1)];
      v  = x[(i+(r-s))&(r-1)];			 
      t ^= (t ^= t<<a) >> b;
      v ^= v << c;		
      x[i] = (v ^= t^(v>>d));
      }
    }
    
  /* Apart from initialisation (above), this is the generator */
  
  t  = x[i = (i+1)&(r-1)];         /* Increment i mod r (r is a power of 2) */
  v  = x[(i+(r-s))&(r-1)];         /* Index is (i - s) mod r */
  t ^= (t ^= t<<a) >> b;	   /* (I + L^a)(I + R^b) */
  v ^= v << c;			   /*  I + L^c */
  x[i] = (v ^= t^(v>>d));	   /* Update circular array */
  return (v + (w+=weil));	   /* Return combination with Weil generator */

#undef wlen
#undef r
#undef s
#undef a
#undef b
#undef c
#undef d
  }

float xor4096f(unsigned long seed)

  {
  /* 32-bit real random number generator with period at least 2^4096-1.
     
     Is is assumed that "unsigned long" is a 32-bit integer
     and "float" is a 32-bit IEEE Standard floating-point number
     with 23 explicit + 1 implicit bits in the fraction.
     
     The algorithm is the same as for the 32-bit integer RNG xor4096s except
     that the high 24-bits of the result (if nonzero) are converted to a
     floating-point number in (0.0, 1.0).
     
     Should be called once with nonzero seed, thereafter with zero seed.
     
     The result should be uniformly distributed in (0.0, 1.0) almost to the
     resolution of the floating-point system. The result is never exactly
     0.0 or 1.0.

     R. P. Brent, 20040802.
     
     */

#define wlen 32
#define r 128
#define s 95
#define a 17
#define b 12
#define c 13
#define d 15

  static unsigned long w, x[r], weil = 0x61c88647;
  static float t24 = 1.0/16777216.0;		/* 0.5**24 */
  unsigned long t, v;
  static int i = -1 ;		   /* i < 0 indicates first call */
  int k;
  if ((i < 0) || (seed != 0)) {	   /* Initialisation necessary */
  
    v = (seed!=0) ? seed: ~seed;   /* v must be nonzero */

    for (k = wlen; k > 0; k--) 	   /* Avoid correlations for close seeds */
      v^=(v^=(v^=v<<13)>>17)<<5;   /* This recurrence has period 2^32-1 */

    for (w = v, k = 0; k < r; k++) /* Initialise circular array */

      x[k] = (v^=(v^=(v^=v<<13)>>17)<<5) + (w+=weil);
      
    for (i = r-1, k = 4*r; k > 0; k--) 
      {  			   /* Discard first 4*r results (Gimeno) */
      t  = x[i = (i+1)&(r-1)];
      v  = x[(i+(r-s))&(r-1)];			 
      t ^= (t ^= t<<a) >> b;
      v ^= v << c;		
      x[i] = (v ^= t^(v>>d));
      }
    }
    
  /* Apart from initialisation (above), this is the generator */
  
  v = 0;			   /* Usually execute while loop once */
  while (v == (unsigned long)0) {  /* Loop until get nonzero 24-bit integer */
    t  = x[i = (i+1)&(r-1)];       /* Increment i mod r (r is a power of 2) */
    v  = x[(i+(r-s))&(r-1)];       /* Index is (i - s) mod r */
    t ^= (t ^= t<<a) >> b;	   /* (I + L^a)(I + R^b) */
    v ^= v << c;		   /*  I + L^c */
    x[i] = (v ^= t^(v>>d));	   /* Update circular array */
    v += (w+=weil);		   /* 32-bit unsigned integer */
    v >>= 8;			   /* 24-bit integer, discard if zero */
    }
  return t24*(float)v;		   /* Scale to (0.0, 1.0) */

#undef wlen
#undef r
#undef s
#undef a
#undef b
#undef c
#undef d
  }

unsigned long long xor4096l(unsigned long long seed)

  {
  /* 64-bit random number generator with period at least 2^4096-1.
     
     Is is assumed that "unsigned long long" is a 64-bit integer.

     The method is a generalisation of Marsaglia's xorgen generators,
     see G. Marsaglia, "Xorshift RNGs", JSS 8, 14 (2003), Sec. 3.1
     (available from http://www.jstatsoft.org/ ).
 
     The primary recurrence is x_k = x_{k-r}A + x_{k-s}B, where

     	   A = (I + L^a)(I + R^b), B = (I + L^c)(I + R^d)

     in the notation of Marsaglia [JSS 8,14 (2003)].
     
     We choose r > 1, 0 < s < r, n = 64*r, and (a, b, c, d) such that the 
     n times n matrix T defining the recurrence has minimal polynomial P
     which is of degree n and primitive over GF(2).  Thus the period is
     2^n-1 and all possible n-bit sequences (except all zeros) occur in the
     output over a full period.  Storage is (r + constant) 64-bit words.
     
     Our generalisation is:
 
     (1) The block matrix has r times r blocks, where r is a parameter.  
         To test primitivity we need to be able to factor 2^n-1.  Marsaglia
         considered some cases with small r.  In our implementation r is a
         power of 2, but this restriction is easily removed. We use a
         circular array, which makes the number of memory references
         independent of r.
         
     (2) Marsaglia's block (I + R^c) is replaced by (I + L^c)(I + R^d).
         
     (3) The block mentioned in (2) is in position (r+1-s,r)
         instead of position (r,r) of the block matrix. (Marsaglia
         considers the case s = 1.)    
         
     By introducing the additional parameters d and s we increase the
     set of candidate recurrences to compensate for the fact that
     primitive recurrences get harder to find as n increases.
         
     The output of the primary recurrence is combined with a Weil generator
     to avoid problems with the binary rank test and correlations related to
     the sparsity of T. This increases the period by a factor of 2^64.
    
     Should be called once with nonzero seed, thereafter with zero seed.

     The generator implemented here has parameters
     (wlen 64, r 64, s 53, a 33, b 26, c 27, d 29) with Weight(P) 961.
     Other parameters can be used by changing the definitions below.

     R. P. Brent, 20040721, last revised 20040802.
     
     */

#define wlen 64
#define r 64
#define s 53
#define a 33
#define b 26
#define c 27
#define d 29
 
  static unsigned long long w, x[r],
    weil = ((long long)0x61c88646<<32) +  (long long)0x80b583eb; 
  unsigned long long t, v;
  static int i = -1 ;		   /* i < 0 indicates first call */
  int k;
  if ((i < 0) || (seed != 0)) {	   /* Initialisation necessary */
  
    v = (seed!=0) ? seed: ~seed;   /* v must be nonzero */

    for (k = wlen; k > 0; k--) 	   /* Avoid correlations for close seeds */
      v^=(v^=v<<7)>>9;		   /* This recurrence has period 2^64-1 */

    for (w = v, k = 0; k < r; k++) /* Initialise circular array */

      x[k] = (v^=(v^=v<<7)>>9) + (w+=weil);
      
    for (i = r-1, k = 4*r; k > 0; k--)  
      { 			   /* Discard first 4*r results (Gimeno) */
      t  = x[i = (i+1)&(r-1)];		
      v  = x[(i+(r-s))&(r-1)];			 
      t ^= (t ^= t<<a) >> b;
      v ^= v << c;		
      x[i] = (v ^= t^(v>>d));
      }
    }
    
  /* Apart from initialisation (above), this is the generator */
  
  t  = x[i = (i+1)&(r-1)];	   /* Increment i mod r (r is a power of 2) */
  v  = x[(i+(r-s))&(r-1)];	   /* Index is (i - s) mod r */
  t ^= (t ^= t<<a) >> b;	   /* (I + L^a)(I + R^b) */
  v ^= v << c;			   /*  I + L^c */
  x[i] = (v ^= t^(v>>d));	   /* Update circular array */
  return (v + (w+=weil));	   /* Return combination with Weil generator */

#undef wlen
#undef r
#undef s
#undef a
#undef b
#undef c
#undef d
  }

double xor4096d(unsigned long long seed)

  {
  /* 64-bit real random number generator with period at least 2^4096-1.
     
     Is is assumed that "unsigned long long" is a 64-bit integer
     and "double" is an IEEE standard 64-bit floating-point number
     with 52 explicit + 1 implicit bits in the fraction.
     
     The method used is as for the 64-bit integer RNG xor4096l,
     then the high 53 bits (if nonzero) are scaled to (0.0, 1.0). 

     Should be called once with nonzero seed, thereafter with zero seed.

     The result should uniformly distributed in (0.0, 1.0) to the resolution
     of the floating-point system.  The result is never exactly 0.0 or 1.0.

     R. P. Brent, 20040802.
     
     */

#define wlen 64
#define r 64
#define s 53
#define a 33
#define b 26
#define c 27
#define d 29
 
  static unsigned long long w, x[r],
    weil = ((long long)0x61c88646<<32) +  (long long)0x80b583eb; 
  unsigned long long t, v;
  static double
    t53 = (double)1/(double)((long long)1<<53);  /* 0.5**53 */  
  static int i = -1 ;		   /* i < 0 indicates first call */
  int k;
  if ((i < 0) || (seed != 0)) {	   /* Initialisation necessary */

    v = (seed!=0) ? seed: ~seed;   /* v must be nonzero */

    for (k = wlen; k > 0; k--) 	   /* Avoid correlations for close seeds */
      v^=(v^=v<<7)>>9;		   /* This recurrence has period 2^64-1 */

    for (w = v, k = 0; k < r; k++) /* Initialise circular array */

      x[k] = (v^=(v^=v<<7)>>9) + (w+=weil);
      
    for (i = r-1, k = 4*r; k > 0; k--)  
      { 			   /* Discard first 4*r results (Gimeno) */
      t  = x[i = (i+1)&(r-1)];		
      v  = x[(i+(r-s))&(r-1)];			 
      t ^= (t ^= t<<a) >> b;
      v ^= v << c;		
      x[i] = (v ^= t^(v>>d));
      }
    }
    
  /* Apart from initialisation (above), this is the generator */

  v  = 0;			     /* Usually execute while loop once */
  while (v == (unsigned long long)0) 
    {				     /* Loop until result nonzero */
    t  = x[i = (i+1)&(r-1)];	     /* Increment i mod r (r = power of 2) */
    v  = x[(i+(r-s))&(r-1)];	     /* Index is (i - s) mod r */
    t ^= (t ^= t<<a) >> b;	     /* (I + L^a)(I + R^b) */
    v ^= v << c;		     /*  I + L^c */
    x[i] = (v ^= t^(v>>d));	     /* Update circular array */
    v += (w+=weil);		     /* 64-bit unsigned integer */
    v >>= 11;			     /* 53-bit integer (possibly zero) */
    }
  return t53*(double)v; 	     /* Scale to (0.0, 1.0) */

#undef wlen
#undef r
#undef s
#undef a
#undef b
#undef c
#undef d
  }

class SeedXORGens
{
    static const unsigned long seed_ul = 0xDEADBEEF; 
public:
    SeedXORGens()
    {
        xor4096s(seed_ul);
        xor4096f(seed_ul);
        xor4096l(seed_ul * static_cast<unsigned long long>(seed_ul));
        xor4096d(seed_ul * static_cast<unsigned long long>(seed_ul));
    }
} SXG;


