#include "lexer.hpp"
#include "keyword_map.h"

TokenStream Lexer::lex_src(std::string_view src) {
    TokenStream stream;

    size_t len = src.size();

    size_t line = 1;
    size_t col = 1;
    size_t start_col = 1;

    auto create_token = [&](Token::Type type, size_t offset,
                            std::string_view value) {
        stream.tokens.push_back({type,
                                 std::string(value),
                                 offset,
                                 value.size(),
                                 {line, start_col}});
        col += value.size();
    };

    for (size_t i = 0; i < len; ++i) {
        char c = src[i];

        if (c == '\n') {
            line++;
            col = 1;
            continue;
        }

        if (std::isspace(c)) {
            col++;
            continue;
        }

        start_col = col;

        switch (c) {
            case '0' ... '9': {
                size_t j = i;
                while (j < len && std::isdigit(src[j])) {
                    j++;
                }
                create_token(Token::NUMBER, i, src.substr(i, j - i));
                i = j - 1;
                break;
            }
            case '\'': {
                size_t s = i;
                i++;

                if (i >= len) {
                    reporter.reportError({line, col},
                                         "Unterminated character literal");
                    return TokenStream{};
                }

                char char_val = 0;
                if (src[i] == '\\') {
                    i++;
                    if (i >= len) {
                        reporter.reportError({line, col},
                                             "Unterminated character literal");
                        return TokenStream{};
                    }

                    if (src[i] == 'n')
                        char_val = '\n';
                    else if (src[i] == 't')
                        char_val = '\t';
                    else if (src[i] == 'r')
                        char_val = '\r';
                    else if (src[i] == '0')
                        char_val = '\0';
                    else if (src[i] == '\\')
                        char_val = '\\';
                    else if (src[i] == '\'')
                        char_val = '\'';
                    else {
                        reporter.reportError(
                            {line, col},
                            std::string("Unknown escape sequence: \\") +
                                src[i]);
                        return TokenStream{};
                    }
                } else {
                    char_val = src[i];
                }
                ++i;
                if (i >= len || src[i] != '\'') {
                    reporter.reportError(
                        {line, col},
                        "Expected '\'' at the end of character literal");
                    return TokenStream{};
                }
                create_token(Token::CHAR, s,
                             std::to_string(static_cast<int>(char_val)));
                break;
            }
            case '+': {
                create_token(Token::PLUS, i, "+");
                break;
            }
            case '-': {
                if (i + 1 < len && src[i + 1] == '>') {
                    create_token(Token::ARROW, i, "->");
                    i++;
                } else {
                    create_token(Token::MINUS, i, "-");
                }
                break;
            }
            case '*': {
                create_token(Token::ASTERISK, i, "*");
                break;
            }
            case '/': {
                create_token(Token::SLASH, i, "/");
                break;
            }
            case '%': {
                create_token(Token::MOD, i, "%");
                break;
            }
            case '(': {
                create_token(Token::LPAREN, i, "(");
                break;
            }
            case ')': {
                create_token(Token::RPAREN, i, ")");
                break;
            }
            case '{': {
                create_token(Token::LBRACE, i, "{");
                break;
            }
            case '}': {
                create_token(Token::RBRACE, i, "}");
                break;
            }
            case '<': {
                if (i + 1 < len && src[i + 1] == '<') {
                    create_token(Token::LSHIFT, i, "<<");
                    i++;
                } else if (i + 1 < len && src[i + 1] == '=') {
                    create_token(Token::LE, i, "<=");
                    i++;
                } else {
                    create_token(Token::LT, i, "<");
                }
                break;
            }
            case '>': {
                if (i + 1 < len && src[i + 1] == '>') {
                    create_token(Token::RSHIFT, i, ">>");
                    i++;
                } else if (i + 1 < len && src[i + 1] == '=') {
                    create_token(Token::GE, i, ">=");
                    i++;
                } else {
                    create_token(Token::GT, i, ">");
                }
                break;
            }
            case '=': {
                if (i + 1 < len && src[i + 1] == '=') {
                    create_token(Token::EQUAL_EQUAL, i, "==");
                    i++;
                } else {
                    create_token(Token::EQUAL, i, "=");
                }
                break;
            }
            case '&': {
                if (i + 1 < len && src[i + 1] == '&') {
                    create_token(Token::AND_AND, i, "&&");
                    i++;
                } else {
                    create_token(Token::AND, i, "&");
                }
                break;
            }
            case '|': {
                if (i + 1 < len && src[i + 1] == '|') {
                    create_token(Token::OR_OR, i, "||");
                    i++;
                } else {
                    create_token(Token::OR, i, "|");
                }
                break;
            }
            case '^': {
                create_token(Token::HAT, i, "^");
                break;
            }
            case ',': {
                create_token(Token::COMMA, i, ",");
                break;
            }
            case '.': {
                create_token(Token::DOT, i, ".");
                break;
            }
            case '!': {
                if (i + 1 < len && src[i + 1] == '=') {
                    create_token(Token::NOT_EQUAL, i, "!=");
                    i++;
                } else {
                    create_token(Token::NOT, i, "!");
                }
                break;
            }
            case '~': {
                create_token(Token::CHILDA, i, "~");
                break;
            }
            case ':': {
                create_token(Token::COLON, i, ":");
                break;
            }
            case ';': {
                create_token(Token::SEMICOLON, i, ";");
                break;
            }
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                break;
            default: {
                if (is_ident1(c)) {
                    size_t s = i;
                    while (i < len && is_ident2(src[i])) {
                        i++;
                    }
                    std::string_view ident = src.substr(s, i - s);
                    create_token(check_keyword(ident), i, ident);
                    i--;
                    break;
                }
                reporter.reportError({line, col},
                                     std::string("Unknown token: ") + c);
                return TokenStream{};
            }
        }
    }

    create_token(Token::EOF_TOK, len, "");

    return stream;
}

bool Lexer::is_ident1(char c) { return isalpha(c) || c == '_'; }

bool Lexer::is_ident2(char c) { return is_ident1(c) || isdigit(c); }

Token::Type Lexer::check_keyword(std::string_view s) {
    for (size_t i = 0; i < std::size(keyword_map); i++) {
        if (s == keyword_map[i].keyword) {
            return keyword_map[i].type;
        }
    }
    return Token::IDENT;
}
