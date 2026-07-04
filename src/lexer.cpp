#include "lexer.hpp"
#include <iostream>

Lexer::Lexer() {
}

Lexer::~Lexer() {
}

TokenStream Lexer::lex_src(std::string_view src) {
    TokenStream stream;

    int len = src.size();
    
    for(int i = 0; i < len; ++i){
        char c = src[i];
        
        switch(c) {
            case '0'...'9': {
                int j = i;
                while(j < len && std::isdigit(src[j])) {
                    j++;
                }
                stream.tokens.push_back({
                    Token::NUMBER,
                    std::string(src.substr(i, j - i))
                });
                i = j - 1;
                break;
            }
            case '+': {
                stream.tokens.push_back({Token::PLUS, "+"});
                break;
            }
            case '-' : {
                stream.tokens.push_back({Token::MINUS, "-"});
                break;
            }
        }
    }

    stream.tokens.push_back({Token::EOF_TOK, ""});

    // トークンストリームをすべて標準出力に吐き出す
    // タイプは文字列として吐き出す
    for(const auto& token : stream.tokens) {
        std::cout << token.to_str() << std::endl;
    }

    return stream;
}