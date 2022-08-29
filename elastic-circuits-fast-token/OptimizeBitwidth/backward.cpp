#include "OptimizeBitwidth/OptimizeBitwidth.h"

#include <cmath>

using uint = unsigned int;

using llvm::Instruction;

// Recall that a typical backward function has prototype :
//void backwardFunc(Info* acc, const Info* iOut, const Info** iOps, unsigned int targetOp);
//These backward function only concern themselves with returning the minimal bitwidth its operands needs
//(Then backwardIterate takes the union of all uses of that operand, and tries to reduce it)

// - acc is the mask of the targeted operand, we must OVERWRITE it
// - iOut is the output of the instruction, we must use it to reduce the size of the operands
// - iOps is the set of all operands of the instruction, it may not necessarly be used by the backward function
// - targetOp designate the operand to reduce, (ie. we should reduce acc as if it was iOps[targetOp])

// Why all the boilerplate ?
//conceptually acc should point to the same address as iOps[targetOp], 
//but this would imply directly modifying it, and we don't want that at all, 
//since our goal is just to create a mask fitting for all users



// truncates iOp to the size of bounds (iOp may be/stay smaller)
static void truncateOperand(const Info* bound, Info* iOp);

// Applies a backward pass to the given Value
//ie. iterate over all its users, and keep the intersection (ie. most restrictive bitmask) between
// 1. its current bitmask
// 2. a bitmask accomodating all its users
bool OptimizeBitwidth::backward(const Value* V) {

    logger.log("BACKWARD on %", V->getName());
    
    Info* out = bit_info.at(V);
    if (out == nullptr) {
        logger.log("  No output for Value %", bit_stats[V]->name);
        return false;
    }

    logger.pushIndent();

    // maximum accomodator : store the bitwidth needed to accomodate all users
    const Info max { computeBackwardAccomodator(V, out) };

    logger.popIndent();
    
    const Info before(*out);
    
    out->keepIntersection(max);
    out->assertValid();

    updateBackwardStats(V, &before, out);
    
    return before != *out;
}

Info OptimizeBitwidth::computeBackwardAccomodator(const Value* V, const Info* infoV) {
    assert(V != nullptr);
    assert(infoV != nullptr);
    
    const unsigned int nb_users = V->getNumUses();
    // if Value has no users, return *infoValue as mask
    if (nb_users == 0) {
        logger.log("This value has no users. No accomodator needed.");
        return *infoV;
    }

    logger.log("Computing bitmask needed to accomodate its % users.", nb_users);
    logger.pushIndent();

    // max will hold the union of all bitmasks needed to accomodate all users.
    //I initialize it to the (not valid) smallest vector possible,
    // -> No need to separate first iteration from the others
    Info max(infoV->cpp_bitwidth);

    bool first = true;
    for (const User* u : V->users()) {
        // we can only backward on instructions
        if (const Instruction* userI = dyn_cast<Instruction>(u)) {
            const unsigned int opcode = userI->getOpcode();
            const Info* userOut = bit_info.at(userI);

            // and these instructions must have a return type (non-void)
            if (userOut == nullptr)
                continue;

            // I is used as an operand in userI, which one ?
            int target = -1;

            std::vector<const Info*> userOps;
            for (unsigned int i = 0 ; i < userI->getNumOperands() ; ++i) {
                Info* iOp = bit_info.at(userI->getOperand(i));
                userOps.push_back(iOp);
                assert (iOp != nullptr);

                if (userI->getOperand(i) == V)
                    target = i;
            }
            assert(target != -1); // impossible, userI is an User of I
            

            // accomodator : overwritten by each backward call
            Info acc(*infoV);
            // Xor has a special backward function
            if (opcode == Instruction::Xor) {
                backwardXor(&acc, userI, target);
            }
            else {
                backward_func f = backward_fs[opcode];
                if (f)
                    f (&acc, userOut, userOps.data(), target);
                else
                    logger.log("No backward function for % (opcode %)", userI->getOpcodeName(), opcode);
            }

            // extend the bitwidth (if needed) to accomodate all previous users + this one
            if (first) {
                max.left = acc.left;
                max.sign_ext = acc.sign_ext;
                max.right = acc.right;

                max.bit_value = acc.bit_value;
                max.unknown_bits = acc.unknown_bits;
                first = false;

                max.assertValid();
            }
            else {
                max.keepUnion(acc);
            }
        }
    }

    logger.popIndent();

    return max;
}



