#ifndef UTILS_H
#define UTILS_H
#include "OptimizeBitwidth/utils.h"
#include "OptimizeBitwidth/info.h"
#include "OptimizeBitwidth/operations.h"
#include <fstream>
#include <iostream>
#include <llvm/Support/FileSystem.h>
#include <system_error>
/*
 * Some additional util functions
 */

void printBitsToFile(Info* info, std::ofstream& file) {
    assert(info != nullptr);
    if (info->m_bitwidth == 0) {
        return;
    }
    for (int i = 0; i <= info->m_bitwidth - 1; i++)
        file << op::get(info->m_bit_value, i) << " ";
    file << std::endl;
}

void printDynBitsToFile(Info* info, std::ofstream& file) {
    assert(info != nullptr);
    if (info->m_bitwidth == 0) {
        return;
    }
    for (int i = 0; i <= info->m_bitwidth - 1; i++)
        file << op::get(info->m_unknown_bits, i) << " ";
    file << std::endl;
}

/*
 * Updates the position of the left most bit
 * according to the latest value information
 */
void updateLeft(Info* info) {
    assert(info != nullptr);
    int i = info->m_left;
    for (i = info->m_left; i >= 0; i--) {
        if (op::get(info->m_bit_value, i) || op::get(info->m_unknown_bits, i))
            break;
    }
    if (i == -1)
        i += 1;
    info->m_left = i;
}

/*
 * Updates the position of the right most most bit
 * according to the latest value information
 */
void updateRight(Info* info) {
    assert(info != nullptr);
    int i = 0;
    for (i = 0; i <= info->m_left; i++) {
        if (op::get(info->m_bit_value, i) || op::get(info->m_unknown_bits, i))
            break;
    }
    if (i == (info->m_left + 1))
        i = 0;
    info->m_right = i;
}

/*
 * Sets dynamic bit to one.
 * TODO: better change on bitmask
 */
void setOneDyn(Info* info, int start_pos, int end_pos) {
    if ((start_pos > end_pos) || (end_pos < 0))
        return;
    assert(info != nullptr);
    for (int i = start_pos; i <= end_pos; i++)
        info->m_unknown_bits = op::set(info->m_unknown_bits, i);
}

/*
 * Sets value bit to one
 */
void setOne(Info* info, int start_pos, int end_pos) {
    if ((start_pos > end_pos) || (end_pos < 0))
        return;
    assert(info != nullptr);
    for (int i = start_pos; i <= end_pos; i++)
        info->m_bit_value = op::set(info->m_bit_value, i);
}

/*
 * Sets dynamic bit to zero
 */
void setZeroDyn(Info* info, int start_pos, int end_pos) {
    if ((start_pos > end_pos) || (end_pos < 0))
        return;
    assert(info != nullptr);
    for (int i = start_pos; i <= end_pos; i++)
        info->m_unknown_bits = op::unset(info->m_unknown_bits, i);
}

/*
 * Sets value bit to zero
 */
void setZero(Info* info, int start_pos, int end_pos) {
    if ((start_pos > end_pos) || (end_pos < 0))
        return;
    assert(info != nullptr);
    for (int i = start_pos; i <= end_pos; i++)
        info->m_bit_value = op::unset(info->m_bit_value, i);
}

/*
 * returns min possible value : here we simply return the value,
 * because min_val means, that all unknown bits are replaced with zero - so it is initially
 */
uintmax_t minPossibleVal(Info* info) {
    assert(info != nullptr);
    return info->m_bit_value;
}

/*
 * returns max possible value : all inknown bits are replaced with 1
 */

uintmax_t maxPossibleVal(Info* info) {
    assert(info != nullptr);
    uintmax_t res = info->m_bit_value;
    for (int i = info->m_right; i <= info->m_left; i++)
        if (op::get(info->m_unknown_bits, i))
            res = op::set(res, i);
    return res;
}

uint8_t findRightDyn(Info* info) {
    for (int i = 0; i < info->m_left; i++)
        if (op::get(info->m_unknown_bits, i))
            return i;
    return 0;
}

/*
 * Creates undefined value
 */
void createUndefined(Info* info) {
    assert(info != nullptr);
    info->m_bit_value = op::S_ZERO;
    info->m_left      = info->m_bitwidth - 1;
    info->m_right     = 0;
    setOneDyn(info, info->m_left, info->m_right);
    info->m_extend_point = info->m_bitwidth - 1;
}
/*
 *Correct dynamic bits according value information
 */
void correctDyn(Info* info) {
    assert(info != nullptr);
    info->m_unknown_bits = op::unset(info->m_unknown_bits, info->m_left + 1, info->m_bitwidth - 1);
    info->m_unknown_bits = op::unset(info->m_unknown_bits, 0, info->m_right - 1);
}

#endif