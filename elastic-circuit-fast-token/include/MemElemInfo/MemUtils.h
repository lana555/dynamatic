#pragma once

#include "llvm/IR/Instructions.h"

using namespace llvm;

/// Find the array base for the memory access instruction I
const Value* findBase(const Instruction* I);
