#include "backend/Backend.h"
#include "symTable/VariableSymbol.h"

namespace gazprea::backend {

std::any Backend::visitProcedureParams(std::shared_ptr<ast::prototypes::ProcedureParamAst> ctx) {
  visit(ctx->getParamType());
  const auto varSym = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  if (const auto it = blockArg.find(ctx->getName()); it != blockArg.end()) {
    const auto arg = it->second;
    varSym->value = arg;
  }
  return {};
}
} // namespace gazprea::backend
