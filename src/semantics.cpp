#include "semantics.hpp"
#include "AST.hpp"
#include <cstddef>

bool Semantics::analyze(ASTNode* ast) {
    if (!ast) {
        return false;
    }

    global_scope.symbols.clear();
    global_scope.parent = nullptr;
    current_scope = &global_scope;

    init_builtins();

    return checkNode(ast);
}

bool Semantics::checkNode(ASTNode* node) {
    if (!node)
        return true;

    auto isAutoCastInteger = [](ASTNode* node) {
        return node->kind == ASTNode::NODE_INTEGER ||
               node->kind == ASTNode::NODE_CHAR_LITERAL;
    };

    switch (node->kind) {
        case ASTNode::NODE_TRANSLATION_UNIT: {
            auto* tu = static_cast<TranslationUnitNode*>(node);
            for (auto* definition : tu->definitions) {
                if (!checkNode(definition))
                    return false;
            }
            break;
        }
        case ASTNode::NODE_FUNCTION_DEFINITION: {
            auto* fd = static_cast<FunctionDefinitionNode*>(node);

            const Type* ret_type = resolveType(fd->type_name, fd->type_loc);

            Symbol* func_sym = declare_function(fd->fn_name, ret_type);
            if (func_sym == nullptr) {
                error(fd->loc, "Function '{}' is already declared",
                      fd->fn_name);
            }
            fd->evaluated_type = ret_type;
            current_func_return_types.push_back(ret_type);

            scopeIn();

            for (auto& it : fd->params) {
                auto* vd = static_cast<VariableDeclareNode*>(it);

                if (current_scope->find(vd->name) != nullptr) {
                    error(vd->loc, "Duplicated parameter '{}'", vd->name);
                }
                if (!checkNode(it))
                    return false;

                const Symbol* param_sym = current_scope->find(vd->name);
                if (param_sym->kind != Symbol::VARIABLE) {
                    error(vd->loc, "'{}' is not a variable", vd->name);
                }

                func_sym->params.push_back(const_cast<Symbol*>(param_sym));
            }

            bool body_ok = checkNode(fd->body);
            scopeOut();
            current_func_return_types.pop_back();

            if (!body_ok)
                return false;
            return true;
        }
        case ASTNode::NODE_STRUCT_DEFINITION:{
            auto* strct = static_cast<StructDefinitionNode*>(node);
            
            if(current_scope->find(strct->strct_name) != nullptr){
                error(strct->loc, "Struct '{}' is already declared", strct->strct_name);
            }
            
            // Temporarily declare type symbol so that it can be used inside the struct
            const Type* strct_type = make_struct(strct->strct_name, nullptr);
            declare_type(strct->strct_name, strct_type);
            
            scopeIn();
            auto* mutable_type = const_cast<Type*>(strct_type);
            mutable_type->scope = current_scope;
            
            int max_align = 1;
            int offset = 0;

            for(auto& it : strct->members){
                if(it->kind == ASTNode::NODE_FUNCTION_DEFINITION){                    
                    // method
                    auto* fd = static_cast<FunctionDefinitionNode*>(it);
                    fd->fn_name = strct->strct_name + "__" + fd->fn_name;
                    
                    fd->params.insert(fd->params.begin(), 
                        new VariableDeclareNode("this", fd->type_loc, "&" + strct->strct_name)
                    );

                    if(!checkNode(it)) return false;                
                } else if(it->kind == ASTNode::NODE_VARIABLE_DECLARE){
                    // field
                    if(!checkNode(it)) return false;

                    auto* vd = static_cast<VariableDeclareNode*>(it);
                    
                    const Symbol* sym = strct_type->scope->find(vd->name);
                    Symbol* mutable_sym = const_cast<Symbol*>(sym);

                    int align = mutable_sym->type_info->align;
                    max_align = std::max(align, max_align);

                    offset = (offset + align - 1) & ~(align - 1);

                    mutable_sym->offset = offset;
                    offset += mutable_sym->type_info->size;
                } else {
                    error(it->loc, "Invalid member of struct '{}'", strct->strct_name);
                }
            }
            scopeOut();

            int total_size = (offset + max_align - 1) & ~(max_align - 1);
            mutable_type->size = total_size;
            mutable_type->align = max_align;

            strct->evaluated_type = mutable_type;
            return true;
        }
        case ASTNode::NODE_FUNCTION_CALL: {
            auto* fc = static_cast<FunctionCallNode*>(node);

            const Symbol* func_sym = current_scope->find_recursive(fc->fn_name);
            if (func_sym == nullptr) {
                error(fc->loc, "Function '{}' is not declared", fc->fn_name);
            }
            if (func_sym->kind != Symbol::FUNCTION) {
                error(fc->loc, "'{}' is not a function", fc->fn_name);
            }

            if (fc->args.size() != func_sym->params.size()) {
                error(
                    fc->loc,
                    "Function '{}' expects {} arguments, but got {} arguments.",
                    fc->fn_name, func_sym->params.size(), fc->args.size());
            }

            for (size_t i = 0; i < fc->args.size(); i++) {
                auto* arg = fc->args[i];
                if (!checkNode(arg))
                    return false;

                if (isAutoCastInteger(arg)) {
                    arg->evaluated_type = func_sym->params[i]->type_info;
                }

                if (arg->evaluated_type != func_sym->params[i]->type_info) {
                    error(arg->loc,
                          "Type mismatch in argument {} of function '{}'. "
                          "Expected "
                          "'{}', but got '{}'",
                          i + 1, fc->fn_name,
                          func_sym->params[i]->type_info->name,
                          arg->evaluated_type->name);
                }
            }

            fc->evaluated_type = func_sym->type_info;
            return true;
        }
        case ASTNode::NODE_VARIABLE_DECLARE: {
            auto* vd = static_cast<VariableDeclareNode*>(node);

            const Type* var_type = resolveType(vd->type_name, vd->type_loc);
            if(var_type == nullptr) return false;

            Symbol* sym = declare_variable(vd->name, var_type);
            if (sym == nullptr) {
                error(vd->loc, "Variable '{}' is already declared", vd->name);
                return false;
            }

            vd->symbol_id = sym->id;
            vd->evaluated_type = var_type;

            if(vd->init_expr != nullptr){
                if(!checkNode(vd->init_expr)) return false;

                if(var_type->name == "unknown"){
                    var_type = vd->init_expr->evaluated_type;
                    sym->type_info = var_type;
                    vd->evaluated_type = var_type;
                } else if (isAutoCastInteger(vd->init_expr)) {
                    vd->init_expr->evaluated_type = var_type;
                }

                if (vd->init_expr->evaluated_type != var_type) {
                    error(vd->init_expr->loc,
                          "Type mismatch in init expression of variable '{}'. "
                          "Expected "
                          "'{}', but got '{}'",
                          vd->name, var_type->name,
                          vd->init_expr->evaluated_type->name);
                }
            }
            return true;
        }
        case ASTNode::NODE_VARIABLE: {
            auto* vd = static_cast<VariableNode*>(node);
            const Symbol* sym = current_scope->find_recursive(vd->name);

            if (sym == nullptr) {
                error(vd->loc, "Variable '{}' is not declared", vd->name);
            }
            if (sym->type_info->name == "unknown") {
                error(vd->loc,
                      "Cannot infer type of variable '{}' before assignment",
                      vd->name);
            }

            vd->symbol_id = sym->id;
            vd->evaluated_type = sym->type_info;
            return true;
        }
        case ASTNode::NODE_MEMBER_ACCESS: {
            auto* ma = static_cast<MemberAccessNode*>(node);
            if (!checkNode(ma->struct_expr)) return false;

            const Type* type = ma->struct_expr->evaluated_type;
            if (type && type->isReference()) {
                type = type->base_type;
            }

            if (!type || !type->isStruct()) {
                error(ma->struct_expr->loc, "Not a struct");
                return false;
            }
            
            const Symbol* sym = type->scope->find(ma->member_name);
            if (sym == nullptr) {
                error(ma->loc, "Member '{}' not found", ma->member_name);
                return false;
            }
            
            ma->evaluated_type = sym->type_info;
            ma->offset = sym->offset;
            return true;
        }
        case ASTNode::NODE_METHOD_CALL:{
            auto* mc = static_cast<MethodCallNode*>(node);
            
            if (!checkNode(mc->object)) return false;

            const Type* type = mc->object->evaluated_type;
            if (type && type->isReference()) {
                type = type->base_type;
            }

            if (!type || !type->isStruct()) {
                error(mc->object->loc, "Not a struct");
                return false;
            }
            
            std::string mangled_name = type->name + "__" + mc->method_name;
            const Symbol* sym = type->scope->find(mangled_name);
            if (sym == nullptr) {
                error(mc->loc, "Method '{}' not found", mc->method_name);
                return false;
            }

            ASTNode* this_arg = mc->object;
            mc->args.insert(mc->args.begin(), this_arg);
            mc->param_types.push_back(make_reference(type));

            for(size_t i = 1; i < mc->args.size(); ++i){
                if(!checkNode(mc->args[i])){
                    return false;
                }

                if(isAutoCastInteger(mc->args[i])){
                    mc->args[i]->evaluated_type = sym->params[i]->type_info;
                }

                if(mc->args[i]->evaluated_type != sym->params[i]->type_info){
                    error(mc->args[i]->loc, "Type mismatch in argument {}", i);
                    return false;
                }

                mc->param_types.push_back(sym->params[i]->type_info);
            }
            
            mc->evaluated_type = sym->type_info;
            return true;
        }
        case ASTNode::NODE_RETURN: {
            auto* rs = static_cast<ReturnNode*>(node);

            if (current_func_return_types.empty()) {
                error(rs->loc, "'return' cannot be used outside of function");
            }

            const Type* return_type = current_func_return_types.back();
            if (!checkNode(rs->expr))
                return false;

            if (isAutoCastInteger(rs->expr)) {
                rs->expr->evaluated_type = return_type;
            }

            if (!isCompatibleType(return_type, rs->expr->evaluated_type)) {
                error(rs->loc,
                      "Return type '{}' does not match expression type '{}'",
                      return_type->name, rs->expr->evaluated_type->name);
            }

            rs->evaluated_type = return_type;
            return true;
        }
        case ASTNode::NODE_INTEGER: {
            node->evaluated_type = i64_t;
            return true;
        }
        case ASTNode::NODE_CHAR_LITERAL: {
            node->evaluated_type = i8_t;
            return true;
        }
        case ASTNode::NODE_UNARY_OP: {
            auto* uo = static_cast<UnaryOpNode*>(node);
            if (!checkNode(uo->value))
                return false;

            if (uo->op.type == Token::NOT) {
                uo->evaluated_type = i64_t;
            } else if(uo->op.type == Token::AND){
                if(uo->value->kind != ASTNode::NODE_VARIABLE){
                    error(uo->loc, "Left value of '&' is not a variable.");
                    return false;
                }
                // reference
                uo->evaluated_type = make_reference(uo->value->evaluated_type);
            } else {
                uo->evaluated_type = uo->value->evaluated_type;
            }
            return true;
        }
        case ASTNode::NODE_BINARY_OP: {
            auto* bo = static_cast<BinaryOpNode*>(node);
            if (!checkNode(bo->left))
                return false;
            if (!checkNode(bo->right))
                return false;

            if (isAutoCastInteger(bo->right)) {
                bo->right->evaluated_type = bo->left->evaluated_type;
            } else if (isAutoCastInteger(bo->left)) {
                bo->left->evaluated_type = bo->right->evaluated_type;
            }

            if (bo->left->evaluated_type != bo->right->evaluated_type) {
                error(bo->loc,
                      "Types of left and right operands do not match. Left: "
                      "'{}', "
                      "Right: '{}'",
                      bo->left->evaluated_type->name,
                      bo->right->evaluated_type->name);
            }

            if (bo->op.type == Token::AND_AND || bo->op.type == Token::OR_OR ||
                bo->op.type == Token::EQUAL_EQUAL ||
                bo->op.type == Token::NOT_EQUAL || bo->op.type == Token::LT ||
                bo->op.type == Token::LE || bo->op.type == Token::GT ||
                bo->op.type == Token::GE) {
                node->evaluated_type = u8_t; // true/false
            } else {
                node->evaluated_type =
                    bo->left->evaluated_type; // arithmetic/bit operations
            }
            return true;
        }
        case ASTNode::NODE_EXPRESSION_STATEMENT: {
            auto* es = static_cast<ExpressionStatementNode*>(node);
            if (!checkNode(es->expr))
                return false;
            es->evaluated_type = es->expr->evaluated_type;
            return true;
        }
        case ASTNode::NODE_ASSIGNMENT: {
            auto* as = static_cast<AssignmentNode*>(node);

            const Type* lvalue_type = nullptr;

            if(as->lvalue->kind == ASTNode::NODE_VARIABLE){
                auto* var_node = static_cast<VariableNode*>(as->lvalue);
                auto* sym =
                    (Symbol*)(current_scope->find_recursive(var_node->name));
                if (sym == nullptr) {
                    error(var_node->loc, "Variable '{}' is not declared",
                          var_node->name);
                }
                if (!checkNode(as->expr))
                    return false;
                if (sym->type_info->name == "unknown") {
                    if(as->expr->evaluated_type->isReference()){
                        error(var_node->loc, "Cannot infer reference type for variable '{}' via assignment. Use initialization 'let {} = ...;' instead.", var_node->name, var_node->name);
                        return false;
                    }
                    sym->type_info = as->expr->evaluated_type;
                } else if (isAutoCastInteger(as->expr)) {
                    as->expr->evaluated_type = sym->type_info;
                }
                var_node->evaluated_type = sym->type_info;
                var_node->symbol_id = sym->id;
                lvalue_type = sym->type_info;
            } else if(as->lvalue->kind == ASTNode::NODE_MEMBER_ACCESS){
                if(!checkNode(as->lvalue)) return false;
                if(!checkNode(as->expr)) return false;

                lvalue_type = as->lvalue->evaluated_type;

                if(isAutoCastInteger(as->expr)){
                    as->expr->evaluated_type = lvalue_type;
                }
            } else {
                error(as->lvalue->loc, 
                    "Left value of assignment is not a variable.");
            }

            if (!isCompatibleType(lvalue_type, as->expr->evaluated_type)) {
                error(as->loc,
                      "Type mismatch in assignment. Left: '{}', Right: '{}'",
                      lvalue_type->name,
                      as->expr->evaluated_type->name);
            }
            as->evaluated_type = lvalue_type;
            return true;
        }
        case ASTNode::NODE_IF: {
            auto* if_node = static_cast<IfNode*>(node);
            if (!checkNode(if_node->condition))
                return false;
            if (!checkNode(if_node->then_block))
                return false;
            if (if_node->else_block) {
                if (!checkNode(if_node->else_block))
                    return false;

                if (isAutoCastInteger(if_node->then_block)) {
                    if_node->then_block->evaluated_type =
                        if_node->else_block->evaluated_type;
                }
                if (isAutoCastInteger(if_node->else_block)) {
                    if_node->else_block->evaluated_type =
                        if_node->then_block->evaluated_type;
                }

                if (if_node->then_block->evaluated_type !=
                    if_node->else_block->evaluated_type) {
                    error(if_node->loc,
                          "Type mismatch between 'then' and 'else' branches. "
                          "Then: "
                          "'{}', Else: '{}'",
                          if_node->then_block->evaluated_type->name,
                          if_node->else_block->evaluated_type->name);
                }
            }
            if_node->evaluated_type = if_node->then_block->evaluated_type;
            return true;
        }
        case ASTNode::NODE_WHILE: {
            auto* while_node = static_cast<WhileNode*>(node);
            if (!checkNode(while_node->condition))
                return false;
            if (!checkNode(while_node->body))
                return false;

            while_node->evaluated_type = while_node->body->evaluated_type;
            return true;
        }
        case ASTNode::NODE_BLOCK: {
            BlockNode* block = static_cast<BlockNode*>(node);
            scopeIn();

            bool ok = true;
            for (auto* stmt : block->statements) {
                if (!checkNode(stmt)) {
                    ok = false;
                    break;
                }
            }
            scopeOut();

            if (!block->statements.empty()) {
                block->evaluated_type =
                    block->statements.back()->evaluated_type;
            }
            return ok;
        }
        default:
            break;
    }

    return true;
}