void OptimizeBitwidth::constructBackwardFunctions()
{
    backward_fs[Instruction::ZExt] = [&] (Info* acc, const Info* iOut, const Info** iOps, unsigned int targetOp) {
        logger.log("BACKWARD ZERO EXTENSION");
        logger.pushIndent();
        
        forward_fs[Instruction::Trunc] (acc, &iOut);

        logger.popIndent();
    };
    
    backward_fs[Instruction::SExt] = [&] (Info* acc, const Info* iOut, const Info** iOps, unsigned int targetOp) {
        logger.log("BACKWARD SIGN EXTENSION");
        logger.pushIndent();
        
        forward_fs[Instruction::Trunc] (acc, &iOut);

        logger.popIndent();
    };

    backward_fs[Instruction::Trunc] = [&] (Info* acc, const Info* iOut, const Info** iOps, unsigned int targetOp) {
        logger.log("BACKWARD TRUNCATE");
        logger.pushIndent();
        
        forward_fs[Instruction::ZExt] (acc, &iOut);

        logger.popIndent();
    };

    // we truncate targeted operand to the size of the output
    backward_fs[Instruction::And] = [&] (Info* acc, const Info* iOut, const Info** iOps, unsigned int targetOp) {
        logger.log("BACKWARD AND/OR");

        truncateOperand(iOut, acc);
    };
    backward_fs[Instruction::Or] = backward_fs[Instruction::And];

    backward_fs[Instruction::LShr] = [&] (Info* acc, const Info* iOut, const Info** iOps, unsigned int targetOp) {
        logger.log("BACKWARD LOGIC/ARITHMETIC SHIFT RIGHT");
        logger.pushIndent();

        if (targetOp == 0) {
            logger.log("Attempting to reduce op0");
            const Info* simulated_ops[] { iOut, iOps[1] };
            forward_fs[Instruction::Shl] (acc, simulated_ops);
        }
        else {
            logger.log("Cannot reduce op1");
        }

        logger.popIndent();
    };
    backward_fs[Instruction::AShr] = backward_fs[Instruction::LShr];
    backward_fs[Instruction::Shl] = [&] (Info* acc, const Info* iOut, const Info** iOps, unsigned int targetOp) {
        logger.log("BACKWARD LOGIC SHIFT LEFT");
        logger.pushIndent();

        if (targetOp == 0) {
            logger.log("Attempting to reduce op0");

            const Info* simulated_ops[] { iOut, iOps[1] };
            forward_fs[Instruction::LShr] (acc, simulated_ops);
        }
        else {
            logger.log("Cannot reduce op1");
        }

        logger.popIndent();
    };

    backward_fs[Instruction::Add] = [&] (Info* acc, const Info* iOut, const Info** iOps, unsigned int targetOp) {
        logger.log("BACKWARD ADD/SUB");
        truncateOperand(iOut, acc);
    };
    backward_fs[Instruction::Sub] = backward_fs[Instruction::Add];
    backward_fs[Instruction::Mul] = [&] (Info* acc, const Info* iOut, const Info** iOps, unsigned int targetOp) {
        logger.log("BACKWARD MUL");

        // let acc==iOps[target] = iOpA, and iOpB = the other operand
        // can reduce iOpA->left if iOpB->right > 0
        //because we know the upper bits of iOpA won't be used in the output (they would yield OOB bits)
        acc->left = std::min(int(acc->left), iOut->left - iOps[targetOp ^ 1]->right);
        acc->sign_ext = std::min(acc->sign_ext, acc->left);
        acc->bit_value &= op::mask(acc->right, acc->left);
        acc->unknown_bits &= op::mask(acc->right, acc->left);
    };

    // we cannot do anything in backward division/modulo, let's not create the lambdas 
    //we can check if a std::function points to a function by testing it : if (functor) { call functor }
    //backward_fs[Instruction::SDiv] = backward_fs[Instruction::UDiv] = [&] (const Info* out, Info** iOps) { };
    //backward_fs[Instruction::SRem] = backward_fs[Instruction::URem] = backward_fs[Instruction::SDiv];
    
    backward_fs[Instruction::PHI] = [&] (Info* acc, const Info* iOut, const Info** iOps, unsigned int targetOp) { 
         logger.log("BACKWARD PHI");

         truncateOperand(iOut, acc);
    };
}

// we must treat backwardXOR separately, 
//because we can only reduce acc if both operands are reducible (cf. paper)
void OptimizeBitwidth::backwardXor(Info* acc, const Instruction* I, unsigned int targetOp) {
    logger.log("BACKWARD XOR");
    const Info* iOut = bit_info.at(I);
    
    Value* opB = I->getOperand(targetOp ^ 1);
    const Info* iOpB = bit_info.at(opB);

    // can we also reduce opB ?
    logger.log("Backward Xor must verify that both ops are truncatable to output's size.");

    logger.pushIndent();

    if (opB->hasOneUse()) {
        logger.log("The other op has only one use (this xor). We can truncate op%.", targetOp);
        truncateOperand(iOut, acc);
    }
    else if (iOpB->left <= iOut->left) {
        logger.log("The other operand is already smaller than output. We can truncate op%.", targetOp);
        truncateOperand(iOut, acc);
    }
    else {
        logger.log("Couldn't check reducibility of op%. Op% won't be truncated.", targetOp ^ 1, targetOp);
    }
    logger.popIndent();
}

void truncateOperand(const Info* bound, Info* iOp) {
    iOp->left = std::min(bound->left, iOp->left);
    iOp->sign_ext = std::min(bound->sign_ext, iOp->sign_ext);
    iOp->right = std::max(bound->right, iOp->right);

    iOp->bit_value &= op::mask(iOp->right, iOp->left);
    iOp->unknown_bits &= op::mask(iOp->right, iOp->left);
}

