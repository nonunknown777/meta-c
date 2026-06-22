#ifndef BRICK_CODEGEN_H
#define BRICK_CODEGEN_H

#include <string>
#include <vector>
#include <memory>
#include "../parser/ast.h"
#include "../parser/package.h"

namespace brick {

struct CodegenResult {
    std::string c_code;
    std::vector<std::string> errors;
    bool success = false;
};

// Generate C code from AST + PackageTable
// Gera codigo C a partir de AST + PackageTable
CodegenResult generate_c(
    const std::vector<std::unique_ptr<ProgramNode>>& asts,
    const PackageTable& packages
);

} // namespace brick
  // namespace brick

#endif
