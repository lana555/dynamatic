#include "OptimizeBitwidth/OptimizeBitwidth.h"
#include "llvm/IR/InstIterator.h"
#include <bitset>
#include <fstream>
#include <iostream>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolutionExpander.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PatternMatch.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Transforms/Utils/LoopUtils.h>
#include <ostream>
#include <stdio.h>
#include <stdlib.h>
#include <system_error>
/*
 * Iterates through every instruction in function and makes a forward and backward passes
 */

// char OptimizeBitwidth::ID = 0;

// static RegisterPass<OptimizeBitwidth> Y("OptimizeBitwidth", "OptimizeBitwidth Pass",
//                              false /* Only looks at CFG */,
//                              false /* Analysis Pass */);

char OptimizeBitwidth::ID = 1;

static RegisterPass<OptimizeBitwidth> Y("OptimizeBitwidth", "OptimizeBitwidth Pass",
                                        true /* Only looks at CFG */, true /* Analysis Pass */);

bool OptimizeBitwidth::runOnFunction(Function& F) {
    m_logger->log("START OF NEW FUNCTION ANALYSIS");
    associatePHI(F);
    bool change = true, run_again = true;
    // create info for arguments
    for (auto B = F.arg_begin(), E = F.arg_end(); B != E; ++B) {
        initialize(&*B, F);
    }
    int i = 0;
    for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
        initialize(&*I); // initialize all Value (Instructions)
        i += 1;
    }

    if (loop_map.size() > 0) { // make initial update of phi values
        for (auto phi : loop_map)
            updatePHIValue(phi.first, phi.second);
    }

    while (run_again) {
        run_again = false; // every cycles based on the value of change we either run all passes
                           // again or proceed to backward iterate
        m_logger->log("New cycle of forward iterations");
        for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
            change = forwardIterate(&*I);
            if (change)
                run_again = true;
        }
    }

    inst_iterator pred_end = --inst_end(F);
    for (inst_iterator I = pred_end, E = inst_begin(F); I != E; --I) {
        backwardIterate(&*I); // run backward pass on every function from the end of the function
    }
    // One thing; after backwardIterate there is no sence at looking ont the bit_value:
    // sometimes the values are zeroed during this pass.
    // We may print stats only after forwardIterate, however some changes may not be tracked there.

#ifdef TESTS
    printStats(F);
#endif
#ifdef STATS
    printMappedStats(F);
#endif

    // #ifdef NO_VECTORIZATION
    //   checkCorrectness();
    // #endif
    clearResources();
    return true;
}

void OptimizeBitwidth::clearResources() {
    op_backward_map.clear();
    op_all_map.clear();
    op_map.clear();

    for (auto stat : stat_map)
        delete (stat.second);
    stat_map.clear();
    phi_s.clear();
    for (auto i : loop_map)
        delete i.second;
    loop_map.clear();
}

// will not fail only if there will be no vectorization
// you should unset the NO_VECTORIZATION flag in  cmakelists
// void OptimizeBitwidth::checkCorrectness() {
//
//   for (auto elem : bit_info) {
//     assert(elem.second->m_left >= 0);
//     assert(elem.second->m_left < op::S_MAX);
//     assert(elem.second->m_right >= 0);
//     assert(elem.second->m_right < op::S_MAX);
//     assert(elem.second->m_right <= elem.second->m_left);
//   }
// }
/*
 * Applies a backward function to every instruction
 */
bool OptimizeBitwidth::backwardIterate(Instruction* I) {
    /*don't do this for load instruction - this a little bit nonintuitive
    we will assume that the value is always loaded to the correct full register*/
    if (isa<LoadInst>(I))
        return true;
    m_logger->log("Inside backwardIterate");
    m_logger->log("INSTRUCTION:");
    printObj(I, *print_ostream);

    Info* i_info = bit_info.at(dyn_cast<Value>(I));
    Info* user_info;
    Info* cur_max  = new Info();
    *cur_max       = *i_info;
    Info* max_prev = new Info(); // every time we will compare cur_max and max_prev

    int opcode;
    bool first = 0;
    for (Value::user_iterator i = I->user_begin(), end = I->user_end(); i != end; ++i) {
        Value* user = dyn_cast<Value>(*i);
        user_info   = bit_info.at(dyn_cast<Value>(*i));
        if (auto I = dyn_cast<Instruction>(*i)) {
            opcode = I->getOpcode();
            m_logger->log("INSTRUCTION USER DUMP:");
            printObj(I, *print_ostream);
        } else { // we process only objects that can be casted to instructions
            m_logger->log("Unprocessible instruction - not an instruction");
            continue;
        }
        switch (opcode) {
            /*
             * These three are so hard to analyze that we do nothing
             */
            case Instruction::SDiv:
            case Instruction::UDiv:
            case Instruction::SRem:
            case Instruction::URem: {
                m_logger->log("Backward div/rem");
                backwardRedirect(i_info, cur_max);
                break;
            }
            /*
             * Here we can simply redirect the data:
             * We don't need more bits in the input, than it is in the output,
             * due to specific of these three operations,
             * so we always truncate the inputs
             */
            case Instruction::Trunc:
            case Instruction::And:
            case Instruction::Or: {
                m_logger->log("Backward and/trunc/or");
                backwardRedirect(user_info, cur_max);
                break;
            }
            case Instruction::Add: {
                m_logger->log("Backward add");
                backwardAdd(i_info, cur_max, user_info);
                break;
            }
            case Instruction::Sub: {
                m_logger->log("Backward sub");
                backwardAdd(i_info, cur_max, user_info);
                break;
            }
            case Instruction::LShr: {
                m_logger->log("Backward lshr");
                backwardShr(user, i_info, cur_max);
                break;
            }
            case Instruction::Xor: {
                m_logger->log("Backward xor");
                backwardXor(dyn_cast<Instruction>(*i), i_info, user_info, cur_max);
                break;
            }
            case Instruction::Mul: {
                m_logger->log("Backward mul");
                backwardMul(dyn_cast<Instruction>(*i), i_info, user_info, cur_max);
                break;
            }
            case Instruction::Shl: {
                m_logger->log("Backward shl");
                backwardShl(dyn_cast<Instruction>(*i), i_info, user_info, cur_max);
                break;
            }
            case Instruction::SExt:
            case Instruction::ZExt: {
                m_logger->log("Backward sext/zext");
                backwardExt(i_info, user_info, cur_max);
                break;
            }
            default:
                break;
        }
        if (!first) {
            *max_prev = *cur_max;
            first     = 1;
        } else {
            setMax(cur_max, max_prev);
            assert(max_prev != nullptr);
            *max_prev = *cur_max;
        }
    }
    if (first) {
        setMin(i_info, cur_max);
    }
    countBackwardStats(I, i_info, 0);

    m_logger->log("End of backward iterate");
    m_logger->log("New i_info dump");
    printObj(I, *print_ostream);
    m_logger->log("to m_left : %, to m_right: %, to m_bit: %", unsigned(i_info->m_left),
                  unsigned(i_info->m_right), unsigned(i_info->m_bitwidth));

    delete cur_max;
    delete max_prev;
    return true;
}

/*
 * An additional function to count only backward stats,
 * statistics here does not correspond to the instruction, we can say for this data :
 * for this instruction so many bits were eliminated after traversing all it's users.
 */
void OptimizeBitwidth::countBackwardStats(Instruction* I, Info* after, int op_code) {
    Value* v = dyn_cast<Value>(I);
    assert(v != nullptr);
    if (dyn_cast<ConstantInt>(v))
        return; // there will be now proit from variables
    std::string op_name = I->getOpcodeName();
    if (op_backward_map.find(op_name) == op_backward_map.end())
        op_backward_map.insert(std::make_pair(op_name, 0));
    Stats* cur_stats = stat_map.at(v);
    op_backward_map.at(op_name) += cur_stats->m_bits_after - after->m_left;
    cur_stats->m_bits_after = after->m_left;
}

/*
 * Reassign the value. Uses copy constructor
 * For all backward passes:
 * propagating upper all values that were identified will be eliminated,
 * because tracking them backward we inevitably poison the good bits in m_bit_value,
 * so in order the result is consistent.
 */
inline void OptimizeBitwidth::backwardRedirect(Info* from, Info* to) { *to = *from; }
/*
 * Obviously here we truncate the length of the user.
 * We can't benefit here from eliminating most significant bits as
 * further it may influence the result, we can eliminate only less significant ones.
 */
void OptimizeBitwidth::backwardExt(Info* i_info, Info* user_info, Info* cur_max) {
    m_logger->log("BACKWARD EXT");
    cur_max->m_left  = i_info->m_left;
    cur_max->m_right = (user_info->m_right > i_info->m_left) ? i_info->m_left : user_info->m_right;
    cur_max->m_bit_value    = op::S_ZERO;
    cur_max->m_unknown_bits = op::S_ZERO;
    setOneDyn(cur_max, cur_max->m_left, cur_max->m_right);
    cur_max->m_extend_point = i_info->m_left;

    m_logger->log("AFTER BACKWARD EXT");
    printBits(cur_max);
    printDynBits(cur_max);
    m_logger->log("Bit info: m_left: %, m_right: %, m_bitwidth: %", unsigned(cur_max->m_left),
                  unsigned(cur_max->m_right), unsigned(cur_max->m_bitwidth));
}

/*Shift passes:
 * Here and in the next one we do shifting only if we know the exact value;
 * We don't need to care about the exact values, as in backward passes we consider only the lengths.
 * We detect the minimum and maximum shift as usual;
 * If there is any unknown bit in shift value - set the result to zero;
 * If not - shift to the opposite side the value and the dynamic bits
 */
void OptimizeBitwidth::backwardShl(Instruction* I, Info* i_info, Info* user_info, Info* cur_max) {
    m_logger->log("BACKWARD SHL");
    Value* op0     = I->getOperand(0);
    Value* op1     = I->getOperand(1);
    Info* op0_info = bit_info.at(op0);
    Info* op1_info = bit_info.at(op1);
    uint8_t min_shift;
    uint8_t max_shift;
    if (op::any(op1_info->m_unknown_bits)) {
        min_shift = minPossibleVal(op1_info);
        max_shift = maxPossibleVal(op1_info);
        if (max_shift > op::S_MAX)
            max_shift = op::S_MAX;
    } else {
        min_shift = max_shift = op1_info->m_bit_value;
    }
    cur_max->m_right = (user_info->m_right > max_shift) ? (user_info->m_right - max_shift) : 0;
    cur_max->m_left  = (user_info->m_left > min_shift) ? (user_info->m_left - min_shift) : 0;
    if (!op::any(op1_info->m_unknown_bits)) {
        m_logger->log("No unknown bits");
        cur_max->m_bit_value    = user_info->m_bit_value >> min_shift;
        cur_max->m_unknown_bits = user_info->m_unknown_bits >> min_shift;
        correctDyn(cur_max);
    } else {
        m_logger->log("some unknown bits");
        cur_max->m_bit_value = op::S_ZERO;
        setOneDyn(cur_max, cur_max->m_right, cur_max->m_left);
        correctDyn(cur_max);
    }

    m_logger->log("AFTER BACKWARD shl");
    printBits(cur_max);
    printDynBits(cur_max);
    m_logger->log("Bit info: m_left: %, m_right: %, m_bitwidth: %", unsigned(cur_max->m_left),
                  unsigned(cur_max->m_right), unsigned(cur_max->m_bitwidth));
}

