#include "backend/Backend.h"
#include "symTable/MethodSymbol.h"
#include "symTable/VariableSymbol.h"

namespace gazprea::backend {

std::any Backend::visitProcedureCall(std::shared_ptr<ast::statements::ProcedureCallAst> ctx) {
  const auto methodSym = std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getSymbol());
  const auto procedureDeclaration =
      std::dynamic_pointer_cast<ast::prototypes::ProcedureAst>(methodSym->getDef());
  const auto prototype = procedureDeclaration->getProto();

  const auto params = prototype->getParams();
  const auto args = ctx->getArgs();
  std::vector<mlir::Value> mlirArgs;

  for (size_t i = 0; i < ctx->getArgs().size(); ++i) {
    visit(args[i]);
    const auto topElement = args[i]->getScope()->getTopElementInStack();
    params[i]->getSymbol()->value = topElement.second;
    auto value = topElement.second;
    mlirArgs.push_back(value);
  }

  auto procOp = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(methodSym->getName());
  builder->create<mlir::LLVM::CallOp>(loc, procOp, mlir::ValueRange(mlirArgs));

  return {};
}

} // namespace gazprea::backend
