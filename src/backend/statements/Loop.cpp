#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitLoop(std::shared_ptr<ast::statements::LoopAst> ctx) {
  // Break flag: false = continue, true = break out of loop
  auto falseVal = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), false);

  if (ctx->getIsPostPredicated()) {
    // For do-while, we always execute the body at least once,
    // then check the condition at the end
    builder->create<mlir::scf::WhileOp>(
        loc, mlir::TypeRange{boolTy()}, // Loop-carried: breakFlag only
        mlir::ValueRange{falseVal},     // Initial: false (no break)
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::ValueRange args) {
          // CONDITION REGION
          // args[0] is breakFlag
          // For do-while: always continue unless breakFlag is set
          // The actual condition is checked in the body
          auto breakFlag = args[0];
          auto notBreak =
              b.create<mlir::LLVM::ICmpOp>(l, mlir::LLVM::ICmpPredicate::eq, breakFlag,
                                           b.create<mlir::LLVM::ConstantOp>(l, boolTy(), false));
          b.create<mlir::scf::ConditionOp>(l, notBreak, args);
        },
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::ValueRange args) {
          // BODY REGION
          loopStack.push_back({args[0], true});
          visit(ctx->getBody());
          loopStack.pop_back();

          // After body, evaluate condition and set breakFlag
          if (b.getInsertionBlock()->empty() ||
              !mlir::isa<mlir::scf::YieldOp>(b.getInsertionBlock()->back())) {
            visit(ctx->getCondition());
            auto [_, condAddr] = ctx->getCondition()->getScope()->getTopElementInStack();
            auto condLoad = b.create<mlir::LLVM::LoadOp>(l, boolTy(), condAddr);
            auto condValue = condLoad.getResult();

            // If condition is false, set breakFlag to true to exit
            auto condIsFalse =
                b.create<mlir::LLVM::ICmpOp>(l, mlir::LLVM::ICmpPredicate::eq, condValue,
                                             b.create<mlir::LLVM::ConstantOp>(l, boolTy(), false));

            auto newBreakFlag = b.create<mlir::LLVM::SelectOp>(
                l, condIsFalse, b.create<mlir::LLVM::ConstantOp>(l, boolTy(), true),
                args[0]); // Keep existing break flag if true

            b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{newBreakFlag});
          }
        });
  } else if (ctx->getIsInfinite()) {
    builder->create<mlir::scf::WhileOp>(
        loc, mlir::TypeRange{boolTy()}, mlir::ValueRange{falseVal},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::ValueRange args) {
          // CONDITION REGION
          auto breakFlag = args[0];
          auto notBreak =
              b.create<mlir::LLVM::ICmpOp>(l, mlir::LLVM::ICmpPredicate::eq, breakFlag,
                                           b.create<mlir::LLVM::ConstantOp>(l, boolTy(), false));
          b.create<mlir::scf::ConditionOp>(l, notBreak, args);
        },
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::ValueRange args) {
          // BODY REGION
          loopStack.push_back({args[0], true});
          visit(ctx->getBody());
          loopStack.pop_back();

          if (b.getInsertionBlock()->empty() ||
              !mlir::isa<mlir::scf::YieldOp>(b.getInsertionBlock()->back())) {
            b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{args[0]});
          }
        });
  } else {
    builder->create<mlir::scf::WhileOp>(
        loc, mlir::TypeRange{boolTy()}, // Loop-carried: breakFlag
        mlir::ValueRange{falseVal},     // Initial: false (no break)
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::ValueRange args) {
          // CONDITION REGION
          auto breakFlag = args[0];

          visit(ctx->getCondition());
          auto [_, condAddr] = ctx->getCondition()->getScope()->getTopElementInStack();
          auto condLoad = b.create<mlir::LLVM::LoadOp>(l, boolTy(), condAddr);
          auto loopCond = condLoad.getResult();

          // Continue if (loopCond && !breakFlag)
          auto notBreak =
              b.create<mlir::LLVM::ICmpOp>(l, mlir::LLVM::ICmpPredicate::eq, breakFlag,
                                           b.create<mlir::LLVM::ConstantOp>(l, boolTy(), false));
          auto combinedCond = b.create<mlir::LLVM::AndOp>(l, loopCond, notBreak);

          b.create<mlir::scf::ConditionOp>(l, combinedCond, args);
        },
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::ValueRange args) {
          // BODY REGION
          loopStack.push_back({args[0], false});
          visit(ctx->getBody());
          loopStack.pop_back();

          // Only create yield if not already created by break statement
          if (b.getInsertionBlock()->empty() ||
              !mlir::isa<mlir::scf::YieldOp>(b.getInsertionBlock()->back())) {
            // Keep breakFlag as is (false means continue, true means break)
            b.create<mlir::scf::YieldOp>(l, mlir::ValueRange{args[0]});
          }
        });
  }
  return {};
}

} // namespace gazprea::backend
