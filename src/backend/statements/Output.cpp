#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitOutput(std::shared_ptr<ast::statements::OutputAst> ctx) {
  visit(ctx->getExpression());

  auto topElement = ctx->getScope()->getTopElementInStack();
  auto type = topElement.first;
  auto valueAddr = topElement.second;
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
  }
  return {};
}

} // namespace gazprea::backend
