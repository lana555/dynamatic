/*
 * Author: Atri Bhattacharyya
 */

#pragma once

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Instructions.h"
#include <set>

using namespace llvm;

class TokenDependenceInfo {
public:
    TokenDependenceInfo(const LoopInfo& _LI) : LI(_LI) {}

    /// Query whether I_B is dependant on I_A: I_A -D-> I_B
    /// Returns true if every token coming to I_A has passed through I_B
    /// without traversing any BB-edge that would increment common induction
    /// variables
    bool hasTokenDependence(const Instruction* I_B, const Instruction* I_A);

    /// Query whether I_B is reversely dependant on I_A: I_A -RD-> I_B
    /// Returns true if every token coming to I_A will pass through I_B
    /// without traversing any BB-edge that would increment common induction
    /// variables
    bool hasRevTokenDependence(const Instruction* I_A, const Instruction* I_B);

    bool hasControlDependence(const Instruction* I1, const Instruction* I0);

private:
    const LoopInfo& LI;
};