void OptimizeBitwidth::backwardShr(Value* v, Info* i_info, Info* cur_max) {
    m_logger->log("BACKWARD SHR");
    Value* op0;
    Value* op1;
    Instruction* I;
    if (I = dyn_cast<Instruction>(v)) {
        op0 = I->getOperand(0);
        op1 = I->getOperand(1);
        assert(op0 != nullptr);
        assert(op1 != nullptr);
    } else {
        m_logger->log("The value could not be converted to instruction");
        return;
    }

    Info* op0_info  = bit_info.at(op0);
    Info* op1_info  = bit_info.at(op1);
    Info* dest_info = bit_info.at(v);

    if (op1_info == i_info) {
        cur_max = i_info;
    } else {
        uint8_t min_shift;
        uint8_t max_shift;
        if (op::any(op1_info->m_unknown_bits)) {
            min_shift = minPossibleVal(op1_info);
            max_shift = maxPossibleVal(op1_info);
            if (max_shift > op::S_MAX)
                max_shift = op::S_MAX;
        } else {
            min_shift = max_shift = op1_info->m_bit_value;
        }

        cur_max->m_left =
            std::max(unsigned(dest_info->m_left + max_shift), unsigned(dest_info->m_bitwidth - 1));

        if (op::any(op1_info->m_unknown_bits)) {
            setOneDyn(cur_max, cur_max->m_right, cur_max->m_bitwidth - 1);
            cur_max->m_right = ((dest_info->m_right + min_shift) <= (dest_info->m_bitwidth - 1))
                                   ? (op0_info->m_right + min_shift)
                                   : 0;
            cur_max->m_left = ((dest_info->m_right + min_shift) <= (dest_info->m_bitwidth - 1))
                                  ? (dest_info->m_left)
                                  : 0;
            cur_max->m_bit_value = op::S_ZERO;
            setOneDyn(cur_max, cur_max->m_right, cur_max->m_left);
        } else {
            m_logger->log("All bit are known");
            cur_max->m_right = ((op0_info->m_right + min_shift) <= (cur_max->m_bitwidth - 1))
                                   ? (op0_info->m_right + min_shift)
                                   : 0;
            cur_max->m_bit_value    = op0_info->m_bit_value << min_shift;
            cur_max->m_unknown_bits = op0_info->m_unknown_bits << min_shift;
            updateLeft(cur_max);
            updateRight(cur_max);
        }

        cur_max->m_extend_point =
            std::min(cur_max->m_left, uint8_t(op0_info->m_extend_point + max_shift));
        correctDyn(cur_max);

        m_logger->log("AFTER BACKWARD shr");
        printBits(cur_max);
        printDynBits(cur_max);
        m_logger->log("Bit info: m_left: %, m_right: %, m_bitwidth: %", unsigned(cur_max->m_left),
                      unsigned(cur_max->m_right), unsigned(cur_max->m_bitwidth));
    }
}

/*
 * In xor instruction we can eliminate bits in one operand only if the other is not used somewhere
 * else. Example: 00110 = xor %a, %b. We want to trucate the upper two bits. If we will do this only
 * for the one operand, the result may change. We want to do this trucation for both. But until no
 * other instructions demand upper bits of one of the operands, we can't do this. So we always check
 * the condition of single usage.
 */
void OptimizeBitwidth::backwardXor(Instruction* I, Info* i_info, Info* user_info, Info* cur_max) {
    // get the operands of the user instruction
    Value* op0      = I->getOperand(0);
    Value* op1      = I->getOperand(1);
    Info* op0_info  = bit_info.at(op0);
    Value* other_in = (op0_info == i_info) ? op1 : op0;

    // now we should check that the other operand is not a constant and is used only once
    if (other_in->hasOneUse() && (dyn_cast<ConstantInt>(other_in) == nullptr)) {
        cur_max->m_left  = user_info->m_left;
        cur_max->m_right = user_info->m_right;
        correctDyn(cur_max);
        cur_max->m_extend_point = std::min(user_info->m_extend_point, cur_max->m_extend_point);
    } else {
        *cur_max = *i_info;
    }
    m_logger->log("After BACKWARD X");
    m_logger->log("Bit info: m_left: %, m_right: %, m_bitwidth: %", unsigned(cur_max->m_left),
                  unsigned(cur_max->m_right), unsigned(cur_max->m_bitwidth));
}

/*
 * when we multiply (suppose we have 6 bit system):
 *     1XX011
 *     X1X000
 *     ------
 *     000000
 *    000000
 *   000000
 *  XXX0XX
 * ..
 * ----------
 *  ..    000
 * you see that three Xs that are on the left, don't change anything ( as they are out of the border
 * of 6 bits so we can set them zero.
 *
 */
void OptimizeBitwidth::backwardMul(Instruction* I, Info* i_info, Info* user_info, Info* cur_max) {
    m_logger->log("BACKWARD MUL");
    Value* op0      = I->getOperand(0);
    Value* op1      = I->getOperand(1);
    Info* op0_info  = bit_info.at(op0);
    Value* other_in = (op0_info == i_info) ? op1 : op0;

    *cur_max = *i_info;
    if (bit_info.at(other_in)->m_right > 0) {
        m_logger->log("There are some zeros on the right");
        cur_max->m_left -= bit_info.at(other_in)->m_right;
        correctDyn(cur_max);
        setZero(cur_max, cur_max->m_left + 1, cur_max->m_bitwidth - 1);
    }
    m_logger->log("After BACKWARD MUL");
    m_logger->log("Bit info: m_left: %, m_right: %, m_bitwidth: %", unsigned(cur_max->m_left),
                  unsigned(cur_max->m_right), unsigned(cur_max->m_bitwidth));
}

/*
 * This function only can change the left most bit:
 * if add result is less then current - why should we keep upper bits in current?
 */
void OptimizeBitwidth::backwardAdd(Info* i_info, Info* cur_max, Info* user_info) {
    m_logger->log("BACKWARD ADD");
    *cur_max = *i_info;
    if (cur_max->m_left > user_info->m_left) {
        cur_max->m_left = user_info->m_left;
        setZero(cur_max, cur_max->m_left + 1, cur_max->m_bitwidth - 1);
        setZeroDyn(cur_max, cur_max->m_left + 1, cur_max->m_bitwidth - 1);
        cur_max->m_extend_point = std::min(cur_max->m_extend_point, cur_max->m_left);
    }
    m_logger->log("AFTER BACKWARD ADD");
    printBits(cur_max);
    printDynBits(cur_max);
    m_logger->log("Bit info: m_left: %, m_right: %, m_bitwidth: %", unsigned(cur_max->m_left),
                  unsigned(cur_max->m_right), unsigned(cur_max->m_bitwidth));
}

// TODO: should be eliminated perhaps
void OptimizeBitwidth::setMoreDefined(Info* to, Info* from) {
    m_logger->log("INSIDE SET MORE DEFINED");
    to->m_left     = std::min(to->m_left, from->m_left);
    to->m_bitwidth = std::min(to->m_bitwidth, from->m_bitwidth);
    to->m_right    = std::min(to->m_right, from->m_right);
    //там какое-то другое правило, я пока не доперла
    to->m_extend_point = std::max(to->m_extend_point, from->m_extend_point);
    to->m_unknown_bits = (to->m_left == from->m_left) ? from->m_unknown_bits : to->m_unknown_bits;
    to->m_bit_value    = (to->m_left == from->m_left) ? from->m_bit_value : to->m_bit_value;
}
/*
 * Combines two inputs to one result, choosing maximum parameters
 */
void OptimizeBitwidth::setMax(Info* to, Info* from) {
    m_logger->log("INSIDE SET MAX");
    to->m_left         = std::max(to->m_left, from->m_left);
    to->m_bitwidth     = std::max(to->m_bitwidth, from->m_bitwidth);
    to->m_right        = std::min(to->m_right, from->m_right);
    to->m_extend_point = std::max(to->m_extend_point, from->m_extend_point);
    to->m_unknown_bits =
        (to->m_unknown_bits | from->m_unknown_bits) | (to->m_bit_value ^ from->m_bit_value);
    to->m_bit_value = to->m_bit_value & from->m_bit_value;

    setZeroDyn(to, to->m_left + 1, to->m_bitwidth - 1);
    setZero(to, to->m_left + 1, to->m_bitwidth - 1);
    setZeroDyn(to, 0, to->m_right - 1);
    setZero(to, 0, to->m_right - 1);

    m_logger->log("to m_left : %, to m_right: %, to m_bit: %", unsigned(to->m_left),
                  unsigned(to->m_right), unsigned(to->m_bitwidth));
    printBits(to);
    printDynBits(to);
    m_logger->log("END OF SET MAX");
}

/*
 * Combines two inputs to one result, choosing minimum parameters
 */
void OptimizeBitwidth::setMin(Info* to, Info* from) {
    m_logger->log("INSIDE SET MIN");
    to->m_left         = std::min(to->m_left, from->m_left);
    to->m_bitwidth     = std::min(to->m_bitwidth, from->m_bitwidth);
    to->m_right        = std::min(to->m_left, std::max(to->m_right, from->m_right));
    to->m_extend_point = std::min(to->m_extend_point, from->m_extend_point);
    to->m_unknown_bits =
        (to->m_unknown_bits | from->m_unknown_bits) | (to->m_bit_value ^ from->m_bit_value);
    to->m_bit_value = to->m_bit_value & from->m_bit_value;

    setZeroDyn(to, to->m_left + 1, to->m_bitwidth - 1);
    setZero(to, to->m_left + 1, to->m_bitwidth - 1);
    setZeroDyn(to, 0, to->m_right - 1);
    setZero(to, 0, to->m_right - 1);
    m_logger->log("to m_left : %, to m_right: %, to m_bit: %", unsigned(to->m_left),
                  unsigned(to->m_right), unsigned(to->m_bitwidth));
    printBits(to);
    printDynBits(to);
    m_logger->log("END OF SET MIN");
}

/*
 * Not used by now, I will keep it until the final version of PHI analysis will be accepted
 */
void OptimizeBitwidth::findPHINodes() {
    LoopInfo& LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    for (LoopInfo::iterator LIT = LI.begin(); LIT != LI.end(); ++LIT) {
        m_logger->log("START OF LOOP PROCESSING");
        Loop* ll      = *LIT;
        BasicBlock* H = ll->getHeader();

        BasicBlock *Incoming = nullptr, *Backedge = nullptr;
        pred_iterator PI = pred_begin(H);
        assert(PI != pred_end(H) && "Loop must have at least one backedge!");
        Backedge = *PI++;
        if (PI == pred_end(H)) {
            m_logger->log("DEAD LOOP!");
        } // dead loop
        Incoming = *PI++;
        if (PI != pred_end(H)) {
            m_logger->log("MULTIPLE BACKEDGES");
        }; // multiple backedges?

        if (ll->contains(Incoming)) {
            if (ll->contains(Backedge))
                m_logger->log("CASE 1");
            std::swap(Incoming, Backedge);
        } else if (!ll->contains(Backedge))
            m_logger->log("CASE 2");

        // Loop over all of the PHI nodes, looking for a canonical indvar.
        for (BasicBlock::iterator I = H->begin(); isa<PHINode>(I); ++I) {
            PHINode* PN = cast<PHINode>(I);
            printObj(PN, *print_ostream);
        }
    }
}

/*
 * Get every loop and process it and all subloops
 */
void OptimizeBitwidth::associatePHI(Function& F) {
    m_logger->log("Inside associatePHI");
    LoopInfo& LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    for (LoopInfo::iterator LIT = LI.begin(); LIT != LI.end(); ++LIT) {
        m_logger->log("START OF LOOP PROCESSING");
        Loop* ll = *LIT;
        processLoop(ll);
        m_logger->log("END OF LOOP PROCESSING");
        phi_s.clear();
        for (auto sll : ll->getSubLoops()) {
            m_logger->log("START OF SUBLOOP PROCESSING");
            processLoop(sll);
            m_logger->log("END OF SUBLOOP PROCESSING");
            phi_s.clear();
        }
    }
}

/*
 * Process header and preheader of the loop, process every block,
 * then process every phi instruction.
 * For special processing we either detect phi nodes in header or
 * in preheader of the loop (where we will find it depends on how llvm was able to optimize the
 * loop)
 */
void OptimizeBitwidth::processLoop(Loop* ll) {
    m_logger->log("Inside process loop");
    /*find all phi nodes, those, that are interesting for us can be ONLY im header or in preheader*/
    BasicBlock* BB  = ll->getHeader();
    BasicBlock* BB1 = ll->getLoopPreheader();
    if (BB) // sometimes loop does not have either header or preheader, so we should check on
            // existence
        processBlock(BB);
    if (BB1)
        processBlock(BB1);
    for (auto phi : phi_s)
        processInstruction(ll, phi);
}

