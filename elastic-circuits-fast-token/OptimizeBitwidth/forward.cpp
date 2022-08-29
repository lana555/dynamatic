#include "OptimizeBitwidth/OptimizeBitwidth.h"
#include <cmath>


using uint = unsigned int;

using llvm::Instruction;

// Exaustive list of llvm's Integer Instructions
// Terminator Ops       : Ret, Br, IndirectBr, Invoke, CallBr, Resume + obscure stuff...
// Binary Ops           : Add,Sub,Mul, UDiv, SDiv, URem, SRem
// Bitwise Binary Ops   : Shl,LShr,AShr, And,Or,Xor
// MemoryOps            : Alloca, Load, Store, Fence + AtomicOps, GetElementPtr
// Casting Ops          : Trunc,ZExt,SExt, PtrToInt,IntToPtr, BitCast, AddrSpaceCast
// Misc Ops             : iCmp, PHI, Select, Freeze, Call, Va_Arg + obscure stuff...
// + Vector Ops (SSE & cie)



// Applies a forward pass to the given Instruction
//Computes the bitwidth of the output, using its operands
bool OptimizeBitwidth::forward(const Instruction* I) {
    unsigned int opcode = I->getOpcode();

    Info* iOut = bit_info.at(I);
    if (iOut == nullptr) {
        logger.log("No output for Instruction % (opcode=%)", I->getName(), I->getOpcodeName());
        return false;
    }

    Info forwardOut(iOut->cpp_bitwidth);

    // PHINodes have special forward function
    if (opcode == llvm::Instruction::PHI) {
        const PHINode* phi = cast<PHINode>(I);
        forwardPHI(&forwardOut, phi);
    }
    else {
        std::vector<const Info*> iOps;
        for (unsigned int i = 0 ; i < I->getNumOperands() ; ++i) {
            iOps.push_back(bit_info.at(I->getOperand(i)));
            assert(iOps.at(i) != nullptr);
        }
        
        forward_func f = forward_fs[opcode];

        if (f)
            f(&forwardOut, iOps.data());
        else
            logger.log("No forward function for % (opcode %)", I->getOpcodeName(), opcode);
    }

    const Info before = *iOut;
    iOut->keepIntersection(forwardOut);

    if (loop_deps.count(I) != 0) {
        logger.log("Instruction is bounded inside a loop.");
        Info bound = loop_deps.at(I).computeCurrentIteratorInfo();

        iOut->left = std::min(iOut->left, bound.left);
        iOut->sign_ext = std::min(iOut->sign_ext, bound.sign_ext);
        iOut->right = std::min(iOut->right, bound.right);

        iOut->bit_value &= op::mask(iOut->right, iOut->left);
        iOut->unknown_bits &= op::mask(iOut->right, iOut->left);
    }

    iOut->assertValid();

    updateForwardStats(I, &before, iOut);

    return before != *iOut;
}

