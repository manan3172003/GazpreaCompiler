#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitContinue(std::shared_ptr<ast::statements::ContinueAst> ctx) {
  if (loopStack.empty()) {
    return {};
  }

  // For SCF loops, continue is implemented by yielding breakFlag = false
  // This causes the loop to:
  // - In while loop: re-evaluate condition and continue if true
  // - In do-while loop: evaluate the actual condition after body and continue if true
  auto falseVal = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), false);
  builder->create<mlir::scf::YieldOp>(loc, mlir::ValueRange({falseVal}));

  return {};
}

} // namespace gazprea::backend
