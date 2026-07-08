#include "x86gen.hpp"
#include <iostream>

void X86Generator::generate(){
    out << ".intel_syntax noprefix" << std::endl;
    out << ".section .text" << std::endl;

    for(auto& func : program.functions){
        gen_function(func);
    }
}

void X86Generator::gen_function(const IRFunction& func){
    out << ".globl " << func.name << std::endl;
    out << ".type " << func.name << ", @function" << std::endl;
    out << func.name << ":" << std::endl;
    
    // NOTE: we will use stack for temporaries.
    size_t stack_size = func.temp_count * 8;
    if(stack_size % 16 != 0){
        stack_size = (stack_size / 16 + 1) * 16;
    }
    // align 16 bytes
    if(stack_size == 0) stack_size = 16;

    // prologue
    out << "  push rbp\n";
    out << "  mov rbp, rsp\n";
    out << "  sub rsp, " << stack_size << "\n";

    // TODO: generate instructions
    for(auto& instr : func.instructions){
        gen_instruction(instr);
    }

    // epilogue
    out << "  leave\n";
    out << "  ret\n";
}

void X86Generator::gen_instruction(const IRInstruction& instr){
    switch(instr.op){
        case IRInstruction::Op::MOV:
            out << "  mov rax, " << instr.src1.to_x86() << std::endl;
            out << "  mov " << instr.dst.to_x86() << ", rax" << std::endl;
            break;
        case IRInstruction::Op::ADD:
            out << "  mov rax, " << instr.src1.to_x86() << std::endl;
            out << "  add rax, " << instr.src2.to_x86() << std::endl;
            out << "  mov " << instr.dst.to_x86() << ", rax" << std::endl;
            break;
        case IRInstruction::Op::SUB:
            out << "  mov rax, " << instr.src1.to_x86() << std::endl;
            out << "  sub rax, " << instr.src2.to_x86() << std::endl;
            out << "  mov " << instr.dst.to_x86() << ", rax" << std::endl;
            break;
        case IRInstruction::Op::MUL:
            out << "  mov rax, " << instr.src1.to_x86() << std::endl;
            out << "  imul rax, " << instr.src2.to_x86() << std::endl;
            out << "  mov " << instr.dst.to_x86() << ", rax" << std::endl;
            break;
        case IRInstruction::Op::DIV:
            out << "  mov rax, " << instr.src1.to_x86() << std::endl;
            out << "  cqo" << std::endl;
            out << "  mov rdi, " << instr.src2.to_x86() << std::endl;
            out << "  idiv rdi" << std::endl;
            out << "  mov " << instr.dst.to_x86() << ", rax" << std::endl;
            break;
        case IRInstruction::Op::MOD:
            out << "  mov rax, " << instr.src1.to_x86() << std::endl;
            out << "  cqo" << std::endl;
            out << "  mov rdi, " << instr.src2.to_x86() << std::endl;
            out << "  idiv rdi" << std::endl;
            out << "  mov " << instr.dst.to_x86() << ", rdx" << std::endl;
            break;
        case IRInstruction::Op::RET:
            out << "  mov rax, " << instr.src1.to_x86() << std::endl;
            break;
        default:
            throw std::runtime_error("Unknown instruction");
    }
}
