#include "lexer.hpp"
#include <iostream>

Lexer::Lexer() {}

Lexer::~Lexer() {}

TokenStream Lexer::lex_src(std::string_view src) {
  TokenStream stream;

  size_t len = src.size();

  for (size_t i = 0; i < len; ++i) {
    char c = src[i];

    switch (c) {
    case '0' ... '9': {
      size_t j = i;
      while (j < len && std::isdigit(src[j])) {
        j++;
      }
      stream.tokens.push_back(
          {Token::NUMBER, std::string(src.substr(i, j - i)), i, j - i});
      i = j - 1;
      break;
    }
    case '+': {
      stream.tokens.push_back({Token::PLUS, "+", i, 1});
      break;
    }
    case '-': {
      stream.tokens.push_back({Token::MINUS, "-", i, 1});
      break;
    }
    case '*': {
      stream.tokens.push_back({Token::ASTERISK, "*", i, 1});
      break;
    }
    case '/': {
      stream.tokens.push_back({Token::SLASH, "/", i, 1});
      break;
    }
    case '%': {
      stream.tokens.push_back({Token::MOD, "%", i, 1});
      break;
    }
    case '(': {
      stream.tokens.push_back({Token::LPAREN, "(", i, 1});
      break;
    }
    case ')': {
      stream.tokens.push_back({Token::RPAREN, ")", i, 1});
      break;
    }
    case '<': {
      if (i + 1 < len && src[i + 1] == '<') {
        stream.tokens.push_back({Token::LSHIFT, "<<", i, 2});
        i++;
      } else {
        throw std::runtime_error("Expected '<' after '<'");
      }
      break;
    }
    case '>': {
      if (i + 1 < len && src[i + 1] == '>') {
        stream.tokens.push_back({Token::RSHIFT, ">>", i, 2});
        i++;
      } else {
        throw std::runtime_error("Expected '>' after '>'");
      }
      break;
    }
    case ';': {
      stream.tokens.push_back({Token::SEMICOLON, ";", i, 1}); break;
    } case ' ':
    case '\t':
    case '\r':
    case '\n':
      break;
    default: {
      std::cerr << "Error: Unknown token: " << c << std::endl;
      return TokenStream{};
    }
    }
  }

  stream.tokens.push_back({Token::EOF_TOK, "", len, 0});

  return stream;
}