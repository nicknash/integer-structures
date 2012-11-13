/*******************************************************************************
+
+  LEDA-R  3.2.3
+
+  memory.h
+
+  Copyright (c) 1995  by  Max-Planck-Institut fuer Informatik
+  Im Stadtwald, 66123 Saarbruecken, Germany     
+  All rights reserved.
+ 
*******************************************************************************/

#ifndef LEDA_MEMORY_H
#define LEDA_MEMORY_H


/*{\Mtext
\section{Memory Management} \label{Memory Management}

LEDA offers an efficient memory management system that is used internally 
for all node, edge and item types. This system can easily be customized for 
user defined classes by the ``LEDA\_MEMORY" macro. You simply have
to add the macro call ``LEDA\_MEMORY($T$)" to the declaration of a class
$T$. This redefines new and delete operators for type $T$, such that
they allocate and deallocate memory using LEDA's internal memory manager. 

{\bf Attention:}
There is a restriction on the size of the type $T$, however. Macro
LEDA\_MEMORY may only be applied to types $T$ with {\bf sizeof(T) \< 256}.
Note that this condition is (for efficiency reasons) not checked. 

We continue the example from section \ref{Overloading}:

\medskip
{\bf struct} $pair$ $\{$\\
\hspace*{.5cm}$double$ $x$;\\
\hspace*{.5cm}$double$ $y$;

\hspace*{.5cm}$pair()$$\{\ x = y = 0;\ \}$\\
\hspace*{.5cm}$pair($const $pair\&\ p)\ \{\ x = p.x;\ y = p.y;\ \}$

\hspace*{.5cm}friend $ostream$\& \ operator\<\<($ostream$\&,const $pair$\&) \hspace{.7cm}$\{$ \dots $\}$\\
\hspace*{.5cm}friend $istream$\& \ operator\>\>($istream$\&,$pair$\&) \hspace{1.85cm}$\{$ \dots $\}$\\
\hspace*{.5cm}friend $int$ \hspace{1.3cm}compare(const $pair\&\ p$, const $pair\&\ q$) $\{$ \dots $\}$

\hspace*{.5cm}LEDA\_MEMORY($pair$)

$\}$;

\smallskip
dictionary\<$pair$,$int$\> D;

}*/


//------------------------------------------------------------------------------
// Memory Management
//------------------------------------------------------------------------------

struct  memory_elem_type { 
memory_elem_type* next; 
};

typedef memory_elem_type* memory_elem_ptr;

extern memory_elem_ptr memory_free_list[];

extern memory_elem_ptr memory_allocate_block(int);
extern memory_elem_ptr allocate_bytes_with_check(int);
extern memory_elem_ptr allocate_words(int);

extern void deallocate_bytes_with_check(void*,int);
extern void deallocate_words(void*,int);
extern void memory_clear();
extern void memory_kill();
extern void print_statistics();

extern int used_memory();

inline memory_elem_ptr allocate_bytes(int bytes)
{ memory_elem_ptr* q = memory_free_list+bytes;
  if (*q==0) *q = memory_allocate_block(bytes);
  memory_elem_ptr p = *q;
  *q = p->next;
  return p;
}

inline void deallocate_bytes(void* p, int bytes)
{ memory_elem_ptr* q = memory_free_list+bytes;
  memory_elem_ptr(p)->next = *q;
  *q = memory_elem_ptr(p);
 }

inline void deallocate_list(void* head,void* tail, int bytes)
{ memory_elem_ptr* q = memory_free_list+bytes;
  memory_elem_ptr(tail)->next = *q;
  *q = memory_elem_ptr(head);
 }



#define LEDA_MEMORY(type)\
\
void* operator new(size_t bytes)\
{ memory_elem_ptr* q = memory_free_list+bytes;\
  if (*q==0) *q = memory_allocate_block(bytes);\
  memory_elem_ptr p = *q;\
  *q = p->next;\
  return p;\
 }\
\
void* operator new(size_t,void* p) { return p; }\
\
void  operator delete(void* p, size_t bytes)\
{ memory_elem_ptr* q = memory_free_list+bytes;\
  memory_elem_ptr(p)->next = *q;\
  *q = memory_elem_ptr(p);\
 }


#define LEDA_MEMORY_WITH_CHECK(type)\
void* operator new(size_t bytes)\
{ return allocate_bytes_with_check(bytes); }\
\
void* operator new(size_t,void* p) { return p; }\
\
void  operator delete(void* p,size_t bytes)\
{ deallocate_bytes_with_check(p,bytes); }


#endif
