#include "backend/Backend.h"
#include "symTable/MethodSymbol.h"
#include "symTable/VariableSymbol.h"

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
    auto [valueType, valueAddr] = popElementFromStack(args[i]);
    auto variableSymbol =
        std::dynamic_pointer_cast<symTable::VariableSymbol>(params[i]->getSymbol());

    if (variableSymbol->getQualifier() == ast::Qualifier::Const) {
      // Param is const
      auto argSymbol = args[i]->getSymbol();
      auto argVarSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(argSymbol);

      if (argVarSymbol && argVarSymbol->getQualifier() == ast::Qualifier::Const) {
        // arg is const: cast arg, copy arg into param, free arg
        params[i]->getSymbol()->value = builder->create<mlir::LLVM::AllocaOp>(
            loc, ptrTy(), getMLIRType(variableSymbol->getType()), constOne());
        valueAddr = castIfNeeded(params[i], valueAddr, valueType, variableSymbol->getType());
        copyValue(variableSymbol->getType(), valueAddr, params[i]->getSymbol()->value);
        mlirArgs.push_back(params[i]->getSymbol()->value);
        freeAllocatedMemory(variableSymbol->getType(), valueAddr);
      } else {
        // arg is var: copy var, cast the copy, copy the copy into param, free the copy
        auto argCopy =
            builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), getMLIRType(valueType), constOne());
        copyValue(valueType, valueAddr, argCopy);

        auto castedCopy = castIfNeeded(params[i], argCopy, valueType, variableSymbol->getType());

        params[i]->getSymbol()->value = builder->create<mlir::LLVM::AllocaOp>(
            loc, ptrTy(), getMLIRType(variableSymbol->getType()), constOne());
        copyValue(variableSymbol->getType(), castedCopy, params[i]->getSymbol()->value);
        mlirArgs.push_back(params[i]->getSymbol()->value);

        freeAllocatedMemory(variableSymbol->getType(), castedCopy);
        freeAllocatedMemory(valueType, argCopy);
      }
    } else {
      // param is var, arg is guaranteed to be var: simply put the address
      params[i]->getSymbol()->value = valueAddr;
      mlirArgs.push_back(valueAddr);
    }
  }
  auto funcOp = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(methodSym->getName());
  auto callOp = builder->create<mlir::LLVM::CallOp>(loc, funcOp, mlir::ValueRange(mlirArgs));

  for (size_t i = 0; i < ctx->getArgs().size(); ++i) {
    if (auto variableSymbol =
            std::dynamic_pointer_cast<symTable::VariableSymbol>(params[i]->getSymbol());
        variableSymbol->getQualifier() == ast::Qualifier::Const) {
      freeAllocatedMemory(variableSymbol->getType(), params[i]->getSymbol()->value);
    }
  }

  // Capture the return value and push to scope stack
  if (callOp.getNumResults() > 0) {
    const auto returnValue = callOp.getResult();
    auto returnAlloca =
        builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), returnValue.getType(), constOne());
    builder->create<mlir::LLVM::StoreOp>(loc, returnValue, returnAlloca.getResult());

    ctx->getSymbol()->value = returnAlloca.getResult();
    pushElementToScopeStack(ctx, methodSym->getReturnType(), returnAlloca.getResult());
  }

  return {};
}

} // namespace gazprea::backend
