#ifndef BRICK_TYPE_CHECKER_H
#define BRICK_TYPE_CHECKER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <stack>
#include "../parser/ast.h"
#include "../parser/package.h"

namespace brick {

struct SymbolInfo {
    std::string type;
    bool is_private = false;
    bool is_param = false;
};

class TypeChecker {
public:
    explicit TypeChecker(const PackageTable& packages);

    std::vector<std::string> check(
        const std::vector<std::unique_ptr<ProgramNode>>& asts
    );

private:
    const PackageTable& packages;
    std::vector<std::string> errors;

    // Global struct/interface definitions
    // Definicoes globais de struct/interface
    std::unordered_map<std::string, StructDecl*> struct_defs;
    std::unordered_map<std::string, InterfaceDecl*> interface_defs;

    // Scoped symbol tables
    // Tabelas de simbolos com escopo
    std::vector<std::unordered_map<std::string, SymbolInfo>> scopes;

    std::string current_package;
    std::string current_struct;
    bool using_io = false;

    void declare_builtins();
    void push_scope();
    void pop_scope();
    void declare(const std::string& name, const std::string& type, bool is_private, bool is_param = false);
    SymbolInfo* lookup(const std::string& name);

    bool is_type_known(const std::string& type_name);
    bool is_struct_type(const std::string& type_name);
    bool is_interface_type(const std::string& type_name);
    bool can_assign(const std::string& from, const std::string& to);
    std::string promote_types(const std::string& a, const std::string& b);

    void check_program(ProgramNode* program);
    void check_struct(StructDecl* sd);
    void check_interface(InterfaceDecl* id);
    void check_function(FuncDecl* fd, const std::string& struct_name);
    void declare_inherited_fields(StructDecl* sd);
    void check_block(BlockStmt* block, const std::string& return_type);
    void check_statement(ASTNode* stmt, const std::string& return_type);
    std::string check_expression(ASTNode* expr);
    void check_constructor_call(const std::string& struct_name, CallExpr* call);

    void add_error(const std::string& msg);
};

} // namespace brick
  // namespace brick

#endif
