#include "ElasticPass/CircuitGenerator.h"
#include "ElasticPass/Utils.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"

// Detect identical MUXes (by checking that they have the same inputs and outputs! Especially in control, )


// Detect identical Branches 


// My goal here is that for each (producer, consumer) pair, I want to enumerate paths and calculate a boolean expression for NOTOKEN, then pass it to Quine-McCluskey and Shannon's
	





