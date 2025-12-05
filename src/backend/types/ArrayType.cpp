#include <backend/Backend.h>

namespace gazprea::backend {
std::any Backend::visitArrayType(std::shared_ptr<ast::types::ArrayTypeAst> ctx) {

  auto arrayTypSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(ctx->getSymbol());

  const auto sizeExpr = ctx->getSizes();
  for (size_t i = 0; i < sizeExpr.size(); ++i) {
    visit(sizeExpr[i]);

    auto [type, valueAddr] = popElementFromStack(sizeExpr[i]);

    // Could be * or an integer value
    mlir::Value recordedSizeAddr = valueAddr;
    if (!type || type->getName() != "integer") {
      int inferredSize = 0;
      if (i < arrayTypSym->inferredElementSize.size()) {
        inferredSize = arrayTypSym->inferredElementSize[i];
      }
      auto ifrValue = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), inferredSize);
      recordedSizeAddr = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), intTy(), constOne());
      builder->create<mlir::LLVM::StoreOp>(loc, ifrValue, recordedSizeAddr);
    }

    arrayTypSym->declaredElementSize.push_back(recordedSizeAddr);
  }

  return {};
}
} // namespace gazprea::backend