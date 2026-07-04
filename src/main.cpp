#include <iostream>
#include "shiro.hpp"

int main(int argc, const char *argv[])
{
    if(argc < 2) {
        std::cerr << "Usage: shiro <filename>" << std::endl;
        return 1;
    }
    if(argc > 3) {
        std::cerr << "Too many arguments" << std::endl;
        return 1;
    }
    
    Compiler compiler;
    if(!compiler.compile_file(argv[1])) {
        return 1;
    }
    return 0;
}