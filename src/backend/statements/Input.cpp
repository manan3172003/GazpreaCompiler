#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitInput(std::shared_ptr<ast::statements::InputAst> ctx) {
  visit(ctx->getLVal());
  auto [targetType, targetAddr] = popElementFromStack(ctx);
  if (!targetType || !targetAddr) {
    return {};
  }
  auto tempAlloca =
      builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(targetType), constOne());
  const auto typeName = targetType->getName();
  if (typeName == "integer") {
    readInteger(tempAlloca);
  } else if (typeName == "real") {
    readReal(tempAlloca);
  } else if (typeName == "character") {
    readCharacter(tempAlloca);
  } else if (typeName == "boolean") {
    readBoolean(tempAlloca);
  }
  copyValue(targetType, tempAlloca, targetAddr);
  return {};
}
} // namespace gazprea::backend
