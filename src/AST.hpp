#ifndef SHIRO_AST_HPP
#define SHIRO_AST_HPP

#include "token.hpp"

struct ASTNode {
    enum Type {
        NODE_TRANSLATION_UNIT,
        NODE_INTEGER,
        NODE_BINARY_OP,
    };

    Type type;

    explicit ASTNode(Type type) : type(type) {}
};

struct TranslationUnitNode : public ASTNode {
    std::vector<ASTNode*> statements;

    explicit TranslationUnitNode() : ASTNode(Type::NODE_TRANSLATION_UNIT) {}
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

#endif // SHIRO_AST_HPP

