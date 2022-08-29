#include "OptimizeBitwidth/OptimizeBitwidth.h"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/ValueMap.h>

#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Support/FileSystem.h>

// these source files implement forward, backward related functions
//each are slightly commented, for further explaination please refer to the bitwidth analysis paper
#include "forward.cpp"
#include "backward.cpp"

// even addresses use a maximum of 32 bits in dynamatic
#define MAX_BITWIDTH 32

// actual value is not important, llvm uses address to identify
char OptimizeBitwidth::ID = 1;
// Register this OB as a llvm::Pass
static RegisterPass<OptimizeBitwidth> Y("OptimizeBitwidth", // cmd_line_name
                                        "OptimizeBitwidth Pass", // display_name
                                        true,  // Doesn't modify CFG
                                        true); // Analysis Pass

// set this flag before the Pass is called by LLVM to enable/disable it
bool OptimizeBitwidth::IS_PASS_ENABLED = true;

void OptimizeBitwidth::setEnabled(bool enabled) { IS_PASS_ENABLED = enabled; }
bool OptimizeBitwidth::isEnabled() { return IS_PASS_ENABLED; }

OptimizeBitwidth::OptimizeBitwidth() 
    : FunctionPass(ID), logger(), 
    ec(errno, std::generic_category()), 
    print_ostream(new llvm::raw_fd_ostream("-", ec, llvm::sys::fs::OpenFlags::F_None))
{
    logger.setPrefix("OB: ");
    logger.disable();

    logger.setStream(print_ostream);

    constructForwardFunctions();
    constructBackwardFunctions();
}
OptimizeBitwidth::~OptimizeBitwidth() {

    delete print_ostream;

    for (auto pInfo : bit_info)
        delete pInfo.second;

    for (auto pStats : bit_stats)
        delete pStats.second;
}

unsigned OptimizeBitwidth::getSize(const Value* v) const {
    assert (v != nullptr);
    
    const Info* info = bit_info.at(v);

    if (info != nullptr) {
        unsigned bitwidth = bit_info.at(v)->left + 1;

        //std::cout << "getSize('" << bit_stats.at(v)->name << "') -> " << bitwidth << std::endl;
        return bitwidth;
    }
    else {
        //std::cout << "getSize('" << v->getName() << "') on void/label -> 0" << std::endl;
        return 0;
    }
}

void OptimizeBitwidth::print(llvm::raw_ostream &O, const llvm::Module *M) const { }

void OptimizeBitwidth::getAnalysisUsage(AnalysisUsage& AU) const {
    //AU.setPreservesCFG();
    AU.setPreservesAll(); // we don't change anything at all, only observe the functions

    //AU.addRequired<LoopPass>(); // is a TransformationPass, not an AnalysisPass
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<DominatorTreeWrapperPass>();
}


bool OptimizeBitwidth::runOnFunction(Function& F) {

    if (!isEnabled())
        return false;

    if (F.empty() || F.getName() == "main")
        return false;

    init(F);

    logger.log("START OF NEW FUNCTION PASS : %", F.getName());
    logger.pushIndent();

    // Algorithm : 
    //do a forward pass through all the instructions, then a backward pass
    //reiterate while there are any changes in bitwidths
    bool run_again = true;
    int iteration = 0;
    //while (run_again && iteration < MAX_ITERATIONS) {
    while (run_again) {
        run_again = false;

        logger.log("runOnFunction : iteration %", iteration);

        logger.log("Forward pass");
        logger.pushIndent();

        // run forward pass on every instruction
        for (auto I = inst_begin(F), E = inst_end(F); I != E; ++I)
            run_again |= forward(&*I);
            
        logger.popIndent();

        logger.log("Backward pass");
        logger.pushIndent();

        // run backward pass on every value, don't forget arguments!
        for (auto BB = F.getBasicBlockList().rbegin(), bbEnd = F.getBasicBlockList().rend() ; BB != bbEnd ; ++BB)
            for (auto I = BB->rbegin(), iEnd = BB->rend() ; I != iEnd ; ++I)
                run_again |= backward(&*I);
                
        for (const Argument& a : F.args())
            run_again |= backward(&a);

        logger.popIndent();

        iteration += 1;
    }
    logger.popIndent();

    logger.log("PASS FINISHED IN % ITERATIONS : %", iteration, F.getName());
    
    printStats(F, *logger.getStream(), iteration);
    //dumpInfos(F, *logger.getStream());

    return false; // we didn't modify F
}


