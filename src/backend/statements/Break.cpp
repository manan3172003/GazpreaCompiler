#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitBreak(std::shared_ptr<ast::statements::BreakAst> ctx) {
  return {};
}

} // namespace gazprea::backend
