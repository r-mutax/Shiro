#include "irgen.hpp"
#include <iostream>

bool IRGenerator::generate(ASTNode* ast) {
    
    if(ast->type != ASTNode::NODE_TRANSLATION_UNIT) {
        throw std::runtime_error("Expected translation unit");
    }

    TranslationUnitNode* tu = static_cast<TranslationUnitNode*>(ast);
    Operand res = Operand::IntVal(0);

    for(auto& stmt : tu->statements){
        res = gen_stmt(stmt);
    }
    instructions.push_back({IRInstruction::Op::RET, res});

    return true;
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
    for(auto& instr : instructions){
        std::cout << instr.to_str() << std::endl;
    }
}

