#ifndef SHIRO_PARSER_HPP
#define SHIRO_PARSER_HPP

#include "AST.hpp"
#include "token.hpp"

class Parser {
    TokenStream& stream;

    ASTNode* parseProgram();
    ASTNode* parseDefinition();
    ASTNode* parseFunctionDefinition();
    ASTNode* parseStatement();
    ASTNode* parseVariableDeclare();
    ASTNode* parseExpression();
    ASTNode* parseAssign();
    ASTNode* parseShift();
    ASTNode* parseMulDivMod();
    ASTNode* parseAddSub();
    ASTNode* parseExpressionStatement();
    ASTNode* parsePrimary();

public:
    Parser(TokenStream& stream) : stream(stream) {}
    ~Parser() = default;

    ASTNode* parse();
};

#endif // SHIRO_PARSER_HPP