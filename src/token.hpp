#include <string>
#include <vector>

struct Token {
    enum Type {
        NUMBER,
        PLUS,
        MINUS,
        EOF_TOK,
    };

    Type type;
    std::string value;

    std::string to_str() const {
        switch(type) {
            case NUMBER:
                return "NUMBER: " + value;
            case PLUS:
                return "PLUS";
            case MINUS:
                return "MINUS";
            case EOF_TOK:
                return "EOF";
        }
        return "UNKNOWN";
    }
};

class TokenStream {
public:
    TokenStream(){}
    ~TokenStream(){}

    std::vector<Token> tokens;
    size_t cursor = 0;

    Token next();
    Token peek();
    bool is_eof();
};

