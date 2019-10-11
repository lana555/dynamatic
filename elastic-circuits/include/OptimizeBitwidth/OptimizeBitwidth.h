#ifndef OPTIMIZE_BITWIDTH_H
#define OPTIMIZE_BITWIDTH_H
#include <map>

#include "assert.h"
#include "info.h"
#include "logger.h"
#include "operations.h"
#include "utils.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/Analysis/AssumptionCache.h>
#include <llvm/Analysis/LazyValueInfo.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/IR/Dominators.h>

using namespace llvm;

/*
 * Main class for computing bit information
 */
class OptimizeBitwidth : public FunctionPass {
public:
    static char ID;
    typedef Info::Stats Stats;
    typedef Info::PHIInfo PHIInfo;
    /*
     * Inside constructor : logger initialization
     */
    OptimizeBitwidth() : FunctionPass(ID) {
        m_logger->setPrefix("OptimizeBitwidth:");
        ec = new std::error_code(errno, std::generic_category());
        print_ostream =
            new raw_fd_ostream(StringRef("debug_func"), *ec, llvm::sys::fs::OpenFlags::F_None);
        m_logger->setStream(*print_ostream);
    }

    void getAnalysisUsage(AnalysisUsage& AU) const override {
        AU.setPreservesCFG();
        AU.addRequired<LoopInfoWrapperPass>();
        AU.addRequired<TargetLibraryInfoWrapperPass>();
        AU.addRequired<AssumptionCacheTracker>();
        AU.addRequired<DominatorTreeWrapperPass>();
    }

    bool runOnFunction(Function& F) override;
    bool forwardIterate(Instruction* I);
    bool backwardIterate(Instruction* I);
    void createBitInfo(Instruction* I);
    void printStats(Function& F);
    bool initialize(Instruction* I);
    bool initialize(Argument* A, Function& F);
    bool isProcessible(Value* value);
    bool countStats(Instruction* I, Info* after, int opcode);
    void intializeStats(Value* v, Info* info, int opcode);
    void setMoreDefined(Info* i1, Info* i2);
    void printMappedStats(Function& F);
    void setMax(Info* to, Info* from);
    void setMin(Info* to, Info* from);
    void printBits(Info* info);
    void printDynBits(Info* info);
    template <typename T> void printObj(T* obj, llvm::raw_fd_ostream& stream);
    void clearResources();

    bool test(void);

    /*
     * functions for working with phi values
     */
    void associatePHI(Function& F);
    void updatePHIValue(PHINode* phi, PHIInfo* info);
    void processLoop(Loop* ll);
    void processBlock(BasicBlock* BB);
    void processInstruction(Loop* ll, PHINode* phi);
    bool checkCompareCondition(Loop* ll, PHINode* phi);
    bool checkInstructionCondition(Loop* ll, PHINode* phi, Value* CMPI);
    bool checkBinary(Value* op, PHINode* phi);
    bool checkCastCondition(PHINode* phi, Instruction* CMPI);
    bool checkSanityCondition(int type, Info* info1, Info* info2);
    int determineBlockOrder(Loop* ll, PHINode* phi);
    void findPHINodes();

    /*
     * set of forward functions
     */
    bool forwardAdd(Instruction* I, Info* cur_val, Info* op0_info, Info* op1_info,
                    bool isSubtraction);
    bool forwardOr(Instruction* I);
    bool forwardAnd(Instruction* I);
    bool forwardShl(Instruction* I);
    bool forwardLShr(Instruction* I);
    bool forwardAShr(Instruction* I);
    bool forwardZExt(Instruction* I);
    bool forwardSExt(Instruction* I);
    bool forwardSub(Instruction* I);
    bool forwardSelect(Instruction* I);
    bool forwardPHI(Instruction* I);
    bool forwardUDiv(Instruction* I);
    bool forwardSDiv(Instruction* I);
    bool forwardSRem(Instruction* I);
    bool forwardURem(Instruction* I);
    bool forwardTrunc(Instruction* I);
    bool forwardXor(Instruction* I);
    bool forwardMul(Instruction* I);

    /*
     * set of backward functions
     */
    inline void backwardRedirect(Info* from, Info* to);
    void backwardMul(Instruction* I, Info* i_info, Info* user_info, Info* cur_max);
    void backwardShl(Instruction* I, Info* i_info, Info* user_info, Info* cur_max);
    void backwardExt(Info* i_info, Info* user_info, Info* cur_max);
    void backwardAdd(Info* i_info, Info* cur_max, Info* user_info);
    void backwardShr(Value* v, Info* i_info, Info* cur_max);
    void backwardXor(Instruction* I, Info* i_info, Info* user_info, Info* cur_max);
    void countBackwardStats(Instruction* I, Info* after, int op_code);
    void eraseValue(Info* info);
    /*
     * for verilog code generation
     */
    unsigned getSize(Value* v);
    bool isInLoopMap(Value* v);
    int getMapSize();
    unsigned getInitialSize(Value* v);
    /*for debug print of map internals*/
    void printMap();
    void checkCorrectness();

    /*for logging*/
    std::error_code* ec;
    raw_fd_ostream* print_ostream;
    virtual ~OptimizeBitwidth() { print_ostream->close(); }

private:
    typedef std::map<Value*, Info*> bit_map;
    bit_map bit_info; // map from llvm Value to Info structures, see info.h
    std::map<PHINode*, PHIInfo*>
        loop_map;                      // map from llvm PHINodes to PHIInfo structures, see info.h
    std::set<PHINode*> phi_s;          // temporary set for phi nodes processing
    std::map<Value*, Stats*> stat_map; // map from llvm Value to Stats structures, see info.h
    std::map<std::string, int> op_map;
    std::map<std::string, int> op_all_map;
    std::map<std::string, int> op_backward_map; // three maps to keep overall statistics on
                                                // eliminated bits for all operations
    Logger* m_logger = new Logger();
};

#endif // OPTIMIZE_BITWIDTH_H