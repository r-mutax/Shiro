#include "token.hpp"

typedef struct KEYWORD_MAP {
    std::string keyword;
    Token::Type type;
} KEYWORD_MAP;

static KEYWORD_MAP keyword_map[] = {
    {   "let",      Token::LET  },
    {   "if",       Token::IF   },
    {   "else",     Token::ELSE },
    {   "while",    Token::WHILE},
};