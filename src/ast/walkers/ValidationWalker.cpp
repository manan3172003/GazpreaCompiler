#include "CompileTimeExceptions.h"
#include "ast/types/AliasTypeAst.h"
#include "ast/types/BooleanTypeAst.h"
#include "ast/types/CharacterTypeAst.h"
#include "ast/types/RealTypeAst.h"
#include "symTable/MethodSymbol.h"
#include "symTable/TupleTypeSymbol.h"
#include "symTable/VariableSymbol.h"
#include "utils/ValidationUtils.h"

#include <ast/walkers/ValidationWalker.h>

namespace gazprea::ast::walkers {
// DO NOT USE FOR BINARY OP COMPARISONS
void ValidationWalker::validateTuple(
    std::shared_ptr<Ast> ctx,
    const std::shared_ptr<symTable::TupleTypeSymbol> &promoteFrom,
    const std::shared_ptr<symTable::TupleTypeSymbol> &promoteTo) {
  const auto promoteFromResolvedTypes = promoteFrom->getResolvedTypes();
  const auto promoteToResolvedTypes = promoteTo->getResolvedTypes();

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

bool ValidationWalker::isOfSymbolType(
    const std::shared_ptr<symTable::Type> &symbolType,
    const std::string &typeName) {
  if (!symbolType)
    throw std::runtime_error("SymbolType should not be null");

  return symbolType->getName() == typeName;
}

std::shared_ptr<symTable::Type> ValidationWalker::resolvedInferredType(
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

std::any ValidationWalker::visitRoot(std::shared_ptr<RootAst> ctx) {
  bool visitedMain = false;
  for (const auto &child : ctx->children) {
    if (child->getNodeType() == NodeType::Procedure &&
        std::dynamic_pointer_cast<prototypes::ProcedureAst>(child)
                ->getProto()
                ->getName() == "main")
      visitedMain = true;
    visit(child);
  }
  if (not visitedMain)
    throw MainError(ctx->getLineNumber(), "Main procedure not found");
  return {};
}
std::any ValidationWalker::visitAssignment(
    std::shared_ptr<statements::AssignmentAst> ctx) {
  return AstWalker::visitAssignment(ctx);
}
std::any ValidationWalker::visitDeclaration(
    std::shared_ptr<statements::DeclarationAst> ctx) {
  if (ctx->getExpr())
    visit(ctx->getExpr());
  auto variableSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  if (ctx->getType()) {
    // Promoting type here

  } else {
    // setting inferred type here
    variableSymbol
        ->setType(ctx->getExpr()->getInferredSymbolType());
    ctx->setType(ctx->getExpr()->getInferredDataType());
  }
  // type check

  return {};
}
std::any
ValidationWalker::visitBlock(std::shared_ptr<statements::BlockAst> ctx) {
  for (const auto &child : ctx->getChildren()) {
    visit(child);
  }
  return {};
}
std::any
ValidationWalker::visitBinary(std::shared_ptr<expressions::BinaryAst> ctx) {
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
    promoteIfNeeded(ctx, leftExpr->getInferredSymbolType(),
                    rightExpr->getInferredSymbolType(),
                    rightExpr->getInferredDataType());
  } else if (isOfSymbolType(leftExpr->getInferredSymbolType(), "real") &&
             isOfSymbolType(rightExpr->getInferredSymbolType(), "integer")) {
    promoteIfNeeded(ctx, rightExpr->getInferredSymbolType(),
                    leftExpr->getInferredSymbolType(),
                    leftExpr->getInferredDataType());
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
std::any
ValidationWalker::visitBreak(std::shared_ptr<statements::BreakAst> ctx) {
  return AstWalker::visitBreak(ctx);
}
std::any
ValidationWalker::visitContinue(std::shared_ptr<statements::ContinueAst> ctx) {
  return AstWalker::visitContinue(ctx);
}
std::any ValidationWalker::visitConditional(
    std::shared_ptr<statements::ConditionalAst> ctx) {
  return AstWalker::visitConditional(ctx);
}
std::any
ValidationWalker::visitInput(std::shared_ptr<statements::InputAst> ctx) {
  const auto curScope = getEnclosingFuncProcScope(ctx->getScope());
  if (curScope && curScope->getScopeType() == symTable::ScopeType::Function) {
    throw StatementError(ctx->getLineNumber(),
                         "Input statement not allowed in functions");
  }
  return {};
}
std::any
ValidationWalker::visitOutput(std::shared_ptr<statements::OutputAst> ctx) {
  const auto curScope = getEnclosingFuncProcScope(ctx->getScope());
  if (curScope && curScope->getScopeType() == symTable::ScopeType::Function) {
    throw StatementError(ctx->getLineNumber(),
                         "Output statement not allowed in functions");
  }
  return {};
}
std::any ValidationWalker::visitProcedure(
    std::shared_ptr<prototypes::ProcedureAst> ctx) {
  auto proto = ctx->getProto();
  if (proto->getName() == "main") {
    if (!proto->getParams().empty())
      throw MainError(ctx->getLineNumber(), "Main cannot have any arguments");
    if (proto->getReturnType() &&
        resolvedInferredType(proto->getReturnType())->getName() != "integer")
      throw MainError(ctx->getLineNumber(), "Main needs to return an integer");
  }
  visit(ctx->getBody());
  return {};
}
std::any ValidationWalker::visitProcedureParams(
    std::shared_ptr<prototypes::ProcedureParamAst> ctx) {
  return AstWalker::visitProcedureParams(ctx);
}
std::any ValidationWalker::visitProcedureCall(
    std::shared_ptr<statements::ProcedureCallAst> ctx) {

  const auto curScope = getEnclosingFuncProcScope(ctx->getScope());
  if (curScope && curScope->getScopeType() == symTable::ScopeType::Function) {
    throw CallError(ctx->getLineNumber(), "Procedure call inside function");
  }

  const auto methodSymbol =
      std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getSymbol());
  if (methodSymbol->getScopeType() == symTable::ScopeType::Function) {
    throw CallError(ctx->getLineNumber(),
                    "Call statement used on non-procedure type");
  }
  return {};
}
std::any
ValidationWalker::visitReturn(std::shared_ptr<statements::ReturnAst> ctx) {
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

  if (ctx->getExpr()->getInferredSymbolType()->getName() !=
      methodSymbol->getReturnType()->getName()) {
    if (not(ctx->getExpr()->getInferredSymbolType()->getName() == "integer" &&
            methodSymbol->getReturnType()->getName() == "real"))
      throw TypeError(ctx->getLineNumber(), "Invalid return type");
  }

  if (ctx->getExpr()->getInferredSymbolType()->getName() == "tuple") {
    auto returnExprTuple = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(
        ctx->getExpr()->getInferredSymbolType());
    auto expectedTuple = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(
        methodSymbol->getReturnType());
    validateTuple(ctx, returnExprTuple, expectedTuple);
  }
  return {};
}
std::any ValidationWalker::visitTupleElementAssign(
    std::shared_ptr<statements::TupleElementAssignAst> ctx) {
  return AstWalker::visitTupleElementAssign(ctx);
}
std::any ValidationWalker::visitTupleUnpackAssign(
    std::shared_ptr<statements::TupleUnpackAssignAst> ctx) {
  return AstWalker::visitTupleUnpackAssign(ctx);
}
std::any ValidationWalker::visitTupleAccess(
    std::shared_ptr<expressions::TupleAccessAst> ctx) {
  return AstWalker::visitTupleAccess(ctx);
}
std::any ValidationWalker::visitTuple(
    std::shared_ptr<expressions::TupleLiteralAst> ctx) {
  auto tupleType = std::make_shared<types::TupleTypeAst>(ctx->token);
  for (const auto &element : ctx->getElements()) {
    visit(element);
    tupleType->addType(element->getInferredDataType());
  }
  ctx->setInferredSymbolType(resolvedInferredType(tupleType));
  ctx->setInferredDataType(tupleType);
  return {};
}
std::any
ValidationWalker::visitTupleType(std::shared_ptr<types::TupleTypeAst> ctx) {
  return AstWalker::visitTupleType(ctx);
}
std::any ValidationWalker::visitTypealias(
    std::shared_ptr<statements::TypealiasAst> ctx) {
  return AstWalker::visitTypealias(ctx);
}
std::any
ValidationWalker::visitFunction(std::shared_ptr<prototypes::FunctionAst> ctx) {
  visit(ctx->getBody());
  return {};
}
std::any ValidationWalker::visitFunctionParam(
    std::shared_ptr<prototypes::FunctionParamAst> ctx) {
  return AstWalker::visitFunctionParam(ctx);
}
std::any ValidationWalker::visitPrototype(
    std::shared_ptr<prototypes::PrototypeAst> ctx) {
  return AstWalker::visitPrototype(ctx);
}
std::any ValidationWalker::visitFuncProcCall(
    std::shared_ptr<expressions::FuncProcCallAst> ctx) {
  const auto methodSymbol =
      std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getSymbol());

  // Indirect way to check if symbol is a procedure
  if (methodSymbol->getScopeType() == symTable::ScopeType::Procedure) {
    // Check if we call a procedure from a function
    const auto curScope = getEnclosingFuncProcScope(ctx->getScope());
    if (curScope && curScope->getScopeType() == symTable::ScopeType::Function)
      throw CallError(ctx->getLineNumber(), "Procedure call inside function");
  }

  if (!methodSymbol->getReturnType())
    throw ReturnError(ctx->getLineNumber(), "Return type is null");

  const auto protoType = std::dynamic_pointer_cast<prototypes::PrototypeAst>(
      methodSymbol->getDef());

  if (protoType->getName() == "main") {
    throw CallError(ctx->getLineNumber(), "Cannot call main function");
  }

  const auto params = protoType->getParams();
  const auto args = ctx->getArgs();

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

      promoteIfNeeded(arg, arg->getInferredSymbolType(),
                      paramVarSymbol->getType(), param->getParamType());

      if (arg->getInferredSymbolType()->getName() !=
          paramVarSymbol->getType()->getName())
        throw TypeError(ctx->getLineNumber(), "Argument type mismatch");
    } else if (methodSymbol->getScopeType() == symTable::ScopeType::Procedure) {
      const auto &param =
          std::dynamic_pointer_cast<prototypes::ProcedureParamAst>(params[i]);
      const auto &paramVarSymbol =
          std::dynamic_pointer_cast<symTable::VariableSymbol>(
              param->getSymbol());
      promoteIfNeeded(arg, arg->getInferredSymbolType(),
                      paramVarSymbol->getType(), param->getParamType());

      if (arg->getInferredSymbolType()->getName() !=
          paramVarSymbol->getType()->getName())
        throw TypeError(ctx->getLineNumber(), "Argument type mismatch");
    }
  }

