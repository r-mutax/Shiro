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

            Symbol* sym = declare(vd->name);
            if(sym == nullptr){
                return false;
            }

            vd->symbol_id = sym->id;
            return true;
        }
        case ASTNode::NODE_VARIABLE:{
            auto* vd = static_cast<VariableNode*>(node);
            const Symbol* sym = current_scope->find_recursive(vd->name);

            if(sym == nullptr){
                std::cerr << "Error: Variable " << vd->name << " is not declared" << std::endl;
                return false;
            }

            vd->symbol_id = sym->id;
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
        case ASTNode::NODE_ASSIGNMENT:{
            auto* as = static_cast<AssignmentNode*>(node);

            if(as->lvalue->type != ASTNode::NODE_VARIABLE){
                std::cerr << "Error: Left value of assignment is not a variable" << std::endl;
                return false;
            }

            auto* vd = static_cast<VariableNode*>(as->lvalue);

            if(!current_scope->find_recursive(vd->name)){
                std::cerr << "Error: Variable " << vd->name << " is not declared" << std::endl;
                return false;
            }
            
            return checkNode(as->lvalue) && checkNode(as->expr);
        }
        case ASTNode::NODE_IF:
        {
            auto* if_node = static_cast<IfNode*>(node);
            if(!checkNode(if_node->condition)) return false;
            if(!checkNode(if_node->then_block)) return false;
            if(if_node->else_block){
                if(!checkNode(if_node->else_block)) return false;
            }
            return true;
        }
        case ASTNode::NODE_BLOCK:
        {
            BlockNode* block = static_cast<BlockNode*>(node);
            scopeIn();

            bool ok = true;
            for(auto* stmt : block->statements){
                if(!checkNode(stmt)) {
                    ok = false;
                    break;
                }
            }

            scopeOut();
            return ok;
        }
        default:
            break;
    }

    return true;    
}
