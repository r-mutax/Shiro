#ifndef SHIRO_PARSER_HPP
#define SHIRO_PARSER_HPP

#include "AST.hpp"
#include "error_reporter.hpp"
#include "token.hpp"

struct ParseError {};

class Parser {
    TokenStream& stream;
    ErrorReporter& reporter;

    ASTNode* parseProgram();
    ASTNode* parseDefinition();
    ASTNode* parseFunctionDefinition();
    ASTNode* parseStructDefinition();
    ASTNode* parseStatement();
    ASTNode* parseVariableDeclare();
    ASTNode* parseReturnStatement();
    ASTNode* parseInitExpression();
    ASTNode* parseExpression();
    ASTNode* parseAssign();
    ASTNode* parseLogicalOr();
    ASTNode* parseLogicalAnd();
    ASTNode* parseBitOr();
    ASTNode* parseBitXor();
    ASTNode* parseBitAnd();
    ASTNode* parseEquality();
    ASTNode* parseRelational();
    ASTNode* parseShift();
    ASTNode* parseAddSub();
    ASTNode* parseMulDivMod();
    ASTNode* parseExpressionStatement();
    ASTNode* parseUnary();
    ASTNode* parsePostfix();
    ASTNode* parsePrimary();
    ASTNode* parseBlock();
    ASTNode* parseIfExpression();
    ASTNode* parseWhileExpression();
    std::pair<std::string, SourceLoc> parseTypeName();

    [[noreturn]] void error(SourceLoc loc, const std::string& msg) {
        reporter.reportError(loc, msg);
        throw ParseError();
    }

    void expect(Token::Type type, const std::string& error_msg) {
        if (stream.peek().type != type) {
            error(stream.peek().loc, error_msg);
        }
        stream.next();
    }

  public:
    Parser(TokenStream& stream, ErrorReporter& reporter)
        : stream(stream), reporter(reporter) {}
    ~Parser() = default;

    ASTNode* parse();
};

#endif // SHIRO_PARSER_HPP