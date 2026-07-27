#include "error_reporter.hpp"
#include <iomanip>
#include <iostream>

void ErrorReporter::SetFile(std::string filename, std::string src) {

    this->filename = filename;
    this->src = src;
    lines.clear();

    std::string_view src_view(this->src);

    // split src file by '\n' to line
    size_t start = 0;
    size_t end = src_view.find('\n');
    while (end != std::string_view::npos) {
        lines.push_back(src_view.substr(start, end - start));
        start = end + 1;
        end = src_view.find('\n', start);
    }
    lines.push_back(src_view.substr(start));
};

void ErrorReporter::reportError(SourceLoc loc, const std::string& msg) {
    has_error = true;

    // error msg print by red character
    std::cerr << filename << ":" << loc.line << ":" << loc.col << ":\033[31m"
              << " error: " << msg << "\033[0m" << std::endl;
    if (loc.line > 0 && loc.line <= lines.size()) {

        std::string_view line_str = lines[loc.line - 1];

        std::cerr << " " << std::setw(4) << loc.line << " | " << line_str
                  << std::endl;
        std::cerr << "      | ";
        for (size_t i = 1; i < loc.col; ++i) {
            std::cerr << " ";
        }
        std::cerr << "^" << std::endl;
    }
}

[[noreturn]] void ErrorReporter::reportFatal(const std::string& msg) {
    has_error = true;
    std::cerr << "Fatal Error : " << msg << std::endl;
    std::exit(EXIT_FAILURE);
}