#include "compiler.hpp"
#include "error_reporter.hpp"
#include "irgen.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "semantics.hpp"
#include "x86gen.hpp"
#include <fstream>
#include <iostream>

Compiler::Compiler() {}

Compiler::~Compiler() {}

bool Compiler::compile_file(std::string_view filename) {
  std::string file_src;

  std::ifstream file_stream(filename.data());
  if (!file_stream.is_open()) {
    reporter.reportFatal("Cannot open file : " + std::string(filename));
    return false;
  }

  std::string line;
  while (std::getline(file_stream, line)) {
    file_src += line + "\n";
  }
  file_stream.close();

  reporter.SetFile(filename.data(), file_src);

  return compile_src(file_src);
}

bool Compiler::compile_src(std::string_view src) {

  std::ostream *out = &std::cout;
  std::ofstream out_file;

  if (!env.output_filename.empty() && env.output_filename != "-") {
    out_file.open(env.output_filename.data());
    if (!out_file.is_open()) {
      reporter.reportFatal("Cannot open output file : " +
                           std::string(env.output_filename));
      return false;
    }
    out = &out_file;
  }

  try {
    Lexer lexer(reporter);
    TokenStream stream = lexer.lex_src(src);
    if (reporter.hasError())
      return false;

    Parser parser(stream, reporter);
    ASTNode *ast = parser.parse();

    if (!ast) {
      reporter.reportFatal("Failed to parse source code");
    }

    Semantics semantics(reporter);
    if (!semantics.analyze(ast)) {
      std::cerr << "Error: Failed to analyze source code" << std::endl;
      return false;
    }

    IRGenerator ir_generator(reporter);
    if (!ir_generator.generate(ast)) {
      std::cerr << "Error: Failed to generate code" << std::endl;
      return false;
    }

    X86Generator x86_generator(ir_generator.get_program(), reporter, *out);
    x86_generator.generate();

  } catch(const ParseError&){
    return false;
  } catch(const SemanticsError&){
    return false;
  } catch(const IRGeneratorError){
    return false;
  } catch(const X86GeneratorError){
    return false;
  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return false;
  }

  return true;
}
