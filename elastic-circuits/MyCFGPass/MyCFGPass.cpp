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

struct timeval start, end;

std::string fname;
static int indx_fname = 0;

void set_clock() { gettimeofday(&start, NULL); }

double elapsed_time() {
    gettimeofday(&end, NULL);

    double elapsed = (end.tv_sec - start.tv_sec);
    elapsed += (double)(end.tv_usec - start.tv_usec) / 1000000.0;
    return elapsed;
}

using namespace llvm;

namespace {
class MyCFGPass : public llvm::FunctionPass {

public:
    static char ID;

    MyCFGPass() : llvm::FunctionPass(ID) {}

    virtual void getAnalysisUsage(AnalysisUsage& AU) const {
        // 			  AU.setPreservesCFG();
        AU.addRequired<OptimizeBitwidth>();
        OptimizeBitwidth::setEnabled(true); // enable OB

        AU.addRequired<MemElemInfoPass>();
    }

    void compileAndProduceDOTFile(Function& F) {
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
            CircuitGenerator* circuitGen = new CircuitGenerator(enode_dag, bbnode_dag, OB, MEI);

            circuitGen->buildDagEnodes(F);

            circuitGen->fixBuildDagEnodes();

            circuitGen->formLiveBBnodes();

            circuitGen->removeRedundantBeforeElastic(bbnode_dag, enode_dag);

            circuitGen->setGetelementPtrConsts(enode_dag);

            circuitGen->addPhi();

            printDotDFG(enode_dag, bbnode_dag, opt_cfgOutdir + "/" + fname + "_graph.dot", done, opt_serialNumber);

            circuitGen->phiSanityCheck(enode_dag);

            circuitGen->addFork();

            circuitGen->forkSanityCheck(enode_dag);

            circuitGen->addBranch();

            circuitGen->branchSanityCheck(enode_dag);

            if (get_pragma_generic("USE_CONTROL")) {
                circuitGen->addControl();
            }

            circuitGen->addMemoryInterfaces(opt_useLSQ);
            circuitGen->addControl();

            circuitGen->addSink();
            circuitGen->addSource();

            set_clock();
            if (opt_buffers)
              circuitGen->addBuffersSimple();

            printf("Time elapsed: %.4gs.\n", elapsed_time());
            fflush(stdout);

            Instruction* instr;
            for (auto& enode : *enode_dag) {
                if (enode->Instr != NULL) {
                    instr = enode->Instr;
                    break;
                }
            }

            std::string dbgInfo("a");

            circuitGen->setMuxes();

            circuitGen->setBBIds();

            circuitGen->setFreqs(F.getName());

            circuitGen->removeRedundantAfterElastic(enode_dag);
            
            if (OptimizeBitwidth::isEnabled())
                circuitGen->setSizes();

            done = true;

            fflush(stdout);

            printDotDFG(enode_dag, bbnode_dag, opt_cfgOutdir + "/" + fname + "_graph.dot", done, opt_serialNumber);

            printDotCFG(bbnode_dag, (opt_cfgOutdir + "/" + fname + "_bbgraph.dot").c_str());

            circuitGen->sanityCheckVanilla(enode_dag);
        }
    }

    bool runOnFunction(Function& F) override { this->compileAndProduceDOTFile(F); }

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
