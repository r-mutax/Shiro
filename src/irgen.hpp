#ifndef SHIRO_IR_HPP
#define SHIRO_IR_HPP

#include "AST.hpp"
#include <unordered_map>
#include <set>
#include <memory>

struct OperandType {
    int bytes;
    bool isUnsigned = false;
};

struct Operand {
    enum Kind {
        TEMP,
        INT_VAL,
        FUNC,
        LLABEL,
    } kind;

    int temp_id;    
    int label_id;
    int64_t imm;
    OperandType type;
    std::string name;

    static OperandType to_operand_type(const Type* type){
        if(type == nullptr){
            return OperandType{8, false};
        }

        return OperandType{type->size, type->isUnsigned};
    }

    static Operand Temp(int id, const Type* type) {
        return Operand { .kind = TEMP, .temp_id = id, .type = to_operand_type(type)};
    }

    static Operand Temp(int id, const OperandType& optype){
        return Operand { .kind = TEMP, .temp_id = id, .type = optype};
    }

    // type is nullable, i64 is default
    static Operand IntVal(int64_t imm, const Type* type = nullptr) {
        return Operand { .kind = INT_VAL, .imm = imm, .type = to_operand_type(type)};
    }

    static Operand Func(std::string name){
        return Operand { .kind = FUNC, .name = name };
    }

    static Operand LLabel(int id) {
        return Operand { .kind = LLABEL, .label_id = id };
    }

    std::string to_str() const {
        switch(kind) {
            case TEMP:
                return "T" + std::to_string(temp_id);
            case INT_VAL:
                return std::to_string(imm);
            case FUNC:
                return name;
            case LLABEL:
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
            case FUNC:
                return name;
            case LLABEL:
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
        LE,     // le src1 and src2 to dst
        LT,     // lt src1 and src2 to dst
        BAND,   // bit and src1 and src2 to dst
        BOR,    // bit or src1 and src2 to dst
        BXOR,   // bit xor src1 and src2 to dst
        BNOT,   // bit not src1 to dst
        NEG,    // negate src1 to dst
        NOT,    // logical not src1 to dst
        CALL,   // call function dst
        RET,    // return value
        EQ,     // check equal src1 and src2 to dst
        NEQ,    // check not equal src1 and src2 to dst
        JZ,     // jump if src1 is zero then jmp label(dst)
        JNZ,    // jump if src1 is not zero then jmp label(dst)
        JMP,    // jump anyware to label(dst)
        LABEL,  // label src1
    } op;

    Operand dst;
    Operand src1;
    Operand src2;

    std::vector<Operand> args;

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
            case BAND:
                return "BAND " + dst.to_str() + ", " + src1.to_str() + ", " + src2.to_str();
            case BOR:
                return "BOR " + dst.to_str() + ", " + src1.to_str() + ", " + src2.to_str();
            case BXOR:
                return "BXOR " + dst.to_str() + ", " + src1.to_str() + ", " + src2.to_str();
            case LE:
                return "LE " + dst.to_str() + ", " + src1.to_str() + ", " + src2.to_str();
            case LT:
                return "LT " + dst.to_str() + ", " + src1.to_str() + ", " + src2.to_str();
            case BNOT:
                return "BNOT " + dst.to_str() + ", " + src1.to_str();
            case NEG:
                return "NEG " + dst.to_str() + ", " + src1.to_str();
            case NOT:
                return "NOT " + dst.to_str() + ", " + src1.to_str();
            case CALL:
                return "CALL " + dst.to_str() + ", " + src1.to_str();
            case RET:
                return "RET " + src1.to_str();
            case EQ:
                return "EQ " + dst.to_str() + ", " + src1.to_str() + ", " + src2.to_str();
            case NEQ:
                return "NEQ " + dst.to_str() + ", " + src1.to_str() + ", " + src2.to_str();
            case JZ:
                return "JZ " + src1.to_str() + ", " + src2.to_str();
            case JNZ:
                return "JNZ " + src1.to_str() + ", " + src2.to_str();
            case JMP:
                return "JMP " + src1.to_str();
            case LABEL:
                return "LABEL " + src1.to_str();
        }
        return "UNKNOWN";
    }
};

struct TempRegInfo {
    int start = -1;
    int end = -1;
    OperandType type;
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
    std::vector<Operand> param_temps;
    std::vector<std::unique_ptr<BasicBlock>> blocks;

    void constructCFG(const std::vector<IRInstruction>& instructions);
    void analyzeLiveness();

    //std::queue<IRInstruction> instructions;
    std::vector<IRInstruction> instructions;
    int temp_count = 0;
    std::vector<TempRegInfo> temp_reg_infos;
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
    std::unordered_map<int, Operand> symid_to_temp;

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

    void emit_lt(Operand dst, Operand src1, Operand src2){
        emit(IRInstruction::LT, dst, src1, src2);
    }

    void emit_le(Operand dst, Operand src1, Operand src2){
        emit(IRInstruction::LE, dst, src1, src2);
    }

    void emit_bitand(Operand dst, Operand src1, Operand src2){
        emit(IRInstruction::BAND, dst, src1, src2);
    }

    void emit_bitor(Operand dst, Operand src1, Operand src2){
        emit(IRInstruction::BOR, dst, src1, src2);
    }

    void emit_bitxor(Operand dst, Operand src1, Operand src2){
        emit(IRInstruction::BXOR, dst, src1, src2);
    }

    void emit_bnot(Operand dst, Operand src){
        emit(IRInstruction::BNOT, dst, src);
    }

    void emit_neg(Operand dst, Operand src){
        emit(IRInstruction::NEG, dst, src);
    }

    void emit_not(Operand dst, Operand src){
        emit(IRInstruction::NOT, dst, src);
    }

    void emit_call(Operand dst, Operand src, std::vector<Operand>& args){
        IRInstruction instr{.op = IRInstruction::CALL, .dst = dst, .src1 = src, .src2 = Operand::IntVal(0)};
        instr.args = args;
        instructions.push_back(instr);
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
        emit(IRInstruction::JZ, Operand::IntVal(0), src, label);
    }

    void emit_jnz(Operand src, Operand label){
        emit(IRInstruction::JNZ, Operand::IntVal(0), src, label);
    }

    void emit_jmp(Operand label){
        emit(IRInstruction::JMP, Operand::IntVal(0), label);
    }

    void emit_label(Operand label){
        emit(IRInstruction::LABEL, Operand::IntVal(0), label);
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