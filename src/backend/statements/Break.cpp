#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitBreak(std::shared_ptr<ast::statements::BreakAst> ctx) {
  if (loopStack.empty()) {
    return {};
  }

  auto exitBlock = loopStack.back().exitBlock;
  builder->create<mlir::cf::BranchOp>(loc, exitBlock);

  return {};
}

} // namespace gazprea::backend
