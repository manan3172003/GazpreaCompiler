#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitIteratorLoop(
    std::shared_ptr<ast::statements::IteratorLoopAst> ctx) {
  return {};
}

} // namespace gazprea::backend
