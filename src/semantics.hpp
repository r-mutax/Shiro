#ifndef SHIRO_SEMANTICS_HPP
#define SHIRO_SEMANTICS_HPP

#include "AST.hpp"

class Semantics {

    bool checkNode(ASTNode* node);

public:
    Semantics() = default;
    ~Semantics() = default;

    bool analyze(ASTNode* ast);
};

#endif // SHIRO_SEMANTICS_HPP