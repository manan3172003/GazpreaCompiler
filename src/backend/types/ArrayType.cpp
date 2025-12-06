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
    } else {
      // Check that the size is >= 0
      mlir::Value sizeValue = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), valueAddr);
      mlir::Value zero = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), 0);
      auto isNegative =
          builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::slt, sizeValue, zero);

      builder->create<mlir::scf::IfOp>(
          loc, isNegative.getResult(), [&](mlir::OpBuilder &b, mlir::Location l) {
            auto throwFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(
                "throwArraySizeError_019addc8_cc3a_71c7_b15f_8745c510199c");
            b.create<mlir::LLVM::CallOp>(l, throwFunc, mlir::ValueRange{});
            b.create<mlir::scf::YieldOp>(l);
          });
      arrayTypSym->addSize(recordedSizeAddr);
    }

    arrayTypSym->declaredElementSize.push_back(recordedSizeAddr);
  }

  return {};
}
} // namespace gazprea::backend