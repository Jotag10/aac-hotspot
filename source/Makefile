HOST_CC = g++
HOST_CC_FLAGS = -g -O2

GEM5_CC = aarch64-linux-g++
GEM5_CC_FLAGS = -g -O3 -static

DATA_DIR = $(HOME)/apps/hotspot/data
GEM5_DIR = $(HOME)/gem5

RM=rm -f

KERNEL = kernel

OBJDIR = build
SOBJ_host = hotspot_host.o
SOBJ_gem5 = hotspot_gem5.o

OBJ_host := $(patsubst %.o,$(OBJDIR)/%.o,$(SOBJ_host))
OBJ_gem5 := $(patsubst %.o,$(OBJDIR)/%.o,$(SOBJ_gem5))


compile_all: hotspot_host hotspot_gem5 hotspot_neon hotspot_sve

compile_host: hotspot_host

compile_gem5: hotspot_gem5

compile_neon: hotspot_neon

compile_sve: hotspot_sve


link_objs_host = $(HOST_CC) $(HOST_CC_FLAGS) -o $@ $(OBJ_host) $(OBJDIR)/$(KERNEL)_$@.o $(LIBS)
link_objs_gem5 = $(GEM5_CC) $(GEM5_CC_FLAGS) -o $@ $(OBJ_gem5) $(OBJDIR)/$(KERNEL)_$@.o $(LIBS)

run_host: hotspot_host
	@echo "======================================================================"
	@echo "WARNING: Running with 1000 iterations... "
	@echo "you may change the number of iterations and the problem size"
	@echo "======================================================================"
	./hotspot_host 1024 1024 1000 1 $(DATA_DIR)/temp_1024 $(DATA_DIR)/power_1024 output_host.out
	
run_gem5: hotspot_gem5
	@echo "======================================================================"
	@echo "WARNING: Running the smallest problem size (64) with 2 iterations... "
	@echo "you should change the number of iterations and the problem size"
	@echo "for the final results"
	@echo "======================================================================"
	$(GEM5_DIR)/build/ARM/gem5.opt $(GEM5_DIR)/configs/example/se.py --cpu-type=ex5_LITTLE --caches --svevl=1 -c hotspot_gem5 -o '64 64 2 1 $(DATA_DIR)/temp_64 $(DATA_DIR)/power_64 output_gem5.out'


#hotspot_host: $(OBJ_host) $(KERNEL).c
#	$(HOST_CC) $(HOST_CC_FLAGS) -o $(OBJDIR)/$(KERNEL)_$@.o -c $(KERNEL).c
#	$(link_objs_host)

hotspot_gem5: $(OBJ_gem5) $(KERNEL).c
	$(GEM5_CC) $(GEM5_CC_FLAGS) -o $(OBJDIR)/$(KERNEL)_$@.o -c $(KERNEL).c
	$(link_objs_gem5)

hotspot_neon: $(OBJ_gem5) $(KERNEL).c
	$(GEM5_CC) $(GEM5_CC_FLAGS) -march=armv8-a+simd -fno-tree-vectorize -o $(OBJDIR)/$(KERNEL)_$@.o -c $(KERNEL).c -DNEON
	$(link_objs)


hotspot_sve: $(OBJ_gem5) $(KERNEL).c
	$(GEM5_CC) $(GEM5_CC_FLAGS) -march=armv8-a+sve -fno-tree-vectorize -o $(OBJDIR)/$(KERNEL)_$@.o -c $(KERNEL).c -DSVE
	$(link_objs)

$(OBJDIR)/$(SOBJ_host) : hotspot.cpp
	$(HOST_CC) $(HOST_CC_FLAGS) -c $< -o $@
	
$(OBJDIR)/$(SOBJ_gem5) : hotspot.cpp
	$(GEM5_CC) $(HOST_CC_gem5) -fno-tree-vectorize -c $< -o $@

clean :

	$(RM) hotspot_host
	$(RM) hotspot_gem5
	$(RM) hotspot_neon
	$(RM) hotspot_sve
	$(RM) -r build/

$(shell mkdir -p $(OBJDIR))
