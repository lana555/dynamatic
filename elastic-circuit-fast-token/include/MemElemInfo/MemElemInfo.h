/*
 * Author: Atri Bhattacharyya
 */

#pragma once

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include <set>

using namespace llvm;

/* Forward declaration */
class IndexAnalysis;

/* An LSQset is a set instructions that have *possible* memory
 * dependences in a function and hence, need runtime address
 * aliasing via an LSQ */
typedef struct LSQset {
    LSQset(const Value* V, const Instruction* I0, const Instruction* I1) {
        base = V;
        insts.insert(I0);
        insts.insert(I1);
    }
    LSQset(const Value* V) { base = V; }

    const Value* base;
    std::set<const Instruction*> insts;

    using iterator = std::set<const Instruction*>::iterator;
    iterator begin() { return insts.begin(); }
    iterator end() { return insts.end(); }
} LSQset;

class MemElemInfo {
    friend class MemElemInfoPass;

public:
    MemElemInfo()  = default;
    ~MemElemInfo() = default;

    /// Returns a set of references to LSQsets, each of which contains those
    /// instructions whose hardware components should be connected to the same
    /// LSQ
    const std::set<LSQset*>& getLSQList() const { return LSQList; }

    /// Returns a reference to the LSQset containing
    const LSQset& getInstLSQ(const Instruction* I) const { return *instToLSQ.at(I); }

    /// To query whetherB B needs to be connected to any LSQs
    bool BBhasLSQ(const BasicBlock* BB) const { return BBtoLSQs.find(BB) != BBtoLSQs.end(); }

    /// Get a set of LSQs to which the block connects
    const std::set<LSQset*>& getBBLSQs(const BasicBlock* BB) const { return BBtoLSQs.at(BB); }

    ///  Query whether the component forI needs to be connected to a LSQ
    bool needsLSQ(const Instruction* I) const { return (instToLSQ.find(I) != instToLSQ.end()); }

private:
    std::set<LSQset*> LSQList;
    std::map<const Instruction*, LSQset*> instToLSQ;
    std::map<const Value*, LSQset*> baseToLSQ;
    std::map<const BasicBlock*, std::set<LSQset*>> BBtoLSQs;
    std::vector<const Instruction*> otherInsts;

    ///  List of instructions within some loop
    std::vector<const Instruction*> loopInstrSet;

    void finalize();
};

class MemElemInfoPass : public FunctionPass {
public:
    static char ID;
    MemElemInfoPass() : FunctionPass(ID) {}
    ~MemElemInfoPass() override = default;

    bool runOnFunction(Function& F) override;
    void print(llvm::raw_ostream& OS, const Module* M) const override;
    void releaseMemory() override;
    void getAnalysisUsage(AnalysisUsage& AU) const override;

    MemElemInfo& getInfo();

private:
    IndexAnalysis* IA;
    AAResults* AA;
    MemElemInfo MEI;

    /// Memory metadata for top-level loops
    struct TLLMeta {
        TLLMeta()  = default;
        ~TLLMeta() = default;

        std::set<const Instruction*> rdInsts;
        std::set<const Instruction*> wrInsts;
        std::map<const Instruction*, int> instToScop;
    };

    /// List of metadata for top-level loops
    std::vector<struct TLLMeta> loopMeta;

    /// Creates TLLMeta structure for each top-level loop
    void processLoop(Loop* L);

    /// For each loop, finds intersecting accesses, then arranges them into sets
    void createSets(struct TLLMeta& LM);

    /// Determine whether memory instructions @p A and @p B share the same array base
    bool equalBase(const Instruction* A, const Instruction* B) const;
};
