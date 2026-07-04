
class Compiler {
public:
    Compiler();
    ~Compiler();

    bool compile_file(const char* filename);
    bool compile_src(const char* src);
};
