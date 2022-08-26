#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <iostream>
#include <cstdint>
#include <cinttypes>
#include <cassert>

namespace op {
using uint = unsigned int;

const uintmax_t S_ZERO      = static_cast<uintmax_t>(0);
const uintmax_t S_ONE       = static_cast<uintmax_t>(1);
const uintmax_t S_ALL_ONES  = ~S_ZERO;

const uint8_t S_MAX_BITS    = sizeof(uintmax_t) * 8;
const uint8_t S_MOST_LEFT   = sizeof(uintmax_t) * 8 - 1;

static inline bool validIdx(int i) { return 0 <= i && i < S_MAX_BITS; }

inline uintmax_t set(uintmax_t num, int i) {
    assert (validIdx(i));
    return num | (S_ONE << i); 
}
inline uintmax_t unset(uintmax_t num, int i) {
    assert (validIdx(i));
    return num & ~(S_ONE << i); 
}
inline uintmax_t getval(uintmax_t num, int i) {
    assert (validIdx(i));
    return (num & (S_ONE << i)); 
}
inline bool get(uintmax_t num, int i) {
    assert (validIdx(i));
    return (num >> i) & S_ONE; 
}
inline uintmax_t set_or_unset(uintmax_t num, int i, bool set_bit) {
    return set_bit ? set(num, i) : unset(num, i);
}

inline uintmax_t mask(uint size) {
    assert(size <= S_MAX_BITS);

    return size == S_MAX_BITS ? 
        S_ALL_ONES : (S_ONE << size) - 1;
}
inline uintmax_t mask(uint from, uint to) {
    assert(to <= S_MOST_LEFT && from <= to);
    return mask(to - from + 1) << from; 
}

inline uintmax_t set(uintmax_t num, uint from, uint to) {
    assert(to <= S_MOST_LEFT && from <= to);
    return num | mask(from, to);
}
inline uintmax_t unset(uintmax_t num, uint from, uint to) {
    assert(to <= S_MOST_LEFT && from <= to);
    return num & ~mask(from, to);
}
inline uintmax_t set_or_unset(uintmax_t num, uint from, uint to, bool set_bit) {
    return set_bit ? set(num, from, to) : unset(num, from, to);
}

inline bool any(uintmax_t n) { return n != S_ZERO; }
inline bool none(uintmax_t n) { return n == S_ZERO; }
inline bool all(uintmax_t n) { return n == S_ALL_ONES; }


// MEGA SLOW, but platform independant
inline int findLeftMostBit(uintmax_t x) {
    if (x == S_ZERO)
        return 0;

    for (int i = S_MOST_LEFT ; i >= 0 ; --i)
        if (get(x, i))
            return i;

    // should never reach here        
    return 0;
}
// MEGA SLOW, but platform independant
inline int findRightMostBit(uintmax_t x) {
    if (x == S_ZERO)
        return 0;

    for (int i = 0 ; i <= S_MOST_LEFT ; ++i)
        if (get(x, i))
            return i;
    
    // should never reach here    
    return 0;
}
inline int findSignExtension(uintmax_t x, uint8_t bitwidth) {
    if (x == S_ZERO)
        return 0;

    int leftMost = findLeftMostBit(x);
    if (leftMost != bitwidth - 1)
        return leftMost;

    for (int i = leftMost - 1 ; i >= 0 ; --i)
        if (!get(x, i))
            return i + 1;

    return 0;
}

inline uint8_t ceil_log2(uintmax_t x) {
    static constexpr unsigned long long t[6] = {
        0xFFFFFFFF00000000ull,
        0x00000000FFFF0000ull,
        0x000000000000FF00ull,
        0x00000000000000F0ull,
        0x000000000000000Cull,
        0x0000000000000002ull
    };
    
    // x & (x - 1) trick to get the LSB
    int y = ((x & (x - 1)) == 0) ? 0 : 1;
    int j = 32;

    // should be unrolled at compilation
    for (int i = 0 ; i < 6 ; i++) {
        int k = ((x & t[i]) == 0) ? 0 : j;

        y += k;
        x >>= k;
        j >>= 1;
    }

    return y;
}

} // namespace op

#endif // OPERATIONS