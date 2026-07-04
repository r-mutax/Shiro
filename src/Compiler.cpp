#include <iostream>
#include "shiro.hpp"

Compiler::Compiler() {
}

Compiler::~Compiler() {
}

bool Compiler::compile_file(const char* filename) {
    std::cout << "Compiling file: " << filename << std::endl;
    return true;
}

bool Compiler::compile_src(const char* src) {
    std::cout << "Compiling src: " << src << std::endl;
    return true;
}
