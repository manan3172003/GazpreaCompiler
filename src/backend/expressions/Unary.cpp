#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitUnary(std::shared_ptr<ast::expressions::UnaryAst> ctx) {
  visit(ctx->getExpression());
  auto [type, valueAddr] = popElementFromStack(ctx->getExpression());
  auto op = ctx->getUnaryOpType();

  if (isTypeArray(type)) {
    auto resultAddr =
        builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(type), constOne());
    copyArrayStruct(type, valueAddr, resultAddr);
    if (op != ast::expressions::UnaryOpType::PLUS) {
      applyUnaryToArray(op, resultAddr, type);
    }
    pushElementToScopeStack(ctx, type, resultAddr);
    return {};
  }

  if (isTypeVector(type)) {
    auto resultAddr =
        builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(type), constOne());
    copyVectorStruct(type, valueAddr, resultAddr);
    if (op != ast::expressions::UnaryOpType::PLUS) {
      applyUnaryToVector(op, resultAddr, type);
    }
    pushElementToScopeStack(ctx, type, resultAddr);
    return {};
  }

  switch (op) {
  case ast::expressions::UnaryOpType::MINUS:
  case ast::expressions::UnaryOpType::NOT: {
    auto newAddr =
        builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(type), constOne());
    auto value = builder->create<mlir::LLVM::LoadOp>(loc, getMLIRType(type), valueAddr);
    auto result = applyUnaryToScalar(op, type, *builder, loc, value);
    builder->create<mlir::LLVM::StoreOp>(loc, result, newAddr);
    pushElementToScopeStack(ctx, type, newAddr);
    break;
  }
  case ast::expressions::UnaryOpType::PLUS:
    pushElementToScopeStack(ctx, type, valueAddr);
    break;
  }

  return {};
}

} // namespace gazprea::backend
