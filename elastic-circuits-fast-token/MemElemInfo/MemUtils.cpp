#include "MemElemInfo/MemUtils.h"

const Value* findBaseInternal(const Value* addr) {
    if (auto* arg = dyn_cast<Argument>(addr)) {
        if (!arg->getType()->isPointerTy())
            llvm_unreachable("Only pointer arguments are considered addresses");
        return addr;
    }

    if (isa<Constant>(addr))
        llvm_unreachable("Cannot determine base address of Constant");

    if (auto* I = dyn_cast_or_null<Instruction>(addr)) {
        if (isa<AllocaInst>(I))
            return addr;
        else if (auto GEPI = dyn_cast<GetElementPtrInst>(I))
            return findBaseInternal(GEPI->getPointerOperand());
        else if (auto SI = dyn_cast<SelectInst>(I)) {
            const auto* trueBase  = findBaseInternal(SI->getTrueValue());
            const auto* falseBase = findBaseInternal(SI->getFalseValue());

            /* Select must choose pointers to same array. Otherwise cannot
             * choose relevant arrayRAM in elastic circuit */
            assert(trueBase == falseBase);
            return trueBase;
        }
    }

    /* We try to find a few known cases of pointer expression. For others,
     * implement when you come across them */
    llvm_unreachable("Cannot  determine base array, aborting...");
}

const Value* findBase(const Instruction* I) {

    Value const* addr;
    if (isa<LoadInst>(I)) {
        addr = static_cast<const LoadInst*>(I)->getPointerOperand();
    } else if (isa<StoreInst>(I)) {
        addr = static_cast<const StoreInst*>(I)->getPointerOperand();
    } else {
        llvm_unreachable("Instruction is not a memory access");
    }
    return findBaseInternal(addr);
}
