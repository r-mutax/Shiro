#include "semantics.hpp"
#include <iostream>

bool Semantics::analyze(ASTNode* ast){
    if(!ast) {
        return false;
    }

    global_scope.symbols.clear();
    global_scope.parent = nullptr;

    current_scope = &global_scope;

    return checkNode(ast);
}

bool Semantics::checkNode(ASTNode* node){
    if(!node) return true;

    switch(node->type){
        case ASTNode::NODE_TRANSLATION_UNIT:{
            auto* tu = static_cast<TranslationUnitNode*>(node);
            for(auto* definition : tu->definitions){
                if(!checkNode(definition)) return false;
            }
            break;
        }
        case ASTNode::NODE_FUNCTION_DEFINITION:{
            auto* fd = static_cast<FunctionDefinitionNode*>(node);

            scopeIn();
            for(auto* stmt : fd->statements){
                if(!checkNode(stmt)) return false;
            }
            scopeOut();
            return true;
        }
        case ASTNode::NODE_VARIABLE_DECLARE:{
            auto* vd = static_cast<VariableDeclareNode*>(node);

            if(!current_scope->declare(vd->name)){
                std::cerr << "Error: Variable " << vd->name << " is already declared" << std::endl;
                return false;
            }
            return true;
        }
        case ASTNode::NODE_VARIABLE:{
            auto* vd = static_cast<VariableNode*>(node);

            if(!current_scope->find_recursive(vd->name)){
                std::cerr << "Error: Variable " << vd->name << " is not declared" << std::endl;
                return false;
            }
            return true;
        }
        case ASTNode::NODE_INTEGER:{
            return true;
        }
        case ASTNode::NODE_BINARY_OP:{
            auto* bo = static_cast<BinaryOpNode*>(node);
            return checkNode(bo->left) && checkNode(bo->right);
        }
        case ASTNode::NODE_EXPRESSION_STATEMENT:{
            auto* es = static_cast<ExpressionStatementNode*>(node);
            return checkNode(es->expr);
        }
        default:
            break;
    }

    return true;    
}
