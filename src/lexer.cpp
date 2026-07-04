#include "lexer.hpp"
#include <iostream>

Lexer::Lexer() {
}

Lexer::~Lexer() {
}

bool Lexer::lex_src(std::string_view src) {
    std::cout << "Lexing src: " << src << std::endl;
    return true;
}