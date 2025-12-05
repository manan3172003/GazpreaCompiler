#include "backend/Backend.h"
#include "symTable/ArrayTypeSymbol.h"
#include "symTable/EmptyArrayTypeSymbol.h"
namespace gazprea::backend {

std::any Backend::visitArray(std::shared_ptr<ast::expressions::ArrayLiteralAst> ctx) {
  if (auto emptyArrayType =
          std::dynamic_pointer_cast<symTable::EmptyArrayTypeSymbol>(ctx->getInferredSymbolType())) {
    auto arrayStructType = getMLIRType(emptyArrayType);
    auto arrayStruct =
        builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), arrayStructType, constOne());
    auto sizeFieldPtr = getArraySizeAddr(*builder, loc, arrayStructType, arrayStruct);
    builder->create<mlir::LLVM::StoreOp>(loc, constZero(), sizeFieldPtr);
    mlir::Value dataPtr = mallocArray(intTy(), constZero());
    auto dataFieldPtr = getArrayDataAddr(*builder, loc, arrayStructType, arrayStruct);
    builder->create<mlir::LLVM::StoreOp>(loc, dataPtr, dataFieldPtr);

    auto is2dValue = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), 0);
    auto is2dFieldPtr = get2DArrayBoolAddr(*builder, loc, arrayStructType, arrayStruct);
    builder->create<mlir::LLVM::StoreOp>(loc, is2dValue, is2dFieldPtr);
    ctx->getScope()->pushElementToScopeStack(emptyArrayType, arrayStruct);
    return {};
  }
  auto arrayType =
      std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(ctx->getInferredSymbolType());
  bool is2dArray = false;
  std::shared_ptr<symTable::Type> elementType;
  if (auto _elementType =
          std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(arrayType->getType())) {
    elementType = _elementType;
    is2dArray = true;
  }

  if (ctx->getElements().empty() && !arrayType->getType()) {
    // Empty array with unknown element type
    auto arrayStructType = getMLIRType(arrayType);
    auto arrayStruct =
        builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), arrayStructType, constOne());

    auto sizeFieldPtr = getArraySizeAddr(*builder, loc, arrayStructType, arrayStruct);
    builder->create<mlir::LLVM::StoreOp>(loc, constZero(), sizeFieldPtr);

    auto dataFieldPtr = getArrayDataAddr(*builder, loc, arrayStructType, arrayStruct);
    auto nullPtr = builder->create<mlir::LLVM::ZeroOp>(loc, ptrTy());
    builder->create<mlir::LLVM::StoreOp>(loc, nullPtr, dataFieldPtr);

    auto is2dValue = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), is2dArray ? 1 : 0);
    auto is2dFieldPtr = get2DArrayBoolAddr(*builder, loc, arrayStructType, arrayStruct);
    builder->create<mlir::LLVM::StoreOp>(loc, is2dValue, is2dFieldPtr);

    pushElementToScopeStack(ctx, arrayType, arrayStruct);
    return {};
  }

  elementType = arrayType->getType();
  auto elementMLIRType = getMLIRType(elementType);
  auto arrayStructType = getMLIRType(arrayType);
  auto arrayStruct =
      builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), arrayStructType, constOne());

  const auto &elements = ctx->getElements();
  mlir::Value arraySize =
      builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), static_cast<int>(elements.size()));
  mlir::Value elementCount = arraySize;

  mlir::Value dataPtr = mallocArray(elementMLIRType, elementCount);

  createArrayFromVector(elements, elementMLIRType, dataPtr, elementType);

  auto sizeFieldPtr = getArraySizeAddr(*builder, loc, arrayStructType, arrayStruct);
  builder->create<mlir::LLVM::StoreOp>(loc, arraySize, sizeFieldPtr);

  auto dataFieldPtr = getArrayDataAddr(*builder, loc, arrayStructType, arrayStruct);
  builder->create<mlir::LLVM::StoreOp>(loc, dataPtr, dataFieldPtr);

  auto is2dValue = builder->create<mlir::LLVM::ConstantOp>(loc, boolTy(), is2dArray ? 1 : 0);
  auto is2dFieldPtr = get2DArrayBoolAddr(*builder, loc, arrayStructType, arrayStruct);
  builder->create<mlir::LLVM::StoreOp>(loc, is2dValue, is2dFieldPtr);

  padArrayIfNeeded(arrayStruct, arrayType,
                   builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), elements.size()),
                   maxSubArraySize(arrayStruct, arrayType));
  pushElementToScopeStack(ctx, arrayType, arrayStruct);

  return {};
}

} // namespace gazprea::backend