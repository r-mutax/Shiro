#ifndef SHIRO_X86GEN_HPP
#define SHIRO_X86GEN_HPP

#include <vector>
#include "irgen.hpp"
#include <iostream>
#include <ostream>

struct X86RegAllocState{
    std::vector<std::pair<std::string, int>> free_regs = {
        {"rbx", -1},
        {"r10", -1},
        {"r11", -1},
        {"r12", -1},
        {"r13", -1},
        {"r14", -1},
        {"r15", -1},
    };
    std::vector<int> temp_to_reg;

    // if dst is same as src1, don't need to move
    bool is_same_temp(const Operand& op1, const Operand& op2){
        if(op1.kind != Operand::TEMP || op2.kind != Operand::TEMP){
            return false;
        }

        int reg1 = temp_to_reg[op1.temp_id];
        int reg2 = temp_to_reg[op2.temp_id];

        if(reg1 != -1 && reg2 != -1){
            return reg1 == reg2;
        }

        int offset1 = temp_to_stack_offset[op1.temp_id];
        int offset2 = temp_to_stack_offset[op2.temp_id];

        if(offset1 != -1 && offset2 != -1){
            return offset1 == offset2;
        }

        return false;
    }
    
    bool is_spill(const Operand& op){
        if(op.kind != Operand::TEMP){
            return false;
        }
        return temp_to_reg[op.temp_id] == -1;
    }
    const std::string SpillDstReg = "rsi";
    const std::string SpillSrc1Reg = "rax";
    const std::string SpillSrc2Reg = "rdi";

    // stack offset for spill
    std::vector<int> temp_to_stack_offset;
    int spill_offset = 8;
};

class X86Generator{
    const IRProgram& program;
    std::ostream& out;

    X86RegAllocState alloc_registers(const IRFunction& func);
    std::string activateDst(const Operand& op, X86RegAllocState& regalloc_state);
    std::string activateSrc1(const Operand& op, X86RegAllocState& regalloc_state);
    std::string activateSrc2(const Operand& op, X86RegAllocState& regalloc_state);
    void deactivateDst(const Operand& op, X86RegAllocState& regalloc_state);

    void gen_function(const IRFunction& func);
    void gen_instruction(const IRInstruction& instr, X86RegAllocState& regalloc_state);

    
    
public:
    X86Generator(const IRProgram& program, std::ostream& out = std::cout) 
        : program(program), out(out) {}
    ~X86Generator() = default;

    void generate();
};

#endif // SHIRO_X86GEN_HPP