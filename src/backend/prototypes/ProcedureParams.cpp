#include "backend/Backend.h"
#include "symTable/VariableSymbol.h"

namespace gazprea::backend {

std::any Backend::visitProcedureParams(std::shared_ptr<ast::prototypes::ProcedureParamAst> ctx) {
  visit(ctx->getParamType());
  const auto varSym = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  const auto it = blockArg.find(ctx->getName());
  if (it != blockArg.end()) {
    const auto arg = it->second;
    if (varSym->getQualifier() == ast::Qualifier::Var) {
      // var parameter: comes as a pointer, store directly
      varSym->value = arg;
    } else {
      // const parameter: comes as a pointer, load and store in alloca
      auto paramType = getMLIRType(varSym->getType());
      auto loadedValue = builder->create<mlir::LLVM::LoadOp>(loc, paramType, arg);
      auto allocaOp = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), paramType, constOne());
      builder->create<mlir::LLVM::StoreOp>(loc, loadedValue, allocaOp.getResult());
      varSym->value = allocaOp.getResult();
    }
  }
  return {};
}
} // namespace gazprea::backend
