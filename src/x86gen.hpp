#ifndef SHIRO_X86GEN_HPP
#define SHIRO_X86GEN_HPP

#include <vector>
#include "irgen.hpp"

class X86Generator{
    const IRProgram& program;

    void gen_function(const IRFunction& func);
    void gen_instruction(const IRInstruction& instr);

public:
    X86Generator(const IRProgram& program) : program(program) {}
    ~X86Generator() = default;

    void generate();
};

#endif // SHIRO_X86GEN_HPP