/**
 * Elastic Pass supporting functions
 */

#pragma once

#include <cassert>

#define PRINT_FILE "print.txt"
#define DOT_FILE "graph.dot"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <list>
#include <regex>
#include <set>
#include <sstream>
#include <stack>
#include <unordered_map>
#include <utility>
#include <vector>

/* for debug support */
#define DEBUG_TYPE "duplicate-bb"
#include "llvm/IR/DebugInfo.h"
#include "llvm/Support/Debug.h"

/* for stat support */
#include "llvm/ADT/Statistic.h"

#include "llvm/IR/CFG.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/RandomNumberGenerator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "MemElemInfo/MemElemInfo.h"
#include "OptimizeBitwidth/OptimizeBitwidth.h"

#include "Nodes.h"
#include "Utils.h"

using namespace llvm;

// default data and address bitwidths
#define DATA_SIZE 32
#define ADDR_SIZE 32
