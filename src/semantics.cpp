#include "semantics.hpp"
#include "AST.hpp"

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

            const Symbol* type_sym =
                current_scope->find_recursive(fd->type_name);
            if (type_sym == nullptr) {
                error(fd->type_loc, "Cannot find type '{}' in this scope",
                      fd->type_name);
            }
            if (type_sym->kind != Symbol::TYPE) {
                error(fd->type_loc, "'{}' is not a type", fd->type_name);
            }
            Symbol* func_sym =
                declare_function(fd->fn_name, type_sym->type_info);
            if (func_sym == nullptr) {
                error(fd->loc, "Function '{}' is already declared",
                      fd->fn_name);
            }
            fd->evaluated_type = type_sym->type_info;
            current_func_return_types.push_back(fd->evaluated_type);

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

            const Symbol* type_sym =
                current_scope->find_recursive(vd->type_name);
            if (type_sym == nullptr) {
                error(vd->type_loc, "Type '{}' is not declared", vd->type_name);
            }
            if (type_sym->kind != Symbol::TYPE) {
                error(vd->type_loc, "'{}' is not a type", vd->type_name);
            }

            Symbol* sym = declare_variable(vd->name, type_sym->type_info);
            if (sym == nullptr) {
                error(vd->loc, "Variable '{}' is already declared", vd->name);
                return false;
            }

            vd->symbol_id = sym->id;
            vd->evaluated_type = type_sym->type_info;
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

            if (return_type != rs->expr->evaluated_type) {
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

            if (as->lvalue->kind != ASTNode::NODE_VARIABLE) {
                error(as->lvalue->loc,
                      "Left value of assignment is not a variable");
            }

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
                sym->type_info = as->expr->evaluated_type;
            } else if (isAutoCastInteger(as->expr)) {
                as->expr->evaluated_type = sym->type_info;
            }

            var_node->symbol_id = sym->id;
            var_node->evaluated_type = sym->type_info;

            if (as->lvalue->evaluated_type != as->expr->evaluated_type) {
                error(as->loc,
                      "Type mismatch in assignment. Left: '{}', Right: '{}'",
                      as->lvalue->evaluated_type->name,
                      as->expr->evaluated_type->name);
            }
            as->evaluated_type = var_node->evaluated_type;
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
    t.isUnsigned = isUnsigned;
    return alloc_type(t);
}
