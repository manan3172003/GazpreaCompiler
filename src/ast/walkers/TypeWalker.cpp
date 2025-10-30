#include "CompileTimeExceptions.h"
#include "ast/types/AliasTypeAst.h"
#include "ast/types/BooleanTypeAst.h"
#include "ast/types/CharacterTypeAst.h"
#include "ast/types/RealTypeAst.h"
#include "symTable/MethodSymbol.h"
#include "symTable/TupleTypeSymbol.h"
#include "symTable/VariableSymbol.h"

#include <ast/walkers/TypeWalker.h>

namespace gazprea::ast::walkers {
// DO NOT USE FOR BINARY OP COMPARISONS
void TypeWalker::validateTuple(
    std::shared_ptr<symTable::TupleTypeSymbol> promoteFrom,
    std::shared_ptr<symTable::TupleTypeSymbol> promoteTo) {
  auto promoteFromResolvedTypes = promoteFrom->getResolvedTypes();
  auto promoteToResolvedTypes = promoteTo->getResolvedTypes();
  auto ctx = promoteFrom->getDef();

  if (promoteFromResolvedTypes.size() != promoteToResolvedTypes.size())
    throw SizeError(ctx->getLineNumber(), "Tuple sizes do not match");

  for (size_t i = 0; i < promoteFromResolvedTypes.size(); i++) {
    if (promoteFromResolvedTypes[i]->getName() ==
        promoteToResolvedTypes[i]->getName())
      continue;
    if (promoteFromResolvedTypes[i]->getName() == "integer" &&
        promoteToResolvedTypes[i]->getName() == "real")
      continue;
    if (promoteFromResolvedTypes[i]->getName() !=
        promoteToResolvedTypes[i]->getName())
      throw TypeError(ctx->getLineNumber(), "Type mismatch");
  }
}

bool TypeWalker::isOfSymbolType(
    const std::shared_ptr<symTable::Type> &symbolType,
    const std::string &typeName) {
  if (!symbolType)
    throw std::runtime_error("SymbolType should not be null");

  return symbolType->getName() == typeName;
}

std::shared_ptr<symTable::Type> TypeWalker::resolvedInferredType(
    const std::shared_ptr<types::DataTypeAst> &dataType) {
  auto globalScope = symTab->getGlobalScope();
  switch (dataType->getNodeType()) {
  case NodeType::IntegerType:
    return std::dynamic_pointer_cast<symTable::Type>(
        globalScope->resolveType("integer"));
  case NodeType::RealType:
    return std::dynamic_pointer_cast<symTable::Type>(
        globalScope->resolveType("real"));
  case NodeType::CharType:
    return std::dynamic_pointer_cast<symTable::Type>(
        globalScope->resolveType("character"));
  case NodeType::BoolType:
    return std::dynamic_pointer_cast<symTable::Type>(
        globalScope->resolveType("boolean"));
  case NodeType::AliasType: {
    const auto aliasTypeNode =
        std::dynamic_pointer_cast<types::AliasTypeAst>(dataType);
    const auto aliasSymType =
        globalScope->resolveType(aliasTypeNode->getAlias());
    return std::dynamic_pointer_cast<symTable::Type>(aliasSymType);
  }
  case NodeType::TupleType: {
    // visiting the tuple
    auto tupleTypeSymbol = std::make_shared<symTable::TupleTypeSymbol>("");
    auto tupleDataType =
        std::dynamic_pointer_cast<types::TupleTypeAst>(dataType);
    for (const auto &subType : tupleDataType->getTypes()) {
      tupleTypeSymbol->addResolvedType(resolvedInferredType(subType));
    }
    return std::dynamic_pointer_cast<symTable::Type>(tupleTypeSymbol);
  }
  default:
    return {};
  }
}

std::any TypeWalker::visitRoot(std::shared_ptr<RootAst> ctx) {
  for (const auto &child : ctx->children) {
    visit(child);
  }
  return {};
}
std::any
TypeWalker::visitAssignment(std::shared_ptr<statements::AssignmentAst> ctx) {
  return AstWalker::visitAssignment(ctx);
}
std::any
TypeWalker::visitDeclaration(std::shared_ptr<statements::DeclarationAst> ctx) {
  visit(ctx->getExpr());
  if (ctx->getType()) {
    // Promoting type here

  } else {
    // setting inferred type here
    std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol())
        ->setType(ctx->getExpr()->getInferredSymbolType());
    ctx->setType(ctx->getExpr()->getInferredDataType());
    // TODO: Check type
  }

