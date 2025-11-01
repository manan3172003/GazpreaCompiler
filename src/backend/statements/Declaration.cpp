#include "backend/Backend.h"

namespace gazprea::backend {
std::any Backend::visitDeclaration(std::shared_ptr<ast::statements::DeclarationAst> ctx) {
  visit(ctx->getExpr());

  auto [type, valueAddr] = ctx->getScope()->getTopElementInStack();
  ctx->getSymbol()->value = valueAddr;

  return {};
}

} // namespace gazprea::backend