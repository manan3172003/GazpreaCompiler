#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitBreak(std::shared_ptr<ast::statements::BreakAst> ctx) {
  if (loopStack.empty()) {
    return {};
  }

  // For SCF loops, break is implemented by setting breakFlag = true
  // This causes the loop to exit because:
  // - In while loop: condition becomes false (loopCond && !breakFlag)
  // - In do-while loop: condition becomes false (!breakFlag)
  auto trueVal = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), true);
  builder->create<mlir::scf::YieldOp>(loc, mlir::ValueRange({trueVal}));

  return {};
}

} // namespace gazprea::backend
