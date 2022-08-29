#include <vector>
#include <map>

#include <llvm/ADT/APInt.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>

#include "info.h"


// Stores all Info* upon which an Argument can depend
//If some Info* dependency changed, then the function using 
//the argument will have to be re-analyzed
class ArgDeps final {
public:
    ArgDeps(uint8_t arg_bitwidth);

    // add an Info* to the dependencies
    void addDependency(const Info* i);

    // Returns the Argument's Info from the CURRENT state of every dependencies :
    //it's the smallest Info that can fit them all
    Info computeCurrentArgInfo() const;

    inline void setSnapshot(const Info& i) { mSnapshot = i; }
    inline Info snapshot() const { return mSnapshot; }

private:
    std::vector<const Info*> mDeps;
    Info mSnapshot;
};

// Stores all Info* upon which a Loop counter can depend
class LoopDeps final {
public:
    using Predicate = llvm::CmpInst::Predicate;
    using MapInfo = std::map<const llvm::Value*, Info*>;

public:
    LoopDeps(const MapInfo& bit_info, const llvm::Value* initVal, const llvm::Value* boundVal, 
        const llvm::Instruction* stepInst, const llvm::Value* stepOffset, const llvm::ICmpInst* cmpInst);

    Info computeCurrentIteratorInfo() const;

    inline const llvm::Value* initValue() const { return mInitVal; }
    inline Info* initInfo() const { return mMapInfo.at(mInitVal); }

    inline const llvm::Value* boundValue() const { return mBoundVal; }
    inline Info* boundInfo() const { return mMapInfo.at(mBoundVal); }

    inline const llvm::Instruction* stepInst() const { return mStepInst; }
    inline Info* stepInstInfo() const { return mMapInfo.at(mStepInst); }

    inline const llvm::Value* stepOffset() const { return mStepOffset; }
    inline Info* stepOffsetInfo() const { return mMapInfo.at(mStepOffset); }

    Predicate getLatchPredicate() const;

    bool isStepOffsetConstant() const;
    intmax_t stepOffsetValue() const;

private:
    const MapInfo& mMapInfo;

    const llvm::Value* const mInitVal;
    const llvm::Value* const mBoundVal;

    const llvm::Instruction* const mStepInst;
    const llvm::Value* const mStepOffset;

    const llvm::ICmpInst* const mCmpInst;

};
// map<Info*, LoopDeps>
