#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitIdentifier(std::shared_ptr<ast::expressions::IdentifierAst> ctx) {
  auto value = ctx->getSymbol()->value;
  ctx->getScope()->pushElementToScopeStack(ctx->getInferredSymbolType(), value);
  return {};
}

} // namespace gazprea::backend
