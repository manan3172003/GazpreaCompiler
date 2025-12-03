#include "backend/Backend.h"
#include "symTable/TupleTypeSymbol.h"

namespace gazprea::backend {

std::any
Backend::visitTupleElementAssign(std::shared_ptr<ast::statements::TupleElementAssignAst> ctx) {
  auto variableSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  auto tupleTy = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(variableSymbol->getType());
  auto sTy = getMLIRType(tupleTy);
  auto gepIndices = std::vector<mlir::Value>{
      builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0),
      builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(),
                                              ctx->getFieldIndex() - 1)};
  auto elementAddr = builder->create<mlir::LLVM::GEPOp>(
      loc, mlir::LLVM::LLVMPointerType::get(builder->getContext()), sTy, variableSymbol->value,
      gepIndices);
  ctx->setEvaluatedAddr(elementAddr);
  return {};
}

} // namespace gazprea::backend
