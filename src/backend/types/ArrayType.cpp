#include <backend/Backend.h>

namespace gazprea::backend {
std::any Backend::visitArrayType(std::shared_ptr<ast::types::ArrayTypeAst> ctx) {

  auto arrayTypSym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(ctx->getSymbol());

  if (!currentFunctionProto) {
    return std::any{};
  }

  const auto sizeExpr = ctx->getSizes();
  for (size_t i = 0; i < sizeExpr.size(); ++i) {
    // Could be * or an integer value
    mlir::Value recordedSizeAddr;

    // Fix for global scope array type definitions (e.g. typealias)
    // We cannot use AllocaOp/StoreOp at global scope because there is no function block to insert
    // into. Instead we create a GlobalOp for the size constant.
    if (!currentFunctionProto &&
        std::dynamic_pointer_cast<ast::expressions::IntegerLiteralAst>(sizeExpr[i])) {
      auto intLiteral = std::dynamic_pointer_cast<ast::expressions::IntegerLiteralAst>(sizeExpr[i]);
      std::string globalName =
          "array_size_" + std::to_string(reinterpret_cast<uintptr_t>(intLiteral.get()));
      auto globalOp = builder->create<mlir::LLVM::GlobalOp>(
          loc, intTy(), /*isConstant=*/true, mlir::LLVM::Linkage::Internal, globalName,
          builder->getIntegerAttr(intTy(), intLiteral->integerValue));
      recordedSizeAddr = builder->create<mlir::LLVM::AddressOfOp>(loc, globalOp);
    } else {
      visit(sizeExpr[i]);

      auto [type, valueAddr] = popElementFromStack(sizeExpr[i]);

      // Could be * or an integer value
      mlir::Value recordedSizeAddr = valueAddr;
      if (type->getName() == "character") {
        int inferredSize = 0;
        if (i < arrayTypSym->inferredElementSize.size()) {
          inferredSize = arrayTypSym->inferredElementSize[i];
        }
        auto ifrValue = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), inferredSize);
        recordedSizeAddr = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), intTy(), constOne());
        builder->create<mlir::LLVM::StoreOp>(loc, ifrValue, recordedSizeAddr);
      }
    }

    arrayTypSym->declaredElementSize.push_back(recordedSizeAddr);
  }

  return {};
}
} // namespace gazprea::backend