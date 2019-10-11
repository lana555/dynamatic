#include "OptimizeBitwidth/info.h"
#include "OptimizeBitwidth/operations.h"
#include "llvm/ADT/APInt.h"
#include <iostream>

/*
 * get the position of leading one from the beginning of the word (counting form the right side)
 * 1XXXXXX <- the position is 6 = bitwidth - 1;
 */
uint8_t Info::getLeftBorder(const APInt& val) {
    return (val.getActiveBits() > 0) ? val.getActiveBits() - 1 : 0;
}

/*
 * get the position of last one from end of the word ( counting form the right side)
 * XXXXX1X <- the position is 1
 */
uint8_t Info::getRightBorder(const APInt& val) {
    uint64_t i = 0;
    while ((i != (op::S_MAX)) && !(op::get(*(val.getRawData()), i))) {
        i += 1;
    }
    if (i == op::S_MAX)
        i = 0;
    return i;
}

// BitVector Info::getBitValue(const APInt& val) {
//   BitVector v(64);
//   uint8_t num = val.getActiveBits();
//   for (int i = 0; i < num; i++)
//     if ((*val.getRawData()) & (1<<i))
//     v.set(i);
//   return v;
// }

void Info::updateParams() {}
