/**
 * Elastic Pass
 *
 * Forming a netlist of dataflow components out of the LLVM IR
 *
 */

#include <cassert>

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/RandomNumberGenerator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "ElasticPass/CircuitGenerator.h"
#include "ElasticPass/Utils.h"
#include "ElasticPass/Head.h"
#include "ElasticPass/Nodes.h"
#include "ElasticPass/Pragmas.h"
#include "OptimizeBitwidth/OptimizeBitwidth.h"


#include <sys/time.h>
#include <time.h>

static cl::opt<bool> opt_useLSQ("use-lsq", cl::desc("Emit LSQs where applicable"), cl::Hidden,
                                cl::init(true), cl::Optional);

static cl::opt<std::string> opt_cfgOutdir("cfg-outdir", cl::desc("Output directory of MyCFGPass"),
                                          cl::Hidden, cl::init("."), cl::Optional);

static cl::opt<bool> opt_buffers("simple-buffers", cl::desc("Naive buffer placement"), cl::Hidden,
                                cl::init(false), cl::Optional);

static cl::opt<std::string> opt_serialNumber("target", cl::desc("Targeted FPGA"), cl::Hidden,
                                cl::init("default"), cl::Optional);

struct timeval start, time_end;

std::string fname;
static int indx_fname = 0;

void set_clock() { gettimeofday(&start, NULL); }

double elapsed_time() {
    gettimeofday(&time_end, NULL);

    double elapsed = (time_end.tv_sec - start.tv_sec);
    elapsed += (double)(time_end.tv_usec - start.tv_usec) / 1000000.0;
    return elapsed;
}

using namespace llvm;

//class ControlDependencyGraph;

namespace {
class MyCFGPass : public llvm::FunctionPass {

public:
    static char ID;

    MyCFGPass() : llvm::FunctionPass(ID) {}

    virtual void getAnalysisUsage(AnalysisUsage& AU) const {
        // 			  AU.setPreservesCFG();
        AU.addRequired<OptimizeBitwidth>();
		OptimizeBitwidth::setEnabled(false);  // pass true to it to enable OB	

        AU.addRequired<MemElemInfoPass>();

		AU.addRequired<LoopInfoWrapperPass>();

    }

