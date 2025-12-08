#include "ast/prototypes/ProcedureParamAst.h"
#include "ast/types/ArrayTypeAst.h"
#include "ast/types/StructTypeAst.h"
#include "ast/types/TupleTypeAst.h"
#include "ast/types/VectorTypeAst.h"
#include "backend/Backend.h"
#include "symTable/ArrayTypeSymbol.h"
#include "symTable/MethodSymbol.h"
#include "symTable/StructTypeSymbol.h"
#include "symTable/TupleTypeSymbol.h"
#include "symTable/VariableSymbol.h"
#include "symTable/VectorTypeSymbol.h"

namespace gazprea::backend {

std::any Backend::visitProcedureCall(std::shared_ptr<ast::statements::ProcedureCallAst> ctx) {
  const auto methodSym = std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getSymbol());
  const auto procedureDeclaration =
      std::dynamic_pointer_cast<ast::prototypes::ProcedureAst>(methodSym->getDef());
  const auto prototype = procedureDeclaration->getProto();

  const auto params = prototype->getParams();
  const auto args = ctx->getArgs();
  std::vector<mlir::Value> mlirArgs;

  auto emitSizeErrorIf = [&](mlir::Value predicate) {
    builder->create<mlir::scf::IfOp>(
        loc, predicate,
        [&](mlir::OpBuilder &b, mlir::Location l) {
          auto throwFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(kThrowArraySizeErrorName);
          b.create<mlir::LLVM::CallOp>(l, throwFunc, mlir::ValueRange{});
          b.create<mlir::scf::YieldOp>(l);
        },
        [&](mlir::OpBuilder &b, mlir::Location l) { b.create<mlir::scf::YieldOp>(l); });
  };

