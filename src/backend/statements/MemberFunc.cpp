#include <backend/Backend.h>

namespace gazprea::backend {
std::any Backend::visitLenMemberFunc(std::shared_ptr<ast::statements::LenMemberFuncAst> ctx) {
  visit(ctx->getLeft());

  auto [vectorType, vectorAddr] = popElementFromStack(ctx);
  auto vectorTypeSym = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(vectorType);
  if (!vectorTypeSym) {
    return {};
  }

  auto lenFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("@vector_len");
  if (!lenFunc) {
    makeLenMemberFunc();
    lenFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("@vector_len");
  }

  auto lenCall = builder->create<mlir::LLVM::CallOp>(loc, lenFunc, mlir::ValueRange{vectorAddr});
  auto lenValue = lenCall.getResult();

  auto lenAlloca = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), intTy(), constOne());
  builder->create<mlir::LLVM::StoreOp>(loc, lenValue, lenAlloca);

  auto integerType =
      std::dynamic_pointer_cast<symTable::Type>(ctx->getScope()->resolveType("integer"));
  if (integerType) {
    ctx->getScope()->pushElementToScopeStack(integerType, lenAlloca);
  }

  return {};
}
std::any Backend::visitAppendMemberFunc(std::shared_ptr<ast::statements::AppendMemberFuncAst> ctx) {
  return {};
}
std::any Backend::visitPushMemberFunc(std::shared_ptr<ast::statements::PushMemberFuncAst> ctx) {
  return {};
}
std::any Backend::visitConcatMemberFunc(std::shared_ptr<ast::statements::ConcatMemberFuncAst> ctx) {
  return {};
}
} // namespace gazprea::backend