/*
 * Process block by finding all phi nodes with two incoming values
 * TODO: fix question with negative numbers while descending
 */
void OptimizeBitwidth::processBlock(BasicBlock* BB) {
    m_logger->log("Inside process block");
    for (Instruction& I : *BB) {
        if (PHINode* phi = dyn_cast<PHINode>(&I))
            if ((phi->getNumIncomingValues() == 2) && (phi_s.find(phi) == phi_s.end())) {
                phi_s.insert(phi);
            }
    }
    m_logger->log("processBlock: The size of PHI SET is: %", int(phi_s.size()));
}

/*
 * Check compare condition : if it is OK,
 * then determine the order of the incoming blocks inside phi node
 */
void OptimizeBitwidth::processInstruction(Loop* ll, PHINode* phi) {
    m_logger->log("PROCESS INSTRUCTION:");
    printObj(phi, *print_ostream);
    bool compareConditionOk = false;
    compareConditionOk      = checkCompareCondition(ll, phi);
    m_logger->log("compare condition: %", compareConditionOk);
    if (loop_map.find(phi) != loop_map.end())
        assert(loop_map.at(phi)->m_cmp != nullptr);
    if (compareConditionOk)
        loop_map.at(phi)->m_order = determineBlockOrder(ll, phi);
    else
        loop_map.erase(phi);
    m_logger->log("loop_map size : %", int(loop_map.size()));
}

/*
 * Compare condition :
 * State: If there is a terminator branch instruction, then to successfully process further,
 * 	  we need to have cmp against phi or one of it's operands or against sext / trunc that has
 * as a predecessor one of phi's operands, which should be again binary operations. Notes: If any of
 * threes conditions is true then put compare instruction.
 */
bool OptimizeBitwidth::checkCompareCondition(Loop* ll, PHINode* phi) {
    assert((ll != nullptr) && (phi != nullptr));
    m_logger->log("CHECK COMPARE CONDITION");
    printObj(phi, *print_ostream);
    Value* v  = phi->getIncomingValue(0);
    Value* v1 = phi->getIncomingValue(1);
    SmallVector<BasicBlock*, 0> ExitingBlocks;
    ll->getExitingBlocks(ExitingBlocks);
    std::vector<Value*> conditions;
    for (auto block : ExitingBlocks) {
        Value* t = block->getTerminator();
        if (BranchInst* BI = dyn_cast<BranchInst>(t))
            if (BI->isConditional())
                conditions.push_back(BI->getCondition());
    }
    if (conditions.size() != 1)
        return false;
    Instruction* CMPI = dyn_cast<Instruction>(conditions.at(0));
    assert(CMPI != nullptr);
    m_logger->log("conditions size: %", conditions.size());
    if (CMPI->getOperand(0) == dyn_cast<Value>(phi)) {

        m_logger->log("FIRST IF");
        PHIInfo* phiinfo = new PHIInfo(conditions.at(0), 0, 0);
        loop_map.insert(std::make_pair(phi, phiinfo));
        bool b1 = checkBinary(v, phi);
        bool b2 = checkBinary(v1, phi);
        m_logger->log("condition : %", b1 || b2);
        if (b1 || b2)
            m_logger->log("Pattern: %", phiinfo->m_pattern);
        return (b1 || b2);

    } else if ((CMPI->getOperand(0) == v) || (CMPI->getOperand(0) == v1)) {

        m_logger->log("SECOND IF");
        PHIInfo* phiinfo = new PHIInfo(conditions.at(0), 0, 0);
        loop_map.insert(std::make_pair(phi, phiinfo));
        Value* inc_op = dyn_cast<Instruction>(CMPI)->getOperand(0);
        bool b        = checkBinary(inc_op, phi);
        m_logger->log("condition : %", b);
        if (b)
            m_logger->log("Pattern: %", phiinfo->m_pattern);
        return b;

    } else if (checkCastCondition(phi, CMPI)) {
        m_logger->log("THIRD IF");
        PHIInfo* phiinfo = new PHIInfo(conditions.at(0), 0, 0);
        loop_map.insert(std::make_pair(phi, phiinfo));
        CastInst* CI = dyn_cast<CastInst>(CMPI->getOperand(0));
        bool b       = checkBinary(CI->getOperand(0), phi);
        m_logger->log("condition : %", b);
        if (b)
            m_logger->log("Pattern: %", phiinfo->m_pattern);
        return b;
    }
    m_logger->log("NOT ALL OK");
    return false;
}
/*
 * Check that inside cast we have one of phi's operand
 */
bool OptimizeBitwidth::checkCastCondition(PHINode* phi, Instruction* CMPI) {
    m_logger->log("CHECK CAST COND");
    if (CastInst* CI = dyn_cast<CastInst>(CMPI->getOperand(0))) {
        m_logger->log("This is cast");
        if ((phi->getIncomingValue(0) == CI->getOperand(0)) ||
            (phi->getIncomingValue(1) == CI->getOperand(0)))
            return true;
    }
    return false;
}

/*
 * Checks if the instruction may be converted to binary operations,
 * and if yes, determine the types of operation. Sets the according field in data structures.
 */
bool OptimizeBitwidth::checkBinary(Value* op, PHINode* phi) {
    m_logger->log("CHECK BINARY");
    if (BinaryOperator* BO = dyn_cast<BinaryOperator>(op)) {
        printObj(BO, *print_ostream);
        m_logger->log("casted to binary operator");
        if (phi == BO->getOperand(0) || phi == BO->getOperand(1)) {
            m_logger->log("Appropriate operand");
            std::string opcode = BO->getOpcodeName();
            if (opcode == "add" || opcode == "mul" || opcode == "shl") {
                m_logger->log("UP BINARY INSTR");
                // we should be aware of llvm's add, %x, -1 like operations, so this is subtraction
                // in fact
                if ((phi == BO->getOperand(0)) && isa<ConstantInt>(BO->getOperand(1)))
                    if (dyn_cast<ConstantInt>(BO->getOperand(1))->isNegative()) {
                        m_logger->log("Need to change the pattern");
                        loop_map.at(phi)->m_pattern = 0;
                        return true;
                    } else if ((phi == BO->getOperand(1)) && isa<ConstantInt>(BO->getOperand(0)))
                        if (dyn_cast<ConstantInt>(BO->getOperand(1))->isNegative()) {
                            m_logger->log("Need to change the pattern");
                            loop_map.at(phi)->m_pattern = 0;
                            return true;
                        }
                loop_map.at(phi)->m_pattern = 1;
                return true;
            } else if (opcode == "sub" || opcode == "udiv" || opcode == "sdiv" ||
                       opcode == "urem" || opcode == "srem" || opcode == "lshr") {
                m_logger->log("DOWN BINARY INSTR");
                loop_map.at(phi)->m_pattern = 0;
                return true;
            } else {
                m_logger->log("All other instructions");
                return false; // other instructions we don't process
            }
        } else {
            m_logger->log("Not good operand");
        }
    }
    return false;
}

/*
 * To correctly determine the border, we should determine the order of the blocks
 * They may come as :  [prev], [post]
 * 		    or [post], [prev]
 * We set return value to zero if the order is ok and to one if reverse
 */
int OptimizeBitwidth::determineBlockOrder(Loop* ll, PHINode* phi) {
    BasicBlock* bb2 = phi->getIncomingBlock(1);
    bool dom1       = (ll->contains(bb2)) ? 1 : 0;
    return (dom1) ? 0 : 1;
}

/*
 * This function performs the update of phi values, which were added inside loop_map
 * during the first pass.
 * Then we assign the value based on the pattern and the order of incoming blocks.
 * To remind: pattern : 1 - accending, 0 - decending.
 *            order   : 1 - reverse,   0 - natural.
 *
 * So:
 *   order - normal,   pattern - accending-> then we are assigning the value against which we are
 * comparing; order - normal,   pattern - decending-> then we are assigning the value of the first
 * incoming value; order - unnormal, pattern - accending-> then we are assigning the value against
 * which we are comparing; order - unnormal, pattern - accending-> then we are assigning the value
 * of the second incoming value; All these rules are correct as we have chosen only special phi
 * nodes which play roles of induction variables, and here we process phi nodes with no more than
 * two incoming values.
 *
 * Negative Values:
 * 	if we have comparision with negative value in descending pattern, we should check this case
 * separately. if in ascending : 1) compare like i > -N : i++ - then the value will be automatically
 * set to -N or to start value - we don't bother 2) compare like < -N - then we can start only with
 * negative value and again everything is ok
 *
 * We also chack two cases:
 * if in ascending pattern we are comparing with the value less than initial
 * if in descending pattern we are comparing with with the value greater than initial
 */
void OptimizeBitwidth::updatePHIValue(PHINode* phi, PHIInfo* info) {
    m_logger->log("UPDATE PHI VALUES");
    Value* inc_0   = phi->getIncomingValue(0);
    Value* inc_1   = phi->getIncomingValue(1);
    Info* phi_info = bit_info.at(dyn_cast<Value>(phi));

    if (loop_map.find(phi) != loop_map.end()) { // we need to check it again as we may have deleted
                                                // in the upper part something
        Instruction* CMPI = dyn_cast<Instruction>(info->m_cmp);

        if ((info->m_pattern == 0) &&
            (bit_info.at(dyn_cast<Instruction>(info->m_cmp)->getOperand(1))->m_left ==
             (bit_info.at(dyn_cast<Instruction>(info->m_cmp)->getOperand(1))->m_bitwidth - 1))) {
            m_logger->log(
                "Case when we have comparision with negative value in descending pattern");
            createUndefined(phi_info);
        } else if (loop_map.at(phi)->m_order == 0) {
            m_logger->log("normal order");
            printDynBits(bit_info.at(CMPI->getOperand(1)));
            printDynBits(bit_info.at(inc_0));
            bool sanityCond =
                (info->m_pattern == 1)
                    ? checkSanityCondition(1, bit_info.at(CMPI->getOperand(1)), bit_info.at(inc_0))
                    : checkSanityCondition(0, bit_info.at(CMPI->getOperand(1)), bit_info.at(inc_0));
            if (sanityCond)
                *phi_info = (info->m_pattern == 1) ? *bit_info.at(CMPI->getOperand(1))
                                                   : *bit_info.at(inc_0);
            else
                createUndefined(phi_info);

        } else {
            m_logger->log("unnormal order");
            printDynBits(bit_info.at(CMPI->getOperand(1)));
            printDynBits(bit_info.at(inc_0));
            if (Instruction* I = dyn_cast<Instruction>(inc_0))
                printObj(inc_0, *print_ostream);
            m_logger->log("Pattern: %", info->m_pattern);
            bool sanityCond =
                (info->m_pattern == 1)
                    ? checkSanityCondition(1, bit_info.at(CMPI->getOperand(1)), bit_info.at(inc_1))
                    : checkSanityCondition(0, bit_info.at(CMPI->getOperand(1)), bit_info.at(inc_1));
            if (sanityCond)
                *phi_info = (info->m_pattern == 1) ? *bit_info.at(CMPI->getOperand(1))
                                                   : *bit_info.at(inc_1);
            else
                createUndefined(phi_info);
        }

    } else {
        m_logger->log("We deleted in the upper part this phi");
    }
    m_logger->log("AFTER update PHI");
    printBits(phi_info);
    printDynBits(phi_info);
    m_logger->log("Bit info: m_left: %, m_right: %, m_bitwidth: %", unsigned(phi_info->m_left),
                  unsigned(phi_info->m_right), unsigned(phi_info->m_bitwidth));
}

/*
 * We will check here some cases that will not be processed because they may produce strange results
 * Sometimes this may be true, but we can end in infinite cycle from the llvm's point of view,
 * so the value of phi will be undefined in both cases
 */
bool OptimizeBitwidth::checkSanityCondition(int type, Info* info1, Info* info2) {
    m_logger->log("Checking sanity condition");
    assert((type == 1) || (type == 0));
    if (type == 1) {
        if (info1->m_left < info2->m_left) {
            m_logger->log("Sanity check fails for ascending pattern");
            return false;
        }
    } else if (type == 0) {
        if (info1->m_left > info2->m_left) {
            m_logger->log("Sanity check fails for descending pattern");
            return false;
        }
    }
    return true;
}

