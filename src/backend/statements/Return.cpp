#include "ast/walkers/ValidationWalker.h"
#include "backend/Backend.h"
#include "symTable/MethodSymbol.h"
#include "symTable/TupleTypeSymbol.h"

namespace gazprea::backend {

std::any Backend::visitReturn(std::shared_ptr<ast::statements::ReturnAst> ctx) {
  if (!ctx->getExpr()) {
    builder->create<mlir::LLVM::ReturnOp>(builder->getUnknownLoc(), mlir::ValueRange{});
    return {};
  }

  visit(ctx->getExpr());
  auto [type, value] = popElementFromStack(ctx);

  const auto methodScope = std::dynamic_pointer_cast<symTable::MethodSymbol>(
      ast::walkers::ValidationWalker::getEnclosingFuncProcScope(ctx->getScope()));

  if (type->getName() == "tuple") {
    // Promote the tuple value to the return value
    castIfNeeded(value, type, methodScope->getReturnType());
    auto loadOp =
        builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(methodScope->getReturnType()), value);
    builder->create<mlir::LLVM::ReturnOp>(builder->getUnknownLoc(), loadOp.getResult());
  } else {
    // handle all scalar types
    castIfNeeded(value, type, methodScope->getReturnType());
    auto loadOp =
        builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(methodScope->getReturnType()), value);
    builder->create<mlir::LLVM::ReturnOp>(builder->getUnknownLoc(), loadOp.getResult());
  }
  // TODO: Handle other return types
  return {};
}
} // namespace gazprea::backend
