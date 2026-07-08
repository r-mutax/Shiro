#include <iostream>
#include <fstream>
#include "compiler.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "semantics.hpp"
#include "irgen.hpp"
#include "x86gen.hpp"

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

    std::ostream* out = &std::cout;
    std::ofstream out_file;

    if(!env.output_filename.empty() && env.output_filename != "-"){
        out_file.open(env.output_filename.data());
        if(!out_file.is_open()) {
            std::cerr << "Error opening output file: " << env.output_filename << std::endl;
            return false;
        }
        out = &out_file;
    }

    try {
        Lexer lexer;
        TokenStream stream = lexer.lex_src(src);

        Parser parser(stream);
        ASTNode* ast = parser.parse();

        if(!ast) {
            std::cerr << "Error: Failed to parse source code" << std::endl;
            return false;
        }

        Semantics semantics;
        if(!semantics.analyze(ast)) {
            std::cerr << "Error: Failed to analyze source code" << std::endl;
            return false;
        }

        IRGenerator ir_generator;
        if(!ir_generator.generate(ast)) {
            std::cerr << "Error: Failed to generate code" << std::endl;
            return false;
        }

        X86Generator x86_generator(ir_generator.get_program(), *out);
        x86_generator.generate();

    } catch(std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }

    return true;
}
