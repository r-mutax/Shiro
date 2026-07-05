#ifndef SHIRO_LEXER_HPP
#define SHIRO_LEXER_HPP

#include <string_view>
#include "token.hpp"

class Lexer {
public:
    Lexer();
    ~Lexer();

    TokenStream lex_src(std::string_view src);
};

#endif // SHIRO_LEXER_HPP
