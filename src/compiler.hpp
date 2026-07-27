#ifndef SHIRO_COMPILER_HPP
#define SHIRO_COMPILER_HPP

#include "error_reporter.hpp"
#include <string>
#include <string_view>

struct CompilerEnvironment {
    std::string output_filename;
    std::string input_filename;

    bool execute = false;
    std::string execute_code;
};

class Compiler {
  public:
    CompilerEnvironment env;
    ErrorReporter reporter;

    Compiler();
    ~Compiler();

    bool compile_file(std::string_view filename);
    bool compile_src(std::string_view src);
};

#endif // SHIRO_COMPILER_HPP
