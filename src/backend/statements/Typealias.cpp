#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitTypealias(std::shared_ptr<ast::statements::TypealiasAst> ctx) {
  visit(ctx->getType());
  return {};
}

} // namespace gazprea::backend
