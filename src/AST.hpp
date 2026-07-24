#ifndef SHIRO_AST_HPP
#define SHIRO_AST_HPP

#include "token.hpp"

struct Type {
    std::string name;
    int size;
    bool isUnsigned;
};

struct ASTNode {
    enum Kind {
        NODE_TRANSLATION_UNIT,
        NODE_FUNCTION_DEFINITION,
        NODE_FUNCTION_CALL,
        NODE_INTEGER,
        NODE_BLOCK,
        NODE_UNARY_OP,
        NODE_BINARY_OP,
        NODE_EXPRESSION_STATEMENT,
        NODE_VARIABLE_DECLARE,
        NODE_VARIABLE,
        NODE_ASSIGNMENT,
        NODE_IF,
        NODE_WHILE,
    };

    Kind kind;
    const Type* evaluated_type = nullptr;

    explicit ASTNode(Kind kind) : kind(kind) {}
};

struct ExpressionStatementNode : public ASTNode {
    ASTNode* expr;

    explicit ExpressionStatementNode(ASTNode* expr) : ASTNode(Kind::NODE_EXPRESSION_STATEMENT), expr(expr) {}
};

struct AssignmentNode : public ASTNode {
    ASTNode* lvalue;
    ASTNode* expr;

    explicit AssignmentNode(ASTNode* lvalue, ASTNode* expr) : ASTNode(Kind::NODE_ASSIGNMENT), lvalue(lvalue), expr(expr){}
};

struct UnaryOpNode : public ASTNode {
    Token op;
    ASTNode* value;

    explicit UnaryOpNode(Token op, ASTNode* value) : ASTNode(Kind::NODE_UNARY_OP), op(op), value(value){};
};

struct BinaryOpNode : public ASTNode{
    Token op;
    ASTNode* left;
    ASTNode* right;

    BinaryOpNode(ASTNode* left, Token op, ASTNode* right) :
        ASTNode(Kind::NODE_BINARY_OP), op(op), left(left), right(right){}
};

struct NumberNode : public ASTNode {
    int64_t value;

    explicit NumberNode(int64_t value) : ASTNode(Kind::NODE_INTEGER), value(value){}
};

struct BlockNode : public ASTNode {
    std::vector<ASTNode*> statements;

    explicit BlockNode(std::vector<ASTNode*> statements) : ASTNode(Kind::NODE_BLOCK), statements(statements){};
};

struct VariableDeclareNode : public ASTNode {
    std::string name;
    std::string type_name;
    int symbol_id = -1;

    explicit VariableDeclareNode(const std::string& n, const std::string& t = "i64") : ASTNode(Kind::NODE_VARIABLE_DECLARE), name(n), type_name(t){};
};

struct VariableNode : public ASTNode {
    std::string name;
    int symbol_id = -1;

    explicit VariableNode(std::string n) : ASTNode(Kind::NODE_VARIABLE), name(n){};
};

struct FunctionCallNode : public ASTNode {
    std::string fn_name;
    std::vector<ASTNode*> args;

    explicit FunctionCallNode(std::string fn_name, std::vector<ASTNode*> args) : ASTNode(Kind::NODE_FUNCTION_CALL), fn_name(fn_name), args(args){};
};

struct IfNode : public ASTNode {
    ASTNode* condition;
    ASTNode* then_block;
    ASTNode* else_block;

    explicit IfNode(ASTNode* condition, ASTNode* then_block, ASTNode* else_block) : ASTNode(Kind::NODE_IF), condition(condition), then_block(then_block), else_block(else_block){};
};

struct WhileNode : public ASTNode {
    ASTNode* condition;
    ASTNode* body;

    explicit WhileNode(ASTNode* condition, ASTNode* body) : ASTNode(Kind::NODE_WHILE), condition(condition), body(body){};
};

struct FunctionDefinitionNode : public ASTNode {
    std::string fn_name;
    std::string type_name;
    std::vector<ASTNode*> params;
    BlockNode* body;

    explicit FunctionDefinitionNode(std::string fn_name, std::string type_name, std::vector<ASTNode*> params ,BlockNode* body) : ASTNode(Kind::NODE_FUNCTION_DEFINITION), fn_name(fn_name), type_name(type_name), params(params), body(body) {}
};

struct TranslationUnitNode : public ASTNode {
    std::vector<ASTNode*> definitions;

    explicit TranslationUnitNode() : ASTNode(Kind::NODE_TRANSLATION_UNIT) {}
};

#endif // SHIRO_AST_HPP

