#include "CompileTimeExceptions.h"
#include "backend/Backend.h"
#include "symTable/ArrayTypeSymbol.h"
#include "symTable/VariableSymbol.h"
#include "symTable/VectorTypeSymbol.h"
#include "utils/BackendUtils.h"

namespace gazprea::backend {
std::any Backend::visitDeclaration(std::shared_ptr<ast::statements::DeclarationAst> ctx) {
  if (ctx->getScope()->getScopeType() == symTable::ScopeType::Global) {
    createGlobalDeclaration(ctx->getExpr()->getInferredSymbolType()->getName(), ctx->getExpr(),
                            ctx->getSymbol(), ctx->getName());
    return {};
  }
  visit(ctx->getType());
  visit(ctx->getExpr());
  auto [type, valueAddr] = popElementFromStack(ctx->getExpr());
  const auto variableSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  if (ctx->getType()->getNodeType() == ast::NodeType::VectorType) {
    auto vectorTypeSymbol =
        std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(variableSymbol->getType());
    if (vectorTypeSymbol) {
      auto newAddr = createVectorValue(vectorTypeSymbol, type, valueAddr);
      ctx->getSymbol()->value = newAddr;
      ctx->getScope()->pushElementToFree(std::make_pair(vectorTypeSymbol, newAddr));
      return {};
    }
  }
  mlir::Value newAddr = builder->create<mlir::LLVM::AllocaOp>(
      loc, ptrTy(), getMLIRType(variableSymbol->getType()), constOne());
  valueAddr = castIfNeeded(ctx, valueAddr, ctx->getExpr()->getInferredSymbolType(),
                           variableSymbol->getType());
  copyValue(variableSymbol->getType(), valueAddr, newAddr);
  ctx->getSymbol()->value = newAddr;
  freeAllocatedMemory(variableSymbol->getType(), valueAddr);
  return {};
}
} // namespace gazprea::backend
