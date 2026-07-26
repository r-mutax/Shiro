#ifndef SHIRO_LEXER_HPP
#define SHIRO_LEXER_HPP

#include <string_view>
#include "token.hpp"
#include "error_reporter.hpp"

class Lexer {
public:

    Lexer(ErrorReporter& reporter) : reporter(reporter) {}
    ~Lexer() {}

    TokenStream lex_src(std::string_view src);
    ErrorReporter& reporter;

private:
    bool is_ident1(char c);
    bool is_ident2(char c);
    Token::Type check_keyword(std::string_view s);
};

#endif // SHIRO_LEXER_HPP