/*
 * applies a forward function to every instruction
 */
bool OptimizeBitwidth::forwardIterate(Instruction* I) {
    bool change = false;
    m_logger->log("Inside forwardIterate");
    uint opcode = I->getOpcode();
    switch (opcode) {
        // add instruction differs as we also may have before it subtraction
        case Instruction::Add: {
            Value* op0 = I->getOperand(0);
            Value* op1 = I->getOperand(1);

            Info* cur_val  = bit_info.at(dyn_cast<Value>(I));
            Info* op0_info = bit_info.at(op0);
            Info* op1_info = bit_info.at(op1);
            change         = forwardAdd(I, cur_val, op0_info, op1_info, false);
            break;
        }
        case Instruction::And:
            change = forwardAnd(I);
            break;
        case Instruction::Or:
            change = forwardOr(I);
            break;
        case Instruction::Shl:
            change = forwardShl(I);
            break;
        case Instruction::LShr:
            change = forwardLShr(I);
            break;
        case Instruction::AShr:
            change = forwardAShr(I);
            break;
        case Instruction::ZExt:
            change = forwardZExt(I);
            break;
        case Instruction::SExt:
            change = forwardSExt(I);
            break;
        case Instruction::Sub:
            change = forwardSub(I);
            break;
        case Instruction::PHI:
            change = forwardPHI(I);
            break;
        case Instruction::Select:
            change = forwardSelect(I);
            break;
        case Instruction::UDiv:
            change = forwardUDiv(I);
            break;
        case Instruction::SDiv:
            change = forwardSDiv(I);
            break;
        case Instruction::SRem:
            change = forwardSRem(I);
            break;
        case Instruction::URem:
            change = forwardURem(I);
            break;
        case Instruction::Trunc:
            change = forwardTrunc(I);
            break;
        case Instruction::Mul:
            change = forwardMul(I);
            break;
        case Instruction::Xor:
            change = forwardXor(I);
        default:
            return false;
    }
    return change;
}

/*
 * Details:
 * 	  we can't benefit much in mul from the values, we can mostly determine the size of the
 * result and the value of multiplication under the least significant bit. Example:
 * 	  XXX100
 * 	  X11000
 * 	 -------
 * 	       0 <- the last bit will be definetly zero, and so on
 * Rule:
 * 	  Set m_left to sum of operand 1 m_left, operand 2 m_left and 1;
 *        set m_right to maximum of operand 1 m_right and operand 2 m_right.
 * 	  multiply values;
 * 	  update bits from rightmost value to the end.
 */
bool OptimizeBitwidth::forwardMul(Instruction* I) {
    m_logger->log("FORWARD MUL");
    printObj(I, *print_ostream);
    Info* cur_val  = bit_info.at(dyn_cast<Value>(I));
    Info* op0_info = bit_info.at(I->getOperand(0));
    Info* op1_info = bit_info.at(I->getOperand(1));

    cur_val->m_left  = std::min(cur_val->m_bitwidth - 1, op0_info->m_left + op1_info->m_left + 1);
    cur_val->m_right = std::max(op0_info->m_right, op1_info->m_right);
    cur_val->m_extend_point =
        std::min(cur_val->m_bitwidth - 1, op0_info->m_extend_point + op1_info->m_extend_point + 1);

    cur_val->m_bit_value  = op0_info->m_bit_value * op1_info->m_bit_value;
    uint8_t rightmost_dyn = std::min(findRightDyn(op0_info), findRightDyn(op1_info));
    setZero(cur_val, rightmost_dyn + 1, cur_val->m_bitwidth - 1);
    setOneDyn(cur_val, rightmost_dyn + 1, cur_val->m_bitwidth - 1);
    correctDyn(cur_val);

    m_logger->log("AFTER MUL");
    printBits(cur_val);
    printDynBits(cur_val);
    m_logger->log("Bit info: m_left: %, m_right: %, m_bitwidth: %", unsigned(cur_val->m_left),
                  unsigned(cur_val->m_right), unsigned(cur_val->m_bitwidth));
    return countStats(I, cur_val, 0);
}

/*
 * If we have our PHI node in loop_map  - then call update PHI,
 * otherwise we set the value to the maximum of all incoming nodes.
 * Example: phi [%a, %1], [%b, %2], [%c, %3]
 * Result value : max(%a, %b, %c)
 */
bool OptimizeBitwidth::forwardPHI(Instruction* I) {
    m_logger->log("FORWARD PHI");
    printObj(I, *print_ostream);
    PHINode* phi  = dyn_cast<PHINode>(I);
    Info* cur_val = bit_info.at(dyn_cast<Value>(I));
    if (loop_map.find(phi) != loop_map.end()) {
        m_logger->log("Inside loop map");
        updatePHIValue(phi, loop_map.at(phi));
    } else { // else : we set simply max of the incoming nodes, maybe we will benefit from it
        m_logger->log("Not inside loop_map");
        int num_val = phi->getNumIncomingValues();
        for (int i = 0; i < num_val; i++) {
            if (i == 0)
                *cur_val = *bit_info.at(phi->getIncomingValue(0));
            else
                setMax(cur_val, bit_info.at(phi->getIncomingValue(i)));
        }
    }
    return countStats(I, cur_val, 0);
}

/*
 * Select : 1) set common bits from both values,
 *             by simply setting the value from one of the operand and then making zero
 *             all bits where the values change by and-ing with reverted unknown bits.
 *          2) unknown bits are set to 1 if one the bit is unknown or if the values differs
 *             (difference in values is checked with xor instruction, this trick will also be used
 * later) 3) set maximum and minimum left most and right most bits.
 */
bool OptimizeBitwidth::forwardSelect(Instruction* I) {
    m_logger->log("FORWARD SELECT");
    printObj(I, *print_ostream);
    Value* op0 = I->getOperand(0);
    Value* op1 = I->getOperand(1);
    Value* op2 = I->getOperand(2);

    Info* cur_val  = bit_info.at(dyn_cast<Value>(I));
    Info* op1_info = bit_info.at(op1);
    Info* op2_info = bit_info.at(op2);

    cur_val->m_left         = std::max(op1_info->m_left, op2_info->m_left);
    cur_val->m_right        = std::min(op1_info->m_right, op2_info->m_right);
    cur_val->m_extend_point = std::max(op1_info->m_extend_point, op2_info->m_extend_point);
    cur_val->m_unknown_bits = ((op1_info->m_unknown_bits | op2_info->m_unknown_bits) |
                               (op1_info->m_bit_value ^ op2_info->m_bit_value));
    cur_val->m_bit_value    = op1_info->m_bit_value & ~(cur_val->m_unknown_bits);

    updateLeft(cur_val);
    updateRight(cur_val);
    correctDyn(cur_val);

    m_logger->log("AFTER SELECT");
    printBits(cur_val);
    printDynBits(cur_val);
    m_logger->log("Bit info: m_left: %, m_right: %, m_bitwidth: %", unsigned(cur_val->m_left),
                  unsigned(cur_val->m_right), unsigned(cur_val->m_bitwidth));

    return countStats(I, cur_val, 0);
}
/*
 * Add instruction is complicated by tracking the carry bit;
 * All situations are matched to three cases:
 * (plus at first we distinctly process the case of when both bits are known)
 * 1) carry_bit is one:
 * 	1.1) X+X -> undef, carry bit is undef
 * 	1.2) X+1 -> undef, carry bit is one
 * 	1.3) X+0 -> undef, carry bit is undef
 * 2) carry bit is zero
 * 	2.1) X+X -> undef, carry bit is undef
 * 	2.2) X+1 -> undef, carry bit is undef
 * 	2.3) X+0 -> undef, carry bit is zero
 * 3) carry bit is undef
 * 	3.1) X+X -> undef, carry bit is undef
 * 	3.2) X+1 -> undef, carry bit is undef
 * 	3.3) X+0 -> undef, carry bit is undef
 * 	3.4) 1+1 -> undef, carry bit is one
 */
bool OptimizeBitwidth::forwardAdd(Instruction* I, Info* cur_val, Info* op0_info, Info* op1_info,
                                  bool isSubtraction) {
    m_logger->log("FORWARD ADD");

    cur_val->m_left     = std::min(unsigned(std::max(op0_info->m_left, op1_info->m_left) + 1),
                               unsigned(cur_val->m_bitwidth - 1));
    cur_val->m_right    = std::min(op0_info->m_right, op1_info->m_right);
    cur_val->m_bitwidth = std::max(op0_info->m_bitwidth, op1_info->m_bitwidth);
    bool known_carry = true; // set the carry bit to true at first, it will be at the start - if add
                             // and 1 - is sub.
    cur_val->m_bit_value = isSubtraction ? op::S_ONE : op::S_ZERO;

    for (int i = 0; i <= cur_val->m_left; i++) {
        m_logger->log("i is equal to : %", i);
        m_logger->log("known carry: %", known_carry);
        m_logger->log("cur_val val:%", op::get(cur_val->m_bit_value, i));
        if (!op::get(op0_info->m_unknown_bits, i) && !op::get(op1_info->m_unknown_bits, i) &&
            known_carry) { // both values are known
            m_logger->log("Both known");
            cur_val->m_bit_value +=
                (op::getval(op0_info->m_bit_value, i) + op::getval(op1_info->m_bit_value, i));
            cur_val->m_unknown_bits = op::unset(cur_val->m_unknown_bits, i);
            m_logger->log("cur_val VAL");
            printBits(cur_val);
            m_logger->log("cur_val DYN");
            printDynBits(cur_val);
        } else {
            m_logger->log("In else branch");
            if (known_carry && op::get(cur_val->m_bit_value, i)) {
                m_logger->log("First case");
                if (op::get(op0_info->m_bit_value | op1_info->m_bit_value, i) &&
                    op::get(op0_info->m_unknown_bits | op1_info->m_unknown_bits, i)) {
                    m_logger->log("Carry is known in first case");
                    known_carry          = true;
                    cur_val->m_bit_value = op::set(cur_val->m_bit_value, i + 1);
                } else {
                    m_logger->log("Carry is not known in first case");
                    known_carry = false;
                }
            } else if (known_carry && !op::get(cur_val->m_bit_value, i)) {
                m_logger->log("Second case");
                if (!op::get(op0_info->m_bit_value ^ op1_info->m_bit_value, i) &&
                    op::get(op0_info->m_unknown_bits ^ op1_info->m_unknown_bits, i)) {
                    m_logger->log("Carry is known in second case");
                    known_carry          = true;
                    cur_val->m_bit_value = op::unset(cur_val->m_bit_value, i + 1);
                } else {
                    m_logger->log("Carry is not known in second case");
                    known_carry = false;
                }
            } else if (!known_carry) {
                m_logger->log("Third case");
                if (op::get(op0_info->m_bit_value & op1_info->m_bit_value, i)) {
                    m_logger->log("Carry is known in third case");
                    known_carry          = true;
                    cur_val->m_bit_value = op::set(cur_val->m_bit_value, i + 1);
                } else {
                    m_logger->log("Carry is not known in third case");
                    known_carry = false;
                }
            }
            cur_val->m_bit_value    = op::unset(cur_val->m_bit_value, i);
            cur_val->m_unknown_bits = op::set(cur_val->m_unknown_bits, i);
        }
    }

    updateLeft(cur_val);
    updateRight(cur_val); // it will mosly have effect for constant values
    correctDyn(cur_val);

    m_logger->log("AFTER ADD");
    printBits(cur_val);
    printDynBits(cur_val);
    m_logger->log("Bit info: m_left: %, m_right: %, m_bitwidth: %", unsigned(cur_val->m_left),
                  unsigned(cur_val->m_right), unsigned(cur_val->m_bitwidth));
    if (isSubtraction)
        delete op1_info;

    return countStats(I, cur_val, 0);
}

