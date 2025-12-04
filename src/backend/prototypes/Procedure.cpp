#include "ast/types/ArrayTypeAst.h"
#include "backend/Backend.h"
#include "symTable/ArrayTypeSymbol.h"
#include "symTable/MethodSymbol.h"

namespace gazprea::backend {

std::any Backend::visitProcedure(std::shared_ptr<ast::prototypes::ProcedureAst> ctx) {
  const auto methodSym =
      std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getProto()->getSymbol());

  const auto savedInsertPoint = builder->saveInsertionPoint();
  const bool isForwardDecl = !ctx->getBody();

  if (methodSym && methodSym->getName() == "main") {

    // Create Builtins before main
    makeLengthBuiltin();
    makeShapeBuiltin();

    auto mainType = mlir::LLVM::LLVMFunctionType::get(intTy(), {}, false);
    auto mainFunc = builder->create<mlir::LLVM::LLVMFuncOp>(loc, "main", mainType);
    mlir::Block *entry = mainFunc.addEntryBlock();
    builder->setInsertionPointToStart(entry);

    blockArg.clear();
    size_t argIndex = 0;
    for (const auto &param : ctx->getProto()->getParams()) {
      const auto paramNode = std::dynamic_pointer_cast<ast::prototypes::ProcedureParamAst>(param);
      blockArg[paramNode->getName()] = entry->getArgument(argIndex++);
    }

    // Store current procedure prototype for access in return statements
    currentFunctionProto = ctx->getProto();

    visit(ctx->getProto());
    visit(ctx->getBody());

    // Clear after procedure body
    currentFunctionProto = nullptr;
  } else {
    mlir::Type procReturnType;
    if (!methodSym->getReturnType()) {
      // Void procedure: no return type
      procReturnType = mlir::LLVM::LLVMVoidType::get(&context);
    } else {
      procReturnType = getMLIRType(methodSym->getReturnType());
    }
    auto procType = mlir::LLVM::LLVMFunctionType::get(
        procReturnType, getMethodParamTypes(ctx->getProto()->getParams()), false);

    auto procOp = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(methodSym->getName());

    if (!procOp) {
      // Function doesn't exist yet, create it
      procOp = builder->create<mlir::LLVM::LLVMFuncOp>(loc, methodSym->getName(), procType);
    }

    if (!isForwardDecl) {
      mlir::Block *entry = procOp.addEntryBlock();
      builder->setInsertionPointToStart(entry);

      blockArg.clear();
      size_t argIndex = 0;
      for (const auto &param : ctx->getProto()->getParams()) {
        const auto paramNode = std::dynamic_pointer_cast<ast::prototypes::ProcedureParamAst>(param);
        blockArg[paramNode->getName()] = entry->getArgument(argIndex++);
      }
      // Store current procedure prototype for access in return statements
      currentFunctionProto = ctx->getProto();

      visit(ctx->getProto());
      visit(ctx->getBody());

      // Clear after procedure body
      currentFunctionProto = nullptr;

      if (!methodSym->getReturnType()) {
        auto *currentBlock = builder->getInsertionBlock();
        const bool needsReturn =
            currentBlock && (currentBlock->empty() ||
                             !currentBlock->back().hasTrait<mlir::OpTrait::IsTerminator>());
        if (needsReturn) {
          // Void procedure: ensure a return at the end
          builder->create<mlir::LLVM::ReturnOp>(builder->getUnknownLoc(), mlir::ValueRange{});
        }
      }
    }
  }
  builder->restoreInsertionPoint(savedInsertPoint);
  return {};
}

} // namespace gazprea::backend
