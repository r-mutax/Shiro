#ifndef SHIRO_SEMANTICS_HPP
#define SHIRO_SEMANTICS_HPP

#include "AST.hpp"
#include <unordered_map>
#include <vector>
#include <memory>
#include <iostream>

struct Symbol {
    std::string name;
    int id;
};

struct Scope {
    std::unordered_map<std::string, Symbol> symbols;
    Scope* parent = nullptr;
    
    // find symbols in this scope
    const Symbol* find(const std::string& name) const {
        auto it = symbols.find(name);
        if(it != symbols.end()){
            return &it->second;
        }
        return nullptr;
    }

    // find symbols in this scope or parent
    const Symbol* find_recursive(const std::string& name) const {
        auto it = symbols.find(name);
        if(it != symbols.end()){
            return &it->second;
        }
        if(parent != nullptr){
            return parent->find_recursive(name);
        }
        return nullptr;
    }

    Symbol* declare(const std::string& name){
        if(find(name) != nullptr){
            return nullptr;
        }
        symbols.emplace(name, Symbol{name, -1});
        return &symbols.at(name);
    }
};

class Semantics {
    Scope global_scope;
    Scope* current_scope = nullptr;
    int symbol_id = 0;

    std::vector<std::unique_ptr<Scope>> allocated_scopes;

    bool checkNode(ASTNode* node);
    
    void scopeIn(){
        auto scope = std::make_unique<Scope>();
        scope->parent = current_scope;
        current_scope = scope.get();
        allocated_scopes.push_back(std::move(scope));
    }

    void scopeOut(){
        if(current_scope != nullptr){
            current_scope = current_scope->parent;
        }
    }

    Symbol* declare(std::string& name){
        Symbol* symbol = current_scope->declare(name);
        if(symbol != nullptr){
            symbol->id = symbol_id++;
        } else {
            std::cerr << "Error: Variable " << name << " is already declared" << std::endl;
            return nullptr;
        }
        return symbol;
    }

public:
    Semantics() = default;
    ~Semantics() = default;

    bool analyze(ASTNode* ast);
};

#endif // SHIRO_SEMANTICS_HPP