/*
 * 1) Subtraction turns into addition with the only change
 *    that we represent sub %a, %b - %b as a negative value,
 *    using twos complement form and then add it with the first operand.
 * 2) Note then when we convert number to negative, we need to revert it's bits
 *   (what we are doing here and change the left bit accordingly after it)
 *   and to add one. To avoid this actual adding, we set the initial value
 *   in the further addition to one.
 */
bool OptimizeBitwidth::forwardSub(Instruction* I) {
    m_logger->log("FORWARD SUB");
    printObj(I, *print_ostream);
    Value* op0 = I->getOperand(0);
    Value* op1 = I->getOperand(1);

    Info* cur_val  = bit_info.at(dyn_cast<Value>(I));
    Info* op0_info = bit_info.at(op0);
    Info* op1_info = bit_info.at(op1);

    Info* op1_neg           = new Info();
    op1_neg->m_bitwidth     = op1_info->m_bitwidth;
    op1_neg->m_bit_value    = ~(op1_info->m_bit_value | op1_info->m_unknown_bits);
    op1_neg->m_extend_point = op1_neg->m_left;
    op1_neg->m_unknown_bits = op1_info->m_unknown_bits;

    // when we have reverted our number
    // move left to the leftmost dynamic or static zero bit
    for (int i = cur_val->m_bitwidth - 1; i >= 0; i--)
        if (!op::get(op1_info->m_bit_value, i)) {
            op1_neg->m_left = i;
            break;
        }
    for (int i = 0; i <= cur_val->m_bitwidth - 1; i++)
        if (!op::get(op1_info->m_bit_value, i)) {
            op1_neg->m_right = i;
            break;
        }

    forwardAdd(I, cur_val, op0_info, op1_neg, true);
    return countStats(I, cur_val, 0);
}

/*
 * 1) How many bits less will we receive through division will be computed
 *    with the help of logarithm function (it computes the mazimum length
 *    of arbitrary number, that can include our minimum possible val).
 * 2) If the operand is negative or we don't know it's first 31st bit-
 *    the result is unknown.
 * 3) m_right is always set to zero - we don't compute the value and
 *    have no idea where least significant bit may be.
 */
bool OptimizeBitwidth::forwardSDiv(Instruction* I) {
    m_logger->log("FORWARD SDIV");
    printObj(I, *print_ostream);
    Value* op0 = I->getOperand(0);
    Value* op1 = I->getOperand(1);

    Info* cur_val  = bit_info.at(dyn_cast<Value>(I));
    Info* op0_info = bit_info.at(op0);
    Info* op1_info = bit_info.at(op1);

    uint8_t shift = unsigned(log2(minPossibleVal(op1_info)));

    cur_val->m_right = 0;

    if ((op1_info->m_left == (op1_info->m_bitwidth - 1)) ||
        (op0_info->m_left == (op0_info->m_bitwidth - 1))) {
        m_logger->log("one of the operand is negative");
        cur_val->m_left         = cur_val->m_bitwidth - 1;
        cur_val->m_extend_point = op0_info->m_extend_point;
    } else {
        m_logger->log("Non of the operand is negative");
        cur_val->m_left = (op0_info->m_left >= shift) ? (op0_info->m_left - shift) : 0;
        cur_val->m_extend_point =
            (op0_info->m_extend_point >= shift) ? (op0_info->m_extend_point - shift) : 0;
    }
    cur_val->m_bit_value = op::S_ZERO;
    setOneDyn(cur_val, cur_val->m_right, cur_val->m_left);
    correctDyn(cur_val);

    m_logger->log("AFTER SDIV");
    printBits(cur_val);
    printDynBits(cur_val);
    m_logger->log("Bit info: m_left: %, m_right: %, m_bitwidth: %", unsigned(cur_val->m_left),
                  unsigned(cur_val->m_right), unsigned(cur_val->m_bitwidth));

    return countStats(I, cur_val, 0);
}

/*
 * 1) The first rare situation when we know all bits
 *    (TODO : it may be eliminated if needed as is done in Sdiv)
 * 2) In the second we don't know
 * shift - is log2 from minimum value od the divider - so we can know how many bits we can get rid
 * of.
 */
bool OptimizeBitwidth::forwardUDiv(Instruction* I) {
    m_logger->log("FORWARD UDIV");
    printObj(I, *print_ostream);
    Value* op0 = I->getOperand(0);
    Value* op1 = I->getOperand(1);

    Info* cur_val  = bit_info.at(dyn_cast<Value>(I));
    Info* op0_info = bit_info.at(op0);
    Info* op1_info = bit_info.at(op1);

    uint8_t shift   = unsigned(log2(minPossibleVal(op1_info)));
    cur_val->m_left = (op0_info->m_left >= shift) ? (op0_info->m_left - shift)
                                                  : 0; //если сдвиг будет 32 то явно будет 0
    cur_val->m_right = 0;

    if (op::none(op0_info->m_unknown_bits | op1_info->m_unknown_bits)) {
        m_logger->log("Both values are known");
        cur_val->m_bit_value    = op0_info->m_bit_value / op1_info->m_bit_value;
        cur_val->m_unknown_bits = op::S_ZERO;
        updateLeft(cur_val);
        updateRight(cur_val);
    } else {
        m_logger->log("Else case");
        cur_val->m_bit_value = op::S_ZERO;
        setOneDyn(cur_val, cur_val->m_right, cur_val->m_left);
    }
    cur_val->m_extend_point =
        (op0_info->m_extend_point >= shift) ? (op0_info->m_extend_point - shift) : 0;
    correctDyn(cur_val);

    m_logger->log("AFTER UDIV");
    printBits(cur_val);
    printDynBits(cur_val);
    m_logger->log("Bit info: m_left: %, m_right: %, m_bitwidth: %", unsigned(cur_val->m_left),
                  unsigned(cur_val->m_right), unsigned(cur_val->m_bitwidth));

    return countStats(I, cur_val, 0);
}

/*
 * The rule for srem is:
 * (-A)remB = - (AremB)
 * Arem(-B) = AremB
 * The case when he second operand is negative is not handled here
 * 1) The op0 is  < 0 then we get negative number and preserve all bits
 * 2) The op0 is >= 0 then there are two cases, when op1 is < 0 or > 0 but
 * for now we treat them equally
 * (according to c and llvm standard)
 */
bool OptimizeBitwidth::forwardSRem(Instruction* I) {
    m_logger->log("FORWARD SREM");
    printObj(I, *print_ostream);
    Value* op0 = I->getOperand(0);
    Value* op1 = I->getOperand(1);

    Info* cur_val  = bit_info.at(dyn_cast<Value>(I));
    Info* op0_info = bit_info.at(op0);
    Info* op1_info = bit_info.at(op1);
    if (op0_info->m_left == (op0_info->m_bitwidth - 1)) {
        m_logger->log("First operand is negative");
        cur_val->m_left         = cur_val->m_bitwidth - 1;
        cur_val->m_extend_point = cur_val->m_bitwidth - 1;
    } else {
        m_logger->log("First operand is not negative");
        cur_val->m_left         = op1_info->m_left;
        cur_val->m_extend_point = op1_info->m_extend_point;
    }
    cur_val->m_right     = 0;
    cur_val->m_bit_value = op::S_ZERO;
    setOneDyn(cur_val, cur_val->m_right, cur_val->m_left);
    correctDyn(cur_val);
    m_logger->log("m_left : %", unsigned(cur_val->m_left));
    m_logger->log("m_right : %", unsigned(cur_val->m_right));
    m_logger->log("m_bit_value : ");
    printBits(cur_val);
    m_logger->log("m_unknown_bits: ");
    printDynBits(cur_val);

    return countStats(I, cur_val, 0);
}
/*
 * 1) The first condition happens really rarely when both values are defined
 *    (TODO : may be eliminated if needed)
 * 2) The second happens when some values are partially known
 */
bool OptimizeBitwidth::forwardURem(Instruction* I) {
    m_logger->log("FORWARD UREM");
    printObj(I, *print_ostream);
    Value* op0 = I->getOperand(0);
    Value* op1 = I->getOperand(1);

    Info* cur_val  = bit_info.at(dyn_cast<Value>(I));
    Info* op0_info = bit_info.at(op0);
    Info* op1_info = bit_info.at(op1);

    if (op::none(op0_info->m_unknown_bits | op1_info->m_unknown_bits)) {
        m_logger->log("Both values are known");
        cur_val->m_bit_value    = op0_info->m_bit_value % op1_info->m_bit_value;
        cur_val->m_unknown_bits = op::S_ZERO;
        updateLeft(cur_val);
        updateRight(cur_val);
        cur_val->m_extend_point = cur_val->m_left;
        correctDyn(cur_val);
    } else {
        m_logger->log("Value is not known");
        cur_val->m_left      = op1_info->m_left;
        cur_val->m_right     = 0;
        cur_val->m_bit_value = op::S_ZERO;
        setOneDyn(cur_val, cur_val->m_right, cur_val->m_left);
        cur_val->m_extend_point = op1_info->m_extend_point;
        correctDyn(cur_val);
    }
    m_logger->log("AFTER UREM");
    printBits(cur_val);
    printDynBits(cur_val);
    m_logger->log("Bit info: m_left: %, m_right: %, m_bitwidth: %", unsigned(cur_val->m_left),
                  unsigned(cur_val->m_right), unsigned(cur_val->m_bitwidth));
    return countStats(I, cur_val, 0);
}

/*
 * 1) We define minimum and maximum shift, as minimum and maximum possible value,
 *    if shift is not known from the beginning; if it is known, we simply take the value.
 * 2) Next step is to check, whether our max and min shift are less than the leftmost
 *    bit. If greater, than we don't need shifting more than m_left, as we will recieve always
 *    zeros anyway (as this is logical shift).
 * 3) When analyzing the possible values for new left and right bits,
 *    we need to make the maximum range for undefined values,
 *    so we apply maxshift to right and minshift to left
 */
bool OptimizeBitwidth::forwardLShr(Instruction* I) {
    m_logger->log("FORWARD LSHR");
    printObj(I, *print_ostream);
    assert(I != nullptr);
    Value* op0 = I->getOperand(0);
    Value* op1 = I->getOperand(1);
    assert(op0 != nullptr);
    assert(op1 != nullptr);

    Info* cur_val  = bit_info.at(dyn_cast<Value>(I));
    Info* op0_info = bit_info.at(op0);
    Info* op1_info = bit_info.at(op1);

    uint8_t min_shift, max_shift;
    if (op1_info->m_unknown_bits) {
        min_shift = minPossibleVal(op1_info);
        max_shift = (maxPossibleVal(op1_info) > op::S_MAX) ? op::S_MAX : maxPossibleVal(op1_info);
    } else {
        m_logger->log("Constants");
        min_shift = op1_info->m_bit_value;
        max_shift = op1_info->m_bit_value;
    }
    /*
     * here we check that the number, on which we are shifting, is negative or
     * the leftmost bit is unknown or
     * max shift is greater than the width of the number.
     * According to c standard the behavior in these cases is undefined.
     */
    if ((op1_info->m_left == op1_info->m_bitwidth - 1) ||
        op::get(op1_info->m_unknown_bits, op1_info->m_bitwidth - 1) ||
        (max_shift >= (cur_val->m_bitwidth))) {
        m_logger->log("Got in branch where right value is bad defined");
        createUndefined(cur_val);
        m_logger->log("AFTER LSHR");
        printBits(cur_val);
        printDynBits(cur_val);
        m_logger->log("Bit info: m_left: %, m_right: %, m_bitwidth: %", unsigned(cur_val->m_left),
                      unsigned(cur_val->m_right), unsigned(cur_val->m_bitwidth));
        return countStats(I, cur_val, 0);
    }

    min_shift = std::min(uint8_t(op0_info->m_left + 1), min_shift);
    max_shift = std::min(uint8_t(op0_info->m_left + 1), max_shift);

    cur_val->m_right =
        (uint8_t(op0_info->m_right) >= max_shift) ? (op0_info->m_right - max_shift) : 0;

    if ((op0_info->m_left) >= min_shift) {
        m_logger->log("(cur_val->m_left+1) >= min_shift");
        cur_val->m_extend_point = op0_info->m_extend_point - min_shift;
        cur_val->m_left         = op0_info->m_left - min_shift;
    } else
        cur_val->m_left = cur_val->m_extend_point = 0;

    if (min_shift) {
        m_logger->log("The value of min shift is partially known or is not zero");
        cur_val->m_bit_value    = op0_info->m_bit_value >> min_shift;
        cur_val->m_unknown_bits = op0_info->m_unknown_bits >> min_shift;
        setZero(cur_val, cur_val->m_left + 1, cur_val->m_bitwidth - 1);
        setZeroDyn(cur_val, cur_val->m_left + 1, cur_val->m_bitwidth - 1);

        if (min_shift != max_shift) {
            m_logger->log(
                "There is no info about the exact value"); // so we set all to dynamic between min
                                                           // shift and max shift
            setZero(cur_val, cur_val->m_right, cur_val->m_left);
            setOneDyn(cur_val, cur_val->m_right, cur_val->m_left);
        }
    } else {
        m_logger->log("The value of min shift is undefined");
        cur_val->m_bit_value = op::S_ZERO;
        cur_val->m_left      = op0_info->m_left;
        setOneDyn(cur_val,
                  (((cur_val->m_right - max_shift) >= 0) ? (cur_val->m_right - max_shift) : 0),
                  std::min(unsigned(op0_info->m_bitwidth - 1), unsigned(op0_info->m_left)));
    }

    if (min_shift == max_shift) { // to be sure that we when we know everything about the shift
        updateLeft(cur_val);      // the values are exact
        updateRight(cur_val);
    }
    correctDyn(cur_val);

    m_logger->log("AFTER LSHR");
    printBits(cur_val);
    printDynBits(cur_val);
    m_logger->log("Bit info: m_left: %, m_right: %, m_bitwidth: %", unsigned(cur_val->m_left),
                  unsigned(cur_val->m_right), unsigned(cur_val->m_bitwidth));

    return countStats(I, cur_val, 0);
}

