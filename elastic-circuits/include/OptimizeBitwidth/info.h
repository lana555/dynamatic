#pragma once

#include <iostream>
#include <string>
#include <cinttypes>


struct Info {
public:    
    struct Stats {
        std::string name;

        // -2  : Argument
        // -1  : Constant
        // 0<= : LLVM opcode
        int opcode;

        unsigned forward_opt;
        unsigned backward_opt;
    };

public:
    // some basic constructors 
    explicit Info(uint8_t originalBitwidth);
    Info(const Info& data) = default;
    Info& operator=(const Info& info);

    // compare two infos field by field
    //makes it easy to detect changes
    bool operator==(const Info& info) const;
    bool operator!=(const Info& info) const;

    
    // Returns the min UNSIGNED value the Info can take, given its current bitmasks
    uintmax_t minUnsignedValuePossible() const;
    // Returns the max UNSIGNED value the Info can take, given its current bitmasks
    uintmax_t maxUnsignedValuePossible() const;

    // Returns the min SIGNED value the Info can take, given its current bitmasks
    //if the bit sign is ? or 1, will return a negative number (the closest to -infinity possible)
    //if the bit sign is 0, will return a positive signed number (the closest to 0 possible)
    intmax_t minSignedValuePossible() const;
    // Returns the max SIGNED value the Info can take, given its current bitmasks
    //if the sign bit is ? or 0, will return a signed positive number (the closest to +infinity possible)
    //if the sign bit is 1, will return the negative number (the closest to 0 possible)
    intmax_t maxSignedValuePossible() const;

    // extends the Info to fit both itself, and the other one
    //ie. extends its bounds if needed, and set known/unknowns bits accordingly
    void keepUnion(const Info& other);

    // restricts the Info to hold the intersection of itself with the other one
    void keepIntersection(const Info& other);

    // tries to reduce bounds (left, right, sign_ext) from existing ones and bit_masks
    void reduceBounds();

    // Debug
    std::string toString() const;

    // does some validity checks on Info.
    //should always hold (at the end of a forward/backward function)
    void assertValid() const;

public:
    uintmax_t unknown_bits;
    uintmax_t bit_value;

    uint8_t right; // lsb's position
    uint8_t left;  // msb's position
    uint8_t sign_ext; // msb's position when not extending the sign

    const uint8_t cpp_bitwidth;
};
