#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitOutput(std::shared_ptr<ast::statements::OutputAst> ctx) {
  visit(ctx->getExpression());

  auto [type, valueAddr] = ctx->getScope()->getTopElementInStack();
  auto a = type;
  mlir::Value value;
  if (type->getName() == "integer") {
    value = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), valueAddr);
    printInt(value);
  } else if (type->getName() == "real") {
    value = builder->create<mlir::LLVM::LoadOp>(loc, floatTy(), valueAddr);
    printFloat(value);
  } else if (type->getName() == "character") {
    value = builder->create<mlir::LLVM::LoadOp>(loc, charTy(), valueAddr);
    printIntChar(value);
  } else if (type->getName() == "boolean") {
    value = builder->create<mlir::LLVM::LoadOp>(loc, boolTy(), valueAddr);
    printBool(value);
  }
  return {};
}

} // namespace gazprea::backend
