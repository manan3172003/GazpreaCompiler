#include "backend/Backend.h"
#include "symTable/MethodSymbol.h"
#include "symTable/VariableSymbol.h"

namespace gazprea::backend {

std::any Backend::visitProcedureCall(std::shared_ptr<ast::statements::ProcedureCallAst> ctx) {
  const auto methodSym = std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getSymbol());

  std::vector<mlir::Value> args;
  for (const auto &arg : ctx->getArgs()) {
    visit(arg);
    auto [type, value] = arg->getScope()->getTopElementInStack();
    args.push_back(value);
  }

  auto procOp = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(methodSym->getName());
  builder->create<mlir::LLVM::CallOp>(loc, procOp, mlir::ValueRange(args));

  return {};
}

} // namespace gazprea::backend
