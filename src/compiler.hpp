
#include <string_view>
class Compiler {
public:
    Compiler();
    ~Compiler();

    bool compile_file(std::string_view filename);
    bool compile_src(std::string_view src);
};
