#include <iostream>
#include "compiler.hpp"

int main(int argc, const char *argv[]){

    Compiler compiler;

    for(int i = 1; i < argc; ++i){
        std::string_view arg = argv[i];

        if(arg == "-o" || arg == "--output"){
            if(i + 1 < argc){
                compiler.env.output_filename = argv[++i];
            } else {
                std::cerr << "Error: -o requires an argument" << std::endl;
                return 1;
            }
        } else if(arg == "-e" || arg == "--execute"){
            compiler.env.execute = true;
            if(i + 1 < argc){
                compiler.env.execute_code = argv[++i];
            } else {
                std::cerr << "Error: -e requires an argument" << std::endl;
                return 1;
            }
        } else {
            // input file
            compiler.env.input_filename = arg;
        }
    }

    if(!compiler.env.execute && compiler.env.input_filename.empty()){
        std::cerr << "Error: No input file specified" << std::endl;
        return 1;
    }

    if(compiler.env.output_filename.empty()){
        compiler.env.output_filename = "a.out";
    }

    bool success = false;
    if(compiler.env.execute){
        success = compiler.compile_src(compiler.env.execute_code);
    } else {
        success = compiler.compile_file(compiler.env.input_filename);
    }
    
    return success ? 0 : 1;
}