#if !defined __COUNT_ALLOC

#define __COUNT_ALLOC

#include <map>

unsigned long long peak_memory;
unsigned long long used_memory;

static const unsigned long OVERHEAD  = 8;
static const unsigned long PARAGRAPH = 16;
static std::map<void*, unsigned long> size_map;

namespace MemCounter
{
    enum ALLOC_OP { DELETE = 0, NEW };
}

template <bool active, class T> void update_mem_counter(MemCounter::ALLOC_OP op, T* ptr, unsigned int num_objs = 1)
{
    if(!active)
    {
        return;
    }
    using namespace MemCounter;
    using namespace std;
    typedef map<void*, unsigned long>::iterator iter;
    if(op == DELETE)
    {
        iter it = size_map.find(static_cast<void*>(ptr));
        used_memory -= (*it).second;
        size_map.erase(it);
    }
    else if(op == NEW)
    {
        unsigned long total = num_objs * sizeof(T) + OVERHEAD;
        if(total < PARAGRAPH) 
        {
            total = PARAGRAPH;
        }
        used_memory += total;
        if(used_memory > peak_memory) 
        {
            peak_memory = used_memory;
        }
        size_map[ptr] = total;
   }
   return;
}

#endif
