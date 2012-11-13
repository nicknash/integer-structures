#include <iostream>
#include <fstream>
#include <cstdlib>
#include <expts/timer.h>

#include <xor_gens/xor_gens.h>

#include <stdmap/stdmap.h>
#include <qtrie/lpcqtrie.h>
#include <btrie/lpcbtrie.h>
#include <btree/btree.h>
#include <veb/stree.h>

#include <count_alloc/count_alloc.h>

const int NUM_SIZES          = 14;
const int RAND_SET_SIZES[NUM_SIZES] = { 1 << 14, 1 << 15, 1 << 16, 1 << 17, 1 << 18, 1 << 19, 1 << 20, 
                                        1 << 21, 1 << 22, 1 << 23, 1 << 24, 1 << 25, 1 << 26, 1 << 27 };

const int NUM_STRUCTS = 5;
enum DATA_STRUCT_ID { STDMAP = 0, BTREE, STREE, LPCBTRIE, QTRIE };
const char* data_struct_names[] = { "stdmap", "btree", "stree", "lpcbtrie", "lpcqtrie" };


const int MAX_INSERT_SIZES[NUM_STRUCTS] = { 1 << 26, 1 << 27, 1 << 25,  1 << 27, 1 << 27 };
const int MAX_DELETE_SIZES[NUM_STRUCTS] = { 1 << 26, 1 << 27, 1 << 21,  1 << 27, 1 << 27 };

enum WORKLOAD_ID { INSERT_LOCATE_OPS = 0, INSERT_DELETE_OPS, VALGRIND_TRACES, GENOME };

#if defined REDEF_NEW

#undef new

extern unsigned long long peak_memory;

void *operator new(size_t size) throw(std::bad_alloc)
{
    peak_memory += size + 8;
    return malloc(size);
}

#endif


template <class DataStruct> void apply_insert_locate(DataStruct* ds, int size, float& insert_time, float& locate_time)
{
    using namespace std;
    Timer t;
    if(sizeof(unsigned long) == 4)
    {
        t.start();
        for(int i = 0; i < size; i++)
        {
            ds->insert(xor4096s(), i);
        }
        insert_time = t.elapsed();
        t.start();
        for(int i = 0; i < size; i++)
        {
            ds->locate(xor4096s());
        }
        locate_time = t.elapsed();
    }
    else if(sizeof(unsigned long) == 8)
    {
        t.start();
        for(int i = 0; i < size; i++)
        {
            ds->insert(xor4096l(), i);
        }
        insert_time = t.elapsed();
        t.start();
        for(int i = 0; i < size; i++)
        {
            ds->locate(xor4096l());
        }
        locate_time = t.elapsed();
    }
    else
    {
        insert_time = -1;
        locate_time = -1;
    }
    return;
}

template <class DataStruct> void do_insert_locate(int max_size)
{
    float insert_time, locate_time;
    using namespace std;
    for(int i = 0; i < NUM_SIZES; i++)
    {
#if defined REDEF_NEW || defined USE_MEM_COUNTING
        peak_memory = 0;
#endif
        DataStruct* ds = new DataStruct;
        int size = RAND_SET_SIZES[i];
        if(size > max_size) 
        {
            break;
        }
        apply_insert_locate(ds, size, insert_time, locate_time);
#if defined REDEF_NEW || defined USE_MEM_COUNTING
        cout << size << " " << peak_memory / (float) size << endl;
#else
        cout << size << " " << 1e6 * insert_time / size << " " << 1e6 * locate_time / size << endl;
#endif        
        delete ds;
    }
    return;
}

template <class DataStruct> void apply_delete_mix(DataStruct* ds, long* workload, bool* is_insert, int size, float& time)
{
    using namespace std;
    Timer t;
    t.start();
    for(int i = 0; i < size; i++)
    {
        if(is_insert[i])
        {
            ds->insert(workload[i], i);
        }
        else
        {
            ds->remove(workload[rand() % (i + 1)]);
        }
    }
    time = t.elapsed();
    return;
}

template <class DataStruct> void do_delete_mix(int max_size)
{
    using namespace std;
    for(int i = 0; i < NUM_SIZES; i++)
    {
        DataStruct* ds = new DataStruct;
        int size = RAND_SET_SIZES[i];
        if(size > max_size) 
        {
            break;
        }
        bool* mix_mask = new bool[size];
        long* workload = new long[size];
        for(int j = 0; j < size; j++)
        {        
            mix_mask[j] = rand() / (RAND_MAX + 1.0) >= 0.5;                
            if(sizeof(long) == 4)
            {
                workload[j] = xor4096s();
            }
            else
            {
                workload[j] = xor4096l();
            }
        }    
        float time;
        apply_delete_mix(ds, workload, mix_mask, size, time);
        cout << size << " " << 1e6 * time / size << endl;
        delete ds;
        delete[] mix_mask;
        delete[] workload;
    }
    return;
}

