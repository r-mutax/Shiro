#ifndef SHIRO_IR_HPP
#define SHIRO_IR_HPP

#include "AST.hpp"
#include <unordered_map>
#include <set>
#include <memory>

struct Operand {
    enum Kind {
        TEMP,
        INT_VAL,
        LABEL,
    } kind;

    int temp_id;    
    int label_id;
    int64_t imm;

    static Operand Temp(int id) {
        return Operand { .kind = TEMP, .temp_id = id};
    }

    static Operand IntVal(int64_t imm) {
        return Operand { .kind = INT_VAL, .imm = imm };
    }

    static Operand Label(int id) {
        return Operand { .kind = LABEL, .label_id = id };
    }

    std::string to_str() const {
        switch(kind) {
            case TEMP:
                return "T" + std::to_string(temp_id);
            case INT_VAL:
                return std::to_string(imm);
            case LABEL:
                return ".L" + std::to_string(label_id);
        }
        return "UNKNOWN";
    }

    std::string to_x86() const {
        switch(kind) {
            case TEMP:
                return "QWORD PTR [rbp - " + std::to_string((temp_id + 1) * 8) + "]";
            case INT_VAL:
                return std::to_string(imm);
            case LABEL:
                return ".L" + std::to_string(label_id);
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
        MOD,    // mod src2 from src1 to dst
        LSHIFT, // left shift src2 from src1 to dst
        RSHIFT, // right shift src2 from src1 to dst
        RET,    // return value
        EQ,     // check equal src1 and src2 to dst
        NEQ,    // check not equal src1 and src2 to dst
        JZ,     // jump if src1 is zero then jmp label(dst)
        JMP,    // jump anyware to label(dst)
        LABEL,  // label src1
    } op;

    Operand dst;
    Operand src1;
    Operand src2;

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
            case LSHIFT:
                return "LSHIFT " + dst.to_str() + ", " + src1.to_str() + ", " + src2.to_str();
            case RSHIFT:
                return "RSHIFT " + dst.to_str() + ", " + src1.to_str() + ", " + src2.to_str();
            case RET:
                return "RET " + src1.to_str();
            case EQ:
                return "EQ " + dst.to_str() + ", " + src1.to_str() + ", " + src2.to_str();
            case NEQ:
                return "NEQ " + dst.to_str() + ", " + src1.to_str() + ", " + src2.to_str();
            case JZ:
                return "JZ " + src1.to_str() + ", " + src2.to_str();
            case JMP:
                return "JMP " + src1.to_str();
            case LABEL:
                return "LABEL " + src1.to_str();
        }
        return "UNKNOWN";
    }
};

struct LiveInterval {
    int start = -1;
    int end = -1;
};

struct BasicBlock {
    int id = -1;
    int start_index = -1;
    int end_index = -1;
    
    std::vector<IRInstruction> instructions;
    
    std::vector<BasicBlock*> preds;
    std::vector<BasicBlock*> succs;

    std::set<int> live_ins;
    std::set<int> live_outs;
    std::set<int> def;
    std::set<int> use;
};

class IRFunction {
    void signLeaderIR();

    BasicBlock* createBB(){
        auto bb = std::make_unique<BasicBlock>();
        bb->id = blocks.size();
        blocks.push_back(std::move(bb));
        return blocks.back().get();
    }

public:
    std::string name;
    std::vector<std::unique_ptr<BasicBlock>> blocks;

    void constructCFG(const std::vector<IRInstruction>& instructions);
    void analyzeLiveness();

    //std::queue<IRInstruction> instructions;
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
    int next_label = 0;
    std::vector<IRInstruction> instructions;
    std::unordered_map<int, int> symid_to_temp;

    void emit(IRInstruction::Op op,
                Operand dst = Operand::IntVal(0),
                Operand src1 = Operand::IntVal(0),
                Operand src2 = Operand::IntVal(0)){
        instructions.push_back(IRInstruction{ .op = op, .dst = dst, .src1 = src1, .src2 = src2 });
    }

    void emit_mov(Operand dst, Operand src){
        emit(IRInstruction::MOV, dst, src);
    }

    void emit_add(Operand dst, Operand src1, Operand src2){
        emit(IRInstruction::ADD, dst, src1, src2);
    }

    void emit_sub(Operand dst, Operand src1, Operand src2){
        emit(IRInstruction::SUB, dst, src1, src2);
    }

    void emit_mul(Operand dst, Operand src1, Operand src2){
        emit(IRInstruction::MUL, dst, src1, src2);
    }

    void emit_div(Operand dst, Operand src1, Operand src2){
        emit(IRInstruction::DIV, dst, src1, src2);
    }

    void emit_mod(Operand dst, Operand src1, Operand src2){
        emit(IRInstruction::MOD, dst, src1, src2);
    }

    void emit_lshift(Operand dst, Operand src1, Operand src2){
        emit(IRInstruction::LSHIFT, dst, src1, src2);
    }

    void emit_rshift(Operand dst, Operand src1, Operand src2){
        emit(IRInstruction::RSHIFT, dst, src1, src2);
    }

    void emit_ret(Operand src){
        emit(IRInstruction::RET, Operand::IntVal(0), src);
    }

    void emit_eq(Operand dst, Operand src1, Operand src2){
        emit(IRInstruction::EQ, dst, src1, src2);
    }

    void emit_neq(Operand dst, Operand src1, Operand src2){
        emit(IRInstruction::NEQ, dst, src1, src2);
    }

    void emit_jz(Operand src, Operand label){
        emit(IRInstruction::JZ, label, src);
    }

    void emit_jmp(Operand label){
        emit(IRInstruction::JMP, label);
    }

    void emit_label(Operand label){
        emit(IRInstruction::LABEL, label);
    }

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