#ifndef SHIRO_SEMANTICS_HPP
#define SHIRO_SEMANTICS_HPP

#include "AST.hpp"
#include <unordered_map>
#include <vector>
#include <memory>
#include <iostream>

struct Symbol {
    enum Kind { VARIABLE, TYPE } kind;
    std::string name;
    int id;
    const Type* type_info = nullptr;

    Symbol(Kind kind, const std::string& name) : kind(kind), name(name){};
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

    Symbol* declare(Symbol::Kind kind, const std::string& name){
        if(find(name) != nullptr){
            return nullptr;
        }
        symbols.emplace(name, Symbol{kind, name});
        return &symbols.at(name);
    }
};

class Semantics {
    Scope global_scope;
    Scope* current_scope = nullptr;
    int symbol_id = 0;

    std::vector<std::unique_ptr<Type>> allocated_types;
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

    Symbol* declare_variable(const std::string& name, const Type* type_info){
        Symbol* symbol = current_scope->declare(Symbol::VARIABLE, name);
        if(symbol != nullptr){
            symbol->id = symbol_id++;
            symbol->type_info = type_info;
        } else {
            std::cerr << "Error: Variable " << name << " is already declared" << std::endl;
            return nullptr;
        }
        return symbol;
    }

    Symbol* declare_type(const std::string& name, const Type* type_info){
        Symbol* symbol = current_scope->declare(Symbol::TYPE, name);
        if(symbol != nullptr){
            symbol->type_info = type_info;
        } else {
            std::cerr << "Error: Type " << name << " is already declared" << std::endl;
            return nullptr;
        }
        return symbol;
    }

    const Type* alloc_type(Type t);
    const Type* make_primitive(const std::string& name, int size, bool isUnsigned);
    void init_builtins();

    const Type* i8_t = nullptr;
    const Type* i16_t = nullptr;
    const Type* i32_t = nullptr;
    const Type* i64_t = nullptr;
    const Type* u8_t = nullptr;
    const Type* u16_t = nullptr;
    const Type* u32_t = nullptr;
    const Type* u64_t = nullptr;
    const Type* unknown = nullptr;

public:
    Semantics() = default;
    ~Semantics() = default;

    bool analyze(ASTNode* ast);
};

#endif // SHIRO_SEMANTICS_HPP
