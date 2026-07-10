#ifndef SHIRO_IR_HPP
#define SHIRO_IR_HPP

#include "AST.hpp"

struct Operand {
    enum Kind {
        TEMP,
        INT_VAL,
    } kind;

    int temp_id;
    int64_t imm;

    static Operand Temp(int id) {
        return { TEMP, id, 0 };
    }

    static Operand IntVal(int64_t imm) {
        return { INT_VAL, 0, imm };
    }

    std::string to_str() const {
        switch(kind) {
            case TEMP:
                return "T" + std::to_string(temp_id);
            case INT_VAL:
                return std::to_string(imm);
        }
        return "UNKNOWN";
    }

    std::string to_x86() const {
        switch(kind) {
            case TEMP:
                return "QWORD PTR [rbp - " + std::to_string((temp_id + 1) * 8) + "]";
            case INT_VAL:
                return std::to_string(imm);
        }
        return "UNKNOWN";
    }
};

struct IRInstruction {
    enum Op {
        MOV,    // copy data from src to dst
        ADD,    // add src1 and src2 to dst
        SUB,    // sub src2 from src1 to dst
        MUL,    // mul src1 and src2 to dst
        DIV,    // div src2 from src1 to dst
        MOD,     // mod src2 from src1 to dst
        RET,    // return value
    } op;

    Operand dst;
    Operand src1;
    Operand src2;

    // constructor for binary operations like ADD, SUB
    IRInstruction(Op op, Operand dst, Operand src1, Operand src2)
        : op(op), dst(dst), src1(src1), src2(src2) {}

    // constructor for unary operations like MOV
    IRInstruction(Op op, Operand dst, Operand src1)
        : op(op), dst(dst), src1(src1), src2(Operand::IntVal(0)) {}

    // constructor for RET
    IRInstruction(Op op, Operand src1)
        : op(op), dst(Operand::IntVal(0)), src1(src1), src2(Operand::IntVal(0)) {}

    std::string to_str() const {
        switch(op) {
            case MOV:
                return "MOV " + dst.to_str() + ", " + src1.to_str();
            case ADD:
                return "ADD " + dst.to_str() + ", " + src1.to_str() + ", " + src2.to_str();
            case SUB:
                return "SUB " + dst.to_str() + ", " + src1.to_str() + ", " + src2.to_str();
            case MUL:
                return "MUL " + dst.to_str() + ", " + src1.to_str() + ", " + src2.to_str();
            case DIV:
                return "DIV " + dst.to_str() + ", " + src1.to_str() + ", " + src2.to_str();
            case MOD:
                return "MOD " + dst.to_str() + ", " + src1.to_str() + ", " + src2.to_str();
            case RET:
                return "RET " + src1.to_str();
        }
        return "UNKNOWN";
    }
};

struct LiveInterval {
    int start = -1;
    int end = -1;
};

class IRFunction {
public:
    std::string name;
    std::vector<IRInstruction> instructions;
    int temp_count = 0;

    std::vector<LiveInterval> live_intervals;
};

class IRProgram {
public:
    std::vector<IRFunction> functions;
};

class IRGenerator {
    IRProgram program;
    int next_temp = 0;
    std::vector<IRInstruction> instructions;

    IRFunction gen_function(ASTNode* node);
    Operand gen_stmt(ASTNode* node);
    Operand gen_expr(ASTNode* node);

public:
    IRGenerator() = default;
    ~IRGenerator() = default;

    bool generate(ASTNode* ast);

    const IRProgram& get_program() const { return program; }

    void dump();
};

#endif // SHIRO_IR_HPP