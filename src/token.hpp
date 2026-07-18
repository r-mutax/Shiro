#ifndef SHIRO_TOKEN_HPP
#define SHIRO_TOKEN_HPP

#include <string>
#include <vector>
#include <stdexcept>

struct Token {
    enum Type {
        NUMBER,                 // 0-9
        PLUS,                   // +
        MINUS,                  // -
        ASTERISK,               // *
        SLASH,                  // /
        MOD,                    // %
        LPAREN,                 // (
        RPAREN,                 // )
        LBRACE,                 // {
        RBRACE,                 // }
        LSHIFT,                 // <<
        RSHIFT,                 // >>
        LT,                     // <
        GT,                     // >
        LE,                     // <= 
        GE,                     // >= 
        SEMICOLON,              // ;
        EQUAL,                  // =
        EQUAL_EQUAL,            // ==
        NOT_EQUAL,              // !=
        LET,                    // let
        IF,                     // if
        WHILE,                  // while
        ELSE,                   // else
        IDENT,                  // Identifier
        EOF_TOK,
    };

    Type type;
    std::string value;
    size_t offset = 0;
    size_t length = 0;

    std::string to_str() const {
        switch(type) {
            case NUMBER:
                return "NUMBER: " + value;
            case PLUS:
                return "PLUS";
            case MINUS:
                return "MINUS";
            case ASTERISK:
                return "ASTERISK";
            case SLASH:
                return "SLASH";
            case MOD:
                return "MOD";
            case LPAREN:
                return "LPAREN";
            case RPAREN:
                return "RPAREN";
            case LBRACE:
                return "LBRACE";
            case RBRACE:
                return "RBRACE";
            case LSHIFT:
                return "LSHIFT";
            case RSHIFT:
                return "RSHIFT";
            case LT:
                return "LT";
            case GT:
                return "GT";
            case LE:
                return "LE";
            case GE:
                return "GE";
            case SEMICOLON:
                return "SEMICOLON";
            case EQUAL:
                return "EQUAL";
            case EQUAL_EQUAL:
                return "EQUAL_EQUAL";
            case NOT_EQUAL:
                return "NOT_EQUAL";
            case LET:
                return "LET";
            case IF:
                return "if";
            case ELSE:
                return "else";
            case WHILE:
                return "while";
            case IDENT:
                return "IDENT: " + value;
            case EOF_TOK:
                return "EOF";
        }
        return "UNKNOWN";
    }
};

class TokenStream {
public:
    TokenStream(){}
    ~TokenStream(){}

    std::vector<Token> tokens;
    size_t cursor = 0;

    const Token& peek() {
        if(cursor >= tokens.size()) {
            throw std::runtime_error("Unexpected EOF");
        }
        return tokens[cursor];
    }

    Token next(){
        if(cursor >= tokens.size()) {
            throw std::runtime_error("Unexpected EOF");
        }
        return tokens[cursor++];
    }

    bool consume(Token::Type type){
        if(peek().type == type){
            cursor++;
            return true;
        }

        return false;
    }
    
    void expect(Token::Type type){
        if(peek().type != type){
            throw std::runtime_error("Expected " + peek().to_str());
        }
        cursor++;
    }

    bool is_eof(){
        if(cursor >= tokens.size()) {
            return true;
        }

        return tokens[cursor].type == Token::EOF_TOK;
    }
};

#endif // SHIRO_TOKEN_HPP