  return {};
}
std::any TypeWalker::visitBlock(std::shared_ptr<statements::BlockAst> ctx) {
  for (const auto &child : ctx->getChildren()) {
    visit(child);
  }
  return {};
}
std::any TypeWalker::visitBinary(std::shared_ptr<expressions::BinaryAst> ctx) {
  auto leftExpr = ctx->getLeft();
  auto rightExpr = ctx->getRight();
  visit(leftExpr);
  visit(rightExpr);

  // Type promote Integer to Real if either of the operands is a real
  if (isOfSymbolType(leftExpr->getInferredSymbolType(), "integer") &&
      isOfSymbolType(rightExpr->getInferredSymbolType(), "integer")) {
    ctx->setInferredSymbolType(leftExpr->getInferredSymbolType());
    ctx->setInferredDataType(leftExpr->getInferredDataType());
  } else if (isOfSymbolType(leftExpr->getInferredSymbolType(), "integer") &&
             isOfSymbolType(rightExpr->getInferredSymbolType(), "real")) {
    ctx->setInferredSymbolType(rightExpr->getInferredSymbolType());
    ctx->setInferredDataType(rightExpr->getInferredDataType());
  } else if (isOfSymbolType(leftExpr->getInferredSymbolType(), "real") &&
             isOfSymbolType(rightExpr->getInferredSymbolType(), "integer")) {
    ctx->setInferredSymbolType(leftExpr->getInferredSymbolType());
    ctx->setInferredDataType(leftExpr->getInferredDataType());
  } else if (isOfSymbolType(leftExpr->getInferredSymbolType(), "real") &&
             isOfSymbolType(rightExpr->getInferredSymbolType(), "real")) {
    ctx->setInferredSymbolType(leftExpr->getInferredSymbolType());
    ctx->setInferredDataType(leftExpr->getInferredDataType());
  } else {
    if (leftExpr->getInferredSymbolType()->getName() !=
        rightExpr->getInferredSymbolType()->getName()) {
      throw TypeError(ctx->getLineNumber(), "Binary operation: Type mismatch");
    }
  }
  // Both left and right expressions from here on will be equal because of the
  // above else statement throwing error

  if (!isValidOp(leftExpr->getInferredSymbolType()->getName(),
                 ctx->getBinaryOpType()))
    throw TypeError(ctx->getLineNumber(), "Invalid binary operation");

  // If the operation is == or != or operands are bool set expression type to
  // boolean
  if (ctx->getBinaryOpType() == expressions::BinaryOpType::EQUAL ||
      ctx->getBinaryOpType() == expressions::BinaryOpType::NOT_EQUAL ||
      isOfSymbolType(ctx->getLeft()->getInferredSymbolType(), "boolean")) {
    auto booleanDataType = std::make_shared<types::BooleanTypeAst>(ctx->token);
    auto booleanTypeSymbol = resolvedInferredType(booleanDataType);
    ctx->setInferredSymbolType(booleanTypeSymbol);
    ctx->setInferredDataType(booleanDataType);
  }

  return {};
}
std::any TypeWalker::visitBreak(std::shared_ptr<statements::BreakAst> ctx) {
  return AstWalker::visitBreak(ctx);
}
std::any
TypeWalker::visitContinue(std::shared_ptr<statements::ContinueAst> ctx) {
  return AstWalker::visitContinue(ctx);
}
std::any
TypeWalker::visitConditional(std::shared_ptr<statements::ConditionalAst> ctx) {
  return AstWalker::visitConditional(ctx);
}
std::any TypeWalker::visitInput(std::shared_ptr<statements::InputAst> ctx) {
  return AstWalker::visitInput(ctx);
}
std::any TypeWalker::visitOutput(std::shared_ptr<statements::OutputAst> ctx) {
  return AstWalker::visitOutput(ctx);
}
std::any
TypeWalker::visitProcedure(std::shared_ptr<prototypes::ProcedureAst> ctx) {
  visit(ctx->getBody());
  return {};
}
std::any TypeWalker::visitProcedureParams(
    std::shared_ptr<prototypes::ProcedureParamAst> ctx) {
  return AstWalker::visitProcedureParams(ctx);
}
std::any TypeWalker::visitProcedureCall(
    std::shared_ptr<statements::ProcedureCallAst> ctx) {
  return AstWalker::visitProcedureCall(ctx);
}
std::any TypeWalker::visitReturn(std::shared_ptr<statements::ReturnAst> ctx) {
  visit(ctx->getExpr());
  auto curScope = ctx->getScope();

  // Recurse up till we reach MethodSymbol scope
  while (!std::dynamic_pointer_cast<symTable::MethodSymbol>(curScope)) {
    if (curScope->getScopeType() == symTable::ScopeType::Global) {
      throw StatementError(ctx->getLineNumber(), "Invalid return");
    }
    curScope = curScope->getEnclosingScope();
  }

  auto methodSymbol =
      std::dynamic_pointer_cast<symTable::MethodSymbol>(curScope);
  auto protoType = std::dynamic_pointer_cast<prototypes::PrototypeAst>(
      methodSymbol->getDef());

  // promote if return statement type is integer and method return type is real

  promoteIfNeeded(ctx->getExpr(), ctx->getExpr()->getInferredSymbolType(),
                  methodSymbol->getReturnType(), protoType->getReturnType());
  // if (ctx->getExpr()->getInferredSymbolType()->getName() == "integer" &&
  //     methodSymbol->getReturnType()->getName() == "real") {
  //   ctx->getExpr()->setInferredSymbolType(methodSymbol->getReturnType());
  //   ctx->getExpr()->setInferredDataType(protoType->getReturnType());
  // }

  if (ctx->getExpr()->getInferredSymbolType()->getName() !=
      methodSymbol->getReturnType()->getName())
    throw TypeError(ctx->getLineNumber(), "Invalid return type");

  if (ctx->getExpr()->getInferredSymbolType()->getName() == "tuple") {
    auto returnExprTuple = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(
        ctx->getExpr()->getInferredSymbolType());
    auto expectedTuple = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(
        methodSymbol->getReturnType());
    validateTuple(returnExprTuple, expectedTuple);
  }
  return {};
}
std::any
TypeWalker::visitTupleAssign(std::shared_ptr<statements::TupleAssignAst> ctx) {
  return AstWalker::visitTupleAssign(ctx);
}
std::any
TypeWalker::visitTupleAccess(std::shared_ptr<expressions::TupleAccessAst> ctx) {
  return AstWalker::visitTupleAccess(ctx);
}
std::any
TypeWalker::visitTuple(std::shared_ptr<expressions::TupleLiteralAst> ctx) {
  auto tupleType = std::make_shared<types::TupleTypeAst>(ctx->token);
  for (const auto &element : ctx->getElements()) {
    visit(element);
    tupleType->addType(element->getInferredDataType());
  }
  ctx->setInferredSymbolType(resolvedInferredType(tupleType));
  ctx->setInferredDataType(tupleType);
  return {};
}
std::any TypeWalker::visitTupleType(std::shared_ptr<types::TupleTypeAst> ctx) {
  return AstWalker::visitTupleType(ctx);
}
std::any
TypeWalker::visitTypealias(std::shared_ptr<statements::TypealiasAst> ctx) {
  return AstWalker::visitTypealias(ctx);
}
std::any
TypeWalker::visitFunction(std::shared_ptr<prototypes::FunctionAst> ctx) {
  visit(ctx->getBody());
  return {};
}
std::any TypeWalker::visitFunctionParam(
    std::shared_ptr<prototypes::FunctionParamAst> ctx) {
  return AstWalker::visitFunctionParam(ctx);
}
std::any
TypeWalker::visitPrototype(std::shared_ptr<prototypes::PrototypeAst> ctx) {
  return AstWalker::visitPrototype(ctx);
}
std::any TypeWalker::visitFuncProcCall(
    std::shared_ptr<expressions::FuncProcCallAst> ctx) {
  auto methodSymbol =
      std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getSymbol());
  if (!methodSymbol->getReturnType())
    throw ReturnError(ctx->getLineNumber(), "Return type is null");

  auto protoType = std::dynamic_pointer_cast<prototypes::PrototypeAst>(
      methodSymbol->getDef());
  auto params = protoType->getParams();
  auto args = ctx->getArgs();

  for (size_t i = 0; i < args.size(); i++) {
    const auto &arg = args[i];
    visit(arg);

    // TODO: Promote args if arg is int and param is real

    if (methodSymbol->getScopeType() == symTable::ScopeType::Function) {
      const auto &param =
          std::dynamic_pointer_cast<prototypes::FunctionParamAst>(params[i]);
      const auto &paramVarSymbol =
          std::dynamic_pointer_cast<symTable::VariableSymbol>(
              param->getSymbol());

      if (paramVarSymbol->getType()->getName() == "real" &&
          arg->getInferredSymbolType()->getName() == "integer") {
        arg->setInferredSymbolType(paramVarSymbol->getType());
        arg->setInferredDataType(param->getParamType());
      }

      if (arg->getInferredSymbolType()->getName() !=
          paramVarSymbol->getType()->getName())
        throw TypeError(ctx->getLineNumber(), "Argument type mismatch");
    } else if (methodSymbol->getScopeType() == symTable::ScopeType::Procedure) {
      const auto &param =
          std::dynamic_pointer_cast<prototypes::ProcedureParamAst>(params[i]);
      const auto &paramVarSymbol =
          std::dynamic_pointer_cast<symTable::VariableSymbol>(
              param->getSymbol());

      if (paramVarSymbol->getType()->getName() == "real" &&
          arg->getInferredSymbolType()->getName() == "integer") {
        arg->setInferredSymbolType(paramVarSymbol->getType());
        arg->setInferredDataType(param->getParamType());
      }

      if (arg->getInferredSymbolType()->getName() !=
          paramVarSymbol->getType()->getName())
        throw TypeError(ctx->getLineNumber(), "Argument type mismatch");
    }
  }

  ctx->setInferredSymbolType(methodSymbol->getReturnType());
  ctx->setInferredDataType(protoType->getReturnType());
  return {};
}
std::any TypeWalker::visitArg(std::shared_ptr<expressions::ArgAst> ctx) {
  visit(ctx->getExpr());
  ctx->setInferredSymbolType(ctx->getExpr()->getInferredSymbolType());
  ctx->setInferredDataType(ctx->getExpr()->getInferredDataType());
  return {};
}
std::any
TypeWalker::visitBool(std::shared_ptr<expressions::BoolLiteralAst> ctx) {
  auto boolType = std::make_shared<types::BooleanTypeAst>(ctx->token);
  ctx->setInferredDataType(boolType);
  ctx->setInferredSymbolType(resolvedInferredType(boolType));
  return {};
}
std::any TypeWalker::visitCast(std::shared_ptr<expressions::CastAst> ctx) {
  return AstWalker::visitCast(ctx);
}
std::any
TypeWalker::visitChar(std::shared_ptr<expressions::CharLiteralAst> ctx) {
  auto charType = std::make_shared<types::CharacterTypeAst>(ctx->token);
  ctx->setInferredDataType(charType);
  ctx->setInferredSymbolType(resolvedInferredType(charType));
  return {};
}
std::any
TypeWalker::visitIdentifier(std::shared_ptr<expressions::IdentifierAst> ctx) {
  auto dataTypeSymbol =
      std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  auto dataType = std::dynamic_pointer_cast<statements::DeclarationAst>(
                      dataTypeSymbol->getDef())
                      ->getType();
  ctx->setInferredDataType(dataType);
  ctx->setInferredSymbolType(resolvedInferredType(dataType));
  return {};
}
std::any TypeWalker::visitIdentifierLeft(
    std::shared_ptr<statements::IdentifierLeftAst> ctx) {
  return AstWalker::visitIdentifierLeft(ctx);
}
std::any
TypeWalker::visitInteger(std::shared_ptr<expressions::IntegerLiteralAst> ctx) {
  auto intType = std::make_shared<types::IntegerTypeAst>(ctx->token);
  ctx->setInferredDataType(intType);
  ctx->setInferredSymbolType(resolvedInferredType(intType));
  return {};
}
std::any
TypeWalker::visitReal(std::shared_ptr<expressions::RealLiteralAst> ctx) {
  auto realType = std::make_shared<types::RealTypeAst>(ctx->token);
  ctx->setInferredDataType(realType);
  ctx->setInferredSymbolType(resolvedInferredType(realType));
  return {};
}
std::any TypeWalker::visitUnary(std::shared_ptr<expressions::UnaryAst> ctx) {
  visit(ctx->getExpression());
  ctx->setInferredSymbolType(ctx->getExpression()->getInferredSymbolType());
  ctx->setInferredDataType(ctx->getExpression()->getInferredDataType());
  return {};
}
std::any TypeWalker::visitLoop(std::shared_ptr<statements::LoopAst> ctx) {
  return AstWalker::visitLoop(ctx);
}
std::any TypeWalker::visitIteratorLoop(
    std::shared_ptr<statements::IteratorLoopAst> ctx) {
  return AstWalker::visitIteratorLoop(ctx);
}
} // namespace gazprea::ast::walkers