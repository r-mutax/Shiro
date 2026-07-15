#ifndef SHIRO_AST_HPP
#define SHIRO_AST_HPP

#include "token.hpp"

struct ASTNode {
    enum Type {
        NODE_TRANSLATION_UNIT,
        NODE_FUNCTION_DEFINITION,
        NODE_INTEGER,
        NODE_BINARY_OP,
        NODE_EXPRESSION_STATEMENT,
        NODE_VARIABLE_DECLARE,
        NODE_VARIABLE,
        NODE_ASSIGNMENT,
    };

    Type type;

    explicit ASTNode(Type type) : type(type) {}
};

struct TranslationUnitNode : public ASTNode {
    std::vector<ASTNode*> definitions;

    explicit TranslationUnitNode() : ASTNode(Type::NODE_TRANSLATION_UNIT) {}
};

struct FunctionDefinitionNode : public ASTNode {
    std::string name;
    std::vector<ASTNode*> statements;

    explicit FunctionDefinitionNode(std::string name, std::vector<ASTNode*> statements) : ASTNode(Type::NODE_FUNCTION_DEFINITION), name(name), statements(statements) {}
};

struct ExpressionStatementNode : public ASTNode {
    ASTNode* expr;

    explicit ExpressionStatementNode(ASTNode* expr) : ASTNode(Type::NODE_EXPRESSION_STATEMENT), expr(expr) {}
};

struct AssignmentNode : public ASTNode {
    ASTNode* lvalue;
    ASTNode* expr;

    explicit AssignmentNode(ASTNode* lvalue, ASTNode* expr) : ASTNode(Type::NODE_ASSIGNMENT), lvalue(lvalue), expr(expr){}
};

struct BinaryOpNode : public ASTNode{
    Token op;
    ASTNode* left;
    ASTNode* right;

    BinaryOpNode(ASTNode* left, Token op, ASTNode* right) :
        ASTNode(Type::NODE_BINARY_OP), op(op), left(left), right(right){}
};

struct NumberNode : public ASTNode {
    int64_t value;

    explicit NumberNode(int64_t value) : ASTNode(Type::NODE_INTEGER), value(value){}
};

struct VariableDeclareNode : public ASTNode {
    std::string name;
    int symbol_id = -1;

    explicit VariableDeclareNode(std::string n) : ASTNode(Type::NODE_VARIABLE_DECLARE), name(n){};
};

struct VariableNode : public ASTNode {
    std::string name;
    int symbol_id = -1;

    explicit VariableNode(std::string n) : ASTNode(Type::NODE_VARIABLE), name(n){};
};

#endif // SHIRO_AST_HPP

