#include "backend/Backend.h"
#include "symTable/MethodSymbol.h"

namespace gazprea::backend {

std::any Backend::visitProcedure(std::shared_ptr<ast::prototypes::ProcedureAst> ctx) {
  const auto methodSym =
      std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getProto()->getSymbol());

  const auto savedInsertPoint = builder->saveInsertionPoint();

  if (methodSym && methodSym->getName() == "main") {
    auto mainType = mlir::LLVM::LLVMFunctionType::get(intTy(), {}, false);
    auto mainFunc = builder->create<mlir::LLVM::LLVMFuncOp>(loc, "main", mainType);
    mlir::Block *entry = mainFunc.addEntryBlock();
    builder->setInsertionPointToStart(entry);

    visit(ctx->getBody());
  } else {
    const auto procReturnType = getMLIRType(methodSym->getReturnType());
    auto procType = mlir::LLVM::LLVMFunctionType::get(
        procReturnType, collectMethodParams(ctx->getProto()->getParams()), false);
    auto procOp = builder->create<mlir::LLVM::LLVMFuncOp>(loc, methodSym->getName(), procType);
    mlir::Block *entry = procOp.addEntryBlock();
    builder->setInsertionPointToStart(entry);

    visit(ctx->getBody());
  }
  builder->restoreInsertionPoint(savedInsertPoint);
  return {};
}

} // namespace gazprea::backend
