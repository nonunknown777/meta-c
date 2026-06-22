#ifndef BRICK_PARSER_H
#define BRICK_PARSER_H

#include <vector>
#include <memory>
#include <string>
#include "ast.h"
#include "../shared/types.h"

namespace brick {

struct ParseResult {
    std::unique_ptr<ProgramNode> ast;
    std::vector<std::string> errors;
};

ParseResult parse(const std::vector<Token>& tokens);

} // namespace brick

#endif
