#ifndef BRICK_LEXER_H
#define BRICK_LEXER_H

#include <vector>
#include <string>
#include "../shared/types.h"

namespace brick {

std::vector<Token> tokenize(const std::string& source, const std::string& filename = "<input>");

} // namespace brick
  // namespace brick

#endif
