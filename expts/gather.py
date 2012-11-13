#!/usr/bin/env python
import sys
import os
import time

num_runs = 30

# N.B. these should in same order as in perf_test.cpp
data_structs = ( "map", "btree", "stree", "lpcbtrie", "lpcqtrie" )
trace_names = ( "top_trace_bin", "amarok_trace_bin", "konq_trace_bin", "kpdf_trace_bin" )

timing_binary = "./timing_perf_test"
lpc_mem_binary = "./lpc_mem_perf_test"
other_mem_binary = "./other_mem_perf_test"

def warn_and_remove(results_dir, time_limit):
    print "*******"
    print "WARNING: Going to remove old results directory: (" + results_dir + ") in %d seconds, press CTRL+C *NOW* to abort."%(time_limit)
    print "*******"
    sys.stdout.flush()
    for i in range(0, time_limit):
        print "%d"%(time_limit - i)
        sys.stdout.flush()
        time.sleep(1)
    os.system("rm -rf " + results_dir)
    os.system("mkdir " + results_dir)


############# MAIN ####################

if len(sys.argv) < 3:
    print "Usage: " + sys.argv[0] + " <results_directory> <data_directory>"
    sys.exit()

results_dir = sys.argv[1]
data_dir = sys.argv[2]


warn_and_remove(results_dir, 5)

print "----------> Running make clean"
os.system("make clean")
print "----------> Done."


print "----------> Running make (timing build)"
print "---------------------------------------"
os.system("make")
os.system("mv ./perf_test " + timing_binary)
os.system("make clean")
print "---------------------------------------"
print "----------> Done."

print "----------> Running make USE_MEM_COUNTING=-DUSE_MEM_COUNTING (LPCBTrie/LPCQTrie mem-counting build)"
print "---------------------------------------"
os.system("make USE_MEM_COUNTING=-DUSE_MEM_COUNTING")
os.system("mv ./perf_test " + lpc_mem_binary)
os.system("make clean")
print "---------------------------------------"
print "----------> Done."

print "----------> Running make REDEF_NEW=-DREDEF_NEW (map/btree/stree mem-counting build)"
print "---------------------------------------"
os.system("make REDEF_NEW=-DREDEF_NEW")
os.system("mv ./perf_test " + other_mem_binary)
os.system("make clean")
print "---------------------------------------"
print "----------> Done."


sys.stdout.flush()

#
# First gather all the timing results
#
print "-----------> Doing random inserts/locates (timing): "

ds_num = 0
for ds in data_structs:    
    print "On data structure: " + ds
    sys.stdout.flush()
    for m in ( "irandom", "drandom"):
        for r in range(0, num_runs):
            os.system(timing_binary + " %d %s > %s/%s_%s_time_%d"%(ds_num, m, results_dir, ds, m, r))
    ds_num = ds_num + 1

print "-----------> Doing genome (timing): "

ds_num = 0
for ds in data_structs:    
    print "On data structure: " + ds
    sys.stdout.flush()
    for r in range(0, num_runs):
        os.system(timing_binary + " %d genome %s/set6_genome.dat > %s/%s_genome_time_%d"%(ds_num, data_dir, results_dir, ds, r))
    ds_num = ds_num + 1

print "-----------> Doing valgrind (timing): "

ds_num = 0
for ds in data_structs:    
    print "On data structure: " + ds
    sys.stdout.flush()
    for t in trace_names:
        for r in range(0, num_runs):
            os.system(timing_binary + " %d valgrind %s/%s > %s/%s_%s_time_%d"%(ds_num, data_dir, t, results_dir, ds, t, r))
    ds_num = ds_num + 1

#
# Now gather the memory results, these are bit messier, we need to deal
# differently with the data structures that count their memory differently.
#

print "--------------> Gathering memory results: "
print "irandom: "
sys.stdout.flush()

os.system(lpc_mem_binary + " 3 irandom > %s/lpcbtrie_irandom_mem"%(results_dir))
os.system(lpc_mem_binary + " 4 irandom > %s/lpcqtrie_irandom_mem"%(results_dir))

os.system(other_mem_binary + " 0 irandom > %s/map_irandom_mem"%(results_dir))
os.system(other_mem_binary + " 1 irandom > %s/btree_irandom_mem"%(results_dir))
os.system(other_mem_binary + " 2 irandom > %s/stree_irandom_mem"%(results_dir))

print "genome: "
sys.stdout.flush()

os.system(lpc_mem_binary + " 3 genome %s/set6_genome.dat > %s/lpcbtrie_genome_mem"%(data_dir, results_dir))
os.system(lpc_mem_binary + " 4 genome %s/set6_genome.dat > %s/lpcqtrie_genome_mem"%(data_dir, results_dir))

os.system(other_mem_binary + " 0 genome %s/set6_genome.dat > %s/map_genome_mem"%(data_dir, results_dir))
os.system(other_mem_binary + " 1 genome %s/set6_genome.dat > %s/btree_genome_mem"%(data_dir, results_dir))
os.system(other_mem_binary + " 2 genome %s/set6_genome.dat > %s/stree_genome_mem"%(data_dir, results_dir))

print "valgrind: "
sys.stdout.flush()

for t in trace_names:
    os.system(lpc_mem_binary + " 3 valgrind %s/%s > %s/lpcbtrie_valgrind_%s_mem"%(data_dir, t, results_dir, t))
    os.system(lpc_mem_binary + " 4 valgrind %s/%s > %s/lpcqtrie_valgrind_%s_mem"%(data_dir, t, results_dir, t))
    os.system(other_mem_binary + " 0 valgrind %s/%s > %s/map_valgrind_%s_mem"%(data_dir, t, results_dir, t))
    os.system(other_mem_binary + " 1 valgrind %s/%s > %s/btree_valgrind_%s_mem"%(data_dir, t, results_dir, t))
    os.system(other_mem_binary + " 2 valgrind %s/%s > %s/stree_valgrind_%s_mem"%(data_dir, t, results_dir, t))
 
print "------------> Removing binaries"
os.system("rm " + timing_binary + " " + lpc_mem_binary + " " + other_mem_binary)
print "------------> ALL GATHERING DONE"
