#ifndef METAWRITER_HPP
#define METAWRITER_HPP

#include "AST.hpp"
#include "error_reporter.hpp"

#include <fstream>
#include <string_view>

class MetaWriter {
    ErrorReporter& reporter;
    const Scope* global_scope;

    std::string srcname;
    std::string meta_filename;
    std::ofstream out;

    std::string calculateHash(std::string_view content);

    void writeHeader(std::string_view src);
    void writePub();
    void writePrivate();
    void writeFunctionDeclare(Symbol& sym);
    void writeStructDeclare(Symbol& sym);
    void writeMethodDeclare(Symbol& method, std::string strct_name);

public:
    MetaWriter(ErrorReporter& reporter, const std::string& srcname) : reporter(reporter), srcname(srcname), meta_filename(srcname + ".meta") {}
    ~MetaWriter() {}

    void write(const Scope* global_scope, std::string_view src);
};

#endif // METAWRITER_HPP