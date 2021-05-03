#ifndef OPTIMIZE_BITWIDTH_H
#define OPTIMIZE_BITWIDTH_H

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <ostream>
#include <iostream>
#include <cassert>
#include <system_error>

#include <llvm/IR/Argument.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>
#include <llvm/Pass.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Support/raw_ostream.h>

#include "info.h"
#include "deps.h"
#include "logger.h"
#include "operations.h"

using namespace llvm;

/*
 * Main class for computing bit information
 */
class OptimizeBitwidth final : public llvm::FunctionPass {
public:
    using Stats = Info::Stats;
    // a forward function takes the instruction's output, and its operands
    using forward_func  = std::function<void(Info* iOut, const Info** iOps)>;

    //acc must be reduced as if it was iOps[usedOp], using iOut and the others iOps if needed
    using backward_func = std::function<void(Info* acc, const Info* iOut, const Info** iOps, unsigned int targetOp)>;

    static const int MAX_ITERATIONS = 5;

    // set or unset to enable/disable the pass
    static void setEnabled(bool enable);
    static bool isEnabled();

private:
    static bool IS_PASS_ENABLED;

public: // ElasticPass interface
    OptimizeBitwidth();
    virtual ~OptimizeBitwidth();

    // Returns the size computed during the pass
    unsigned getSize(const Value* v) const;

public: // LLVM Interface
    static char ID;
    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    void print(llvm::raw_ostream &O, const llvm::Module *M) const override;

    // called by PassManager
    // Runs the forward/backward passes on F until convergence
    bool runOnFunction(Function& F) override;

private: // forward functions (see forward.cpp)
    bool forward(const Instruction* I);

    // Constructs most of the forward functions (lambdas) 
    //and binds the more complicated ones
    //EXCEPTION : forwardPHI, which will have to be handled separately by forward iterate
    void constructForwardFunctions();

    void forwardAdd(Info* out, const Info* iOp0, const Info* iOp1, bool carryIn);
    void forwardPHI(Info* out, const PHINode* phi);

private: // backward functions (see backward.cpp)
    bool backward(const Value* v);
    Info computeBackwardAccomodator(const Value* v, const Info* out);

    // Constructs the backward functions (lambdas) 
    //minus some exeptions below
    void constructBackwardFunctions();
    void backwardXor(Info* acc, const Instruction* I, unsigned int targetOp);

private: // Boring initialization stuff

    void init(const Function& F);

    // Initializes an Info struct, for the given Value, while checking it was not already done
    bool initializeInfo(const Value* v, const DataLayout& DL);
    // Initializes Info structs for the given Instruction 
    void initializeInst(const Instruction* I, const DataLayout& DL);

    // Sets the dependencies between Function Arguments and Values of the CallInst
    void initializeArgDeps(const CallInst* callI);
    // sets the argument to an accomodator of the current state of dependencies
    //returns true if its Info changed
    bool updateArgumentFromDependencies(const Argument* a);

    // Sets the dependencies on loop's iterators and their bounds
    void initializeLoop(const Loop* L, const LoopInfo& LI, DominatorTree& DT);

private: // Statistics 

    // Initializes statistics
    void initializeStats(const Value* v, const Info* info);

    void updateForwardStats(const Instruction* I, const Info* before, const Info* after);
    void updateBackwardStats(const Value* v, const Info* before, const Info* after);

    template <class OStream>
    void dumpInfos(const Function& F, OStream& os) const;
    void dumpInfos(const Function& F) const;

    template <class OStream>
    void printStats(const Function& F, OStream& os, unsigned iterations) const;

public: // logging
    Logger logger;
    std::error_code ec;
    llvm::raw_ostream* print_ostream;

private: // make public for tests
    // maps Instruction::opcode -> forward/backward function
    std::map<unsigned int, forward_func > forward_fs;
    std::map<unsigned int, backward_func> backward_fs;

    // map from llvm::Value* to Info*, ie. a bimask, see info.h
    std::map<const Value*, Info*> bit_info;
    // map from llvm::Argument* to a list of their dependencies
    //(ie. all Info* they can be replaced with on a call), see deps.h (DISABLED)
    std::map<const Argument*, ArgDeps> arg_deps;
    
    // map from llvm::Value* to a LoopDependencies object,
    //to bound loop iterators and 
    std::map<const Value*, LoopDeps> loop_deps;

    // for statistics
    std::map<const Value*, Stats*> bit_stats;
};

template <class OStream>
void OptimizeBitwidth::dumpInfos(const Function& F, OStream& os) const {

    os << "Function name : " << F.getName().data() << "\n\n";

    int j = 0;
    for (const_inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {

        os << j << " : " << I->getOpcodeName()
            << " (name: " << I->getName() << ')'
            << " (#operands: " << I->getNumOperands() << ')' << '\n';

        const Info* info = bit_info.at(&*I);
        // print output's info, if any
        if (info != nullptr) {
            os << "  Out (" << I->getName().str() << "): "
                << info->toString() << '\n';
        }
        else {
            os << "  No output value (void)" << '\n';
        }

        // print operands' info
        for (unsigned int i = 0; i < I->getNumOperands(); i++) {
            info = bit_info.at(I->getOperand(i));

            // TODO : br take label as operand, -> nullptr Info?
            if (info != nullptr)
                os << "  Op" << i << " (" << I->getOperand(i)->getName().str() << "): "
                    << info->toString() << '\n';
            else
                os << "  nullptr" << '\n';
        }

        j += 1;
    }

    os << '\n';
}

template <class OStream>
void OptimizeBitwidth::printStats(const Function& F, OStream& os, unsigned iterations) const {
    
    os << "OptimizeBitwidth : Optimization of function " << F.getName().str() 
        << " finished in " << iterations << " iterations.\n";
    os << "Printing stats (for more details on states for each individual value, use dumpInfos).\n";

    unsigned saved_fdw = 0, saved_bwd = 0;
    unsigned total_opti = 0, original = 0;

    for (const auto& pair : bit_stats) {
        saved_fdw += pair.second->forward_opt;
        saved_bwd += pair.second->backward_opt;
    }

    for (const auto& pair : bit_info) {
        if (pair.second != nullptr) {
            total_opti += pair.second->left + 1;
            original   += pair.second->cpp_bitwidth;
        }
    }

    os << "Saved bits during forward passes : " << saved_fdw << '\n';
    os << "Saved bits during backward passes : " << saved_bwd << '\n';
    os << "Saved bits in constants : " << original - total_opti - saved_fdw - saved_bwd << '\n'; 
    os << "Used bits after OB : " << total_opti <<  ", vs. Originaly used bits : " << original << '\n';
    os << " => Reduction of used bits : " << static_cast<float>(total_opti) / original * 100 << '\n';
}

#endif // OPTIMIZE_BITWIDTH_H