// Recall that llvm IR doesn't differentiate unsigned from signed ints (iN all the same, with N in [1, 2^23 - 1])
//For most instructions, this doesn't make a difference (Add/Sub, And/Or...)
//But we need to differentiate some instructions now
// - Casts : ZeroExtension, SignExtension, Truncation
// - Right Shifts : LShR, AShR
// - Division and remainder : UDiv, SDiv, URem, SRem
void OptimizeBitwidth::constructForwardFunctions() 
{
    // Zero extension : we have nothing much to do, as we precisely want to ignore useless zeros
    forward_fs[Instruction::ZExt] = [&] (Info* out, const Info** iOps) {
        assert(iOps[0]->cpp_bitwidth <= out->cpp_bitwidth); // non-strict inequality, because of 32bits addresses

        out->left = iOps[0]->left;
        out->sign_ext = iOps[0]->sign_ext;
        out->right = iOps[0]->right;

        out->bit_value = iOps[0]->bit_value;
        out->unknown_bits = iOps[0]->unknown_bits;
    };

    // Sign extension : propagates 0/1/? bits from sign_ext to new_bitwidth
    forward_fs[Instruction::SExt] = [&] (Info* out, const Info** iOps) {
        assert(iOps[0]->cpp_bitwidth <= out->cpp_bitwidth);

        //if the value is (potentially) negative : must propagate 1/?
        if (iOps[0]->left == iOps[0]->cpp_bitwidth - 1) {
            out->left = out->cpp_bitwidth - 1;
            out->sign_ext = iOps[0]->sign_ext;
            out->right = iOps[0]->right;

            out->bit_value = iOps[0]->bit_value;
            out->unknown_bits = iOps[0]->unknown_bits;
          
            // sign bit is unknown : propagate ?
            if (op::get(out->unknown_bits, out->sign_ext)) {
                out->unknown_bits |= op::mask(out->sign_ext, out->left);

                if (out->right < out->sign_ext)
                    out->bit_value &= op::mask(out->right, out->sign_ext - 1);
                else
                    out->bit_value = 0;
            }
            // sign bit is 1 : propagate 1
            else {
                out->bit_value |= op::mask(out->sign_ext, out->left);

                if (out->right < out->sign_ext)
                    out->unknown_bits &= op::mask(out->right, out->sign_ext - 1);
                else
                    out->unknown_bits = 0;
            }
        }
        // if iOp0 is positive => ZExt
        else {
            forward_fs[Instruction::ZExt](out, iOps);
        }
    };

    // Truncation : self-explainatory
    forward_fs[Instruction::Trunc] = [&] (Info* out, const Info** iOps) {
        assert(iOps[0]->cpp_bitwidth >= out->cpp_bitwidth); // non-strict ineq. because of 32 bits addresses

        out->left     = std::min(iOps[0]->left, uint8_t(out->cpp_bitwidth - 1));
        out->sign_ext = std::min(iOps[0]->sign_ext, out->left);
        out->right    = std::min(iOps[0]->right, uint8_t(out->cpp_bitwidth - 1));

        out->bit_value = iOps[0]->bit_value & op::mask(out->right, out->left);
        out->unknown_bits = iOps[0]->unknown_bits & op::mask(out->right, out->left);

        out->reduceBounds();
    };


    // Forward and : apply and logic to operands
    // ? & 0 -> 0
    // ? & 1 -> ?
    // ? & ? -> ?
    forward_fs[Instruction::And] = [&] (Info* out, const Info** iOps) {
        out->left = std::min(iOps[0]->left, iOps[1]->left);
        out->sign_ext = std::min(iOps[0]->sign_ext, iOps[1]->sign_ext);
        out->right = std::max(iOps[0]->right, iOps[1]->right);

        // and the bit values
        out->bit_value = iOps[0]->bit_value & iOps[1]->bit_value;
        // ? bits stay ? when and-ed with 1 or ?
        out->unknown_bits = (iOps[0]->unknown_bits & iOps[1]->unknown_bits) 
            | (iOps[0]->unknown_bits & iOps[1]->bit_value) | (iOps[1]->unknown_bits & iOps[0]->bit_value);

        out->reduceBounds();
    };
    
    // Forward or : apply or logic to operands
    // ? | 0 -> ?
    // ? | 1 -> 1
    // ? | ? -> ? 
    forward_fs[Instruction::Or] = [&] (Info* out, const Info** iOps) {
        out->left = std::max(iOps[0]->left, iOps[1]->left);
        out->sign_ext = std::max(iOps[0]->sign_ext, iOps[1]->sign_ext);
        out->right = std::min(iOps[0]->right, iOps[1]->right);

        // or the bit values
        out->bit_value = iOps[0]->bit_value | iOps[1]->bit_value;
        // ? bits stay only when or-ed with 0
        out->unknown_bits = (iOps[0]->unknown_bits | iOps[1]->unknown_bits) & ~out->bit_value;

        out->reduceBounds();
    };

    // Forward xor : apply xor logic to operands
    // ? ^ 0 -> ?
    // ? ^ 1 -> ?
    // ? ^ ? -> ?
    forward_fs[Instruction::Xor] = [&] (Info* out, const Info** iOps) {
        out->left = std::max(iOps[0]->left, iOps[1]->left);
        out->sign_ext = std::max(iOps[0]->sign_ext, iOps[1]->sign_ext);
        out->right = std::min(iOps[0]->right, iOps[1]->right);

        // ? bits are 'poisonous' : union of both's unknown bits
        out->unknown_bits = iOps[0]->unknown_bits | iOps[1]->unknown_bits;
        // bit values are xor-ed, then masked with unknown bits
        //to avoid that some bits are 1 while they are in fact unknown
        out->bit_value = (iOps[0]->bit_value ^ iOps[1]->bit_value) & ~out->unknown_bits;
        
        out->reduceBounds();
    };



    // Forward Logic Shift Right : we distinguish two cases
    // - iOp1 (shift) is totally known => we can compute the exact result of the shift
    // - iOp1 has some unknown bits => we return a conservative estimate of the shift
    forward_fs[Instruction::LShr] = [&] (Info* out, const Info** iOps) {
        logger.log("FORWARD LOGIC SHIFT RIGHT");

        logger.pushIndent();
        if (op::none(iOps[1]->unknown_bits)) {
            const uint8_t shift = std::min(iOps[1]->bit_value, uintmax_t(op::S_MAX_BITS));

            logger.log("Shifting by a constant : %", int(shift));

            if (iOps[0]->left - shift < 0) {
                // shift by too much, result is 0
                out->right = out->left = out->sign_ext = 0;
                out->bit_value = out->unknown_bits = 0;
            }
            else {
                out->left = iOps[0]->left - shift;
                out->sign_ext = std::max(0, iOps[0]->sign_ext - shift);
                out->right = std::max(0, iOps[0]->right - shift);
                
                // C behavior is implementation-defined (may do an Arithmetic shift instead in some cases)
                //=> need to force the introduced bits to 0
                out->bit_value = (iOps[0]->bit_value >> shift) & op::mask(out->right, out->left);
                out->unknown_bits = (iOps[0]->unknown_bits >> shift) & op::mask(out->right, out->left);

                out->reduceBounds();
            }
        }
        else {
            logger.log("Shifting with some unknown bits, result will be a conservative estimate.");
            const uint8_t min_shift = std::min(iOps[1]->minUnsignedValuePossible(), uintmax_t(op::S_MAX_BITS));
            const uint8_t max_shift = std::min(iOps[1]->maxUnsignedValuePossible(), uintmax_t(op::S_MAX_BITS));

            
            if (iOps[0]->left - min_shift < 0) {
                // shift by too much, result is 0
                out->right = out->left = out->sign_ext = 0;
                out->bit_value = out->unknown_bits = 0;
            }
            else {
                out->left = iOps[0]->left - min_shift;
                out->sign_ext = std::max(0, iOps[0]->sign_ext - min_shift);
                out->right = std::max(0, iOps[0]->right - max_shift);

                out->bit_value = op::S_ZERO;
                out->unknown_bits = op::mask(out->right, out->left);
            }
        }
        logger.popIndent();
    };
    // Forward Arithmetic Shift Right : ~ same as forwardLShR, but propagating sign bit
    forward_fs[Instruction::AShr] = [&] (Info* out, const Info** iOps) {
        logger.log("FORWARD ARITHMETIC SHIFT RIGHT");

        logger.pushIndent();
        if (iOps[0]->left == iOps[0]->cpp_bitwidth - 1) {
            logger.log("iOp0 may be negative : extending 1/? bit.");
            // extend limit to up bitwidth
            out->left = out->cpp_bitwidth - 1;

            if (op::none(iOps[1]->unknown_bits)) {
                const uint8_t shift = std::min(iOps[1]->bit_value, uintmax_t(op::S_MAX_BITS));

                logger.log("Shifting by a constant : %", int(shift));

                out->right = std::max(0, iOps[0]->right - shift);
                out->sign_ext = std::max(0, iOps[0]->sign_ext - shift);

                out->bit_value = iOps[0]->bit_value >> shift;
                out->unknown_bits = iOps[0]->unknown_bits >> shift;

                // if sign bit is unknown :
                if (op::get(iOps[0]->unknown_bits, iOps[0]->sign_ext)) {
                    out->unknown_bits |= op::mask(out->sign_ext, out->left);
                    out->bit_value &= op::mask(out->right, out->sign_ext);
                }
                // else, sign bit=1
                else {
                    out->bit_value |= op::mask(out->sign_ext, out->left);
                    out->unknown_bits &= op::mask(out->right, out->sign_ext);
                }
                out->reduceBounds();
            } 
            else {
                logger.log("Shifting with some unknown bits, result will be a (really) conservative estimate.");

                const uint8_t min_shift = std::min(iOps[1]->minUnsignedValuePossible(), uintmax_t(op::S_MAX_BITS));
                const uint8_t max_shift = std::min(iOps[1]->maxUnsignedValuePossible(), uintmax_t(op::S_MAX_BITS));
                
                out->right = std::max(0, iOps[0]->right - max_shift);
                out->sign_ext = std::max(0, iOps[0]->sign_ext - min_shift);

                out->bit_value = op::S_ZERO;
                out->unknown_bits = op::mask(out->right, out->left);
            }
        }
        else {
            logger.log("iOp0 is positive -> calling forwardLShR");
            forward_fs[Instruction::LShr] (out, iOps);
        }
        logger.popIndent();
    };
    // Forward (Logic) Shift Left : ~ same as forwardLShR, but left instead of right
    forward_fs[Instruction::Shl] = [&] (Info* out, const Info** iOps) {
        logger.log("FORWARD LOGIC SHIFT LEFT");

        logger.pushIndent();
        if (op::none(iOps[1]->unknown_bits)) {
            const uint8_t shift = std::min(iOps[1]->bit_value, uintmax_t(op::S_MAX_BITS));

            logger.log("Shifting by a constant : %", int(shift));

            if (iOps[0]->right + shift >= out->cpp_bitwidth) {
                // we shift by too much, result is 0
                out->right = out->left = out->sign_ext = 0;
                out->bit_value = out->unknown_bits = 0;
            } 
            else {
                out->right = iOps[0]->right + shift;
                out->left  = std::min(out->cpp_bitwidth - 1, iOps[0]->left + shift);
                out->sign_ext = std::min(out->cpp_bitwidth - 1, iOps[0]->sign_ext + shift);
                
                out->bit_value = (iOps[0]->bit_value << shift) & op::mask(out->right, out->left);
                out->unknown_bits = (iOps[0]->unknown_bits << shift) & op::mask(out->right, out->left);
                
                out->reduceBounds();
            }
        }
        else {
            logger.log("Shifting with some unknown bits, result will be a conservative estimate.");
            const uint8_t min_shift = std::min(iOps[1]->minUnsignedValuePossible(), uintmax_t(op::S_MAX_BITS));
            const uint8_t max_shift = std::min(iOps[1]->maxUnsignedValuePossible(), uintmax_t(op::S_MAX_BITS));

            if (iOps[0]->right + min_shift >= out->cpp_bitwidth) {
                // we shift by too much, result is 0
                out->right = out->left = out->sign_ext = 0;
                out->bit_value = out->unknown_bits = 0;
            }
            else {
                out->right = iOps[0]->right + min_shift;
                out->left  = std::min(out->cpp_bitwidth - 1, iOps[0]->left + max_shift);
                out->sign_ext = std::min(out->cpp_bitwidth - 1, iOps[0]->sign_ext + max_shift);

                out->bit_value = op::S_ZERO;
                out->unknown_bits = op::mask(out->right, out->left);
            }
        }
        logger.popIndent();
    };


    // Addition is passed to forwardAdd()
    forward_fs[Instruction::Add] = [&] (Info* out, const Info** iOps) {
        forwardAdd(out, iOps[0], iOps[1], false);
    };
    // Substraction is passed to forwardAdd(), with ~iOp1 and a carry in.
    forward_fs[Instruction::Sub] = [&] (Info* out, const Info** iOps) {
        
        // construct complement of iOp1
        Info iOp1(iOps[1]->cpp_bitwidth);
        iOp1.bit_value = ~iOps[1]->bit_value & ~iOps[1]->unknown_bits & op::mask(iOp1.cpp_bitwidth);
        iOp1.unknown_bits = iOps[1]->unknown_bits;

        iOp1.right = op::findRightMostBit(iOp1.bit_value | iOp1.unknown_bits);
        iOp1.left = op::findLeftMostBit(iOp1.bit_value | iOp1.unknown_bits);
        iOp1.sign_ext = iOps[1]->sign_ext;

        // set carryIn to 1 to simulate two's complement
        forwardAdd(out, iOps[0], &iOp1, true);
    };
    
    // Forward multiplication : we actually can't gain much here,
    // we can only compute the right most k-first bits,
    // where k is the number of right-most bits known in every operand
    forward_fs[Instruction::Mul]  = [&] (Info* out, const Info** iOps) {
        
        out->right = std::max(iOps[0]->right, iOps[1]->right);
        out->left = std::min(int(out->cpp_bitwidth - 1), iOps[0]->left + iOps[1]->left + 1);
        out->sign_ext = std::min(int(out->left), iOps[0]->sign_ext + iOps[1]->sign_ext + 1);

        uint8_t knownBitsInBothOps;
        if (op::none(iOps[0]->unknown_bits | iOps[1]->unknown_bits))
            knownBitsInBothOps = out->cpp_bitwidth;
        else
            knownBitsInBothOps = std::min(op::findRightMostBit(iOps[0]->unknown_bits), 
                                            op::findRightMostBit(iOps[1]->unknown_bits));

        uint8_t maxZeroRightBits = std::max(iOps[0]->right, iOps[1]->right);

        // use the better optimisation
        if (knownBitsInBothOps <= maxZeroRightBits) {
            out->bit_value = op::S_ZERO;
            out->unknown_bits = op::mask(maxZeroRightBits, out->left);
        }
        else {
            out->bit_value = (iOps[0]->bit_value * iOps[1]->bit_value) & op::mask(knownBitsInBothOps);

            if (knownBitsInBothOps == out->cpp_bitwidth)
                out->unknown_bits = 0;
            else
                out->unknown_bits = op::mask(knownBitsInBothOps, out->left);
            
            out->reduceBounds();
        }
    };
    
    // Returns the UNSIGNED quotient
    forward_fs[Instruction::UDiv] = [&] (Info* out, const Info** iOps) {
        logger.log("FORWARD UNSIGNED DIV");

        int T;
        if (iOps[1]->minUnsignedValuePossible() != 0)
            T = std::log2(iOps[1]->minUnsignedValuePossible());
        else 
            T = 0;

        out->left = std::max(0, iOps[0]->left - T);
        out->sign_ext = std::max(0, iOps[0]->sign_ext - T);
        out->right = 0;
        
        out->bit_value = op::S_ZERO;
        out->unknown_bits = op::mask(out->right, out->left);
    };
    // Returns the SIGNED quotient of the two ops, rounded toward 0
    forward_fs[Instruction::SDiv] = [&] (Info* out, const Info** iOps) {
        logger.log("FORWARD SIGNED DIV");
        logger.pushIndent();

        // T is the largest st. 2^T <= min(|iOp1.PossibleValues|)
        // then we can shift by T to emulate division
        int T;
        bool op0_maybe_neg = iOps[0]->left == iOps[0]->cpp_bitwidth - 1;
        bool op1_maybe_neg = iOps[1]->left == iOps[1]->cpp_bitwidth - 1;

        if (op1_maybe_neg) {
            // if we are sure that op1 is negative, can take the shift to be log(|iOp1.maxSIGNEDValue|)
            if (op::get(iOps[1]->bit_value, iOps[1]->left))
                T = std::log2(std::abs(iOps[1]->maxSignedValuePossible())); 
            // we don't know the sign of op1, 
            //ex: op0 / ?111 -> could be -1 or 7 => need all bits of op0
            else
                T = 0;

            logger.log("Divisor could be negative, assumed shift is %", T);
        }
        else {
            if (iOps[1]->minUnsignedValuePossible() != 0)
                T = std::log2(iOps[1]->minUnsignedValuePossible());
            else 
                T = 0;

            logger.log("Divisor is non-negative, assumed shift is % (div=%)", T, iOps[1]->minUnsignedValuePossible());
        }

        if (op0_maybe_neg || op1_maybe_neg) {
            logger.log("At least one operand may be negative, must prepare for result to be negative");
            out->left = out->cpp_bitwidth - 1;
        }
        else {
            logger.log("Both operands are positive, result will be too");
            out->left = std::max(0, iOps[0]->left - T);
        }

        out->sign_ext = std::max(0, iOps[0]->sign_ext - T);
        out->right = 0;
        
        out->bit_value = op::S_ZERO;
        out->unknown_bits = op::mask(out->right, out->left);

        logger.popIndent();
    };
    // Returns the UNSIGNED remaining
    forward_fs[Instruction::URem] = [&] (Info* out, const Info** iOps) {
        logger.log("FORWARD UNSIGNED REM");

        out->left = iOps[1]->left;
        out->sign_ext = iOps[1]->sign_ext;
        out->right = 0;

        out->bit_value = op::S_ZERO;
        out->unknown_bits = op::mask(out->right, out->left);
    };
    // Returns the SIGNED remaining, which has the same sign as the DIVIDEND (=iOp0) -> /!\ -> != modulo
    forward_fs[Instruction::SRem] = [&] (Info* out, const Info** iOps) {
        logger.log("FORWARD SIGNED REM");
        logger.pushIndent();

        if (iOps[0]->left == iOps[0]->cpp_bitwidth - 1) {
            logger.log("Dividend may be negative");
            out->left = out->cpp_bitwidth - 1;
        }
        else {
            logger.log("Dividend is positive");
            if (iOps[1]->left == iOps[1]->cpp_bitwidth - 1)
                out->left = iOps[1]->sign_ext;
            else
                out->left = iOps[1]->left;
        }

        out->sign_ext = iOps[1]->sign_ext;
        out->right = 0;

        out->bit_value = op::S_ZERO;
        out->unknown_bits = op::mask(out->right, out->left);

        logger.popIndent();
    };
    

    // Forward Select (ie. ternary op => a0 ? a1 : a2)
    forward_fs[Instruction::Select] = [&] (Info* out, const Info** iOps) {
        // extend output vector to fit both inputs :

        *out = *iOps[1]; // copy iOp1
        out->keepUnion(*iOps[2]); // extend it to fit iOp2
        
        out->reduceBounds();
    };

}


 
// Forward add : progress bit by bit, while keeping track of the carry
// subbing is the same as adding : just replace iOP1 by its complement and set carryIn to 1 
// (a-b <=> a+b', where b' is the two's complement of b)
void OptimizeBitwidth::forwardAdd(Info* out, const Info* iOp0, const Info* iOp1, bool carryIn) {
    assert(out->cpp_bitwidth == iOp0->cpp_bitwidth && out->cpp_bitwidth == iOp1->cpp_bitwidth);

    logger.log("FORWARD ADD");
    
    out->left   = std::min(std::max(iOp0->left, iOp1->left) + 1, out->cpp_bitwidth - 1);
    out->sign_ext = std::min(std::max(iOp0->sign_ext, iOp1->sign_ext) + 1, int(out->left));
    out->right  = 0;

    // at the start, everything is unknown
    out->bit_value = carryIn ? 1 : 0;
    out->unknown_bits = op::S_ZERO;

    const uintmax_t combinedKnownBits = ~(iOp0->unknown_bits | iOp1->unknown_bits);
    const uintmax_t carryOutAlwaysZero = combinedKnownBits & ~iOp0->bit_value & ~iOp1->bit_value;
    const uintmax_t carryOutAlwaysOne = iOp0->bit_value & iOp1->bit_value;
    const uintmax_t carryOutUnknown = ~(carryOutAlwaysZero | carryOutAlwaysOne);
    
    bool carryInIsKnown = true;
    uintmax_t bitMask = 1;
    uintmax_t processedMask = 0;
    for (int i = 0 ; i <= out->left ; ++i) {
        // all the bits are known, can compute the output
        if (carryInIsKnown && (combinedKnownBits & bitMask)) {
            // compute both output and carryOut
            out->bit_value += (iOp0->bit_value & bitMask) + (iOp1->bit_value & bitMask);
            // carryInIsKnown = true // already true
        }
        else {
            // clear the carryIn that was (maybe) written in bit_value
            out->bit_value &= processedMask;
            out->unknown_bits |= bitMask;

            carryInIsKnown = (carryOutUnknown & bitMask) == 0;

            // add a carryIn for next bit if needed
            if (carryInIsKnown && (carryOutAlwaysOne & bitMask))
                out->bit_value += bitMask << 1;
        }
        processedMask |= bitMask;
        bitMask <<= 1;
    }

    // may have 1 excess carry
    out->bit_value &= processedMask;

    out->reduceBounds();
}


// PHI instructions are virtual instructions that output the value corresponding to the label by which control reached them
//Ex: phi [%a, lbl1], [%b, lbl2], [%c, lbl3] => will output one of the three %a, %b, %c
//-> simply make output big enough to store every value
void OptimizeBitwidth::forwardPHI(Info* out, const PHINode* phi) {
    logger.log("FORWARD PHI");

    const unsigned num_incoming = phi->getNumIncomingValues();
    if (num_incoming != 0) {
        const Info* in0 = bit_info.at(phi->getIncomingValue(0));
        
        *out = *in0; // copy in0
        
        for (unsigned i = 1 ; i < num_incoming ; i++) {
            const Info* in = bit_info.at(phi->getIncomingValue(i));
            out->keepUnion(*in);
        }
    }
}
