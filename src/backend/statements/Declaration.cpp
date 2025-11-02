#include "backend/Backend.h"
#include "symTable/VariableSymbol.h"
#include "utils/BackendUtils.h"

namespace gazprea::backend {
std::any Backend::visitDeclaration(std::shared_ptr<ast::statements::DeclarationAst> ctx) {
  visit(ctx->getExpr());

  auto [type, valueAddr] = ctx->getScope()->getTopElementInStack();
  ctx->getSymbol()->value = valueAddr;
  castIfNeeded(valueAddr, ctx->getExpr()->getInferredSymbolType(),
               std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol())->getType());
  return {};
}

} // namespace gazprea::backend