CC=gcc
CPP=g++
DEBUG=-g
PROFILE=-pg
RELEASE=-O3
ASSERT=-DNDEBUG
LIBS=#-lpapi# -ltcmalloc
CPAPI=-Wall -pedantic $(RELEASE) $(ASSERT) 
CPPOPTS=-Wall $(RELEASE) -I../ $(USE_MEM_COUNTING) $(REDEF_NEW)
PROGRAM=perf_test

#SRCS=burst_trie.c bucket_struct.c stat_gather.c clock.c avl_tree.c sorted_array.c counter_search.c sequential_search.c heap_search.c svector.c 
#OBJS=$(SRCS:%.c=%.o)
OBJS=xor_gens.o timer.o

# should add all trie code to the dependencies here
$(PROGRAM): perf_test.cpp $(OBJS)
	$(CPP) $(CPPOPTS) perf_test.cpp -o perf_test timer.o xor_gens.o

#
# Instrumentation.
#

timer.o: timer.cpp timer.h
	$(CPP) -c $(CPPOPTS) timer.cpp

xor_gens.o: ../xor_gens/xor_gens.cpp ../xor_gens/xor_gens.h
	$(CPP) -c $(CPPOPTS) ../xor_gens/xor_gens.cpp

debug: RELEASE=
debug: ASSERT=
debug: COPTS+=-g
debug: CPPOPTS+=-g
debug: $(PROGRAM)

prof: RELEASE=
prof: COPTS+=$(PROFILE)
prof: CPPOPTS+=$(PROFILE)

prof: $(PROGRAM)

clean:
	rm -f *.o gmon.out  
