#include "ast/prototypes/FunctionAst.h"
#include "ast/prototypes/ProcedureAst.h"
#include "ast/types/ArrayTypeAst.h"
#include "ast/walkers/ValidationWalker.h"
#include "backend/Backend.h"
#include "symTable/ArrayTypeSymbol.h"
#include "symTable/MethodSymbol.h"
#include "symTable/TupleTypeSymbol.h"

namespace gazprea::backend {

std::any Backend::visitReturn(std::shared_ptr<ast::statements::ReturnAst> ctx) {
  if (!ctx->getExpr()) {
    builder->create<mlir::LLVM::ReturnOp>(builder->getUnknownLoc(), mlir::ValueRange{});
    return {};
  }

  visit(ctx->getExpr());
  auto [returnType, returnValue] = popElementFromStack(ctx->getExpr());

  const auto methodScope = std::dynamic_pointer_cast<symTable::MethodSymbol>(
      ast::walkers::ValidationWalker::getEnclosingFuncProcScope(ctx->getScope()));
  const auto methodReturnType = methodScope->getReturnType();
  // freeResourcesUntilFunction(ctx->getScope());
  //
  // For array return types, compute sizes if needed (must be done in function body scope)
  if (auto returnArrayType =
          std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(methodReturnType)) {
    if (returnArrayType->getSizes().empty() && currentFunctionProto) {
      // Access the function's return type AST to get size expressions
      auto returnTypeAst = currentFunctionProto->getReturnType();
      if (auto arrayReturnTypeAst =
              std::dynamic_pointer_cast<ast::types::ArrayTypeAst>(returnTypeAst)) {
        // Visit and compute the sizes from the array type AST (in function body scope)
        for (const auto &sizeExpr : arrayReturnTypeAst->getSizes()) {
          visit(sizeExpr);
          auto [sizeType, sizeAddr] = popElementFromStack(sizeExpr);
          if (sizeType->getName() == "integer") {
            returnArrayType->addSize(sizeAddr);
          }
        }
      }
    }
  }
  returnValue = castIfNeeded(ctx, returnValue, returnType, methodReturnType);

  // Create a copy of the return value
  auto returnCopy = builder->create<mlir::LLVM::AllocaOp>(
      loc, ptrTy(), getMLIRType(methodReturnType), constOne());
  copyValue(methodReturnType, returnValue, returnCopy);

  // Now free the original returnValue if it's heap-allocated
  freeAllocatedMemory(methodReturnType, returnValue);

  // Load and return the copy
  auto loadOp = builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(methodReturnType), returnCopy);
  builder->create<mlir::LLVM::ReturnOp>(builder->getUnknownLoc(), loadOp.getResult());

  return {};
}
} // namespace gazprea::backend
