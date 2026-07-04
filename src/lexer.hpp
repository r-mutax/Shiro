
#include <string_view>
#include "token.hpp"

class Lexer {
public:
    Lexer();
    ~Lexer();

    TokenStream lex_src(std::string_view src);
};
