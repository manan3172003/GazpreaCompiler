#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitArg(std::shared_ptr<ast::expressions::ArgAst> ctx) {
  visit(ctx->getExpr());
  return {};
}

} // namespace gazprea::backend
