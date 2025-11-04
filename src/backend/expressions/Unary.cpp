#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitUnary(std::shared_ptr<ast::expressions::UnaryAst> ctx) {
  visit(ctx->getExpression());
  auto [type, valueAddr] = ctx->getExpression()->getScope()->getTopElementInStack();
  auto newAddr = builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(type), constOne());
  auto value = builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(type), valueAddr);
  switch (ctx->getUnaryOpType()) {
  case ast::expressions::UnaryOpType::MINUS: {
    if (type->getName() == "real") {
      auto negValue = builder->create<mlir::LLVM::FNegOp>(loc, value);
      builder->create<mlir::LLVM::StoreOp>(loc, negValue, newAddr);
      ctx->getScope()->pushElementToScopeStack(type, newAddr);
    } else {
      auto negConst = builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), -1);
      auto result = builder->create<mlir::LLVM::MulOp>(loc, value, negConst);
      builder->create<mlir::LLVM::StoreOp>(loc, result, newAddr);
      ctx->getScope()->pushElementToScopeStack(type, newAddr);
    }
    break;
  }
  case ast::expressions::UnaryOpType::NOT: {
    auto notValue = builder->create<mlir::LLVM::XOrOp>(
        loc, value, builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 1));
    builder->create<mlir::LLVM::StoreOp>(loc, notValue, newAddr);
    ctx->getScope()->pushElementToScopeStack(type, newAddr);
    break;
  }
  case ast::expressions::UnaryOpType::PLUS:
    ctx->getScope()->pushElementToScopeStack(type, valueAddr);
    break;
  }

  return {};
}

} // namespace gazprea::backend
