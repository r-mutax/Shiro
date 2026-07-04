#include <iostream>
#include <fstream>
#include "compiler.hpp"
#include "lexer.hpp"

Compiler::Compiler() {
}

Compiler::~Compiler() {
}

bool Compiler::compile_file(std::string_view filename) {
    std::string file_src;
    
    std::ifstream file_stream(filename.data());
    if(!file_stream.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return false;
    }

    std::string line;
    while(std::getline(file_stream, line)) {
        file_src += line + "\n";
    }
    file_stream.close();

    return compile_src(file_src);
}

bool Compiler::compile_src(std::string_view src) {

    Lexer lexer;
    lexer.lex_src(src);

    return true;
}
