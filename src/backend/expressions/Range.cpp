#include "CompileTimeExceptions.h"
#include "backend/Backend.h"

namespace gazprea::backend {

std::any Backend::visitRange(std::shared_ptr<ast::expressions::RangeAst> ctx) {
  visit(ctx->getStart());
  auto [startType, startAddr] = popElementFromStack(ctx->getStart());
  if (startType->getName() != "integer") {
    throw TypeError(ctx->getLineNumber(),
                    "Range start expression must be of type integer, got " + startType->getName());
  }

  auto startValue = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), startAddr);

  visit(ctx->getEnd());
  auto [endType, endAddr] = popElementFromStack(ctx->getEnd());
  if (endType->getName() != "integer") {
    throw TypeError(ctx->getLineNumber(),
                    "Range end expression must be of type integer, got " + endType->getName());
  }

  auto endValue = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), endAddr);

  auto diff = builder->create<mlir::arith::SubIOp>(loc, endValue, startValue);
  auto arraySize = builder->create<mlir::arith::AddIOp>(loc, diff, constOne());

  auto arrayStructType = structTy({intTy(), ptrTy(), boolTy()});
  auto arrayStructAddr =
      builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), arrayStructType, constOne(), 0);

  auto sizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, arrayStructAddr);
  builder->create<mlir::LLVM::StoreOp>(loc, arraySize, sizeAddr);

  auto dataPtr = mallocArray(intTy(), arraySize);

  auto dataPtrAddr = getArrayDataAddr(*builder, loc, arrayStructType, arrayStructAddr);
  builder->create<mlir::LLVM::StoreOp>(loc, dataPtr, dataPtrAddr);

  auto is2DAddr = get2DArrayBoolAddr(*builder, loc, arrayStructType, arrayStructAddr);
  builder->create<mlir::LLVM::StoreOp>(loc, constFalse(), is2DAddr);

  builder->create<mlir::scf::ForOp>(
      loc, constZero(), arraySize, constOne(), mlir::ValueRange{},
      [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
        auto value = b.create<mlir::arith::AddIOp>(l, startValue, i);

        auto elementPtr =
            b.create<mlir::LLVM::GEPOp>(l, ptrTy(), intTy(), dataPtr, mlir::ValueRange{i});

        b.create<mlir::LLVM::StoreOp>(l, value, elementPtr);

        // Yield
        b.create<mlir::scf::YieldOp>(l);
      });

  pushElementToScopeStack(ctx, ctx->getInferredSymbolType(), arrayStructAddr);

  return {};
}

} // namespace gazprea::backend
