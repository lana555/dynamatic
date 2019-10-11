#ifndef INFO_H
#define INFO_H
#include "operations.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/raw_ostream.h"
#include <bitset>
#include <fstream>
#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string>

using namespace llvm;
/*
 * Information class:contains information about: left most bit,
 * 						 right most bit,
 * 						 bitwidth - it does not change from the
 * initialization
 */
class Info {
public:
    /*
     * Description: structure to keep statistics
     */
    struct Stats {
        int m_bits_before;
        int m_bits_after;
        int m_total_bit_width;
        int m_opcode; //-1 - information about the value (is ConstantInt object in llvm), 0 - all
                      // other
        std::string m_opname;

        void dump(llvm::raw_fd_ostream& stream) {
            stream << "Bits before: " << m_bits_before << "\n";
            stream << "Bits after: " << m_bits_after << "\n";
            stream << "Opcode: " << m_opcode << "\n";
            stream << "Opname: " << m_opname << "\n";
        }
    };

    /*
     * Description : structure to keep information about PHI nodes
     */
    struct PHIInfo {
        Value* m_cmp;  // compare instruction, that is corresponding to this phi node
        int m_pattern; // 0 -decent (sub/div), 1 - accent(add/mul);
        int m_order;   // 0 -normal,           1 - reverse (order of incoming blocks)
        PHIInfo(Value* cmp, int pattern, int order)
            : m_cmp(cmp), m_pattern(pattern), m_order(order) {}
    };

    void updateParams();

    uint8_t m_bitwidth     = op::S_MAX;
    uint8_t m_left         = 0;
    uint8_t m_right        = 0;
    uint64_t value         = 0;
    uint8_t m_extend_point = 0;
    bool m_in_cycle        = false; // for further analysis of induction variables

    uintmax_t m_unknown_bits;
    uintmax_t m_bit_value;

    uint8_t getLeftBorder(const APInt& val);
    uint8_t getRightBorder(const APInt& val);

    /*some constructors for conveniece*/
    Info& operator=(const Info& info) {
        if (this == &info) {
            return *this;
        }
        this->m_left         = info.m_left;
        this->m_right        = info.m_right;
        this->m_bit_value    = info.m_bit_value;
        this->m_unknown_bits = info.m_unknown_bits;
        this->m_extend_point = info.m_extend_point;
        return *this;
    }
    Info(const Info& data) = default;
    Info()                 = default;
};

#endif // INFO_H