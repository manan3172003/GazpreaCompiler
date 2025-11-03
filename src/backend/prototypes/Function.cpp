#include "backend/Backend.h"
#include "symTable/MethodSymbol.h"

namespace gazprea::backend {

std::any Backend::visitFunction(std::shared_ptr<ast::prototypes::FunctionAst> ctx) {
  const auto methodSym =
      std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getProto()->getSymbol());
  const auto savedInsertPoint = builder->saveInsertionPoint();
  const bool isForwardDecl = !ctx->getBody();

  const auto funcReturnType = getMLIRType(methodSym->getReturnType());
  auto funcType = mlir::LLVM::LLVMFunctionType::get(
      funcReturnType, getMethodParamTypes(ctx->getProto()->getParams()), false);

  auto funcOp = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(methodSym->getName());
  if (!funcOp) {
    funcOp = builder->create<mlir::LLVM::LLVMFuncOp>(loc, methodSym->getName(), funcType);
  }

  if (!isForwardDecl) {
    mlir::Block *entry = funcOp.addEntryBlock();
    builder->setInsertionPointToStart(entry);

    blockArg.clear();
    size_t argIndex = 0;
    for (const auto &param : ctx->getProto()->getParams()) {
      const auto paramNode = std::dynamic_pointer_cast<ast::prototypes::FunctionParamAst>(param);
      blockArg[paramNode->getName()] = entry->getArgument(argIndex++);
    }
    visit(ctx->getProto());
    visit(ctx->getBody());
  }
  builder->restoreInsertionPoint(savedInsertPoint);
  return {};
}

} // namespace gazprea::backend
