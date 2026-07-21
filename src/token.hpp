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
        AND,                    // &
        AND_AND,                // &&
        HAT,                    // ^
        OR,                     // |
        OR_OR,                  // ||
        LT,                     // <
        GT,                     // >
        LE,                     // <= 
        GE,                     // >=
        COLON,                  // : 
        SEMICOLON,              // ;
        EQUAL,                  // =
        EQUAL_EQUAL,            // ==
        NOT_EQUAL,              // !=
        NOT,                    // !
        CHILDA,                 // ~
        LET,                    // let
        IF,                     // if
        WHILE,                  // while
        ELSE,                   // else
        UNKNOWN,                // unknown
        I8,                     // i8
        I16,                    // i16
        I32,                    // i32
        I64,                    // i64
        U8,                     // u8
        U16,                    // u16
        U32,                    // u32
        U64,                    // u64
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
            case AND:
                return "AND";
            case AND_AND:
                return "AND_AND";
            case HAT:
                return "HAT";
            case OR:
                return "OR";
            case OR_OR:
                return "OR_OR";
            case NOT:
                return "NOT";
            case LT:
                return "LT";
            case GT:
                return "GT";
            case LE:
                return "LE";
            case GE:
                return "GE";
            case COLON:
                return "COLON";
            case SEMICOLON:
                return "SEMICOLON";
            case EQUAL:
                return "EQUAL";
            case EQUAL_EQUAL:
                return "EQUAL_EQUAL";
            case NOT_EQUAL:
                return "NOT_EQUAL";
            case CHILDA:
                return "CHILDA";
            case LET:
                return "LET";
            case IF:
                return "if";
            case ELSE:
                return "else";
            case WHILE:
                return "while";
            case I8:
                return "i8";
            case I16:
                return "i16";
            case I32:
                return "i32";
            case I64:
                return "i64";
            case U8:
                return "u8";
            case U16:
                return "u16";
            case U32:
                return "u32";
            case U64:
                return "u64";
            case UNKNOWN:
                return "unknown";
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