/*
 * 1) define minimum and maximum shifts (see previous comment)
 * 2) if the leftmost (I mean the bit which is on the position m_bitwidth-1)
 *    is one - then we need to promote this one to the left after shifting, e.x.
 *    10110, ashr on two -> 11101 - the first two ones are because the very first bit was one.
 *    Here it MUST be, that m_left is equal to m_bitwidth-1, so I assert this,
 *    than we simply shift the value and set ones on the left side.
 * 3) if the leftmost bit is unknown, then we propagate the info about the sign until the beginning,
 *    based on this X value, extendfrom is then set to the previous m_left position. The new m_left
 * is set to the leftmost bit. 4) if the leftmost bit is zero, then m_left is somewhere further , so
 * this is a case of logic shift
 */
bool OptimizeBitwidth::forwardAShr(Instruction* I) {
    m_logger->log("FORWARD ASHR");
    printObj(I, *print_ostream);
    assert(I != nullptr);
    Value* op0 = I->getOperand(0);
    Value* op1 = I->getOperand(1);
    assert(op0 != nullptr);
    assert(op1 != nullptr);

    Info* cur_val  = bit_info.at(dyn_cast<Value>(I));
    Info* op0_info = bit_info.at(op0);
    Info* op1_info = bit_info.at(op1);

    uint8_t min_shift, max_shift;
    if (op1_info->m_unknown_bits) {
        min_shift = minPossibleVal(op1_info);
        max_shift = (maxPossibleVal(op1_info) > op::S_MAX) ? op::S_MAX : maxPossibleVal(op1_info);
    } else {
        min_shift = op1_info->m_bit_value;
        max_shift = op1_info->m_bit_value;
    }

    // here special case for the undefined left most bit, negative leftmost bit or shift is more
    // than bit width the last condition is the most crucial to check, as during execution it always
    // leads to undefined results)
    if ((op1_info->m_left == op1_info->m_bitwidth - 1) ||
        op::get(op1_info->m_unknown_bits, op1_info->m_bitwidth - 1) ||
        (max_shift >= (cur_val->m_bitwidth))) {
        m_logger->log("Got in branch where right value is bad defined");
        createUndefined(cur_val);
        m_logger->log("AFTER ASHR");
        printBits(cur_val);
        printDynBits(cur_val);
        m_logger->log("Bit info: m_left: %, m_right: %, m_bitwidth: %", unsigned(cur_val->m_left),
                      unsigned(cur_val->m_right), unsigned(cur_val->m_bitwidth));
        return countStats(I, cur_val, 0);
    }

    min_shift = std::min(uint8_t(op0_info->m_left + 1), min_shift);
    max_shift = std::min(uint8_t(op0_info->m_left + 1), max_shift);
    // we don't shift further than the border
    cur_val->m_right = (op0_info->m_right > max_shift) ? (op0_info->m_right - max_shift) : 0;
    cur_val->m_extend_point =
        (op0_info->m_extend_point > min_shift) ? (op0_info->m_extend_point - min_shift) : 0;

    // these are three cases, describing what bit is on the left side
    uint64_t one_on_left  = op::get(op0_info->m_bit_value, op0_info->m_bitwidth - 1);
    uint64_t zero_on_left = !op::get(op0_info->m_bit_value, op0_info->m_bitwidth - 1);
    uint64_t dyn_on_left  = op::get(op0_info->m_unknown_bits, op0_info->m_bitwidth - 1);

    if (min_shift) {
        m_logger->log("Shift is partially known or is not zero");
        cur_val->m_bit_value    = op0_info->m_bit_value >> min_shift;
        cur_val->m_unknown_bits = op0_info->m_unknown_bits >> min_shift;
        if (one_on_left) {
            // We just preserve the sign here
            m_logger->log("FIRST CASE, negative value");
            assert((op0_info->m_bitwidth - 1) == op0_info->m_left);

            setOne(cur_val, cur_val->m_bitwidth - min_shift - 1, cur_val->m_bitwidth - 1);
            cur_val->m_left = cur_val->m_bitwidth - 1;
        } else if (dyn_on_left) {
            m_logger->log("SECOND CASE, unknown upper bit");
            assert((cur_val->m_bitwidth - 1) == cur_val->m_left);
            // we set zero on every position we have shifted
            setZero(cur_val, cur_val->m_bitwidth - min_shift - 1, cur_val->m_bitwidth - 1);
            setOneDyn(cur_val, cur_val->m_bitwidth - min_shift - 1, cur_val->m_bitwidth - 1);
            cur_val->m_left = cur_val->m_bitwidth - 1;
        } else if (zero_on_left & ~(dyn_on_left)) {
            m_logger->log("THIRD CASE, real zero on left");
            cur_val->m_left = (op0_info->m_left > min_shift) ? (op0_info->m_left - min_shift) : 0;
            setZero(cur_val, cur_val->m_left + 1, cur_val->m_bitwidth - 1);
            setZeroDyn(cur_val, cur_val->m_left + 1, cur_val->m_bitwidth - 1);
        }
        if (min_shift != max_shift) {
            m_logger->log("There is no info about the exact value");
            // bits on the left are not dynamic
            setZero(cur_val, cur_val->m_right, cur_val->m_left);
            setOneDyn(cur_val, cur_val->m_right, cur_val->m_left);
        }
    } else {
        m_logger->log("UNKNOWN VAL");
        cur_val->m_bit_value = op::S_ZERO;
        cur_val->m_left      = op0_info->m_left;
        setOneDyn(cur_val,
                  (((cur_val->m_right - max_shift) >= 0) ? (cur_val->m_right - max_shift) : 0),
                  std::min(unsigned(op0_info->m_bitwidth - 1), unsigned(op0_info->m_left)));
    }

    if (min_shift == max_shift) {
        updateLeft(cur_val);
        updateRight(cur_val);
    }
    correctDyn(cur_val);

    m_logger->log("AFTER ASHR");
    printBits(cur_val);
    printDynBits(cur_val);
    m_logger->log("Bit info: m_left: %, m_right: %, m_bitwidth: %", unsigned(cur_val->m_left),
                  unsigned(cur_val->m_right), unsigned(cur_val->m_bitwidth));
    return countStats(I, cur_val, 0);
}
/*
 * 1) Determine max/min shifts
 * 2) The result is undefined if left most bit ( I mean the very left most - which is bitwidth-1)
 *    is unknown or 1 or the shift is more than the bitwidth ( this is according llvm documentation)
 */
bool OptimizeBitwidth::forwardShl(Instruction* I) {
    m_logger->log("FORWARD SHL");
    printObj(I, *print_ostream);
    assert(I != nullptr);
    Value* op0 = I->getOperand(0);
    Value* op1 = I->getOperand(1);

    assert(op0 != nullptr);
    assert(op1 != nullptr);

    Info* cur_val  = bit_info.at(dyn_cast<Value>(I));
    Info* op0_info = bit_info.at(op0);
    Info* op1_info = bit_info.at(op1);
    m_logger->log("Bits of two operands");
    printBits(op0_info);
    printBits(op1_info);
    m_logger->log("Dynamic bits of two operands");
    printDynBits(op0_info);
    printDynBits(op1_info);

    uint8_t min_shift, max_shift;
    if (op1_info->m_unknown_bits) {
        min_shift = minPossibleVal(op1_info);
        max_shift = (maxPossibleVal(op1_info) > op::S_MAX) ? op::S_MAX : maxPossibleVal(op1_info);
    } else {
        min_shift = op1_info->m_bit_value;
        max_shift = op1_info->m_bit_value;
    }
    m_logger->log("max_shift: %, min_shift : %", unsigned(max_shift), unsigned(min_shift));

    if ((op1_info->m_left == op1_info->m_bitwidth - 1) ||
        op::get(op1_info->m_unknown_bits, op1_info->m_bitwidth - 1) ||
        (max_shift >= (cur_val->m_bitwidth))) {
        m_logger->log("Got in branch where right value is bad defined");
        createUndefined(cur_val);
        m_logger->log("AFTER SHL");
        printBits(cur_val);
        printDynBits(cur_val);
        m_logger->log("Bit info: m_left: %, m_right: %, m_bitwidth: %", unsigned(cur_val->m_left),
                      unsigned(cur_val->m_right), unsigned(cur_val->m_bitwidth));
        return countStats(I, cur_val, 0);
    }
    // if m_right will be further than bitwidth we think that the behavior is undefined
    if ((op0_info->m_right + max_shift) > (cur_val->m_bitwidth - 1)) {
        m_logger->log("Got in branch where right position is bad defined");
        createUndefined(cur_val);
        m_logger->log("AFTER SHL");
        printBits(cur_val);
        printDynBits(cur_val);
        m_logger->log("Bit info: m_left: %, m_right: %, m_bitwidth: %", unsigned(cur_val->m_left),
                      unsigned(cur_val->m_right), unsigned(cur_val->m_bitwidth));
        return countStats(I, cur_val, 0);
    }

    cur_val->m_left =
        std::min(unsigned(cur_val->m_bitwidth - 1), unsigned(op0_info->m_left + max_shift));
    if (op::any(op1_info->m_unknown_bits)) {
        m_logger->log("There are unknown bits");
        cur_val->m_right     = op0_info->m_right;
        cur_val->m_bit_value = op::S_ZERO; // the value will be zero because we will have zeros on
                                           // the right and unknown values on the left
        setOneDyn(cur_val, cur_val->m_right, cur_val->m_left);
    } else {
        m_logger->log("All bits are known");
        cur_val->m_right = ((op0_info->m_right + min_shift) <= (cur_val->m_bitwidth - 1))
                               ? (op0_info->m_right + min_shift)
                               : cur_val->m_bitwidth - 1;
        cur_val->m_bit_value    = op0_info->m_bit_value << min_shift;
        cur_val->m_unknown_bits = op0_info->m_unknown_bits << min_shift;
        updateLeft(cur_val);
        updateRight(cur_val);
    }

    cur_val->m_extend_point =
        std::min(cur_val->m_left, uint8_t(op0_info->m_extend_point + max_shift));
    correctDyn(cur_val);
    m_logger->log("AFTER SHL");
    printBits(cur_val);
    printDynBits(cur_val);
    m_logger->log("Bit info: m_left: %, m_right: %, m_bitwidth: %", unsigned(cur_val->m_left),
                  unsigned(cur_val->m_right), unsigned(cur_val->m_bitwidth));

    return countStats(I, cur_val, 0);
}

