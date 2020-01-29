CLANG = ../../../llvm-6.0/bin/clang # change depending on where your installation of clang is
OPT = ../../../llvm-6.0/bin/opt # change depending on where your installation of opt is
BUILDDIR = _build
BUILDDIR_FILE = _build/$(name)

OPTIMIZATON_LEVEL = #O1, O2, O3
OPTIMIZATION_FLAGS = #-fno-builtin, -fno-vectorize

$(BUILDDIR):
	mkdir -p $(BUILDDIR)
	mkdir -p $(BUILDDIR_FILE)

test_f: $(BUILDDIR)
	mkdir -p $(BUILDDIR_FILE)
	-rm -rf $(BUILDDIR_FILE)/*.dot
	$(CLANG) -Xclang -disable-O0-optnone -emit-llvm -S $(OPTIMIZATION_LEVEL) $(OPTIMIZATION_FLAGS)-c $(name).cpp -o $(BUILDDIR_FILE)/$(name).ll
	$(OPT) -mem2reg  $(BUILDDIR_FILE)/$(name).ll -S -o $(BUILDDIR_FILE)/$(name)_mem2reg.ll
	$(OPT) -loop-rotate -constprop  $(BUILDDIR_FILE)/$(name)_mem2reg.ll -S -o $(BUILDDIR_FILE)/$(name)_mem2reg_constprop.ll
	$(OPT) -simplifycfg $(BUILDDIR_FILE)/$(name)_mem2reg_constprop.ll -S -o $(BUILDDIR_FILE)/$(name)_mem2reg_constprop_simplifycfg.ll
	$(OPT) -die -instcombine -lowerswitch $(BUILDDIR_FILE)/$(name)_mem2reg_constprop_simplifycfg.ll -S -o $(BUILDDIR_FILE)/$(name)_mem2reg_constprop_simplifycfg_die.ll

	cc -c ../FrequencyCounterPass/log_FrequencyCounter.c
	$(CLANG) -fPIC -Xclang -load -Xclang ../_build/FrequencyCounterPass/libFrequencyCounterPass.so -c $(BUILDDIR_FILE)/$(name)_mem2reg_constprop_simplifycfg_die.ll
	cc -fPIC $(name)_mem2reg_constprop_simplifycfg_die.o log_FrequencyCounter.o
	./a.out
	rm a.out $(name)_mem2reg_constprop_simplifycfg_die.o log_FrequencyCounter.o

	-$(CLANG) -Xclang -load -Xclang ../_build/FrequencyDataGatherPass/libFrequencyDataGatherPass.so $(BUILDDIR_FILE)/$(name)_mem2reg_constprop_simplifycfg_die.ll -S
	rm *.s

	#-$(OPT) -load ../_build/MemElemInfo/libLLVMMemElemInfo.so -load ../_build/ElasticPass/libElasticPass.so -load ../_build/OptimizeBitwidth/libLLVMOptimizeBitWidth.so -load ../_build/MyCFGPass/libMyCFGPass.so -polly-process-unprofitable -mycfgpass  $(BUILDDIR_FILE)/$(name)_mem2reg_constprop_simplifycfg_die.ll -S -use-lsq=false -simple-buffers=true

	-$(OPT) -load ../_build/MemElemInfo/libLLVMMemElemInfo.so -load ../_build/ElasticPass/libElasticPass.so -load ../_build/OptimizeBitwidth/libLLVMOptimizeBitWidth.so -load ../_build/MyCFGPass/libMyCFGPass.so -polly-process-unprofitable -mycfgpass  $(BUILDDIR_FILE)/$(name)_mem2reg_constprop_simplifycfg_die.ll -S -use-lsq=false 

	#-$(OPT) -load ../_build/MemElemInfo/libLLVMMemElemInfo.so -load ../_build/ElasticPass/libElasticPass.so -load ../_build/OptimizeBitwidth/libLLVMOptimizeBitWidth.so -load ../_build/MyCFGPass/libMyCFGPass.so -polly-process-unprofitable -mycfgpass  $(BUILDDIR_FILE)/$(name)_mem2reg_constprop_simplifycfg_die.ll -S
	rm *_freq.txt mapping.txt

	mv print.txt $(BUILDDIR_FILE) || true
	#mv *_graph.dot $(BUILDDIR_FILE)
	

$(BUILDDIR_FILE)/$(name)_graph.dot: test

graph_f: test_f
	mv *_graph.dot  $(BUILDDIR_FILE)
	mv *bbgraph.dot  $(BUILDDIR_FILE)
	-rm -rf $(BUILDDIR_FILE)/main_graph.dot
	-rm -rf $(BUILDDIR_FILE)/main_bbgraph.dot
	dot -Tpng $(BUILDDIR_FILE)/*_graph.dot > $(BUILDDIR_FILE)/$(name)_graph.png
	dot -Tpng $(BUILDDIR_FILE)/*bbgraph.dot > $(BUILDDIR_FILE)/$(name)_bbgraph.png
	mv $(BUILDDIR_FILE)/*_graph.dot  $(BUILDDIR_FILE)/$(name)_graph.dot
	mv $(BUILDDIR_FILE)/*bbgraph.dot  $(BUILDDIR_FILE)/$(name)_bbgraph.dot
	


test: $(BUILDDIR)
	mkdir -p $(BUILDDIR_FILE)
	-rm -rf $(BUILDDIR_FILE)/*.dot
	$(CLANG) -Xclang -disable-O0-optnone -emit-llvm -S $(OPTIMIZATION_LEVEL) $(OPTIMIZATION_FLAGS)-c $(name).cpp -o $(BUILDDIR_FILE)/$(name).ll
	$(OPT) -mem2reg  $(BUILDDIR_FILE)/$(name).ll -S -o $(BUILDDIR_FILE)/$(name)_mem2reg.ll
	$(OPT) -loop-rotate -constprop  $(BUILDDIR_FILE)/$(name)_mem2reg.ll -S -o $(BUILDDIR_FILE)/$(name)_mem2reg_constprop.ll
	$(OPT) -simplifycfg $(BUILDDIR_FILE)/$(name)_mem2reg_constprop.ll -S -o $(BUILDDIR_FILE)/$(name)_mem2reg_constprop_simplifycfg.ll
	$(OPT) -die -instcombine -lowerswitch $(BUILDDIR_FILE)/$(name)_mem2reg_constprop_simplifycfg.ll -S -o $(BUILDDIR_FILE)/$(name)_mem2reg_constprop_simplifycfg_die.ll


	
#-$(OPT) -load ../_build/MemElemInfo/libLLVMMemElemInfo.so -load ../_build/ElasticPass/libElasticPass.so -load ../_build/OptimizeBitwidth/libLLVMOptimizeBitWidth.so -load ../_build/MyCFGPass/libMyCFGPass.so -polly-process-unprofitable -mycfgpass  $(BUILDDIR_FILE)/$(name)_mem2reg_constprop_simplifycfg_die.ll -S -cfg-outdir=$(BUILDDIR_FILE) -use-lsq=false -simple-buffers=true

#-$(OPT) -load ../_build/MemElemInfo/libLLVMMemElemInfo.so -load ../_build/ElasticPass/libElasticPass.so -load ../_build/OptimizeBitwidth/libLLVMOptimizeBitWidth.so -load ../_build/MyCFGPass/libMyCFGPass.so -polly-process-unprofitable -mycfgpass  $(BUILDDIR_FILE)/$(name)_mem2reg_constprop_simplifycfg_die.ll -S -cfg-outdir=$(BUILDDIR_FILE) -use-lsq=false -simple-buffers=true

	-$(OPT) -load ../_build/MemElemInfo/libLLVMMemElemInfo.so -load ../_build/ElasticPass/libElasticPass.so -load ../_build/OptimizeBitwidth/libLLVMOptimizeBitWidth.so -load ../_build/MyCFGPass/libMyCFGPass.so -polly-process-unprofitable -mycfgpass  $(BUILDDIR_FILE)/$(name)_mem2reg_constprop_simplifycfg_die.ll -S -cfg-outdir=$(BUILDDIR_FILE)


	#mv print.txt $(BUILDDIR_FILE) || true
	#mv *_graph.dot $(BUILDDIR_FILE)
	
graph: test
	dot -Tpng $(BUILDDIR_FILE)/*_graph.dot > $(BUILDDIR_FILE)/$(name)_graph.png
	dot -Tpng $(BUILDDIR_FILE)/*bbgraph.dot > $(BUILDDIR_FILE)/$(name)_bbgraph.png
	mv $(BUILDDIR_FILE)/*_graph.dot  $(BUILDDIR_FILE)/$(name)_graph.dot || true
	mv $(BUILDDIR_FILE)/*bbgraph.dot  $(BUILDDIR_FILE)/$(name)_bbgraph.dot || true