  auto compareSize = [&](mlir::Value actual, mlir::Value expected) {
    auto mismatch =
        builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::ne, actual, expected);
    emitSizeErrorIf(mismatch);
  };

  auto evaluateSizeExprs =
      [&](const std::vector<std::shared_ptr<ast::expressions::ExpressionAst>> &sizeExprs) {
        std::vector<mlir::Value> sizes;
        for (const auto &expr : sizeExprs) {
          if (!expr)
            continue;
          visit(expr);
          auto [sizeType, sizeAddr] = popElementFromStack(expr);
          (void)sizeType;
          sizes.push_back(builder->create<mlir::LLVM::LoadOp>(loc, intTy(), sizeAddr));
        }
        return sizes;
      };

  auto indexConst = [&](int idx) {
    return builder->create<mlir::LLVM::ConstantOp>(loc, builder->getI32Type(), idx);
  };

  std::function<void(std::shared_ptr<ast::types::DataTypeAst>, std::shared_ptr<symTable::Type>,
                     mlir::Value)>
      checkSizes = [&](std::shared_ptr<ast::types::DataTypeAst> paramTypeAst,
                       std::shared_ptr<symTable::Type> argTypeSym, mlir::Value argAddr) {
        if (!paramTypeAst || !argTypeSym || !argAddr)
          return;

        switch (paramTypeAst->getNodeType()) {
        case ast::NodeType::ArrayType: {
          auto paramArrayAst = std::dynamic_pointer_cast<ast::types::ArrayTypeAst>(paramTypeAst);
          auto argArraySym = std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(argTypeSym);
          if (!paramArrayAst || !argArraySym)
            return;

          auto arrayStructType = getMLIRType(argArraySym);
          auto sizeAddr = getArraySizeAddr(*builder, loc, arrayStructType, argAddr);
          auto actualSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), sizeAddr);
          const auto declaredSizes = evaluateSizeExprs(paramArrayAst->getSizes());
          if (!declaredSizes.empty())
            compareSize(actualSize, declaredSizes.front());

          auto nestedAst =
              std::dynamic_pointer_cast<ast::types::ArrayTypeAst>(paramArrayAst->getType());
          auto elementArraySym =
              std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(argArraySym->getType());
          if (nestedAst && elementArraySym) {
            auto dataAddr = getArrayDataAddr(*builder, loc, arrayStructType, argAddr);
            auto dataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), dataAddr);
            auto subArrayStructType = getMLIRType(elementArraySym);

            builder->create<mlir::scf::ForOp>(
                loc, constZero(), actualSize, constOne(), mlir::ValueRange{},
                [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i,
                    mlir::ValueRange iterArgs) {
                  auto subArrayPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), subArrayStructType,
                                                                 dataPtr, mlir::ValueRange{i});

                  mlir::OpBuilder::InsertionGuard guard(*builder);
                  builder->setInsertionPoint(b.getInsertionBlock(), b.getInsertionPoint());
                  checkSizes(nestedAst, elementArraySym, subArrayPtr);

                  b.create<mlir::scf::YieldOp>(l);
                });
          }
          break;
        }
        case ast::NodeType::VectorType: {
          auto paramVectorAst = std::dynamic_pointer_cast<ast::types::VectorTypeAst>(paramTypeAst);
          auto argVectorSym = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(argTypeSym);
          if (!paramVectorAst || !argVectorSym)
            return;

          auto vectorStructType = getMLIRType(argVectorSym);
          auto sizePtr = gepOpVector(vectorStructType, argAddr, VectorOffset::Size);
          auto actualSize = builder->create<mlir::LLVM::LoadOp>(loc, intTy(), sizePtr);
          if (argVectorSym->inferredSize > 0) {
            auto expected =
                builder->create<mlir::LLVM::ConstantOp>(loc, intTy(), argVectorSym->inferredSize);
            compareSize(actualSize, expected);
          }

          auto dataPtrField = gepOpVector(vectorStructType, argAddr, VectorOffset::Data);
          auto dataPtr = builder->create<mlir::LLVM::LoadOp>(loc, ptrTy(), dataPtrField);
          auto elementTypeSym = argVectorSym->getType();
          auto elementAst = paramVectorAst->getElementType();
          auto elementMLIRType = getMLIRType(elementTypeSym);

          builder->create<mlir::scf::ForOp>(
              loc, constZero(), actualSize, constOne(), mlir::ValueRange{},
              [&](mlir::OpBuilder &b, mlir::Location l, mlir::Value i, mlir::ValueRange iterArgs) {
                auto elementPtr = b.create<mlir::LLVM::GEPOp>(l, ptrTy(), elementMLIRType, dataPtr,
                                                              mlir::ValueRange{i});

                mlir::OpBuilder::InsertionGuard guard(*builder);
                builder->setInsertionPoint(b.getInsertionBlock(), b.getInsertionPoint());
                checkSizes(elementAst, elementTypeSym, elementPtr);

                b.create<mlir::scf::YieldOp>(l);
              });
          break;
        }
        case ast::NodeType::TupleType: {
          auto tupleAst = std::dynamic_pointer_cast<ast::types::TupleTypeAst>(paramTypeAst);
          auto tupleSym = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(argTypeSym);
          if (!tupleAst || !tupleSym)
            return;
          auto tupleStructType = getMLIRType(tupleSym);
          const auto &resolvedTypes = tupleSym->getResolvedTypes();
          const auto subTypes = tupleAst->getTypes();
          for (size_t idx = 0; idx < subTypes.size() && idx < resolvedTypes.size(); ++idx) {
            auto elementPtr = builder->create<mlir::LLVM::GEPOp>(
                loc, ptrTy(), tupleStructType, argAddr,
                mlir::ValueRange{indexConst(0), indexConst(static_cast<int>(idx))});
            checkSizes(subTypes[idx], resolvedTypes[idx], elementPtr);
          }
          break;
        }
        case ast::NodeType::StructType: {
          auto structAst = std::dynamic_pointer_cast<ast::types::StructTypeAst>(paramTypeAst);
          auto structSym = std::dynamic_pointer_cast<symTable::StructTypeSymbol>(argTypeSym);
          if (!structAst || !structSym)
            return;
          auto structType = getMLIRType(structSym);
          const auto &resolvedTypes = structSym->getResolvedTypes();
          const auto subTypes = structAst->getTypes();
          for (size_t idx = 0; idx < subTypes.size() && idx < resolvedTypes.size(); ++idx) {
            auto elementName = structAst->getElementName(idx + 1);
            auto elementIdx = static_cast<int>(structSym->getIdx(elementName)) - 1;
            auto elementPtr = builder->create<mlir::LLVM::GEPOp>(
                loc, ptrTy(), structType, argAddr,
                mlir::ValueRange{indexConst(0), indexConst(elementIdx)});
            checkSizes(subTypes[idx], resolvedTypes[idx], elementPtr);
          }
          break;
        }
        default:
          break;
        }
      };

  for (size_t i = 0; i < ctx->getArgs().size(); ++i) {
    visit(args[i]);
    auto [valueType, valueAddr] = popElementFromStack(args[i]);
    auto variableSymbol =
        std::dynamic_pointer_cast<symTable::VariableSymbol>(params[i]->getSymbol());

    const bool isVarParam = variableSymbol && variableSymbol->getQualifier() == ast::Qualifier::Var;
    auto paramAst = std::dynamic_pointer_cast<ast::prototypes::ProcedureParamAst>(params[i]);
    auto paramTypeAst = paramAst ? paramAst->getParamType() : nullptr;
    if (isVarParam) {
      checkSizes(paramTypeAst, valueType, valueAddr);
    }

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
  auto callOp = builder->create<mlir::LLVM::CallOp>(loc, procOp, mlir::ValueRange(mlirArgs));

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
    freeAllocatedMemory(methodSym->getReturnType(), returnAlloca.getResult());
  }

  return {};
}

} // namespace gazprea::backend
