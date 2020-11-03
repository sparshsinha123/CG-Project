########################################################################
####################### Makefile Template ##############################
########################################################################

# Compiler settings - Can be customized.
CURRENTDIR = $(shell pwd)
CC = g++
CXXFLAGS = -Wall -std=c++14 -D_GLIBCXX_USE_CXX11_ABI=0 -Wall -I $(CURRENTDIR)/src/libtorch/include/torch/csrc/api/include -I $(CURRENTDIR)/src/libtorch/include
LDFLAGS = -lGL -lglut -lGLU -L $(CURRENTDIR)/src/libtorch/lib -lasmjit -lbenchmark -lbenchmark_main -lc10d -lc10 -lcaffe2_detectron_ops -lcaffe2_module_test_dynamic -lcaffe2_observers -lCaffe2_perfkernels_avx2 -lCaffe2_perfkernels_avx512 -lCaffe2_perfkernels_avx -lcaffe2_protos -lclog -lcpuinfo -lcpuinfo_internals -ldnnl -lfbgemm -lfbjni -lfmt -lfmt -lfmt -lfoxi_loader -lgloo -lgmock -lgmock_main -lgomp -lgtest -lgtest_main -ljitbackend_test -lmkldnn -lnnpack -lnnpack_reference_layers -lonnx -lonnx_proto -lprocess_group_agent -lprotobuf -lprotobuf-lite -lprotoc -lpthreadpool -lpytorch_jni -lpytorch_qnnpack -lqnnpack -lshm -ltensorpipe -ltensorpipe_agent -ltensorpipe_uv -ltorchbind_test -ltorch_cpu -ltorch_global_deps -ltorch_python -ltorch -lXNNPACK

# Makefile settings - Can be customized.
APPNAME = cd
EXT = .cpp
SRCDIR = src
OBJDIR = obj

############## Do not change anything from here downwards! #############
SRC = $(wildcard $(SRCDIR)/*$(EXT))
OBJ = $(SRC:$(SRCDIR)/%$(EXT)=$(OBJDIR)/%.o)
DEP = $(OBJ:$(OBJDIR)/%.o=%.d)
# UNIX-based OS variables & settings
RM = rm
DELOBJ = $(OBJ)
# Windows OS variables & settings
DEL = del
EXE = .exe
WDELOBJ = $(SRC:$(SRCDIR)/%$(EXT)=$(OBJDIR)\\%.o)

########################################################################
####################### Targets beginning here #########################
########################################################################

all: $(APPNAME)

# Builds the app
$(APPNAME): $(OBJ)
	$(CC) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Creates the dependecy rules
%.d: $(SRCDIR)/%$(EXT)
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:%.d=$(OBJDIR)/%.o) >$@

# Includes all .h files
-include $(DEP)

# Building rule for .o files and its .c/.cpp in combination with all .h
$(OBJDIR)/%.o: $(SRCDIR)/%$(EXT)
	$(CC) $(CXXFLAGS) -o $@ -c $<

################### Cleaning rules for Unix-based OS ###################
# Cleans complete project
.PHONY: clean
clean:
	$(RM) $(DELOBJ) $(DEP) $(APPNAME)

# Cleans only all files with the extension .d
.PHONY: cleandep
cleandep:
	$(RM) $(DEP)

#################### Cleaning rules for Windows OS #####################
# Cleans complete project
.PHONY: cleanw
cleanw:
	$(DEL) $(WDELOBJ) $(DEP) $(APPNAME)$(EXE)

# Cleans only all files with the extension .d
.PHONY: cleandepw
cleandepw:
	$(DEL) $(DEP)