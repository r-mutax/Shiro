#include "semantics.hpp"
#include "AST.hpp"
#include <iostream>

bool Semantics::analyze(ASTNode* ast){
    if(!ast) {
        return false;
    }

    global_scope.symbols.clear();
    global_scope.parent = nullptr;
    current_scope = &global_scope;

    init_builtins();

    return checkNode(ast);
}

bool Semantics::checkNode(ASTNode* node){
    if(!node) return true;

    switch(node->kind){
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

            const Symbol* type_sym = current_scope->find_recursive(vd->type_name);
            if(type_sym == nullptr){
                std::cerr << "Error: Type " << vd->type_name << " is not declared" << std::endl;
                return false;
            }
            if(type_sym->kind != Symbol::TYPE){
                std::cerr << "Error: " << vd->type_name << " is not a type" << std::endl;
                return false;
            }

            Symbol* sym = declare_variable(vd->name, type_sym->type_info);
            if(sym == nullptr){
                return false;
            }

            vd->symbol_id = sym->id;
            vd->evaluated_type = type_sym->type_info;
            return true;
        }
        case ASTNode::NODE_VARIABLE:{
            auto* vd = static_cast<VariableNode*>(node);
            const Symbol* sym = current_scope->find_recursive(vd->name);

            if(sym == nullptr){
                std::cerr << "Error: Variable " << vd->name << " is not declared" << std::endl;
                return false;
            }
            if(sym->type_info->name == "unknown"){
                std::cerr << "Error: Cannot infer type of variable " << vd->name << " before assignment" << std::endl;
                return false;
            }

            vd->symbol_id = sym->id;
            vd->evaluated_type = sym->type_info;
            return true;
        }
        case ASTNode::NODE_INTEGER:{
            node->evaluated_type = i64_t;
            return true;
        }
        case ASTNode::NODE_UNARY_OP:{
            auto* uo = static_cast<UnaryOpNode*>(node);
            if(!checkNode(uo->value)) return false;

            if(uo->op.type == Token::NOT){
                uo->evaluated_type = i64_t;
            } else {
                uo->evaluated_type = uo->value->evaluated_type;
            }
            return true;
        }
        case ASTNode::NODE_BINARY_OP:{
            auto* bo = static_cast<BinaryOpNode*>(node);
            if(!checkNode(bo->left)) return false;
            if(!checkNode(bo->right)) return false;

            if(bo->right->kind == ASTNode::NODE_INTEGER){
                bo->right->evaluated_type = bo->left->evaluated_type;
            } else if(bo->left->kind == ASTNode::NODE_INTEGER){
                bo->left->evaluated_type = bo->right->evaluated_type;
            }

            if(bo->left->evaluated_type != bo->right->evaluated_type){
                std::cerr << "Error: Types of left and right operands do not match" << std::endl;
                return false;
            }

            if (bo->op.type == Token::AND_AND || bo->op.type == Token::OR_OR ||
                bo->op.type == Token::EQUAL_EQUAL || bo->op.type == Token::NOT_EQUAL ||
                bo->op.type == Token::LT || bo->op.type == Token::LE ||
                bo->op.type == Token::GT || bo->op.type == Token::GE) {
                node->evaluated_type = u8_t; // true/false
            } else {
                node->evaluated_type = bo->left->evaluated_type; // arithmetic/bit operations
            }
            return true;
        }
        case ASTNode::NODE_EXPRESSION_STATEMENT:{
            auto* es = static_cast<ExpressionStatementNode*>(node);
            if(!checkNode(es->expr)) return false;
            es->evaluated_type = es->expr->evaluated_type;
            return true;
        }
        case ASTNode::NODE_ASSIGNMENT:{
            auto* as = static_cast<AssignmentNode*>(node);

            if(as->lvalue->kind != ASTNode::NODE_VARIABLE){
                std::cerr << "Error: Left value of assignment is not a variable" << std::endl;
                return false;
            }

            auto* var_node = static_cast<VariableNode*>(as->lvalue);
            auto* sym = (Symbol*)(current_scope->find_recursive(var_node->name));
            if(sym == nullptr){
                std::cerr << "Error: Variable " << var_node->name << " is not declared" << std::endl;
                return false;
            }

            if(!checkNode(as->expr)) return false;

            if(sym->type_info->name == "unknown"){
                sym->type_info = as->expr->evaluated_type;
            } else if(as->expr->kind == ASTNode::NODE_INTEGER){
                as->expr->evaluated_type = sym->type_info;
            }

            var_node->symbol_id = sym->id;
            var_node->evaluated_type = sym->type_info;

            if(as->lvalue->evaluated_type != as->expr->evaluated_type){
                std::cerr << "Error: Type mismatch in assignment" << std::endl;
                return false;
            }
            as->evaluated_type = var_node->evaluated_type;
            return true;
        }
        case ASTNode::NODE_IF:
        {
            auto* if_node = static_cast<IfNode*>(node);
            if(!checkNode(if_node->condition)) return false;
            if(!checkNode(if_node->then_block)) return false;
            if(if_node->else_block){
                if(!checkNode(if_node->else_block)) return false;

                if(if_node->then_block->evaluated_type
                    != if_node->else_block->evaluated_type){
                    std::cerr << "Error: Type mismatch between 'then' and 'else' branches" << std::endl;
                    return false;
                }
            }
            if_node->evaluated_type = if_node->then_block->evaluated_type;
            return true;
        }
        case ASTNode::NODE_WHILE:
        {
            auto* while_node = static_cast<WhileNode*>(node);
            if(!checkNode(while_node->condition)) return false;
            if(!checkNode(while_node->body)) return false;

            while_node->evaluated_type = while_node->body->evaluated_type;
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

            if(!block->statements.empty()){
                block->evaluated_type = block->statements.back()->evaluated_type;
            }
            return ok;
        }
        default:
            break;
    }

    return true;    
}

void Semantics::init_builtins(){
    i8_t = make_primitive("i8", 1, false);
    i16_t = make_primitive("i16", 2, false);
    i32_t = make_primitive("i32", 4, false);
    i64_t = make_primitive("i64", 8, false);
    u8_t = make_primitive("u8", 1, true);
    u16_t = make_primitive("u16", 2, true);
    u32_t = make_primitive("u32", 4, true);
    u64_t = make_primitive("u64", 8, true);
    unknown = make_primitive("unknown", 8, false);

    declare_type("i8", i8_t);
    declare_type("i16", i16_t);
    declare_type("i32", i32_t);
    declare_type("i64", i64_t);
    declare_type("u8", u8_t);
    declare_type("u16", u16_t);
    declare_type("u32", u32_t);
    declare_type("u64", u64_t);
    declare_type("unknown", unknown);
}

const Type* Semantics::alloc_type(Type t){
    auto type_ptr = std::make_unique<Type>(std::move(t));
    const Type* res = type_ptr.get();
    allocated_types.push_back(std::move(type_ptr));
    return res;
}

const Type* Semantics::make_primitive(const std::string& name, int size, bool isUnsigned){
    Type t;
    t.name = name;
    t.size = size;
    t.isUnsigned = isUnsigned;
    return alloc_type(t);
}

