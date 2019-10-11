#ifndef OPERATIONS_H
#define OPERATIONS_H
#include <bitset>
#include <iostream>
#include <stdint.h>
/*
 * This file provide simple inline functions for bit manipulating on uintmax_t.
 * There could be created a class, but there is no need to, operating in terms of
 * the bit optimizations is easier with numbers
 *
 */
namespace op {
const uintmax_t S_ONE     = static_cast<uintmax_t>(1);
const uintmax_t S_ZERO    = static_cast<uintmax_t>(0);
const uint8_t S_MAX       = sizeof(uintmax_t) * 8;
const uint8_t S_CMP_INDEX = 2;

inline uintmax_t set(uintmax_t num, int i) { return num | (S_ONE << i); }

inline uintmax_t unset(uintmax_t num, int i) { return num & ~(S_ONE << i); }

inline uintmax_t getval(uintmax_t num, int i) { return (num & (S_ONE << i)); }

inline bool get(uintmax_t num, int i) { return (num >> i) & S_ONE; }

inline uintmax_t set(uintmax_t num, int start, int end) {
    if ((end == -1) || (start > end))
        return num;
    for (int i = start; i <= end; i++)
        num |= (S_ONE << i);
    return num;
}

inline uintmax_t unset(uintmax_t num, int start, int end) {
    if ((end == -1) || (start > end))
        return num;
    for (int i = start; i <= end; i++)
        num &= ~(S_ONE << i);
    return num;
}

inline bool any(uintmax_t n) { return (n != S_ZERO); }

inline bool none(uintmax_t n) { return (n == S_ZERO); }

inline bool all(uintmax_t n) { return (~(n) == S_ZERO); }

} // namespace op

#endif // OPERATIONS