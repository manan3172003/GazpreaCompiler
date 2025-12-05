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
    auto [valueType, valueAddr] = popElementFromStack(args[i]);
    auto variableSymbol =
        std::dynamic_pointer_cast<symTable::VariableSymbol>(params[i]->getSymbol());

    // castIfNeeded now handles scalar-to-array conversion and returns the final address
    if (variableSymbol->getQualifier() == ast::Qualifier::Const) {
      params[i]->getSymbol()->value = builder->create<mlir::LLVM::AllocaOp>(
          loc, ptrTy(), getMLIRType(variableSymbol->getType()), constOne());
      valueAddr = castIfNeeded(params[i], valueAddr, valueType, variableSymbol->getType());
      copyValue(variableSymbol->getType(), valueAddr, params[i]->getSymbol()->value);
      mlirArgs.push_back(params[i]->getSymbol()->value);
      freeAllocatedMemory(variableSymbol->getType(), valueAddr);
    } else {
      params[i]->getSymbol()->value = valueAddr;
      mlirArgs.push_back(valueAddr);
    }
  }

  auto procOp = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(methodSym->getName());
  builder->create<mlir::LLVM::CallOp>(loc, procOp, mlir::ValueRange(mlirArgs));

  for (size_t i = 0; i < ctx->getArgs().size(); ++i) {
    if (auto variableSymbol =
            std::dynamic_pointer_cast<symTable::VariableSymbol>(params[i]->getSymbol());
        variableSymbol->getQualifier() == ast::Qualifier::Const) {
      freeAllocatedMemory(variableSymbol->getType(), params[i]->getSymbol()->value);
    }
  }

  return {};
}

} // namespace gazprea::backend
