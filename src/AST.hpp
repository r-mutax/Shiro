#ifndef SHIRO_AST_HPP
#define SHIRO_AST_HPP

#include "token.hpp"
#include <unordered_map>

struct Scope;

struct Type {
    std::string name;
    int size;
    bool isUnsigned;
    Scope* scope;
    int align;

    const Type* base_type = nullptr;
    mutable const Type* ref_type = nullptr;

    bool isStruct() const {
        return scope != nullptr;
    }
    
    bool isReference() const {
        return base_type != nullptr;
    }
};

struct Symbol {
    enum Kind { VARIABLE, FUNCTION, TYPE } kind;
    std::string name;
    int id;
    const Type* type_info = nullptr;
    std::vector<Symbol*> params;
    int offset = 0;

    Symbol(Kind kind, const std::string& name) : kind(kind), name(name){};
};

struct Scope {
    std::unordered_map<std::string, Symbol> symbols;
    Scope* parent = nullptr;

    // find symbols in this scope
    const Symbol* find(const std::string& name) const {
        auto it = symbols.find(name);
        if (it != symbols.end()) {
            return &it->second;
        }
        return nullptr;
    }

    // find symbols in this scope or parent
    const Symbol* find_recursive(const std::string& name) const {
        auto it = symbols.find(name);
        if (it != symbols.end()) {
            return &it->second;
        }
        if (parent != nullptr) {
            return parent->find_recursive(name);
        }
        return nullptr;
    }

    Symbol* declare(Symbol::Kind kind, const std::string& name) {
        if (find(name) != nullptr) {
            return nullptr;
        }
        symbols.emplace(name, Symbol{kind, name});
        return &symbols.at(name);
    }
};

struct ASTNode {
    enum Kind {
        NODE_TRANSLATION_UNIT,
        NODE_FUNCTION_DEFINITION,
        NODE_STRUCT_DEFINITION,
        NODE_FUNCTION_CALL,
        NODE_INTEGER,
        NODE_CHAR_LITERAL,
        NODE_BLOCK,
        NODE_UNARY_OP,
        NODE_BINARY_OP,
        NODE_EXPRESSION_STATEMENT,
        NODE_VARIABLE_DECLARE,
        NODE_VARIABLE,
        NODE_ASSIGNMENT,
        NODE_IF,
        NODE_WHILE,
        NODE_RETURN,
        NODE_MEMBER_ACCESS,
    };

    Kind kind;
    const Type* evaluated_type = nullptr;
    SourceLoc loc;

    explicit ASTNode(Kind kind) : kind(kind) {}
};

struct ExpressionStatementNode : public ASTNode {
    ASTNode* expr;

    explicit ExpressionStatementNode(ASTNode* expr)
        : ASTNode(Kind::NODE_EXPRESSION_STATEMENT), expr(expr) {}
};

struct AssignmentNode : public ASTNode {
    ASTNode* lvalue;
    ASTNode* expr;

    explicit AssignmentNode(ASTNode* lvalue, ASTNode* expr)
        : ASTNode(Kind::NODE_ASSIGNMENT), lvalue(lvalue), expr(expr) {}
};

struct UnaryOpNode : public ASTNode {
    Token op;
    ASTNode* value;

    explicit UnaryOpNode(Token op, ASTNode* value)
        : ASTNode(Kind::NODE_UNARY_OP), op(op), value(value){};
};

struct BinaryOpNode : public ASTNode {
    Token op;
    ASTNode* left;
    ASTNode* right;

    BinaryOpNode(ASTNode* left, Token op, ASTNode* right)
        : ASTNode(Kind::NODE_BINARY_OP), op(op), left(left), right(right) {}
};

struct NumberNode : public ASTNode {
    int64_t value;

    explicit NumberNode(int64_t value)
        : ASTNode(Kind::NODE_INTEGER), value(value) {}
};

struct CharLiteralNode : public ASTNode {
    int64_t value;

    explicit CharLiteralNode(int64_t value)
        : ASTNode(Kind::NODE_CHAR_LITERAL), value(value) {}
};

struct BlockNode : public ASTNode {
    std::vector<ASTNode*> statements;

    explicit BlockNode(std::vector<ASTNode*> statements)
        : ASTNode(Kind::NODE_BLOCK), statements(statements){};
};

struct VariableDeclareNode : public ASTNode {
    std::string name;
    std::string type_name;
    SourceLoc type_loc;
    int symbol_id = -1;
    bool is_pub = false;

    explicit VariableDeclareNode(const std::string& n, SourceLoc type_loc,
                                 const std::string& t = "i64")
        : ASTNode(Kind::NODE_VARIABLE_DECLARE), name(n), type_name(t),
          type_loc(type_loc){};
};

struct VariableNode : public ASTNode {
    std::string name;
    int symbol_id = -1;

    explicit VariableNode(std::string n)
        : ASTNode(Kind::NODE_VARIABLE), name(n){};
};

struct MemberAccessNode : public ASTNode {
    ASTNode* struct_expr;
    std::string member_name;
    int offset = 0;

    explicit MemberAccessNode(ASTNode* struct_expr, std::string member_name)
        : ASTNode(Kind::NODE_MEMBER_ACCESS), struct_expr(struct_expr), member_name(member_name){};
};

struct FunctionCallNode : public ASTNode {
    std::string fn_name;
    std::vector<ASTNode*> args;

    explicit FunctionCallNode(std::string fn_name, std::vector<ASTNode*> args)
        : ASTNode(Kind::NODE_FUNCTION_CALL), fn_name(fn_name), args(args){};
};

struct IfNode : public ASTNode {
    ASTNode* condition;
    ASTNode* then_block;
    ASTNode* else_block;

    explicit IfNode(ASTNode* condition, ASTNode* then_block,
                    ASTNode* else_block)
        : ASTNode(Kind::NODE_IF), condition(condition), then_block(then_block),
          else_block(else_block){};
};

struct WhileNode : public ASTNode {
    ASTNode* condition;
    ASTNode* body;

    explicit WhileNode(ASTNode* condition, ASTNode* body)
        : ASTNode(Kind::NODE_WHILE), condition(condition), body(body){};
};

struct ReturnNode : public ASTNode {
    ASTNode* expr;

    explicit ReturnNode(ASTNode* expr)
        : ASTNode(Kind::NODE_RETURN), expr(expr){};
};

struct StructDefinitionNode : public ASTNode {
    std::string strct_name;
    std::vector<ASTNode*> members;

    explicit StructDefinitionNode(std::string strct_name, std::vector<ASTNode*> members)
        : ASTNode(Kind::NODE_STRUCT_DEFINITION), strct_name(strct_name), members(members){};
};

struct FunctionDefinitionNode : public ASTNode {
    std::string fn_name;
    std::string type_name;
    std::vector<ASTNode*> params;
    BlockNode* body;
    SourceLoc type_loc;
    bool is_pub = false;

    explicit FunctionDefinitionNode(std::string fn_name, std::string type_name,
                                    std::vector<ASTNode*> params,
                                    BlockNode* body, SourceLoc type_loc)
        : ASTNode(Kind::NODE_FUNCTION_DEFINITION), fn_name(fn_name),
          type_name(type_name), params(params), body(body), type_loc(type_loc) {
    }
};

struct TranslationUnitNode : public ASTNode {
    std::vector<ASTNode*> definitions;

    explicit TranslationUnitNode() : ASTNode(Kind::NODE_TRANSLATION_UNIT) {}
};

#endif // SHIRO_AST_HPP