void OptimizeBitwidth::init(const Function& F) {
    logger.log("1. Initialization");
    
    DataLayout DL(F.getParent());
    
    logger.log("1.1 Initializing global values...");
    logger.pushIndent();

    for (const GlobalVariable& G : F.getParent()->getGlobalList())
        initializeInfo(&G, DL);

    logger.popIndent();
    
    logger.log("1.2 Initializing instructions for all functions...");
    logger.pushIndent();

    for (auto I = inst_begin(F), Iend = inst_end(F) ; I != Iend ; ++I)
        initializeInst(&*I, DL);

    logger.popIndent();


    // Arguments should be reduced to the union of all Values
    // ex: if f is called several times : f(15) ; f(28) ; f(-3)
    //  -> the Argument's info can be reduced to bitmask needed for [-3 ; 28]
    logger.log("1.3 Initializing Arguments of functions...");
    logger.log("1.3.1 Creating empty Arguments and their dependencies...");
    logger.pushIndent();

    for (const Argument& a : F.args()) {
        initializeInfo(&a, DL);
        const Info* iA = bit_info.at(&a);
        arg_deps.insert( { &a, ArgDeps(iA->cpp_bitwidth) });
    }

    logger.popIndent();

    //logger.log("1.3.2 Setting up dependencies of Arguments...");
    //logger.pushIndent();
    //for (const_inst_iterator I = inst_begin(F), Iend = inst_end(F) ; I != Iend ; ++I)
    //    if (const CallInst* callI = dyn_cast<CallInst>(&*I))
    //        initializeArgDeps(callI);
    //logger.popIndent();

    // Reducing the Arguments once before the algorithm, to benefit from constants
    //logger.log("1.3.3 Reducing arguments to fit all dependencies...");
    //for (const Function& F : M)
    //    for (const Argument& a : F.args())
    //        updateArgumentFromDependencies(&a);


    logger.log("1.4 Setting up LoopBounds");
    logger.pushIndent();

    LoopInfo& LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    DominatorTree& DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    for (const Loop* L : LI)
        initializeLoop(L, LI, DT);

    logger.popIndent();
}


bool OptimizeBitwidth::initializeInfo(const Value* v, const DataLayout& DL) {
    assert(v != nullptr);

    if (bit_info.find(v) != bit_info.end())
        return false;

    if (auto* cInt = dyn_cast<ConstantInt>(v)) {
        logger.log("Value is a constant");
        // if constant, set the bitwidth and overwrite bit_vector,
        //as we know its exact value
        uint8_t bitwidth = cInt->getValue().getBitWidth();
        Info* info = new Info(bitwidth);
        
        intmax_t value = *cInt->getValue().getRawData();
        info->unknown_bits = op::S_ZERO;
        info->bit_value    = value;

        info->reduceBounds();

        bit_info[v] = info;
    }
    else if (v->getType()->isSized()) {
        logger.log("Value type is non-void");
        
        const uint8_t bitwidth = std::min(DL.getTypeSizeInBits(v->getType()), uint64_t(MAX_BITWIDTH));

        bit_info[v] = new Info(bitwidth);
    }
    else if (v->getType()->isLabelTy()) {
        logger.log("Value type is label, aka BasicBlock itself (%): used as operand in branchs, Phi", v->getName());
        
        bit_info[v] = nullptr;
    }
    else {
        logger.log("Value type is void (%). Setting nullptr in bit_info", v->getName());
        bit_info[v] = nullptr;
    }

    initializeStats(v, bit_info[v]);
    if (bit_info[v] != nullptr)
        bit_info[v]->assertValid();

    return true;
}

// Initializes Info structs for the given Instruction 
//(ie. one for its output, and one for each operand, if not already created)
void OptimizeBitwidth::initializeInst(const Instruction* I, const DataLayout& DL) {
    assert (I != nullptr);
    
    logger.log("Instruction opcode name : % ", I->getOpcodeName());
    
    logger.pushIndent();

    logger.log("Initializing return value (%)", I->getName());
    if (!initializeInfo(I, DL))
        logger.log("  Already initialized.");

    logger.log("Initializing operands - if any");
    for (unsigned int i = 0; i < I->getNumOperands(); i++) {
        const Value* operand = I->getOperand(i);

        logger.pushIndent();
        if (initializeInfo(operand, DL))
            logger.log("Op% (%) initialized.", i, operand->getName());
        else
            logger.log("Op% (%) already initialized.", i, operand->getName());

        logger.popIndent();
    }
    logger.popIndent();
}


