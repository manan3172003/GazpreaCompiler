#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitSingularIndex(std::shared_ptr<ast::expressions::SingularIndexExprAst> ctx) {
  visit(ctx->getSingularIndexExpr());
  return {};
}

} // namespace gazprea::backend