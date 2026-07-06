#ifndef SHIRO_PARSER_HPP
#define SHIRO_PARSER_HPP

#include "AST.hpp"
#include "token.hpp"

class Parser {
    TokenStream& stream;

    ASTNode* parseProgram();
    ASTNode* parseExpression();
    ASTNode* parseAddSub();
    ASTNode* parseExpressionStatement();
    ASTNode* parsePrimary();

public:
    Parser(TokenStream& stream) : stream(stream) {}
    ~Parser() = default;

    ASTNode* parse();
};

#endif // SHIRO_PARSER_HPP