// Tries to find LoopBounds & dependencies, and to setup them
void OptimizeBitwidth::initializeLoop(const Loop* L, const LoopInfo& LI, DominatorTree& DT) {
    // take care of subloops
    logger.pushIndent();
    for (const Loop* l : L->getSubLoops())
        initializeLoop(l, LI, DT);
        
    logger.popIndent();

    // Lana 7.6.2021. Removed LCSSA check
    //if (!L->isLCSSAForm(DT)) {
    //    logger.log("Loop not in LCSSA form. Cannot reduce iterators.");
    //    return;
    //}

    BasicBlock* header = L->getHeader();
    BasicBlock* preheader = L->getLoopPredecessor();
    BasicBlock* latch = L->getLoopLatch();

    if (preheader == nullptr || header == nullptr || latch == nullptr) {
        logger.log("Loops should be in Loop-Simplify-Form. In particular, "
            "they should have a header, a preheader and an unique backedge.");
        return;
    }

    const BranchInst* BI = dyn_cast<BranchInst>(latch->getTerminator());
    if (BI == nullptr || !BI->isConditional()) {
        logger.log("Loop doesn't have any conditional branches.");
        return;
    }
        
    const ICmpInst* CI = dyn_cast<ICmpInst>(BI->getCondition());
    if (CI == nullptr) {
        logger.log("Comparison is not Integer.");
        return;
    }

    const PHINode* indVar = nullptr;
    const Instruction* stepInst = nullptr;
    int boundedCmpOp = -1;

    for (int i = 0 ; i < 2 ; ++i) {
        Value* op = CI->getOperand(i);

        if (isa<Constant>(op))
            continue;

        for (const PHINode& phi : header->phis()) {
            // case 1: phi itself used in cmp
            // indVar = phi[{0, preheader}, {stepInst, latch}]
            // stepInst = indVar + step
            // finalValue = ... stuff
            // cmp = IndVar < FinalValue
            if (op == &phi) {
                if (boundedCmpOp != -1) {
                    logger.log("Found conflicting phis in loop header.");
                    return;
                }

                indVar = &phi;
                stepInst = dyn_cast<Instruction>(phi.getIncomingValueForBlock(latch));
                boundedCmpOp = i;
            }

            // case 2: stepInst used in cmp
            // IndVar = phi[{0, preheader}, {StepInst, latch}]
            // StepInst = IndVar + step
            // cmp = StepInst < FinalValue
            for (const Value* v : phi.operand_values()) {
                if (op == v) {
                    if (boundedCmpOp != -1) {
                        logger.log("Found conflicting phis in loop header.");
                        return;
                    }

                    indVar = &phi;
                    stepInst = dyn_cast<Instruction>(v);
                    boundedCmpOp = i;
                }
            }
        }
    }

    if (boundedCmpOp == -1 || stepInst == nullptr) {
        logger.log("Couldn't find a phi for the loop's iterator.");
        return;
    }

    Value* stepOffset;
    if (stepInst->getOperand(0) == indVar)
        stepOffset = stepInst->getOperand(1);
    else if (stepInst->getOperand(1) == indVar)
        stepOffset = stepInst->getOperand(0);
    else {
        logger.log("Step instruction is not using indVar as an operand.");
        return;
    }

    LoopDeps deps(bit_info, indVar->getIncomingValueForBlock(preheader), CI->getOperand(boundedCmpOp ^ 1),
        stepInst, stepOffset, CI);

    loop_deps.insert({ indVar, deps });
    loop_deps.insert({ stepInst, deps });

    logger.log("LoopDeps set for indVar '%' and stepInst '%'.", indVar->getName(), stepInst->getName());
}


// Initializes statistics
//Used only for statiscal purpose
void OptimizeBitwidth::initializeStats(const Value* v, const Info* info) {
    assert(v != nullptr);
    
    Stats* s = new Stats();
    if (auto* instr = dyn_cast<Instruction>(v)) {
        s->opcode = instr->getOpcode();
        s->name = instr->hasName() ? instr->getName().str() : instr->getOpcodeName();
    }
    else if (isa<ConstantInt>(v)) {
        s->opcode = -1;
        s->name = "Constant";
        if (v->hasName())
            s->name += ' ' + v->getName().str();
    }
    else if (isa<Argument>(v)) { 
        s->opcode = -2;
        s->name = "Arg " + v->getName().str();
    }
    else if (v->getType()->isLabelTy()) {
        s->opcode = -3;
        s->name = "Label " + v->getName().str();
    }
    else {
        s->opcode = -1000;
        s->name = "Unknown type Value " + v->getName().str();
    }
    s->forward_opt = 0;
    s->backward_opt = 0;

    bit_stats[v] = s;
}