/*
 * Zero extension simply increases the size of variable, so we can truncate it to the previous size.
 */
bool OptimizeBitwidth::forwardZExt(Instruction* I) {
    assert(I != nullptr);
    m_logger->log("FORWARD ZEXT");
    printObj(I, *print_ostream);
    Value* op0     = I->getOperand(0);
    Info* cur_val  = bit_info.at(dyn_cast<Value>(I));
    Info* op0_info = bit_info.at(op0);
    *cur_val       = *op0_info;
    updateLeft(cur_val);
    updateRight(cur_val);

    m_logger->log("AFTER ZEXT");
    printBits(cur_val);
    printDynBits(cur_val);
    m_logger->log("Bit info: m_left: %, m_right: %, m_bitwidth: %", unsigned(cur_val->m_left),
                  unsigned(cur_val->m_right), unsigned(cur_val->m_bitwidth));

    return countStats(I, cur_val, 0);
}

/*
 * Sign extension should also deal with signs :
 * it will propagate the 1 or unknown bit until the bitwidth
 */
bool OptimizeBitwidth::forwardSExt(Instruction* I) {
    m_logger->log("FORWARD SEXT");
    assert(I != nullptr);
    printObj(I, *print_ostream);

    Value* op0 = I->getOperand(0);

    DataLayout* data = new DataLayout(I->getModule());

    Info* cur_val  = bit_info.at(dyn_cast<Value>(I));
    Info* op0_info = bit_info.at(op0);

    m_logger->log("SEXT: cur_val bitwidth before copy : %", unsigned(cur_val->m_bitwidth));
    *cur_val = *op0_info; // if sign is zero or unknown
    m_logger->log("at first zero case");
    printBits(cur_val);
    printDynBits(cur_val);

    assert(cur_val->m_left == op0_info->m_left);
    assert(cur_val->m_right == op0_info->m_right);

    if (op0_info->m_left == (op0_info->m_bitwidth - 1)) { // is sign is one or unknown
        m_logger->log("SIGN EXTEND : EXTENDING ONE");
        if (op::get(op0_info->m_unknown_bits, op0_info->m_bitwidth - 1)) {
            cur_val->m_extend_point = op0_info->m_bitwidth - 1;
            setOneDyn(cur_val, op0_info->m_bitwidth, cur_val->m_bitwidth - 1);
            setZero(cur_val, op0_info->m_bitwidth, cur_val->m_bitwidth - 1);
        } else {
            cur_val->m_extend_point = cur_val->m_bitwidth - 1;
            setZeroDyn(cur_val, op0_info->m_bitwidth, cur_val->m_bitwidth - 1);
            setOne(cur_val, op0_info->m_bitwidth, cur_val->m_bitwidth - 1);
        }
        cur_val->m_left = cur_val->m_bitwidth - 1;
    }

    updateLeft(cur_val);
    updateRight(cur_val);
    correctDyn(cur_val);

    m_logger->log("AFTER SEXT");
    printBits(cur_val);
    printDynBits(cur_val);
    m_logger->log("Bit info: m_left: %, m_right: %, m_bitwidth: %", unsigned(cur_val->m_left),
                  unsigned(cur_val->m_right), unsigned(cur_val->m_bitwidth));

    delete data;
    return countStats(I, cur_val, 0);
}

/*
 * And ands the values.
 * if only one will have zero according to any resaons , then the result bit will be zero
 * Reverting this value will give us one in the mentioned case.
 * Then if both values are zeros and known - the result will be zero
 * If two bits are unknown - the result will be unknown
 * If only on bit is unknown - everything depends on the other:
 * 	if the other is zero - the result is zero;
 * 	if one - the resut is inknown;
 * 	if unknown the result is unknown.
 */
bool OptimizeBitwidth::forwardAnd(Instruction* I) {
    assert(I != nullptr);
    m_logger->log("FORWARD AND");
    printObj(I, *print_ostream);
    Value* op0 = I->getOperand(0);
    Value* op1 = I->getOperand(1);
    assert(op0 != nullptr);
    assert(op1 != nullptr);

    Info* cur_val  = bit_info.at(dyn_cast<Value>(I));
    Info* op0_info = bit_info.at(op0);
    Info* op1_info = bit_info.at(op1);

    cur_val->m_left      = std::min(op0_info->m_left, op1_info->m_left);
    cur_val->m_right     = std::max(op0_info->m_right, op1_info->m_right);
    cur_val->m_bit_value = (op0_info->m_bit_value & op1_info->m_bit_value);
    cur_val->m_unknown_bits =
        ~(cur_val->m_bit_value) & ((op0_info->m_unknown_bits | op0_info->m_bit_value) &
                                   (op1_info->m_unknown_bits | op1_info->m_bit_value));
    cur_val->m_extend_point = std::max(op0_info->m_extend_point, op1_info->m_extend_point);

    updateLeft(cur_val);
    updateRight(cur_val);
    correctDyn(cur_val);

    m_logger->log("AFTER AND");
    printBits(cur_val);
    printDynBits(cur_val);
    m_logger->log("Bit info: m_left: %, m_right: %, m_bitwidth: %", unsigned(cur_val->m_left),
                  unsigned(cur_val->m_right), unsigned(cur_val->m_bitwidth));

    return countStats(I, cur_val, 0);
}

/*
 * Pretty the same mechanism as with and, but here is another boolean table.
 */
bool OptimizeBitwidth::forwardOr(Instruction* I) {
    assert(I != nullptr);
    m_logger->log("FORWARD OR");
    printObj(I, *print_ostream);
    Value* op0 = I->getOperand(0);
    Value* op1 = I->getOperand(1);
    assert(op0 != nullptr);
    assert(op1 != nullptr);

    Info* cur_val  = bit_info.at(dyn_cast<Value>(I));
    Info* op0_info = bit_info.at(op0);
    Info* op1_info = bit_info.at(op1);

    cur_val->m_left  = std::max(op0_info->m_left, op1_info->m_left);
    cur_val->m_right = std::min(op0_info->m_right, op1_info->m_right);

    cur_val->m_bit_value |= (op0_info->m_bit_value | op1_info->m_bit_value);
    cur_val->m_unknown_bits =
        (op0_info->m_unknown_bits | op1_info->m_unknown_bits) & ~cur_val->m_bit_value;
    cur_val->m_extend_point = std::max(op0_info->m_extend_point, op1_info->m_extend_point);

    updateLeft(cur_val);
    updateRight(cur_val);
    correctDyn(cur_val);

    m_logger->log("AFTER OR");
    printBits(cur_val);
    printDynBits(cur_val);
    m_logger->log("Bit info: m_left: %, m_right: %, m_bitwidth: %", unsigned(cur_val->m_left),
                  unsigned(cur_val->m_right), unsigned(cur_val->m_bitwidth));

    return countStats(I, cur_val, 0);
}

/*
 * When truncating we may check, if the bit width to which we are truncating is less or greater
 * than the actual m_left of the operand. And we of course set the minimum.
 */
bool OptimizeBitwidth::forwardTrunc(Instruction* I) {
    assert(I != nullptr);
    m_logger->log("FORWARD TRUNC");
    printObj(I, *print_ostream);
    Value* op0 = I->getOperand(0);
    assert(op0 != nullptr);

    Info* cur_val       = bit_info.at(dyn_cast<Value>(I));
    Info* op0_info      = bit_info.at(op0);
    int truncated_width = op0_info->m_bitwidth;

    m_logger->log("Identifying minimum between: % and %", unsigned(cur_val->m_bitwidth - 1),
                  unsigned(op0_info->m_left));
    cur_val->m_left      = std::min(uint8_t(cur_val->m_bitwidth - 1), op0_info->m_left);
    cur_val->m_right     = std::min(cur_val->m_left, op0_info->m_right);
    cur_val->m_bit_value = op0_info->m_bit_value;
    setZero(cur_val, cur_val->m_left + 1, truncated_width - 1);
    cur_val->m_unknown_bits = op0_info->m_unknown_bits;
    setZeroDyn(cur_val, cur_val->m_left + 1, truncated_width - 1);
    updateLeft(cur_val);
    updateRight(cur_val);
    correctDyn(cur_val);
    cur_val->m_extend_point = std::min(cur_val->m_left, op0_info->m_extend_point);
    m_logger->log("AFTER TRUNC");
    printBits(cur_val);
    printDynBits(cur_val);
    m_logger->log("Bit info: m_left: %, m_right: %, m_bitwidth: %", unsigned(cur_val->m_left),
                  unsigned(cur_val->m_right), unsigned(cur_val->m_bitwidth));

    return countStats(I, cur_val, 0);
}
/*
 * We xor the results but , if the bit is unknown, after anding with reverted unknown bits we will
 * receive zero
 */
bool OptimizeBitwidth::forwardXor(Instruction* I) {
    assert(I != nullptr);
    m_logger->log("FORWARD XOR");
    printObj(I, *print_ostream);
    Value* op0 = I->getOperand(0);
    Value* op1 = I->getOperand(1);
    assert(op0 != nullptr);

    Info* cur_val  = bit_info.at(dyn_cast<Value>(I));
    Info* op0_info = bit_info.at(op0);
    Info* op1_info = bit_info.at(op1);

    cur_val->m_left         = std::max(op0_info->m_left, op1_info->m_left);
    cur_val->m_right        = std::min(op0_info->m_right, op1_info->m_right);
    cur_val->m_extend_point = std::max(op0_info->m_extend_point, op1_info->m_extend_point);

    cur_val->m_unknown_bits = op0_info->m_unknown_bits | op1_info->m_unknown_bits;
    cur_val->m_bit_value =
        (op0_info->m_bit_value ^ op1_info->m_bit_value) & ~(cur_val->m_unknown_bits);
    m_logger->log("AFTER XOR");
    printBits(cur_val);
    printDynBits(cur_val);
    m_logger->log("Bit info: m_left: %, m_right: %, m_bitwidth: %", unsigned(cur_val->m_left),
                  unsigned(cur_val->m_right), unsigned(cur_val->m_bitwidth));

    return countStats(I, cur_val, 0);
}

/*
 * initialization of the information
 */
bool OptimizeBitwidth::initialize(Instruction* I) {
    createBitInfo(I);
    return 0;
}

bool OptimizeBitwidth::initialize(Argument* A, Function& F) {
    m_logger->log("Inside initialize argument");
    printObj(A, *print_ostream);
    Info* info       = new Info();
    DataLayout* data = new DataLayout(F.getParent());
    info->m_bitwidth = data->getTypeSizeInBits(A->getType());
    info->m_left     = info->m_bitwidth - 1;
    info->m_right    = 0;
    setOneDyn(info, 0, info->m_bitwidth - 1);
    info->m_bit_value    = op::S_ZERO;
    info->m_extend_point = info->m_left;
    bit_info.insert(std::pair<Value*, Info*>(dyn_cast<Value>(A), info));
    intializeStats(dyn_cast<Value>(A), info, 0);
}

/*
 * This function crates an initial bit information.
 * We iterate through all instructions and check their operands :
 * 1) If operand is the exact ConstanInt - we set the exact information;
 * 2) If operand is the Value :
 * 2.1) If this operand has void type : set all to zeros and length to max;
 * 2.2) If this operand has exct type : set information according to this type;
 *  (To extract all information about size for the current program we get the results of
 *  data layout analysis)
 * 3) The last step - it to process the instruction itself (not it's operands), if
 *    it is not already in the map.
 * 3.1) They can also be void type, then :
 * 3.1.1) If any of the first operand has the type, set it to this type;
 * 3.1.2) If not - set the maximum type size;
 *        ( branch instructions were separated as in this case:
 *          br %cond, %label1, %label2 - the type will be i1, but I still
 *          want all branch instructions to keep maximum size)
 */
