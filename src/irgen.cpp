#include "irgen.hpp"
#include <iostream>

bool IRGenerator::generate(ASTNode* ast) {

    if(ast->type != ASTNode::NODE_TRANSLATION_UNIT) {
        throw std::runtime_error("Expected translation unit");
    }

    TranslationUnitNode* tu = static_cast<TranslationUnitNode*>(ast);
    for(auto& def : tu->definitions){
        IRFunction func = gen_function(def);
        program.functions.push_back(func);
    }

    return true;
}

IRFunction IRGenerator::gen_function(ASTNode* node){

    if(node->type != ASTNode::NODE_FUNCTION_DEFINITION) {
        throw std::runtime_error("Expected function definition");
    }

    FunctionDefinitionNode* func_def = static_cast<FunctionDefinitionNode*>(node);
    IRFunction func;
    func.name = func_def->name;

    instructions.clear();
    next_temp = 0;
    symid_to_temp.clear();

    Operand ret = Operand::IntVal(0);
    for(auto& stmt : func_def->statements){
        ret = gen_stmt(stmt);
    }
    emit_ret(ret);
    
    func.instructions = instructions;
    func.temp_count = next_temp;

    // Compute live intervals for each temporary variable
    func.live_intervals.resize(next_temp);
    for(size_t i = 0; i < instructions.size(); i++){
        IRInstruction& instr = instructions[i];
        
        auto update_interval = [&](const Operand& op){
            if(op.kind == Operand::TEMP){
                int tid = op.temp_id;
                if(func.live_intervals[tid].start == -1){
                    func.live_intervals[tid].start = i;
                }
                func.live_intervals[tid].end = i;
            }
        };

        update_interval(instr.dst);
        update_interval(instr.src1);
        update_interval(instr.src2);
    }
    
    return func;
}

Operand IRGenerator::gen_stmt(ASTNode* node){
    if(node->type == ASTNode::NODE_EXPRESSION_STATEMENT){
        ExpressionStatementNode* stmt = static_cast<ExpressionStatementNode*>(node);
        return gen_expr(stmt->expr);
    } else if(node->type == ASTNode::NODE_VARIABLE_DECLARE){
        VariableDeclareNode* vd = static_cast<VariableDeclareNode*>(node);
        int temp = next_temp++;
        symid_to_temp[vd->symbol_id] = temp;
        emit_mov(Operand::Temp(temp), Operand::IntVal(0));
        return Operand::Temp(temp);
    } else {
        throw std::runtime_error("Unexpected statement");
    }
}

Operand IRGenerator::gen_expr(ASTNode* node){
    if(node->type == ASTNode::NODE_INTEGER){
        NumberNode* num = static_cast<NumberNode*>(node);
        return Operand::IntVal(num->value);
    } else if(node->type == ASTNode::NODE_VARIABLE){
        VariableNode* v = static_cast<VariableNode*>(node);
        auto it = symid_to_temp.find(v->symbol_id);
        if(it == symid_to_temp.end()){
            throw std::runtime_error("IRGen Error: variable'" + v->name + "'not found in symbol map");
        }
        return Operand::Temp(symid_to_temp[v->symbol_id]);
    } else if(node->type == ASTNode::NODE_IF){
        IfNode* if_node = static_cast<IfNode*>(node);
        Operand condition = gen_expr(if_node->condition);

        int else_label = next_label++;
        int end_label = next_label++;
        Operand res_temp = Operand::Temp(next_temp++);

        emit_jz(condition, Operand::Label(else_label));

        // Then
        Operand then_val = gen_expr(if_node->then_block);
        emit_mov(res_temp, then_val);
        emit_jmp(Operand::Label(end_label));

        // else
        emit_label(Operand::Label(else_label));

        if(if_node->else_block){
            Operand else_val = gen_expr(if_node->else_block);
            emit_mov(res_temp, else_val);
        } else {
            emit_mov(res_temp, Operand::IntVal(0));
        }
        emit_label(Operand::Label(end_label));
        return res_temp;
    } else if(node->type == ASTNode::NODE_ASSIGNMENT){
        AssignmentNode* as = static_cast<AssignmentNode*>(node);
        VariableNode* var = static_cast<VariableNode*>(as->lvalue);
        
        Operand value = gen_expr(as->expr);
        auto it = symid_to_temp.find(var->symbol_id);

        if(it == symid_to_temp.end()){
            throw std::runtime_error("IRGen Error: variable'" + var->name + "'not found in symbol map");
        }
        emit_mov(Operand::Temp(it->second), value);
        return value;
    } else if(node->type == ASTNode::NODE_BINARY_OP){
        BinaryOpNode* bin_op = static_cast<BinaryOpNode*>(node);
        Operand left = gen_expr(bin_op->left);
        Operand right = gen_expr(bin_op->right);
        Operand ret = Operand::Temp(next_temp++);

        switch(bin_op->op.type){
            case Token::EQUAL_EQUAL:
                emit_eq(ret, left, right);
                break;
            case Token::NOT_EQUAL:
                emit_neq(ret, left, right);
                break;
            case Token::LSHIFT:
                emit_lshift(ret, left, right);
                break;
            case Token::RSHIFT:
                emit_rshift(ret, left, right);
                break;
            case Token::PLUS:
                emit_add(ret, left, right);
                break;
            case Token::MINUS:
                emit_sub(ret, left, right);
                break;
            case Token::ASTERISK:
                emit_mul(ret, left, right);
                break;
            case Token::SLASH:
                emit_div(ret, left, right);
                break;
            case Token::MOD:
                emit_mod(ret, left, right);
                break;
            default:
                throw std::runtime_error("Unexpected operator: " + bin_op->op.to_str());
        }

        return ret;
    }

    throw std::runtime_error("Expected expression");
}

void IRGenerator::dump(){
    for(auto& func : program.functions){
        std::cout << "define " << func.name << " {" << std::endl;
        for(auto& instr : func.instructions){
            std::cout << "  " << instr.to_str() << std::endl;
        }
        std::cout << "}" << std::endl;
    }
}

