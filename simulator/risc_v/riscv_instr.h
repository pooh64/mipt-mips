/* riscv_instr.h - instruction parser for risc_v
 * @author Alexandr Misevich
 * Copyright 2018 MIPT-MIPS
 */

#ifndef RISCV_INSTR_H
#define RISCV_INSTR_H

#include "riscv_register/riscv_register.h"

#include <func_sim/operation.h>
#include <func_sim/trap_types.h>
#include <infra/endian.h>
#include <infra/macro.h>
#include <infra/types.h>

template <typename T>
class RISCVInstr : public BaseInstruction<T, RISCVRegister>
{
    private:
        using MyDatapath = typename BaseInstruction<T, RISCVRegister>::MyDatapath;
        uint32 instr = NO_VAL32;

        char imm_type = ' ';
        char imm_print_type = ' ';

        std::string generate_disasm() const;
    public:
        static const constexpr Endian endian = Endian::little;

        RISCVInstr() = delete;
        explicit RISCVInstr( uint32 bytes, Addr PC = 0);

         bool is_same_bytes( uint32 bytes) const {
            return bytes == instr;
        }
        
        bool is_same( const RISCVInstr& rhs) const {
            return this->PC == rhs.PC && is_same_bytes( rhs.instr);
        }

        bool is_same_checker( const RISCVInstr& /* rhs */) const { return false; }

        constexpr bool is_nop() const { return instr == 0x0U; }

        std::string get_disasm() const;
        std::string string_dump() const;
        std::string bytes_dump() const;
};

template <typename T>
static inline std::ostream& operator<<( std::ostream& out, const RISCVInstr<T>& /* rhs */)
{
    return out << "";
}

#endif //RISCV_INSTR_H
