#include "token.hpp"

typedef struct KEYWORD_MAP {
    std::string keyword;
    Token::Type type;
} KEYWORD_MAP;

static KEYWORD_MAP keyword_map[] = {
    {   "let",      Token::LET      },
    {   "if",       Token::IF       },
    {   "else",     Token::ELSE     },
    {   "while",    Token::WHILE    },
    {   "i8",       Token::I8       },
    {   "i16",      Token::I16      },
    {   "i32",      Token::I32      },
    {   "i64",      Token::I64      },
    {   "u8",       Token::U8       },
    {   "u16",      Token::U16      },
    {   "u32",      Token::U32      },
    {   "u64",      Token::U64      },
    {   "unknown",  Token::UNKNOWN  },
    {   "fn",       Token::FN       },
    {   "return",   Token::RETURN   },
};