#ifndef ERROR_REPORTER_HPP
#define ERROR_REPORTER_HPP

#include "token.hpp"
#include <string>
#include <string_view>
#include <vector>

class ErrorReporter {

    std::string filename;
    std::string src;
    std::vector<std::string_view> lines;

    bool has_error = false;

  public:
    ErrorReporter() = default;
    void SetFile(std::string filename, std::string src);
    void reportError(SourceLoc loc, const std::string& msg);
    [[noreturn]] void reportFatal(const std::string& msg);

    bool hasError() const { return has_error; }
};

#endif
