#include "backend/Backend.h"
#include "symTable/StructTypeSymbol.h"

namespace gazprea::backend {

std::any
Backend::visitStructElementAssign(std::shared_ptr<ast::statements::StructElementAssignAst> ctx) {
  auto variableSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  const auto structTy =
      std::dynamic_pointer_cast<symTable::StructTypeSymbol>(variableSymbol->getType());
  auto sTy = getMLIRType(structTy);
  auto gepIndices = std::vector<mlir::Value>{
      builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), 0),
      builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(),
                                              structTy->getIdx(ctx->getElementName()) - 1)};
  auto elementAddr = builder->create<mlir::LLVM::GEPOp>(
      loc, mlir::LLVM::LLVMPointerType::get(builder->getContext()), sTy, variableSymbol->value,
      gepIndices);
  ctx->setEvaluatedAddr(elementAddr);
  return {};
}

} // namespace gazprea::backend