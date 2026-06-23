#ifndef BRICK_BUILD_EVAL_H
#define BRICK_BUILD_EVAL_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "ast.h"
#include "macro_expander.h"

namespace brick {

struct BuildEvalResult {
    bool success = true;
    std::vector<std::string> errors;
};

// Evaluate all build {} blocks in the AST, replacing them with emitted code
BuildEvalResult eval_build_blocks(
    std::vector<std::unique_ptr<ProgramNode>>& asts,
    MacroTable& macro_table
);

} // namespace brick

#endif