    void compileAndProduceDOTFile(Function& F , LoopInfo& LI) {
        fname = F.getName().str();

        // main is used for frequency extraction, we do not use its dataflow graph
        if (fname != "main") {

            bool done = false;

            auto M = F.getParent();

            pragmas(0, M->getModuleIdentifier());

            std::vector<BBNode*>* bbnode_dag = new std::vector<BBNode*>;
            std::vector<ENode*>* enode_dag   = new std::vector<ENode*>;
            OptimizeBitwidth* OB             = &getAnalysis<OptimizeBitwidth>();
            MemElemInfo& MEI                 = getAnalysis<MemElemInfoPass>().getInfo();

            //----- Internally constructs elastic circuit and adds elastic components -----//

            // Naively building circuit
            CircuitGenerator* circuitGen = new CircuitGenerator(enode_dag, bbnode_dag, OB, MEI);//, CDGP);//, CDG);

	    // calling functions implemented in `AddComp.cpp`
            circuitGen->buildDagEnodes(F);
            circuitGen->fixBuildDagEnodes();
            circuitGen->removeRedundantBeforeElastic(bbnode_dag, enode_dag);
            circuitGen->setGetelementPtrConsts(enode_dag);

#if 1
			circuitGen->buildPostDomTree(F);


			// AYA: 01/10/2021
			circuitGen->Fix_LLVM_PhiPreds();

	    	circuitGen->addStartC();

			// This function takes care of all the memory stuff; mainly operates on the CntrlOrder network
			circuitGen->addMemoryInterfaces(opt_useLSQ);

			/////////////////////////////////////////// TEMPORARILY FOR DEBUGGING!!!
			std::ofstream dbg_file_15;
    		dbg_file_15.open("check_CntrlOrderNetwork.txt");
			for(auto& enode: *enode_dag) {
				if(enode->CntrlOrderPreds->size() > 0) {
					dbg_file_15 << "\n\nNew node of type: " << getNodeDotTypeNew(enode);
					if(enode->BB != nullptr)
						dbg_file_15 << " and BB" << circuitGen->BBMap->at(enode->BB)->Idx  + 1 << " has the following preds in the CntrlOrderPreds\n";
					else 
						dbg_file_15 << " and BB_NULL " << " has the following preds in the CntrlOrderPreds\n";
						
				}
				// loop over the control order network to check its predecessors and successors
				for(auto& pred: *enode->CntrlOrderPreds) {
					if(pred->BB != nullptr)
						dbg_file_15 << getNodeDotTypeNew(pred) << " in BB" << circuitGen->BBMap->at(pred->BB)->Idx  + 1 << ", ";
					else
						dbg_file_15 << getNodeDotTypeNew(pred) << " in BB_NULL, ";
				}
				dbg_file_15 << "\n\n";
			}
			///////////////////////////////////////////////////////////////////////////////
	
			circuitGen->FindLoopDetails(LI);

			/* Aya: 23/05/2022: Applying FPL's algorithm
			 */
			circuitGen->identifyBBsForwardControlDependency();
					
			circuitGen->printCDFG();
			std::ofstream dbg_file_5;
    		dbg_file_5.open("check_final_loops_details.txt");
			circuitGen->debugLoopsOutputs(dbg_file_5);
			dbg_file_5.close();

			// the following function is responsible for adding Merges for re-generation!
			circuitGen->checkLoops(F, LI, CircuitGenerator::data);
			circuitGen->checkLoops(F, LI, CircuitGenerator::constCntrl);
			// 10/08/2022: added the following copy of checkLoops to operate on the CntrlOrder network
			circuitGen->checkLoops(F, LI, CircuitGenerator::memDeps);

			//////// TEMPORARILY FOR DEBUGGING!!
			std::ofstream dbg_file;
    		dbg_file.open("PROD_CONS_BBs.txt");

			for (auto& enode : *enode_dag) {
				for (auto& succ : *enode->CntrlSuccs) {
					dbg_file << "\n---------------------------------------------------\n";
					if(enode->BB!=nullptr && succ->BB!=nullptr)
						dbg_file << "Producer in BB" << aya_getNodeDotbbID(circuitGen->BBMap->at(enode->BB)) << " to a Consumer in BB " << aya_getNodeDotbbID(circuitGen->BBMap->at(succ->BB)) << "\n";
				}
			}
			
			circuitGen->removeExtraPhisWrapper(CircuitGenerator::data);
			circuitGen->removeExtraPhisWrapper(CircuitGenerator::constCntrl);
			circuitGen->removeExtraPhisWrapper(CircuitGenerator::memDeps);

			// AYA: 06/10/2021: added this to make sure the predecessors of any Phi we added above is compatible with the order of the LLVM convention to be able to convert them to muxes later!
			circuitGen->Fix_my_PhiPreds(LI, CircuitGenerator::data);  // to fix my phi_
			circuitGen->Fix_my_PhiPreds(LI, CircuitGenerator::constCntrl);  // to fix my irredundant phi_c1
			circuitGen->Fix_my_PhiPreds(LI, CircuitGenerator::memDeps);
			/////////////////////////////////////////////////

			circuitGen->addSuppress_with_loops(CircuitGenerator::data); // connect producers and consumers of the datapath
			circuitGen->addSuppress_with_loops(CircuitGenerator::constCntrl);  // trigger constants from START through the data-less network
			circuitGen->addSuppress_with_loops(CircuitGenerator::memDeps);


			// TODO AS OF 19/08/2022:
			// 1) Complete and test the insertion of Branches in the CntrlOrder network (i.e, the connection of the true and false succs in addFork)
					// in addBrSuccs to be more precise
			// 2) Extend the following two functions to add support for the networkType network_flag
			// 3) Test the histogram example then test a case of mem deps across 2 different BBs (but no backward edge)
			// 4) Then extend to support backward edge and test it
			// 5) Then integrate with Riya the case of needing elastic JOin
			// 6) Then integrate with smartBuffers
			circuitGen->removeExtraBranchesWrapper(CircuitGenerator::data);
			circuitGen->removeExtraBranchesWrapper(CircuitGenerator::constCntrl);
			circuitGen->removeExtraBranchesWrapper(CircuitGenerator::memDeps);


			// TEMPORARILY FOR DEBUGGING!!!
			//printDotCFG(bbnode_dag, (opt_cfgOutdir + "/" + fname + "_bbgraph.dot").c_str());

			// Aya: 16/06/2022: final version of setMuxes that converts only the Merges at the confluence points not at the loop headers into MUXes
				// this is why internally it does not need to operate on Phi_c because those are never inserted except at loop headers (for regeneration)
			circuitGen->setMuxes_nonLoop(); 

			circuitGen->deleteLLVM_Br();

			// AYA: 15/11/2021: Trigger all constants (except those triggering an unconditional branch) through a Source component in the same BB

			//circuitGen->addSourceForConstants();

			// AYA: 20/03/2022
			//circuitGen->triggerReturn();

		// IMP Note: this function is also important to connect the predecessors of all branches!!!
			circuitGen->addFork();

			//circuitGen->remove_SUPP_COND_Negation();

			// my new implementation of converting merges to muxes
			//circuitGen->setMuxes();

	   // call a function to create the exit node, NEED TO CHECK RETURN LOGIC!!
	      	circuitGen->addExitC();

	      	// Aya: added this temporarily for debugging!!
	      	//circuitGen->FixSingleBBLSQ_special();

			if (opt_buffers)
				circuitGen->addBuffersSimple();//_OLD();

	// Aya: The following is meant for optimizing bit width
       		//if (OptimizeBitwidth::isEnabled()) {
			//	circuitGen->setSizes();
			//}
	
	////////////////////////////////////////////////// DONEEE!

	// These were in the original Dynamatic, not sure if I need to fix and add them!!
		// circuitGen->removeRedundantAfterElastic(enode_dag);

	// this is important in filling the frequency field of BBs needed later in buffers
			circuitGen->setFreqs(F.getName());
						
	        aya_printDotDFG(enode_dag, bbnode_dag, opt_cfgOutdir + "/" + fname + "_graph.dot", opt_serialNumber);
	        printDotCFG(bbnode_dag, (opt_cfgOutdir + "/" + fname + "_bbgraph.dot").c_str());
#endif

          //  circuitGen->sanityCheckVanilla(enode_dag);
        }
    }

    bool runOnFunction(Function& F) override {
		LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
		this->compileAndProduceDOTFile(F, LI); 
    }

    void print(llvm::raw_ostream& OS, const Module* M) const override {}
};
} // namespace

char MyCFGPass::ID = 1;

static RegisterPass<MyCFGPass> Z("mycfgpass", "Creates new CFG pass",
                                 false, // modifies the CFG!
                                 false);

/* for clang pass registration
 */
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

static void registerClangPass(const PassManagerBuilder&, legacy::PassManagerBase& PM) {
    PM.add(new MyCFGPass());
}

static RegisterStandardPasses RegisterClangPass(PassManagerBuilder::EP_EarlyAsPossible,
                                                registerClangPass);

bool fileExists(const char* fileName) {
    FILE* file = NULL;
    if ((file = fopen(fileName, "r")) != NULL) {
        fclose(file);

        return true;

    } else {

        return false;
    }
}
