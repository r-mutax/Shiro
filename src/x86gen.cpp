#include "x86gen.hpp"
#include <iostream>

void X86Generator::generate(){
    std::cout << ".intel_syntax noprefix" << std::endl;
    std::cout << ".section .text" << std::endl;

    for(auto& func : program.functions){
        gen_function(func);
    }
}

void X86Generator::gen_function(const IRFunction& func){
    std::cout << ".globl " << func.name << std::endl;
    std::cout << ".type " << func.name << ", @function" << std::endl;
    std::cout << func.name << ":" << std::endl;
    
    // NOTE: we will use stack for temporaries.
    size_t stack_size = func.temp_count * 8;
    if(stack_size % 16 != 0){
        stack_size = (stack_size / 16 + 1) * 16;
    }
    // align 16 bytes
    if(stack_size == 0) stack_size = 16;

    // prologue
    std::cout << "  push rbp\n";
    std::cout << "  mov rbp, rsp\n";
    std::cout << "  sub rsp, " << stack_size << "\n";

    // TODO: generate instructions
    for(auto& instr : func.instructions){
        gen_instruction(instr);
    }

    // epilogue
    std::cout << "  leave\n";
    std::cout << "  ret\n";
}

void X86Generator::gen_instruction(const IRInstruction& instr){
    switch(instr.op){
        case IRInstruction::Op::MOV:
            std::cout << "  mov rax, " << instr.src1.to_x86() << std::endl;
            std::cout << "  mov " << instr.dst.to_x86() << ", rax" << std::endl;
            break;
        case IRInstruction::Op::ADD:
            std::cout << "  mov rax, " << instr.src1.to_x86() << std::endl;
            std::cout << "  add rax, " << instr.src2.to_x86() << std::endl;
            std::cout << "  mov " << instr.dst.to_x86() << ", rax" << std::endl;
            break;
        case IRInstruction::Op::SUB:
            std::cout << "  mov rax, " << instr.src1.to_x86() << std::endl;
            std::cout << "  sub rax, " << instr.src2.to_x86() << std::endl;
            std::cout << "  mov " << instr.dst.to_x86() << ", rax" << std::endl;
            break;
        case IRInstruction::Op::RET:
            std::cout << "  mov rax, " << instr.src1.to_x86() << std::endl;
            break;
        default:
            throw std::runtime_error("Unknown instruction");
    }
}
