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

    Operand ret = Operand::IntVal(0);
    for(auto& stmt : func_def->statements){
        ret = gen_stmt(stmt);
    }
    instructions.push_back({IRInstruction::Op::RET, ret});
    
    func.instructions = instructions;
    func.temp_count = next_temp;
    
    return func;
}

Operand IRGenerator::gen_stmt(ASTNode* node){
    if(node->type == ASTNode::NODE_EXPRESSION_STATEMENT){
        ExpressionStatementNode* stmt = static_cast<ExpressionStatementNode*>(node);
        return gen_expr(stmt->expr);
    } else {
        throw std::runtime_error("Unexpected statement");
    }
}

Operand IRGenerator::gen_expr(ASTNode* node){
    if(node->type == ASTNode::NODE_INTEGER){
        NumberNode* num = static_cast<NumberNode*>(node);
        return Operand::IntVal(num->value);
    }
    else if(node->type == ASTNode::NODE_BINARY_OP){
        BinaryOpNode* bin_op = static_cast<BinaryOpNode*>(node);
        Operand left = gen_expr(bin_op->left);
        Operand right = gen_expr(bin_op->right);
        Operand ret = Operand::Temp(next_temp++);

        switch(bin_op->op.type){
            case Token::PLUS:
                instructions.push_back({IRInstruction::Op::ADD, ret, left, right});
                break;
            case Token::MINUS:
                instructions.push_back({IRInstruction::Op::SUB, ret, left, right});
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

