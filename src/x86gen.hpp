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

    std::string get_reg_8(const std::string& reg64){
        if(reg64 == "rax") return "al";
        if(reg64 == "rsi") return "sil";
        if(reg64 == "rdi") return "dil";
        if(reg64 == "rbx") return "bl";
        if(reg64 == "rcx") return "cl";
        if(reg64 == "rdx") return "dl";
        if(reg64 == "r8")  return "r8b";
        if(reg64 == "r9")  return "r9b";
        if(reg64 == "r10") return "r10b";
        if(reg64 == "r11") return "r11b";
        if(reg64 == "r12") return "r12b";
        if(reg64 == "r13") return "r13b";
        if(reg64 == "r14") return "r14b";
        if(reg64 == "r15") return "r15b";
        return reg64;
    }

    std::string get_reg_16(const std::string& reg64){
        if (reg64 == "rax") return "ax";
        if (reg64 == "rsi") return "si";
        if (reg64 == "rdi") return "di";
        if (reg64 == "rbx") return "bx";
        if (reg64 == "rcx") return "cx";
        if (reg64 == "rdx") return "dx";
        if (reg64 == "r8")  return "r8w";
        if (reg64 == "r9")  return "r9w";
        if(reg64 == "r10") return "r10w";
        if(reg64 == "r11") return "r11w";
        if(reg64 == "r12") return "r12w";
        if(reg64 == "r13") return "r13w";
        if(reg64 == "r14") return "r14w";
        if(reg64 == "r15") return "r15w";
        return reg64;
    }

    std::string get_reg_32(const std::string& reg64) {
        if (reg64 == "rax") return "eax";
        if (reg64 == "rsi") return "esi";
        if (reg64 == "rdi") return "edi";
        if (reg64 == "rbx") return "ebx";
        if (reg64 == "rcx") return "ecx";
        if (reg64 == "rdx") return "edx";
        if (reg64 == "r8")  return "r8d";
        if (reg64 == "r9")  return "r9d";
        if(reg64 == "r10") return "r10d";
        if(reg64 == "r11") return "r11d";
        if(reg64 == "r12") return "r12d";
        if(reg64 == "r13") return "r13d";
        if(reg64 == "r14") return "r14d";
        if(reg64 == "r15") return "r15d";
        return reg64;
    }

    void load_from_spill(std::ostream& out, const Operand& op, std::string reg){
        if(op.type.isUnsigned){
            switch(op.type.bytes){
                case 1:
                out << "  movzx " << reg << ", BYTE PTR [rbp - " << temp_to_stack_offset[op.temp_id] << "]" << std::endl;
                break;
                case 2:
                out << "  movzx " << reg << ", WORD PTR [rbp - " << temp_to_stack_offset[op.temp_id] << "]" << std::endl;
                break;
                case 4:
                out << "  mov " << get_reg_32(reg) << ", DWORD PTR [rbp - " << temp_to_stack_offset[op.temp_id] << "]" << std::endl;
                break;
                case 8:
                out << "  mov " << reg << ", QWORD PTR [rbp - " << temp_to_stack_offset[op.temp_id] << "]" << std::endl;
                break;
            }
        } else {
            switch(op.type.bytes){
                case 1:
                out << "  movsx " << reg << ", BYTE PTR [rbp - " << temp_to_stack_offset[op.temp_id] << "]" << std::endl;
                break;
                case 2:
                out << "  movsx " << reg << ", WORD PTR [rbp - " << temp_to_stack_offset[op.temp_id] << "]" << std::endl;
                break;
                case 4:
                out << "  movsxd " << reg << ", DWORD PTR [rbp - " << temp_to_stack_offset[op.temp_id] << "]" << std::endl;
                break;
                case 8:
                out << "  mov " << reg << ", QWORD PTR [rbp - " << temp_to_stack_offset[op.temp_id] << "]" << std::endl;
                break;
            }
        }
    }

    inline int align_to(int n, int align) {
        if (align <= 0) return n;
        return (n + align - 1) / align * align;
    }

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