#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitIdentifierLeft(std::shared_ptr<ast::statements::IdentifierLeftAst> ctx) {
  auto variableSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  ctx->setEvaluatedAddr(variableSymbol->value);
  return {};
}

} // namespace gazprea::backend
