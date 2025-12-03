#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitRangedIndexExpr(std::shared_ptr<ast::expressions::RangedIndexExprAst> ctx) {
  visit(ctx->getLeftIndexExpr());
  visit(ctx->getRightIndexExpr());
  return {};
}

} // namespace gazprea::backend