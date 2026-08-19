#ifndef SHIRO_SEMANTICS_HPP
#define SHIRO_SEMANTICS_HPP

#include "AST.hpp"
#include "error_reporter.hpp"
#include "token.hpp"
#include <format>
#include <memory>
#include <vector>

struct SemanticsError {};

class Semantics {
    ErrorReporter& reporter;
    Scope global_scope;
    Scope* current_scope = nullptr;
    int symbol_id = 0;

    std::vector<const Type*> current_func_return_types;

    std::vector<std::unique_ptr<Type>> allocated_types;
    std::vector<std::unique_ptr<Scope>> allocated_scopes;

    bool checkNode(ASTNode* node);

    bool isCompatibleType(const Type* t1, const Type* t2);

    void scopeIn() {
        auto scope = std::make_unique<Scope>();
        scope->parent = current_scope;
        current_scope = scope.get();
        allocated_scopes.push_back(std::move(scope));
    }

    void scopeOut() {
        if (current_scope != nullptr) {
            current_scope = current_scope->parent;
        }
    }

    Symbol* declare_variable(const std::string& name, const Type* type_info) {
        Symbol* symbol = current_scope->declare(Symbol::VARIABLE, name);
        if (symbol != nullptr) {
            symbol->id = symbol_id++;
            symbol->type_info = type_info;
        } else {
            return nullptr;
        }
        return symbol;
    }

    Symbol* declare_function(const std::string& name, const Type* ret_type) {
        Symbol* symbol = current_scope->declare(Symbol::FUNCTION, name);
        if (symbol != nullptr) {
            // function need not system id
            symbol->type_info = ret_type;
        } else {
            return nullptr;
        }
        return symbol;
    }

    Symbol* declare_type(const std::string& name, const Type* type_info) {
        Symbol* symbol = current_scope->declare(Symbol::TYPE, name);
        if (symbol != nullptr) {
            symbol->type_info = type_info;
        } else {
            return nullptr;
        }
        return symbol;
    }

    const Type* resolveType(std::string_view type_name, SourceLoc loc);
    const Type* alloc_type(Type t);
    const Type* make_primitive(const std::string& name, int size,
                               bool isUnsigned);
    const Type* make_struct(const std::string& name, const Scope* cs);
    const Type* make_reference(const Type* t);
    const Type* make_array(const Type* base_type, const int64_t len);
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

    template <typename... Args>
    [[noreturn]] void error(SourceLoc loc, std::string_view fmt,
                            Args&&... args) {
        std::string msg = std::vformat(fmt, std::make_format_args(args...));
        reporter.reportError(loc, msg);
        throw SemanticsError();
    }

  public:
    Semantics(ErrorReporter& reporter) : reporter(reporter) {}
    ~Semantics() = default;

    bool analyze(ASTNode* ast);
    const Scope* getGlobalScope() const { return &global_scope; }
};

#endif // SHIRO_SEMANTICS_HPP
