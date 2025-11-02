#include "backend/Backend.h"
#include "utils/BackendUtils.h"

namespace gazprea::backend {
std::any Backend::visitDeclaration(std::shared_ptr<ast::statements::DeclarationAst> ctx) {
  visit(ctx->getExpr());

  auto [type, valueAddr] = ctx->getScope()->getTopElementInStack();
  ctx->getSymbol()->value = valueAddr;
  utils::castIfNeeded(builder, loc, valueAddr, ctx->getExpr()->getInferredSymbolType(),
                      ctx->getInferredType());
  return {};
}

} // namespace gazprea::backend