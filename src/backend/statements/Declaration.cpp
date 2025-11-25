#include "CompileTimeExceptions.h"
#include "backend/Backend.h"
#include "symTable/ArrayTypeSymbol.h"
#include "symTable/VariableSymbol.h"
#include "utils/BackendUtils.h"

namespace gazprea::backend {
std::any Backend::visitDeclaration(std::shared_ptr<ast::statements::DeclarationAst> ctx) {
  if (ctx->getScope()->getScopeType() == symTable::ScopeType::Global) {
    createGlobalDeclaration(ctx->getExpr()->getInferredSymbolType()->getName(), ctx->getExpr(),
                            ctx->getSymbol(), ctx->getName());
    return {};
  }
  visit(ctx->getExpr());
  auto [type, valueAddr] = ctx->getScope()->getTopElementInStack();
  auto variableSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  auto newAddr = builder->create<mlir::LLVM::AllocaOp>(
      loc, ptrTy(), getMLIRType(variableSymbol->getType()), constOne());
  copyValue(type, valueAddr, newAddr);
  computeArraySizeIfArray(ctx, type, newAddr);
  arraySizeValidation(variableSymbol, type, newAddr);
  ctx->getSymbol()->value = newAddr;
  castIfNeeded(newAddr, ctx->getExpr()->getInferredSymbolType(), variableSymbol->getType());
  return {};
}

} // namespace gazprea::backend