#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitReturn(std::shared_ptr<ast::statements::ReturnAst> ctx) {
  if (ctx->getExpr()) {
    visit(ctx->getExpr());

    auto [type, value] = ctx->getScope()->getTopElementInStack();
    ctx->getScope()->popElementFromScopeStack();

    if (ctx->getExpr()->getNodeType() == ast::NodeType::IntegerLiteral) {
      auto loadOp = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), value);
      builder->create<mlir::LLVM::ReturnOp>(builder->getUnknownLoc(), loadOp.getResult());
      return {};
    }
    // TODO: Handle other return types
  } else {
    // (void return)
    builder->create<mlir::LLVM::ReturnOp>(builder->getUnknownLoc(), constZero());
  }

  return {};
}
} // namespace gazprea::backend
