#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitBreak(std::shared_ptr<ast::statements::BreakAst> ctx) {
  if (loopStack.empty()) {
    return {};
  }

  // Jump directly to the loop exit block
  auto exitBlock = loopStack.back().exitBlock;
  builder->create<mlir::cf::BranchOp>(loc, exitBlock);

  return {};
}

} // namespace gazprea::backend