template <class DataStruct> void apply_genome(char* genome_file_name)
{
    using namespace std;

    const int STR_LEN = 9;
    ifstream in(genome_file_name);
    if(!in)
    {
        cerr << "FATAL ERROR: Couldn't open genome file: " << genome_file_name << endl;
        return;
    }
    int size;
    in >> size;
    unsigned long* data = new unsigned long[size];
    for(int i = 0; i < size; i++)
    {
        string s;
        in >> s;
        data[i] = 0;
        for(int j = 0; j < STR_LEN; j++)
        {
            switch(s[j])
            {
                case 'a':
                break;
                case 'c':
                    data[i] |= 1;
                break;
                case 'g':
                    data[i] |= 2;
                break;
                case 't':
                    data[i] |= 3;
                break;
            }
            data[i] <<= 2;
        }         
    }
    in.close();

    peak_memory = 0; // ignore the data just allocated
    DataStruct ds;
    Timer t;
    
    for(int i = 0; i < size; i++)
    {
        data[i] = (data[i] << 18) | data[(i + 1) % size];
    }
    t.start();
    for(int i = 0; i < size; i++)
    {
        ds.insert(data[i], i);
    }
#if !defined REDEF_NEW && !defined USE_MEM_COUNTING
    cout << t.elapsed() << " ";
    t.start();
    for(int i = 0; i < size; i++)
    {
        ds.locate(data[i]);
    }
#endif

#if defined REDEF_NEW || defined USE_MEM_COUNTING
    cout << peak_memory << endl;
#else
    cout << t.elapsed() << endl;
#endif    

    delete[] data;
    return;
}

template <class DataStruct> void apply_valgrind_trace(char* file_name)
{
    using namespace std;
    ifstream in(file_name, ios::binary);
    if(!in)
    {
        cerr << "Couldn't open valgrind trace: " << file_name << endl;
        return;
    }
    unsigned long* ops;
    bool* is_store;
    unsigned long num_ops;
    in >> num_ops;
    in.get();
    ops = new unsigned long[num_ops];
    is_store = new bool[num_ops];

    in.read((char*) is_store, num_ops * sizeof(*is_store));
    in.read((char*) ops, num_ops * sizeof(*ops));

    peak_memory = 0;
    DataStruct ds;
    Timer t;
    t.start();
    for(unsigned long i = 0; i < num_ops; i++)
    {
        if(is_store[i])
        {
            ds.insert(ops[i], i);
        }
        else
        {
            ds.locate(ops[i]);
        }
    }
#if defined REDEF_NEW || defined USE_MEM_COUNTING
    cout << peak_memory << endl;
#else
    cout << t.elapsed() << endl;
#endif    
    delete[] ops;
    delete[] is_store;
    return;
}

template <class DataStruct> void apply_workload(WORKLOAD_ID workload, DATA_STRUCT_ID data_struct, char* file_name)
{
    switch(workload)
    {
        case INSERT_LOCATE_OPS:
            do_insert_locate<DataStruct>(MAX_INSERT_SIZES[data_struct]);
        break;
        case INSERT_DELETE_OPS:
            do_delete_mix<DataStruct>(MAX_DELETE_SIZES[data_struct]);
        break;
        case VALGRIND_TRACES:
            apply_valgrind_trace<DataStruct>(file_name);
        break;
        case GENOME:
            apply_genome<DataStruct>(file_name);
        break;
    }
    return;
}

int main(int argc, char** argv)
{
    using namespace std;
    if(argc < 3)
    {
        // In the first usage we'll (ultimately) test: locate and memory used
        // output is: size locate_time search_time i/d_time memory
        cerr << "Usage 1: " << argv[0] << " <data structure> irandom" << endl;
        // Insert/delete mix
        // output is: size time
        cerr << "Usage 2: " << argv[0] << " <data structure> drandom" << endl;
        // In the second usage, we test the trace processing time and the memory used.
        // output is: time memory
        cerr << "Usage 3: " << argv[0] << " <data structure> valgrind <trace name>" << endl;
        // In the third usage, we test insertion and self-search time on the genome
        // output is insert_time search_time memory
        cerr << "Usage 4: " << argv[0] << " <data structure> genome <genome file>" << endl;

        cerr << "----------------------" << endl;
        cerr << "Valid data structures:" << endl;
        cerr << "----------------------" << endl;
        for(int i = 0; i < NUM_STRUCTS; i++)
        {
            cerr << i << " ---> " << data_struct_names[i] << endl;
        }
        return 0;
    }
    DATA_STRUCT_ID data_struct = static_cast<DATA_STRUCT_ID>(atoi(argv[1]));

    WORKLOAD_ID workload;
    char* file_name = 0;
    switch(argv[2][0])
    {
        case 'i':
            workload = INSERT_LOCATE_OPS;
        break;
        case 'd':
            workload = INSERT_DELETE_OPS;
        break;
        case 'v':
            workload = VALGRIND_TRACES;            
            file_name = argv[3];
        break;
        case 'g':
            workload = GENOME;           
            file_name = argv[3];
        break;
        default:
            cerr << "Invalid workload specified." << endl;
            return 0;
        break;
    }
    typedef unsigned long ul;
    switch(data_struct)
    {
        case STDMAP:
            apply_workload<STDMap<ul, ul> >(workload, data_struct, file_name);
        break;
        case BTREE:
            apply_workload<BTree<ul, ul> >(workload, data_struct, file_name);
        break;
        case STREE:
            apply_workload<STree<ul, ul> >(workload, data_struct, file_name);
        break;
        case LPCBTRIE:
#if defined USE_MEM_COUNTING
            apply_workload<LPCBTrie<ul, ul, true> >(workload, data_struct, file_name);
#else
            apply_workload<LPCBTrie<ul, ul> >(workload, data_struct, file_name);
#endif            
        break;
        case QTRIE:
#if defined USE_MEM_COUNTING
        apply_workload<LPCQTrie<ul, ul, true> >(workload, data_struct, file_name);
#else
        apply_workload<LPCQTrie<ul, ul> >(workload, data_struct, file_name);
#endif        
        break;
        default:
            cerr << "Invalid data structure specified!" << endl;
        break;
    }
    return 0;
}

