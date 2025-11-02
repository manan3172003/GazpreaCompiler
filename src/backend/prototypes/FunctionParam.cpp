#include "backend/Backend.h"
#include "symTable/VariableSymbol.h"

namespace gazprea::backend {

std::any Backend::visitFunctionParam(std::shared_ptr<ast::prototypes::FunctionParamAst> ctx) {
  const auto varSym = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  const auto it = blockArg.find(ctx->getName());
  if (it != blockArg.end()) {
    auto arg = it->second;
    // Function parameters are always 'const'
    auto paramType = getMLIRType(varSym->getType());
    auto loadedValue = builder->create<mlir::LLVM::LoadOp>(loc, paramType, arg);
    auto allocaOp = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), paramType, constOne());
    builder->create<mlir::LLVM::StoreOp>(loc, loadedValue, allocaOp.getResult());
    varSym->value = allocaOp.getResult();
  }
  return {};
}

} // namespace gazprea::backend
