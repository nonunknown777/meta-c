#ifndef BRICK_MACRO_EXPANDER_H
#define BRICK_MACRO_EXPANDER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "ast.h"

namespace brick {

using MacroTable = std::unordered_map<std::string, MacroDecl*>;

struct ExpandResult {
    bool success = true;
    std::vector<std::string> errors;
};

// Deep-clone an AST subtree (for macro body instantiation)
std::unique_ptr<ASTNode> clone_ast(const ASTNode* node);

// Expand all macro calls in the AST
ExpandResult expand_macros(
    std::vector<std::unique_ptr<ProgramNode>>& asts,
    MacroTable& macro_table
);

// Collect MacroDecl nodes from AST into the table, removing them from AST
void collect_macros(
    std::vector<std::unique_ptr<ProgramNode>>& asts,
    MacroTable& macro_table
);

} // namespace brick

#endif
