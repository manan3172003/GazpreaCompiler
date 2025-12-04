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
    freeResourcesUntilFunction(ctx->getScope());
    builder->create<mlir::LLVM::ReturnOp>(builder->getUnknownLoc(), mlir::ValueRange{});
    return {};
  }

  visit(ctx->getExpr());

  bool isLValue = ctx->getExpr()->isLValue();

  mlir::Value returnValue;
  std::shared_ptr<symTable::Type> returnType;

  if (isLValue) {
    // LValue: Copy needed because the original might be freed
    auto [type, value] = popElementFromStack(ctx); // Adds to elementsToFree
    returnType = type;

    if (type->getName().substr(0, 5) == "array" || type->getName().substr(0, 6) == "vector" ||
        type->getName() == "tuple") {
      // Deep copy
      auto temp =
          builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(type), constOne());
      copyValue(type, value, temp);
      returnValue = temp;
    } else {
      // Scalar
      returnValue = value;
    }
  } else {
    // RValue: Steal (don't add to elementsToFree)
    auto top = ctx->getScope()->getTopElementInStack();
    ctx->getScope()->popElementFromScopeStack();
    returnType = top.first;
    returnValue = top.second;
  }

  const auto methodScope = std::dynamic_pointer_cast<symTable::MethodSymbol>(
      ast::walkers::ValidationWalker::getEnclosingFuncProcScope(ctx->getScope()));

  freeResourcesUntilFunction(ctx->getScope());

  // For array return types, compute sizes if needed (must be done in function body scope)
  auto methodReturnType = methodScope->getReturnType();
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
  auto loadOp = builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(methodScope->getReturnType()),
                                                    returnValue);
  builder->create<mlir::LLVM::ReturnOp>(builder->getUnknownLoc(), loadOp.getResult());

  return {};
}
} // namespace gazprea::backend
