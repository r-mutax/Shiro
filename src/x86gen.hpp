#ifndef SHIRO_X86GEN_HPP
#define SHIRO_X86GEN_HPP

#include <vector>
#include "irgen.hpp"
#include <iostream>
#include <ostream>

class X86Generator{
    const IRProgram& program;
    std::ostream& out;

    void gen_function(const IRFunction& func);
    void gen_instruction(const IRInstruction& instr);

public:
    X86Generator(const IRProgram& program, std::ostream& out = std::cout) 
        : program(program), out(out) {}
    ~X86Generator() = default;

    void generate();
};

#endif // SHIRO_X86GEN_HPP