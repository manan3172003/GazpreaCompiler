#include "backend/Backend.h"
#include "symTable/MethodSymbol.h"

namespace gazprea::backend {

std::any Backend::visitFuncProcCall(std::shared_ptr<ast::expressions::FuncProcCallAst> ctx) {
  const auto methodSym = std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getSymbol());

  std::shared_ptr<ast::prototypes::PrototypeAst> protoType;
  if (methodSym->getScopeType() == symTable::ScopeType::Procedure) {
    const auto procedureDecl =
        std::dynamic_pointer_cast<ast::prototypes::ProcedureAst>(methodSym->getDef());
    protoType = procedureDecl->getProto();
  } else if (methodSym->getScopeType() == symTable::ScopeType::Function) {
    const auto functionDecl =
        std::dynamic_pointer_cast<ast::prototypes::FunctionAst>(methodSym->getDef());
    protoType = functionDecl->getProto();
  }
  const auto params = protoType->getParams();
  const auto args = ctx->getArgs();
  std::vector<mlir::Value> mlirArgs;
  for (size_t i = 0; i < ctx->getArgs().size(); ++i) {
    visit(args[i]);
    const auto [_, valueAddr] = popElementFromStack(args[i]);
    params[i]->getSymbol()->value = valueAddr;
    mlirArgs.push_back(valueAddr);
  }
  auto funcOp = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(methodSym->getName());
  auto callOp = builder->create<mlir::LLVM::CallOp>(loc, funcOp, mlir::ValueRange(mlirArgs));

  // Capture the return value and push to scope stack
  if (callOp.getNumResults() > 0) {
    const auto returnValue = callOp.getResult();
    auto returnAlloca =
        builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), returnValue.getType(), constOne());
    builder->create<mlir::LLVM::StoreOp>(loc, returnValue, returnAlloca.getResult());

    ctx->getSymbol()->value = returnAlloca.getResult();
    ctx->getScope()->pushElementToScopeStack(methodSym->getReturnType(), returnAlloca.getResult());
  }

  return {};
}

} // namespace gazprea::backend
