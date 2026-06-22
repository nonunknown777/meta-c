#ifndef BRICK_LSP_H
#define BRICK_LSP_H

#include <string>
#include <vector>
#include "types.h"
#include "../parser/ast.h"

namespace brick {

struct LspSymbol {
    std::string name;
    std::string kind;       // "struct", "interface", "function", "block", "field", "param", "variable"
    std::string type_name;  // resolved type or declared type
    SourceLocation location;
};

struct LspError {
    std::string message;
    SourceLocation location;
    int severity; // 1=error, 2=warning, 3=info
};

struct LspOutput {
    std::vector<Token> tokens;
    std::vector<LspSymbol> symbols;
    std::vector<LspError> errors;
};

// Collect symbols from the AST recursively
void collect_symbols(ASTNode* node, std::vector<LspSymbol>& symbols);

// Parse error strings like "file:line: message" into structured LspError
LspError parse_error_string(const std::string& err_str, const std::string& default_file);

// Emit JSON for LSP consumption
std::string emit_lsp_json(const LspOutput& output);

} // namespace brick

#endif
