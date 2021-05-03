#include "OptimizeBitwidth/info.h"
#include "OptimizeBitwidth/operations.h"

#include <cassert>
#include <iostream>
#include <sstream>
#include <algorithm>


Info::Info(uint8_t originalBitwidth) 
    : cpp_bitwidth(originalBitwidth)
{
    assert(0 < cpp_bitwidth);
    assert(cpp_bitwidth <= op::S_MAX_BITS);

    left = cpp_bitwidth - 1;
    sign_ext = left;
    right = 0;

    unknown_bits = op::mask(cpp_bitwidth);
    bit_value = op::S_ZERO;
}

Info& Info::operator=(const Info& info) {
    if (this == &info)
        return *this;

    assert(this->cpp_bitwidth == info.cpp_bitwidth);

    this->left     = info.left;
    this->right    = info.right;
    this->sign_ext = info.sign_ext;
    this->unknown_bits = info.unknown_bits;
    this->bit_value    = info.bit_value;

    return *this;
}


bool Info::operator==(const Info& info) const {
    assert(this->cpp_bitwidth == info.cpp_bitwidth);

    return this->left == info.left && 
            this->right == info.right &&
            this->sign_ext == info.sign_ext &&
            this->unknown_bits == info.unknown_bits &&
            this->bit_value == info.bit_value;
}
bool Info::operator!=(const Info& info) const {
    return !(*this == info);
}


uintmax_t Info::minUnsignedValuePossible() const {
    return bit_value;
}
uintmax_t Info::maxUnsignedValuePossible() const {
    return bit_value | unknown_bits;
}

intmax_t Info::minSignedValuePossible() const {
    // if the value can be negative,
    //need to manually extend sign bit to intmax_t size
    if (left == cpp_bitwidth - 1)
        return op::mask(sign_ext, op::S_MOST_LEFT) | bit_value;
    else
        return bit_value;
}
intmax_t Info::maxSignedValuePossible() const {
    // if we know for sure that the value is negative,
    //extend sign bit and set any possible bit to 1 
    //(the more bits are set, the closer to 0 we get)
    if (left == cpp_bitwidth - 1 && op::get(bit_value, left))
        return op::mask(sign_ext, op::S_MOST_LEFT) | bit_value | unknown_bits;
    // otherwise the max value is reached when all bits but the last are set
    else
        return op::unset(bit_value | unknown_bits, cpp_bitwidth - 1);
}

void Info::keepUnion(const Info& other) {
    assert(cpp_bitwidth == other.cpp_bitwidth);

    left     = std::max(left, other.left);
    sign_ext = std::max(sign_ext, other.sign_ext);
    right    = std::min(right, other.right);

    // need to have unknowns where bit value differs too
    unknown_bits = unknown_bits | other.unknown_bits | (bit_value ^ other.bit_value);
    bit_value = (bit_value | other.bit_value) & ~unknown_bits;

    reduceBounds();
}
void Info::keepIntersection(const Info& other) {
    assert(cpp_bitwidth == other.cpp_bitwidth);

    left     = std::min(left, other.left);
    sign_ext = std::min(sign_ext, other.sign_ext);
    right    = std::max(right, other.right);

    unknown_bits = unknown_bits & other.unknown_bits;
    bit_value = (bit_value | other.bit_value) & ~unknown_bits;

    reduceBounds();
}

void Info::reduceBounds() {
    const uintmax_t all_bits = unknown_bits | bit_value;
    
    // all bits are known and 0 ?
    if (op::none(all_bits)) {
        right = left = sign_ext = 0;
        return;
    }

    right = op::findRightMostBit(all_bits);
    left = op::findLeftMostBit(all_bits);

    if (left == cpp_bitwidth - 1 && op::get(bit_value, left))
        sign_ext = std::min(int(sign_ext), op::findSignExtension(bit_value, cpp_bitwidth));
    else
        sign_ext = std::min(sign_ext, left);
    
}

std::string Info::toString() const {
    std::stringstream ss;
    ss << "(l=" << int(left) << ",s=" << int(sign_ext) << ",r=" << int(right) << ") - [";
    
    for (int i = left ; i > sign_ext ; --i) {
        if (op::get(unknown_bits, i))
            ss << 'S';
        else if (op::get(bit_value, i))
            ss << '1';
        else
            ss << 'E'; // shouldn't be displayed, ever
    }
    for (int i = sign_ext ; i >= 0 ; --i) {
        if (op::get(unknown_bits, i))
            ss << '?';
        else if (op::get(bit_value, i))
            ss << '1';
        else
            ss << '0';
    }
    ss << ']';
    return ss.str();
}

void Info::assertValid() const {
    assert(right <= left);
    assert(right <= sign_ext && sign_ext <= left);
    assert(left < cpp_bitwidth);

    uintmax_t all_bits = bit_value | unknown_bits;
    if (op::none(all_bits)) {
        assert(left == 0);
        assert(sign_ext == 0);
        assert(right == 0);
    }
    else {
        assert(left == op::findLeftMostBit(all_bits));
        assert(right == op::findRightMostBit(all_bits));
    }

    // lastly, we can't have a '1' in bit_value when the same bit is unknown :
    //either bit is known, and then bit_value can be 0/1
    //or bit is unknown, and then bit_value must be 0
    for (int i = right ; i <= left ; ++i) {
        assert(!(op::get(unknown_bits, i) && op::get(bit_value, i)));
    }
}

