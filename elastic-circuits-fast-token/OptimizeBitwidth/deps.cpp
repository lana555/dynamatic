#include "OptimizeBitwidth/deps.h"
#include "OptimizeBitwidth/operations.h"

#include <cassert>
#include <cmath>

ArgDeps::ArgDeps(uint8_t arg_bitwidth) 
    : mDeps(), mSnapshot(arg_bitwidth)
{ }

void ArgDeps::addDependency(const Info* i) {
    // add i to dependencies iff it's not already in
    if (std::find(mDeps.begin(), mDeps.end(), i) == mDeps.end())
        mDeps.push_back(i);
}

// Returns the Argument's Info from the CURRENT state of every dependencies :
//it's the smallest Info that can fit them all
Info ArgDeps::computeCurrentArgInfo() const {
    const uint8_t bitwidth = mSnapshot.cpp_bitwidth;

    Info res(bitwidth);

    if (mDeps.empty())
        return res;

    res.left = std::min(bitwidth - 1, int(mDeps.at(0)->left));
    res.sign_ext = std::min(res.left, mDeps.at(0)->sign_ext);
    res.right = std::min(res.left, mDeps.at(0)->right);

    res.unknown_bits = mDeps.at(0)->unknown_bits & op::mask(res.right, res.left);
    res.bit_value = mDeps.at(0)->bit_value & op::mask(res.right, res.left);

    for (unsigned int i = 1 ; i < mDeps.size() ; ++i)
        res.keepUnion(*mDeps.at(i));

    return res;
}


using namespace llvm;

LoopDeps::LoopDeps(const MapInfo& bit_info, const Value* initVal, const Value* boundVal, 
    const Instruction* stepInst, const llvm::Value* stepOffset, const ICmpInst* cmpInst)
    : mMapInfo(bit_info), mInitVal(initVal), mBoundVal(boundVal), 
    mStepInst(stepInst), mStepOffset(stepOffset), mCmpInst(cmpInst)
{ 
    assert(initVal != nullptr);
    assert(boundVal != nullptr);
    assert(stepInst != nullptr);
    assert(stepOffset != nullptr);
    assert(cmpInst != nullptr);
}

Info LoopDeps::computeCurrentIteratorInfo() const {
    Info res(initInfo()->cpp_bitwidth);

    Predicate p = getLatchPredicate();

    // we deal only with integer bounds (FP are not reducable)
    if (!CmpInst::isIntPredicate(p))
        return res;

    switch (p)
    {
    // can't do much if we need eq
    case Predicate::ICMP_EQ: 
        return res;

    // special case, we can't deduce if we don't have enough info on stepOffset
    case Predicate::ICMP_NE: { 
        
        // bit sign is unknown, don't have enough info
        if (op::get(stepOffsetInfo()->unknown_bits, stepOffsetInfo()->cpp_bitwidth - 1))
            return res;

        // step is negative, != behaves like >
        if (op::get(stepOffsetInfo()->bit_value, stepOffsetInfo()->cpp_bitwidth - 1))
            goto pred_sgt;
        // step is positive, != behaves like <
        else
            goto pred_slt;

    } break;


    // stepInst <(=) bound
    //--> ASSUME stepOffset > 0
    //--> step cannot exceed bound + step 
    //BUT if initVal is negative, need to set left to bitwidth - 1
    pred_slt:
    case Predicate::ICMP_SLT:
    case Predicate::ICMP_SLE: {

        intmax_t minInit = initInfo()->minSignedValuePossible();
        intmax_t maxBound = boundInfo()->maxSignedValuePossible();
        intmax_t maxStep = stepOffsetInfo()->maxSignedValuePossible();

        intmax_t maxValueUnderBound = maxBound + maxStep;
        if (CmpInst::isTrueWhenEqual(p))
            maxValueUnderBound += 1;

        // init value and bound positive
        if (0 <= minInit) {            
            res.left = std::min(res.cpp_bitwidth - 2, op::findLeftMostBit(maxValueUnderBound));
            res.sign_ext = res.left;
            res.right = 0;
        }
        else {

            res.left = res.cpp_bitwidth - 1;
            res.sign_ext = std::min(res.left,
                std::max(initInfo()->sign_ext, uint8_t(op::findSignExtension(maxValueUnderBound, res.cpp_bitwidth))));
            res.right = 0;
        }
    } break;


    // stepInst >(=) bound, 
    //so if bound is negative, need left = bitwidth - 1
    // ASSUME stepOffset < 0
    pred_sgt:
    case Predicate::ICMP_SGT:
    case Predicate::ICMP_SGE: {
        
        intmax_t maxInit = initInfo()->maxSignedValuePossible();
        intmax_t minBound = boundInfo()->minSignedValuePossible();
        intmax_t minStep = stepOffsetInfo()->minSignedValuePossible();

        intmax_t minValUnderBound = minBound + minStep;
        if (CmpInst::isTrueWhenEqual(p))
            minValUnderBound -= 1;

        // if minValue is positive (second eq. to check for overflow)
        if (0 <= minValUnderBound && 0 <= minBound) {
            res.left = std::min(res.cpp_bitwidth - 2, int(initInfo()->left));
            res.sign_ext = res.left;
            res.right = 0;
        }
        else {
            res.left = res.cpp_bitwidth - 1;
            res.sign_ext = std::min(res.left,
                std::max(initInfo()->sign_ext, uint8_t(op::findSignExtension(minValUnderBound, res.cpp_bitwidth))));
            res.right = 0;
        }
    } break;


    // stepInst <(=) bound (or indVar <(=) bound)
    //--> stepInst = indVar + X, ASSUME 0 < X
    //--> stepInst cannot exceed bound + X (+1)
    case Predicate::ICMP_ULT:
    case Predicate::ICMP_ULE: {
        if (op::get(boundInfo()->maxUnsignedValuePossible(), op::S_MOST_LEFT)
            || op::get(stepOffsetInfo()->maxUnsignedValuePossible(), op::S_MOST_LEFT)) {
            // risk of overflow, cannot add the numbers
            res.left = res.cpp_bitwidth - 1;
        }
        else {
            uintmax_t maxValUnderBound = boundInfo()->maxUnsignedValuePossible();
            maxValUnderBound += stepOffsetInfo()->maxUnsignedValuePossible();
            if (CmpInst::isTrueWhenEqual(p))
                maxValUnderBound += 1;

            res.left = std::min(res.cpp_bitwidth - 1, op::findLeftMostBit(maxValUnderBound));
        }
        
        res.sign_ext = res.left;
        res.right = 0;
    } break;


    //  stepInst >(=) bound, 
    //since comparison is unsigned, stepInst cannot exceed initVal, and cannot be negative
    case Predicate::ICMP_UGT:
    case Predicate::ICMP_UGE: {
        res.left = std::min(res.cpp_bitwidth - 1, int(initInfo()->left));
        res.sign_ext = std::min(res.left, initInfo()->sign_ext);
        res.right = 0;
    } break;
    
    default:
        break;
    }

    res.bit_value &= op::mask(res.right, res.left);
    res.unknown_bits &= op::mask(res.right, res.left);
    return res;
}

LoopDeps::Predicate LoopDeps::getLatchPredicate() const {
    if (boundValue() == mCmpInst->getOperand(1) 
        || mCmpInst->getPredicate() == Predicate::ICMP_NE 
        || mCmpInst->getPredicate() == Predicate::ICMP_EQ)
        return mCmpInst->getPredicate();
    else
        return mCmpInst->getInversePredicate();
}

bool LoopDeps::isStepOffsetConstant() const {
    return op::none(stepOffsetInfo()->unknown_bits);
}