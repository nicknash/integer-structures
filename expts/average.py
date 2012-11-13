#!/usr/bin/env python
import sys
import os
import time

num_runs = 30

# N.B. these should in same order as in perf_test.cpp
data_structs = ( "map", "btree", "stree", "lpcbtrie", "lpcqtrie" )
trace_names = ( "top_trace_bin", "amarok_trace_bin", "konq_trace_bin", "kpdf_trace_bin" )


############# MAIN ####################

if len(sys.argv) < 2:
    print "Usage: " + sys.argv[0] + " <results_directory>"
    sys.exit()

results_dir = sys.argv[1]


####
# Initialisation
####
file_names = []
for ds in data_structs:
    file_names.append(ds + "_irandom_time")
    file_names.append(ds + "_drandom_time")
    file_names.append(ds + "_genome_time")
    for t in trace_names:
        file_names.append(ds + "_" + t + "_time")

os.system("rm -rf averaged_" + results_dir)
os.system("mkdir averaged_" + results_dir)

for f in file_names:
    output = open("averaged_" + results_dir + "/" + f, "w")
    files = []    
    for r in range(0, num_runs):
        files.append(open(results_dir + "/" + f + "_%d"%(r)))
    # For line of each file, average all its columns
    # and write out the new columns to a file
    # called file_key
    quit = False
    while not quit:
        column_totals = [ 0, 0, 0 ]
        for f in files:
            line = f.readline()
            if line == "":
                quit = True
                break            
            line_list = line.split()
#            column_totals[0] = float(line_list[0])
            idx = 0
            for l in line_list[0:]:
                column_totals[idx] += float(l)
                idx += 1
        if not quit:            
#            output.write("%d "%(column_totals[0]))
            for c in column_totals[0:]:
                output.write("%f "%(c / len(files)))
            output.write("\n")
    # Close all the files
    for f in files:
        f.close()
    output.close()

os.system("cp " + results_dir + "/*mem* averaged_" + results_dir)

"""
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

os.system(lpc_mem_binary + " 3 genome %s/set6_genome.dat > %s/lpcbtrie_irandom_mem"%(data_dir, results_dir))
os.system(lpc_mem_binary + " 4 genome %s/set6_genome.dat > %s/lpcqtrie_irandom_mem"%(data_dir, results_dir))

os.system(other_mem_binary + " 0 genome %s/set6_genome.dat > %s/map_irandom_mem"%(data_dir, results_dir))
os.system(other_mem_binary + " 1 genome %s/set6_genome.dat > %s/btree_irandom_mem"%(data_dir, results_dir))
os.system(other_mem_binary + " 2 genome %s/set6_genome.dat > %s/stree_irandom_mem"%(data_dir, results_dir))

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

"""
