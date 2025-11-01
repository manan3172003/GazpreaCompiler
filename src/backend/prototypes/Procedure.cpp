#include "backend/Backend.h"
#include "symTable/MethodSymbol.h"

namespace gazprea::backend {

std::any Backend::visitProcedure(std::shared_ptr<ast::prototypes::ProcedureAst> ctx) {
  const auto methodSym =
      std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getProto()->getSymbol());

  if (methodSym && methodSym->getName() == "main") {
    auto mainType = mlir::LLVM::LLVMFunctionType::get(intTy(), {}, false);
    mlir::LLVM::LLVMFuncOp mainFunc =
        builder->create<mlir::LLVM::LLVMFuncOp>(loc, "main", mainType);
    mlir::Block *entry = mainFunc.addEntryBlock();
    builder->setInsertionPointToStart(entry);

    // Get the integer format string we already created.
    mlir::LLVM::GlobalOp formatString;
    if (!(formatString = module.lookupSymbol<mlir::LLVM::GlobalOp>("intFormat"))) {
      llvm::errs() << "missing format string!\n";
      return 1;
    }

    // Get the format string and print 415
    // mlir::Value formatStringPtr = builder->create<mlir::LLVM::AddressOfOp>(loc, formatString);
    // mlir::Value intToPrint = builder->create<mlir::LLVM::ConstantOp>(loc, intTy()(), 415);
    // mlir::ValueRange args = {formatStringPtr, intToPrint};
    // mlir::LLVM::LLVMFuncOp printfFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("printf");
    // builder->create<mlir::LLVM::CallOp>(loc, printfFunc, args);

    visit(ctx->getBody());

    // Return 0
    mlir::Value zero =
        builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), builder->getIntegerAttr(intTy(), 0));
    builder->create<mlir::LLVM::ReturnOp>(builder->getUnknownLoc(), zero);

  } else {
    // Create a new Procedure
  }
  return {};
}
} // namespace gazprea::backend
