#include "backend/Backend.h"
#include "symTable/ArrayTypeSymbol.h"

namespace gazprea::backend {

std::any Backend::visitGenerator(std::shared_ptr<ast::expressions::GeneratorAst> ctx) {

  auto generatorType = ctx->getInferredSymbolType();
  auto dimensions = ctx->getDimensionCount();

  if (dimensions == 1) {
    auto domainExpr = ctx->getDomainExprs()[0];

    visit(domainExpr);
    auto [domainType, domainArrayAddr] = popElementFromStack(domainExpr->getDomainExpression());

    auto domainArrayType = getMLIRType(domainType);
    auto domainSizeAddr = getArraySizeAddr(*builder, loc, domainArrayType, domainArrayAddr);
    auto domainSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), domainSizeAddr);
    auto domainDataPtrAddr = getArrayDataAddr(*builder, loc, domainArrayType, domainArrayAddr);
    auto domainDataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), domainDataPtrAddr);

    auto arrayTypeSymbol = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(generatorType);
    auto elementType = arrayTypeSymbol->getType();
    auto elementMLIRType = getMLIRType(elementType);

    auto resultStructType = getMLIRType(generatorType);
    auto resultArrayAddr =
        builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), resultStructType, constOne(), 0);

    auto resultSizeAddr = getArraySizeAddr(*builder, loc, resultStructType, resultArrayAddr);
    builder->create<mlir::LLVM::StoreOp>(loc, domainSize, resultSizeAddr);
    auto resultDataPtr = mallocArray(elementMLIRType, domainSize);
    auto resultDataPtrAddr = getArrayDataAddr(*builder, loc, resultStructType, resultArrayAddr);
    builder->create<mlir::LLVM::StoreOp>(loc, resultDataPtr, resultDataPtrAddr);

    auto resultIs2DAddr = get2DArrayBoolAddr(*builder, loc, resultStructType, resultArrayAddr);
    builder->create<mlir::LLVM::StoreOp>(loc, constFalse(), resultIs2DAddr);
    std::string iteratorName = domainExpr->getIteratorName();
    builder->create<mlir::scf::ForOp>(
        loc, constZero(), domainSize, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value loopIdx, mlir::ValueRange iterArgs) {
          auto domainElementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), intTy(), domainDataPtr,
                                                              mlir::ValueRange{loopIdx});
          auto domainElementValue = b.create<mlir::LLVM::LoadOp>(l, intTy(), domainElementPtr);
          auto iteratorAddr = b.create<mlir::LLVM::AllocaOp>(l, ptrTy(), intTy(), constOne(), 0);
          b.create<mlir::LLVM::StoreOp>(l, domainElementValue, iteratorAddr);
          blockArg[iteratorName] = iteratorAddr;
          visit(ctx->getGeneratorExpression());
          auto [exprType, exprAddr] = popElementFromStack(ctx->getGeneratorExpression());
          auto resultElementPtr = b.create<mlir::LLVM::GEPOp>(
              l, ptrTy(), elementMLIRType, resultDataPtr, mlir::ValueRange{loopIdx});
          if (elementType->getName() == "array") {
            auto exprValue = b.create<mlir::LLVM::LoadOp>(l, elementMLIRType, exprAddr);
            b.create<mlir::LLVM::StoreOp>(l, exprValue, resultElementPtr);
          } else {
            auto exprValue = b.create<mlir::LLVM::LoadOp>(l, elementMLIRType, exprAddr);
            b.create<mlir::LLVM::StoreOp>(l, exprValue, resultElementPtr);
          }

          blockArg.erase(iteratorName);
          b.create<mlir::scf::YieldOp>(l);
        });

    pushElementToScopeStack(ctx, generatorType, resultArrayAddr);

  } else if (dimensions == 2) {
    auto domainExpr1 = ctx->getDomainExprs()[0];
    auto domainExpr2 = ctx->getDomainExprs()[1];
    visit(domainExpr1);
    auto [domain1Type, domain1ArrayAddr] = popElementFromStack(domainExpr1->getDomainExpression());

    auto domain1ArrayType = getMLIRType(domain1Type);
    auto domain1SizeAddr = getArraySizeAddr(*builder, loc, domain1ArrayType, domain1ArrayAddr);
    auto domain1Size = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), domain1SizeAddr);
    auto domain1DataPtrAddr = getArrayDataAddr(*builder, loc, domain1ArrayType, domain1ArrayAddr);
    auto domain1DataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), domain1DataPtrAddr);

    visit(domainExpr2);
    auto [domain2Type, domain2ArrayAddr] = popElementFromStack(domainExpr2->getDomainExpression());

    auto domain2ArrayType = getMLIRType(domain2Type);
    auto domain2SizeAddr = getArraySizeAddr(*builder, loc, domain2ArrayType, domain2ArrayAddr);
    auto domain2Size = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), domain2SizeAddr);
    auto domain2DataPtrAddr = getArrayDataAddr(*builder, loc, domain2ArrayType, domain2ArrayAddr);
    auto domain2DataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), domain2DataPtrAddr);
    auto arrayTypeSymbol = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(generatorType);
    auto innerArrayType = arrayTypeSymbol->getType(); // array<element_type>
    auto innerArrayMLIRType = getMLIRType(innerArrayType);
    auto innerArrayTypeSymbol =
        std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(innerArrayType);
    auto elementType = innerArrayTypeSymbol->getType();
    auto elementMLIRType = getMLIRType(elementType);
    auto outerStructType = getMLIRType(generatorType);
    auto resultArrayAddr =
        builder->create<mlir::LLVM::AllocaOp>(loc, ptrTy(), outerStructType, constOne(), 0);
    auto outerSizeAddr = getArraySizeAddr(*builder, loc, outerStructType, resultArrayAddr);
    builder->create<mlir::LLVM::StoreOp>(loc, domain1Size, outerSizeAddr);
    auto outerDataPtr = mallocArray(innerArrayMLIRType, domain1Size);
    auto outerDataPtrAddr = getArrayDataAddr(*builder, loc, outerStructType, resultArrayAddr);
    builder->create<mlir::LLVM::StoreOp>(loc, outerDataPtr, outerDataPtrAddr);

    auto outerIs2DAddr = get2DArrayBoolAddr(*builder, loc, outerStructType, resultArrayAddr);
    builder->create<mlir::LLVM::StoreOp>(loc, constTrue(), outerIs2DAddr);

    std::string iterator1Name = domainExpr1->getIteratorName();
    std::string iterator2Name = domainExpr2->getIteratorName();

    builder->create<mlir::scf::ForOp>(
        loc, constZero(), domain1Size, constOne(), mlir::ValueRange{},
        [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value outerIdx, mlir::ValueRange iterArgs) {
          auto domain1ElementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), intTy(), domain1DataPtr,
                                                               mlir::ValueRange{outerIdx});
          auto domain1ElementValue = b.create<mlir::LLVM::LoadOp>(l, intTy(), domain1ElementPtr);
          auto iterator1Addr = b.create<mlir::LLVM::AllocaOp>(l, ptrTy(), intTy(), constOne(), 0);
          b.create<mlir::LLVM::StoreOp>(l, domain1ElementValue, iterator1Addr);
          blockArg[iterator1Name] = iterator1Addr;

          auto innerArrayStructPtr = b.create<mlir::LLVM::GEPOp>(
              l, ptrTy(), innerArrayMLIRType, outerDataPtr, mlir::ValueRange{outerIdx});

          auto innerSizeAddr = getArraySizeAddr(b, l, innerArrayMLIRType, innerArrayStructPtr);
          b.create<mlir::LLVM::StoreOp>(l, domain2Size, innerSizeAddr);

          auto innerDataPtr = mallocArray(elementMLIRType, domain2Size);
          auto innerDataPtrAddr = getArrayDataAddr(b, l, innerArrayMLIRType, innerArrayStructPtr);
          b.create<mlir::LLVM::StoreOp>(l, innerDataPtr, innerDataPtrAddr);

          auto innerIs2DAddr = get2DArrayBoolAddr(b, l, innerArrayMLIRType, innerArrayStructPtr);
          b.create<mlir::LLVM::StoreOp>(l, constFalse(), innerIs2DAddr);

          b.create<mlir::scf::ForOp>(
              l, constZero(), domain2Size, constOne(), mlir::ValueRange{},
              [&](mlir::OpBuilder &b2, mlir::Location l2, mlir::Value innerIdx,
                  mlir::ValueRange iterArgs2) {
                auto domain2ElementPtr = b2.create<mlir::LLVM::GEPOp>(
                    l2, ptrTy(), intTy(), domain2DataPtr, mlir::ValueRange{innerIdx});
                auto domain2ElementValue =
                    b2.create<mlir::LLVM::LoadOp>(l2, intTy(), domain2ElementPtr);

                auto iterator2Addr =
                    b2.create<mlir::LLVM::AllocaOp>(l2, ptrTy(), intTy(), constOne(), 0);
                b2.create<mlir::LLVM::StoreOp>(l2, domain2ElementValue, iterator2Addr);
                blockArg[iterator2Name] = iterator2Addr;
                visit(ctx->getGeneratorExpression());
                auto [exprType, exprAddr] = popElementFromStack(ctx->getGeneratorExpression());

                auto resultElementPtr = b2.create<mlir::LLVM::GEPOp>(
                    l2, ptrTy(), elementMLIRType, innerDataPtr, mlir::ValueRange{innerIdx});

                auto exprValue = b2.create<mlir::LLVM::LoadOp>(l2, elementMLIRType, exprAddr);
                b2.create<mlir::LLVM::StoreOp>(l2, exprValue, resultElementPtr);
                blockArg.erase(iterator2Name);

                b2.create<mlir::scf::YieldOp>(l2);
              });

          blockArg.erase(iterator1Name);
          b.create<mlir::scf::YieldOp>(l);
        });

    pushElementToScopeStack(ctx, generatorType, resultArrayAddr);
  }

  return {};
}

} // namespace gazprea::backend
