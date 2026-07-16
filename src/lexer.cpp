#include "lexer.hpp"
#include "keyword_map.h"
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
    case '=': {
      if(i + 1 < len && src[i + 1] == '='){
        stream.tokens.push_back({Token::EQUAL_EQUAL, "==", i, 2});
        i++;
      }else{
        stream.tokens.push_back({Token::EQUAL, "=", i, 1});
      }
      break;
    }
    case '!' : {
      if(i + 1 < len && src[i + 1] == '='){
        stream.tokens.push_back({Token::NOT_EQUAL, "!=", i, 2});
        i++;
      }else{
        throw std::runtime_error("Expected '=' after '!'");
      }
      break;
    }
    case ';': {
      stream.tokens.push_back({Token::SEMICOLON, ";", i, 1}); break;
    }
    case ' ':
    case '\t':
    case '\r':
    case '\n':
      break;
    default: {
      if(is_ident1(c)){
        size_t s = i;
        while(i < len && is_ident2(src[i])){
          i++;
        }
        std::string_view ident = src.substr(s, i - s);
        stream.tokens.push_back({check_keyword(ident), std::string(ident), s, i - s });
        i--;  
        break;
      }
      std::cerr << "Error: Unknown token: " << c << std::endl;
      return TokenStream{};
    }
    }
  }

  stream.tokens.push_back({Token::EOF_TOK, "", len, 0});

  return stream;
}

bool Lexer::is_ident1(char c){
    return isalpha(c) || c == '_';
}

bool Lexer::is_ident2(char c){
    return is_ident1(c) || isdigit(c);
}

Token::Type Lexer::check_keyword(std::string_view s){
    for(size_t i = 0; i < std::size(keyword_map); i++){
        if(s == keyword_map[i].keyword){
            return keyword_map[i].type;
        }
    }
    return Token::IDENT;
}

