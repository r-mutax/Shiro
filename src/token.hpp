#include <string>
#include <vector>

struct Token {
    enum Type {
        NUMBER,
        EOF_TOK,
    };

    Type type;
    std::string value;
};

class TokenStream {
public:
    TokenStream(){}
    ~TokenStream(){}

    std::vector<Token> tokens;
    int current_index = 0;

    Token next();
    Token peek();
    bool is_eof();
};