void OptimizeBitwidth::createBitInfo(Instruction* I) {
    m_logger->log("Instruction opcode name : % ", I->getOpcodeName());
    m_logger->log("S_MAX : %", unsigned(op::S_MAX));
    for (unsigned int i = 0; i < I->getNumOperands(); i++) {
        if (bit_info.find(I->getOperand(i)) == bit_info.end()) {
            m_logger->log("Operand number: %", i);
            Info* info;
            if (auto* Op = dyn_cast<ConstantInt>(I->getOperand(i))) {
                info                 = new Info();
                DataLayout* data     = new DataLayout(I->getModule());
                info->m_bitwidth     = data->getTypeSizeInBits(I->getOperand(i)->getType());
                info->m_left         = info->getLeftBorder(Op->getValue());
                info->m_right        = info->getRightBorder(Op->getValue());
                info->m_unknown_bits = op::S_ZERO;
                info->m_bit_value    = *Op->getValue().getRawData();
                info->m_extend_point = info->m_left;
                intializeStats(I->getOperand(i), info, -1);
                delete data;
            } else {
                info             = new Info();
                DataLayout* data = new DataLayout(I->getModule());
                printObj(I->getOperand(i), *print_ostream);
                printObj(I->getOperand(i)->getType(), *print_ostream);

                if (I->getOperand(i)->getType()->isVoidTy() ||
                    I->getOperand(i)->getType()->isLabelTy()) {
                    m_logger->log("Operand type is void");
                    info->m_bitwidth = 0;
                    info->m_left     = 0;
                    info->m_right    = 0;
                    info->m_bit_value &= op::S_ZERO;
                    info->m_extend_point = info->m_left;
                    setOneDyn(info, 0, op::S_MAX - 1);
                } else {
                    m_logger->log("Operand type is non-void");
                    info->m_bitwidth = data->getTypeSizeInBits(I->getOperand(i)->getType());
                    info->m_left     = info->m_bitwidth - 1;
                    info->m_right    = 0;
                    setOneDyn(info, 0, info->m_bitwidth - 1);
                    info->m_bit_value    = op::S_ZERO;
                    info->m_extend_point = info->m_left;
                }
                intializeStats(I->getOperand(i), info, 0);
                delete data;
            }
            bit_info.insert(std::pair<Value*, Info*>(I->getOperand(i), info));
        } else {
            m_logger->log("This operand number % was already processed");
        }
    }

    if (bit_info.find(I) == bit_info.end()) {
        printObj(dyn_cast<Value>(I), *print_ostream);
        Info* info       = new Info();
        DataLayout* data = new DataLayout(I->getModule());
        if ((dyn_cast<Value>(I))->getType()->isVoidTy()) {
            m_logger->log("Instruction void type");
            // this place should be corrected
            if (I->getNumOperands() > 0) {
                if (!isProcessible(I->getOperand(0)) || isa<BranchInst>(I))
                    info->m_bitwidth = op::S_MAX;
                else
                    info->m_bitwidth = data->getTypeSizeInBits(I->getOperand(0)->getType());
            }
        } else {
            m_logger->log("Instruction non-void type");
            info->m_bitwidth = data->getTypeSizeInBits((dyn_cast<Value>(I))->getType());
            if (I->getNumOperands() > 0)
                m_logger->log("instruction operand 0 type: %",
                              data->getTypeSizeInBits(I->getOperand(0)->getType()));
        }
        info->m_left  = info->m_bitwidth - 1;
        info->m_right = 0;
        setOneDyn(info, 0, info->m_bitwidth - 1);
        info->m_bit_value &= op::S_ZERO;
        info->m_extend_point = info->m_left;
        bit_info.insert(std::pair<Value*, Info*>(dyn_cast<Value>(I), info));
        intializeStats(dyn_cast<Value>(I), info, 0);
        delete data;
    }
}

bool OptimizeBitwidth::isProcessible(Value* value) {
    if (value->getType()->isVoidTy() || value->getType()->isLabelTy() ||
        value->getType()->isEmptyTy())
        return false;
    return true;
}

/*
 * Function to initialize statistics.
 * We don't need stats for the execution flow of the program,
 * btut rint them to see how many bits were liminated overall
 */
void OptimizeBitwidth::intializeStats(Value* v, Info* info, int opcode) {
    assert(v != nullptr && info != nullptr);
    m_logger->log("INITIALIZE STATS");
    if (stat_map.find(v) == stat_map.end()) {
        m_logger->log("Value was not found");
        Stats* s = new Stats();
        if (opcode == -1)
            s->m_opcode = -1;
        else if (Instruction* II = dyn_cast<Instruction>(v))
            s->m_opcode = II->getOpcode();
        else
            /*we don't process things that cannot be converted to instructions or aren't values
             */
            return;
        if (Instruction* I = dyn_cast<Instruction>(v))
            printObj(I, *print_ostream);
        s->m_bits_before = info->m_bitwidth - 1;
        s->m_bits_after  = info->m_left;
        s->m_opname =
            (opcode == -1) ? v->getName().str() : dyn_cast<Instruction>(v)->getOpcodeName();
        stat_map.insert(std::make_pair(v, s));
        if (opcode == -1) {
            m_logger->log("ELIMINATING BITS IN VAL");
            if (op_map.find("val") == op_map.end()) {
                op_map.insert(std::make_pair("val", info->m_bitwidth - 1 - info->m_left));
                op_all_map.insert(std::make_pair("val", info->m_bitwidth));
            } else {
                op_map.at("val") += (info->m_bitwidth - 1 - info->m_left);
                op_all_map.at("val") += (info->m_bitwidth);
            }
        } else {
            op_map.insert(std::make_pair(dyn_cast<Instruction>(v)->getOpcodeName(), 0));
            if (op_all_map.find(dyn_cast<Instruction>(v)->getOpcodeName()) == op_all_map.end())
                op_all_map.insert(
                    std::make_pair(dyn_cast<Instruction>(v)->getOpcodeName(), info->m_bitwidth));
            else
                op_all_map.at(dyn_cast<Instruction>(v)->getOpcodeName()) += info->m_bitwidth;
        }
    }
}
/*
 * If the leftmost bit has changed  - write this in statistic.
 * TODO: log the changes in the rightmost bits.
 */
bool OptimizeBitwidth::countStats(Instruction* I, Info* after, int opcode) {
    m_logger->log("COUNT STATS");
    assert(stat_map.find(dyn_cast<Value>(I)) != stat_map.end() &&
           "For some reason this value was not inserted");

    Value* v           = dyn_cast<Value>(I);
    Stats* cur_stats   = stat_map.at(v);
    std::string opname = (opcode == -1) ? "val" : I->getOpcodeName();
    m_logger->log(opname.c_str());
    assert(op_map.find(opname) != op_map.end());
    int change = (cur_stats->m_bits_after - after->m_left);
    op_map.at(opname) += change;
    cur_stats->m_bits_after = after->m_left;
    m_logger->log("AFTER COUNT STATS :");
    printObj(I, *print_ostream);
    m_logger->log("change : %", change);
    return (change > 0);
}

void OptimizeBitwidth::printMappedStats(Function& F) {
    m_logger->log("Function name : %", F.getName().str());
    if (stat_map.size() == 0)
        return; /*for some reason ma was not initialize, we return then*/
    m_logger->log("Print mapped stats");
    m_logger->log("Statistics on most significant bits eliminated");
    for (auto const& i : stat_map) {
        m_logger->log("***********");
        printObj(i.first, *print_ostream);
        i.second->dump(*print_ostream);
        m_logger->log("left val: %, right val: %", int(bit_info.at(i.first)->m_left),
                      int(bit_info.at(i.first)->m_right));
        printBits(bit_info.at(i.first));
        printDynBits(bit_info.at(i.first));
        m_logger->log("***********");
    }
    // std::cout<<"Function name: "<<F.getName().str()<<std::endl;
    m_logger->log("OVERALL STATISTICS");
    int backward_num;
    // std::cout<<"\033[1;33m"<<"Instruction\t\t"<<"Reduced bits(forward)\t\t"<<"Reduced
    // bits(backward)\t\t"<<"Percentage reduced"<<"\033[0m"<<std::endl;
    for (auto const& i : op_map) {
        // std::cout<<"\033[1;32m"<<i.first<<":\t\t\t\t"<<i.second<<"\t\t\t\t";
        if (op_backward_map.find(i.first) == op_backward_map.end()) {
            backward_num = 0;
        } else
            backward_num = op_backward_map.at(i.first);
        // std::cout<<backward_num<<"\t\t"<<"\033[0m\t\t";
        // std::cout<<"\033[1;32m"<<(i.second*1.0)/op_all_map[i.first] * 100<<"\033[0m"<<std::endl;
    }
}
/*
 * This is an additional function that prints statistics in file for good testing
 */
void OptimizeBitwidth::printStats(Function& F) {
    m_logger->log("STATS PRINTING");
    std::ofstream stat;
    stat.open("stats");
    int j = 0;
    for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
        // Instruction num, instruction name
        stat << j << std::endl << (&*I)->getOpcodeName() << std::endl;
        // Num operands of the instruction
        stat << (&*I)->getNumOperands() << std::endl;
        for (unsigned int i = 0; i < (&*I)->getNumOperands(); i++) {
            Info* cur_info = bit_info.at((&*I)->getOperand(i));
            printObj((&*I)->getOperand(i), *print_ostream);
            // left border
            stat << unsigned(cur_info->m_left) << std::endl;
            // right border
            stat << unsigned(cur_info->m_right) << std::endl;
            // bitwidth
            stat << unsigned(cur_info->m_bitwidth) << std::endl;
            // value
            printBitsToFile(cur_info, stat);
            // unknown bits
            printDynBitsToFile(cur_info, stat);
        }

        j += 1;
    }
    stat << "INSTRUCTION PRINTING" << std::endl;
    for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
        // Instruction num, instruction name
        stat << j << std::endl << (&*I)->getOpcodeName() << std::endl;
        // Num operands of the instruction
        Info* cur_info = bit_info.at(dyn_cast<Value>(&*I));
        // left border
        stat << unsigned(cur_info->m_left) << std::endl;
        // right border
        stat << unsigned(cur_info->m_right) << std::endl;
        // bitwidth
        stat << unsigned(cur_info->m_bitwidth) << std::endl;
        // value
        printBitsToFile(cur_info, stat);
        // unknown bits
        printDynBitsToFile(cur_info, stat);
    }

    stat.close();
}

/*
 * only for TESTING purpose
 */
template <typename T> void OptimizeBitwidth::printObj(T* obj, llvm::raw_fd_ostream& stream) {
    obj->print(stream);
    stream << '\n';
}
void OptimizeBitwidth::printBits(Info* info) {
    assert(info != nullptr);
    if (info->m_bitwidth == 0) {
        return;
    }
    for (int i = 0; i <= info->m_bitwidth - 1; i++)
        *print_ostream << op::get(info->m_bit_value, i) << " ";
    *print_ostream << '\n';
}

void OptimizeBitwidth::printDynBits(Info* info) {
    assert(info != nullptr);
    if (info->m_bitwidth == 0) {
        return;
    }
    for (int i = 0; i <= info->m_bitwidth - 1; i++)
        *print_ostream << op::get(info->m_unknown_bits, i) << " ";
    *print_ostream << '\n';
}

/*
 * for verilog
 */

unsigned OptimizeBitwidth::getSize(Value* v) {
    return unsigned((bit_info.at(v)->m_left) + 1); // add one because this is the left position
}

bool OptimizeBitwidth::isInLoopMap(Value* v) {
    if (isa<PHINode>(v))
        return (loop_map.find(dyn_cast<PHINode>(v)) != loop_map.end());
}

int OptimizeBitwidth::getMapSize() { return bit_info.size(); }

unsigned OptimizeBitwidth::getInitialSize(Value* v) { return bit_info.at(v)->m_bitwidth; }

void OptimizeBitwidth::printMap() {
    // std::cout<<"MAAP PRINTING"<<std::endl;
    for (auto i : bit_info) {
        if (Instruction* I = dyn_cast<Instruction>(i.first))
            I->dump();
        else if (ConstantInt* CI = dyn_cast<ConstantInt>(i.first))
            CI->dump();
        else if (Argument* A = dyn_cast<Argument>(i.first))
            A->dump();
    }
}