void OptimizeBitwidth::updateForwardStats(const Instruction* I, const Info* before, const Info* after) {
    assert(bit_stats.find(I) != bit_stats.end());
    Stats* stats = bit_stats.at(I);

    logger.log("Forward Statistics : %", stats->name);
    logger.pushIndent();

    int dL = before->left - after->left;
    int dS = before->sign_ext - after->sign_ext;
    int dR = after->right - before->right;

    if (dL != 0 || dS != 0 || dR != 0) {
        logger.log("Instruction '%' gained % bits in this iteration : % => %", 
            stats->name, dL + dR, before->toString(), after->toString());
    }
    //stats->forward_opt += before->sign_ext - after->sign_ext;
    stats->forward_opt += before->left - after->left;

    logger.popIndent();
}
void OptimizeBitwidth::updateBackwardStats(const Value* v, const Info* before, const Info* after) {
    assert(bit_stats.find(v) != bit_stats.end());
    Stats* stats = bit_stats.at(v);

    logger.log("Backward Statistics : %", stats->name);
    logger.pushIndent();

    int dL = before->left - after->left;
    int dS = before->sign_ext - after->sign_ext;
    int dR = after->right - before->right;

    if (dL != 0 || dS != 0 || dR != 0) {
        logger.log("Value '%' gained % bits in this iteration : % => %", 
            stats->name, dL + dR, before->toString(), after->toString());
    }
    
    //stats->backward_opt += before->sign_ext - after->sign_ext;
    stats->backward_opt += before->left - after->left;

    logger.popIndent();
}


// Prints statistics on every Value* inside the given Function
//into the file "OB_stats_function.txt"
void OptimizeBitwidth::dumpInfos(const Function& F) const {
    std::string filename { ("OB_stats_" + F.getName() + ".txt").str() };

    logger.log("Writing Stats in %", filename);
    std::ofstream stat(filename, std::ios_base::out);

    dumpInfos(F, stat);
    
    stat.close();
}







// UNUSED:
/*
bool OptimizeBitwidth::runOnModule(Module& M) {

    init(M);
    
    logger.log("2. Running algorithm on functions.");
    logger.pushIndent();

    std::set<const Function*> to_run;
    for (const Function& F : M)
        to_run.insert(&F);

    while (!to_run.empty()) {
        
        // run all function at least once
        for (const Function* F : to_run)
            runOnFunction(*F);

        // now, we must re-run functions that had a change in arguments,
        //because we gained Info on their Argument in calls from other functions
        to_run.clear();

        //for (const Function& F : M)
        //    for (const Argument& arg : F.args())
        //        if (updateArgumentFromDependencies(&arg)) {
        //            logger.log("Argument % of Function % changed. Function will be re-analyzed to see if we can profit from it.",
        //                bit_stats[&arg]->name, F.getName());
        //            to_run.insert(&F);
        //        }
    }

    logger.popIndent();

    return false; // we didn't modify the Module
}
// Sets the dependencies between Function Arguments, and the Values of the CallInstruction
void OptimizeBitwidth::initializeArgDeps(const CallInst* callI) {

    const unsigned int num_args = callI->getNumArgOperands();
    Function* called = callI->getCalledFunction();

    logger.log("Found a call : %. Function_called= %. Nb_args= %.", 
        bit_stats[callI]->name, called->getName(), num_args);

    assert(called->arg_size() == num_args);

    logger.pushIndent();
    for (unsigned int i = 0 ; i < num_args ; ++i) {
        const Argument* arg = called->arg_begin() + i;

        const Value* v = callI->getArgOperand(i);
        const Info* iVal = bit_info[v];
        assert(iVal != nullptr);

        logger.log("Argument% (%) has a dependency on %", i, arg->getName(), bit_stats[v]->name);
        
        arg_deps.at(arg).addDependency(iVal);
    }
    logger.popIndent();
}

bool OptimizeBitwidth::updateArgumentFromDependencies(const Argument* A) {
    Info* iArg = bit_info.at(A);
    ArgDeps& argDep = arg_deps.at(A);

    Info computedIArg = argDep.computeCurrentArgInfo();
    if (argDep.snapshot() != computedIArg) {
        // update the Argument
        *iArg = computedIArg;
        // update the snapshot. 
        //Can't use iArg as snapshot, because the iArg itself may be changed 
        //during the backward function pass, which could lead to false positives/negatives
        argDep.setSnapshot(computedIArg);
        return true;
    }
    return false;
}
*/