#ifndef UTILS_H
#define UTILS_H
#include "info.h"
#include "operations.h"
#include <fstream>
#include <iostream>
#include <llvm/Support/FileSystem.h>
#include <system_error>
/*
 * Some additional util functions
 */

void printBitsToFile(Info* info, std::ofstream& file);

void printDynBitsToFile(Info* info, std::ofstream& file);

/*
 * Updates the position of the left most bit
 * according to the latest value information
 */
void updateLeft(Info* info);

/*
 * Updates the position of the right most most bit
 * according to the latest value information
 */
void updateRight(Info* info);

/*
 * Sets dynamic bit to one.
 * TODO: better change on bitmask
 */
void setOneDyn(Info* info, int start_pos, int end_pos);

/*
 * Sets value bit to one
 */
void setOne(Info* info, int start_pos, int end_pos);

/*
 * Sets dynamic bit to zero
 */
void setZeroDyn(Info* info, int start_pos, int end_pos);

/*
 * Sets value bit to zero
 */
void setZero(Info* info, int start_pos, int end_pos);

/*
 * returns min possible value : here we simply return the value,
 * because min_val means, that all unknown bits are replaced with zero - so it is initially
 */
uintmax_t minPossibleVal(Info* info);

/*
 * returns max possible value : all inknown bits are replaced with 1
 */

uintmax_t maxPossibleVal(Info* info);

uint8_t findRightDyn(Info* info);

/*
 * Creates undefined value
 */
void createUndefined(Info* info);
/*
 *Correct dynamic bits according value information
 */
void correctDyn(Info* info);

#endif