const Type* Semantics::resolveType(std::string_view type_name, SourceLoc loc){
    if(type_name.starts_with('&')){
        const Type* base = resolveType(type_name.substr(1), loc);
        if(base == nullptr) return nullptr;
        return make_reference(base);
    }

    const Symbol* sym = current_scope->find_recursive(std::string(type_name));
    if(sym == nullptr){
        error(loc, "Type '{}' is not declared", type_name);
        return nullptr;
    }
    if(sym->kind != Symbol::TYPE){
        error(loc, "'{}' is not a type", type_name);
        return nullptr;
    }
    return sym->type_info;
}

bool Semantics::isCompatibleType(const Type* t1, const Type* t2){
    if(t1 == nullptr || t2 == nullptr) return false;

    if(t1 == t2) return true;

    Type* base1 = const_cast<Type*>(t1);
    Type* base2 = const_cast<Type*>(t2);

    if(base1 && base1->isReference()){
        base1 = const_cast<Type*>(base1->base_type);
    }
    if(base2 && base2->isReference()){
        base2 = const_cast<Type*>(base2->base_type);
    }

    return base1 == base2;
}

void Semantics::init_builtins() {
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

const Type* Semantics::alloc_type(Type t) {
    auto type_ptr = std::make_unique<Type>(std::move(t));
    const Type* res = type_ptr.get();
    allocated_types.push_back(std::move(type_ptr));
    return res;
}

const Type* Semantics::make_primitive(const std::string& name, int size,
                                      bool isUnsigned) {
    Type t;
    t.name = name;
    t.size = size;
    t.align = size;
    t.isUnsigned = isUnsigned;
    t.scope = nullptr;
    return alloc_type(t);
}

const Type* Semantics::make_struct(const std::string& name, const Scope* cs){
    Type t;
    t.name = name;
    t.size = 0;
    t.align = 0;
    t.isUnsigned = false;
    t.scope = const_cast<Scope*>(cs);
    return alloc_type(t);
}

const Type* Semantics::make_reference(const Type* t){
    if(t == nullptr) return nullptr;

    if(t->ref_type){
        return t->ref_type;
    }

    if(t->isReference()){
        return t;
    }

    Type ref;
    ref.name = "&" + t->name;
    ref.size = 8;
    ref.align = 8;
    
    ref.isUnsigned = false;
    ref.scope = nullptr;
    ref.base_type = t;

    t->ref_type = alloc_type(ref);
    return t->ref_type;
}
