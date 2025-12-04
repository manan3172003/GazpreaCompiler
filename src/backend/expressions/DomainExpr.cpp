#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitDomainExpr(std::shared_ptr<ast::expressions::DomainExprAst> ctx) {
  visit(ctx->getDomainExpression());
  return {};
}

} // namespace gazprea::backend