  ctx->setInferredSymbolType(methodSymbol->getReturnType());
  ctx->setInferredDataType(protoType->getReturnType());
  return {};
}
std::any ValidationWalker::visitArg(std::shared_ptr<expressions::ArgAst> ctx) {
  visit(ctx->getExpr());
  ctx->setInferredSymbolType(ctx->getExpr()->getInferredSymbolType());
  ctx->setInferredDataType(ctx->getExpr()->getInferredDataType());
  return {};
}
std::any
ValidationWalker::visitBool(std::shared_ptr<expressions::BoolLiteralAst> ctx) {
  const auto boolType = std::make_shared<types::BooleanTypeAst>(ctx->token);
  ctx->setInferredDataType(boolType);
  ctx->setInferredSymbolType(resolvedInferredType(boolType));
  return {};
}
std::any
ValidationWalker::visitCast(std::shared_ptr<expressions::CastAst> ctx) {
  visit(ctx->getExpression());
  if (ctx->getExpression()->getInferredDataType()->getNodeType() == NodeType::TupleType) {
    const auto targetTupleTypeSymbol = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(ctx->getResolvedTargetType());
    const auto curTupleTypeSymbol = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(ctx->getExpression()->getInferredSymbolType());
    const auto targetSubTypes = targetTupleTypeSymbol->getResolvedTypes();
    const auto curSubTypes = curTupleTypeSymbol->getResolvedTypes();
    if (curSubTypes.size() != targetSubTypes.size()) throw SizeError(ctx->getLineNumber(), "Tuple sizes do not match");
    for (size_t i = 0; i < curSubTypes.size(); i++) {
      if (!utils::isPromotable(curSubTypes[i]->getName(), targetSubTypes[i]->getName())) {
        throw TypeError(ctx->getLineNumber(), "Tuple sub type not promotable");
      }
    }
  } else {
    // check scalar is promotable
    const auto promoteTo = ctx->getResolvedTargetType()->getName();
    const auto promoteFrom = ctx->getExpression()->getInferredSymbolType()->getName();
    if (!utils::isPromotable(promoteFrom, promoteTo)) {
      throw TypeError(ctx->getLineNumber(), "Type not promotable");
    }
  }
  ctx->setInferredSymbolType(ctx->getResolvedTargetType());
  ctx->setInferredDataType(ctx->getExpression()->getInferredDataType());
  return {};
}
std::any
ValidationWalker::visitChar(std::shared_ptr<expressions::CharLiteralAst> ctx) {
  const auto charType = std::make_shared<types::CharacterTypeAst>(ctx->token);
  ctx->setInferredDataType(charType);
  ctx->setInferredSymbolType(resolvedInferredType(charType));
  return {};
}
std::any ValidationWalker::visitIdentifier(
    std::shared_ptr<expressions::IdentifierAst> ctx) {
  const auto dataTypeSymbol =
      std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  auto astNode = dataTypeSymbol->getDef();

  if (astNode->getNodeType() == NodeType::Declaration) {
    auto dataType =
        std::dynamic_pointer_cast<statements::DeclarationAst>(astNode)
            ->getType();
    ctx->setInferredDataType(dataType);
    ctx->setInferredSymbolType(resolvedInferredType(dataType));
  } else if (astNode->getNodeType() == NodeType::FunctionParam) {
    auto dataType =
        std::dynamic_pointer_cast<prototypes::FunctionParamAst>(astNode)
            ->getParamType();
    ctx->setInferredDataType(dataType);
    ctx->setInferredSymbolType(resolvedInferredType(dataType));
  } else if (astNode->getNodeType() == NodeType::ProcedureParam) {
    auto dataType =
        std::dynamic_pointer_cast<prototypes::ProcedureParamAst>(astNode)
            ->getParamType();
    ctx->setInferredDataType(dataType);
    ctx->setInferredSymbolType(resolvedInferredType(dataType));
  }

  return {};
}
std::any ValidationWalker::visitIdentifierLeft(
    std::shared_ptr<statements::IdentifierLeftAst> ctx) {
  return AstWalker::visitIdentifierLeft(ctx);
}
std::any ValidationWalker::visitInteger(
    std::shared_ptr<expressions::IntegerLiteralAst> ctx) {
  auto intType = std::make_shared<types::IntegerTypeAst>(ctx->token);
  ctx->setInferredDataType(intType);
  ctx->setInferredSymbolType(resolvedInferredType(intType));
  return {};
}
std::any
ValidationWalker::visitReal(std::shared_ptr<expressions::RealLiteralAst> ctx) {
  auto realType = std::make_shared<types::RealTypeAst>(ctx->token);
  ctx->setInferredDataType(realType);
  ctx->setInferredSymbolType(resolvedInferredType(realType));
  return {};
}
std::any
ValidationWalker::visitUnary(std::shared_ptr<expressions::UnaryAst> ctx) {
  visit(ctx->getExpression());
  ctx->setInferredSymbolType(ctx->getExpression()->getInferredSymbolType());
  ctx->setInferredDataType(ctx->getExpression()->getInferredDataType());
  return {};
}
std::any ValidationWalker::visitLoop(std::shared_ptr<statements::LoopAst> ctx) {
  return AstWalker::visitLoop(ctx);
}
std::any ValidationWalker::visitIteratorLoop(
    std::shared_ptr<statements::IteratorLoopAst> ctx) {
  return AstWalker::visitIteratorLoop(ctx);
}
} // namespace gazprea::ast::walkers