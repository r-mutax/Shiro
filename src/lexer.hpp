#ifndef SHIRO_LEXER_HPP
#define SHIRO_LEXER_HPP

#include "error_reporter.hpp"
#include "token.hpp"
#include <string_view>

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
