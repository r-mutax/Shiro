#ifndef SHIRO_TOKEN_HPP
#define SHIRO_TOKEN_HPP

#include <string>
#include <vector>
#include <stdexcept>

struct Token {
    enum Type {
        NUMBER,
        PLUS,
        MINUS,
        ASTERISK,
        SLASH,
        MOD,
        LPAREN,
        RPAREN,
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

    bool is_eof(){
        if(cursor >= tokens.size()) {
            return true;
        }

        return tokens[cursor].type == Token::EOF_TOK;
    }
};

#endif // SHIRO_TOKEN_HPP
