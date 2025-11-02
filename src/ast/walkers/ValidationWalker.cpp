#include "CompileTimeExceptions.h"
#include "ast/types/BooleanTypeAst.h"
#include "ast/types/CharacterTypeAst.h"
#include "ast/types/RealTypeAst.h"
#include "symTable/MethodSymbol.h"
#include "symTable/TupleTypeSymbol.h"
#include "symTable/VariableSymbol.h"
#include "utils/ValidationUtils.h"
#include <ast/walkers/ValidationWalker.h>

namespace gazprea::ast::walkers {

std::any ValidationWalker::visitRoot(std::shared_ptr<RootAst> ctx) {
  bool visitedMain = false;
  for (const auto &child : ctx->children) {
    if (child->getNodeType() == NodeType::Procedure &&
        std::dynamic_pointer_cast<prototypes::ProcedureAst>(child)->getProto()->getName() == "main")
      visitedMain = true;
    visit(child);
  }
  if (not visitedMain)
    throw MainError(ctx->getLineNumber(), "Main procedure not found");
  return {};
}
std::any ValidationWalker::visitAssignment(std::shared_ptr<statements::AssignmentAst> ctx) {
  visit(ctx->getLVal());
  visit(ctx->getExpr());

  const auto exprTypeSymbol = ctx->getExpr()->getInferredSymbolType();
  if (ctx->getLVal()->getNodeType() == NodeType::IdentifierLeft) {
    const auto idAssignStat =
        std::dynamic_pointer_cast<statements::IdentifierLeftAst>(ctx->getLVal());
    validateVariableAssignmentTypes(idAssignStat, exprTypeSymbol);
  } else if (ctx->getLVal()->getNodeType() == NodeType::TupleElementAssign) {
    const auto tupleElementAssignStat =
        std::dynamic_pointer_cast<statements::TupleElementAssignAst>(ctx->getLVal());
    validateTupleElementAssignmentTypes(tupleElementAssignStat, exprTypeSymbol);
  } else if (ctx->getLVal()->getNodeType() == NodeType::TupleUnpackAssign) {
    const auto tupleUnpackAssignStat =
        std::dynamic_pointer_cast<statements::TupleUnpackAssignAst>(ctx->getLVal());
    validateTupleUnpackAssignmentTypes(tupleUnpackAssignStat, exprTypeSymbol);
  } // TODO: add more assignment types here in part 2
  return {};
}
std::any ValidationWalker::visitDeclaration(std::shared_ptr<statements::DeclarationAst> ctx) {
  const auto variableSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  if (not ctx->getType()) {
    // setting inferred type here
    variableSymbol->setType(ctx->getExpr()->getInferredSymbolType());
    ctx->setType(ctx->getExpr()->getInferredDataType());
  }

  // type check
  // We're going to have an expression since we'll set defaults in AstBuilder
  visit(ctx->getExpr());
  const auto declarationType =
      std::dynamic_pointer_cast<symTable::Type>(ctx->getType()->getSymbol());
  const auto expressionType = ctx->getExpr()->getInferredSymbolType();
  if (not typesMatch(declarationType, expressionType))
    throw TypeError(ctx->getLineNumber(), "Type mismatch");

  return {};
}
std::any ValidationWalker::visitBlock(std::shared_ptr<statements::BlockAst> ctx) {
  bool visitedAllDeclarations = false;
  for (const auto &child : ctx->getChildren()) {
    if (!std::dynamic_pointer_cast<statements::DeclarationAst>(child))
      visitedAllDeclarations = true;
    else {
      if (visitedAllDeclarations)
        throw StatementError(child->getLineNumber(),
                             "Declarations are only allowed on the top of the block");
    }
    visit(child);
  }
  return {};
}
std::any ValidationWalker::visitBinary(std::shared_ptr<expressions::BinaryAst> ctx) {
  auto leftExpr = ctx->getLeft();
  auto rightExpr = ctx->getRight();
  visitExpression(leftExpr);
  visitExpression(rightExpr);

  // Type promote Integer to Real if either of the operands is a real
  if (isOfSymbolType(leftExpr->getInferredSymbolType(), "integer") &&
      isOfSymbolType(rightExpr->getInferredSymbolType(), "integer")) {
    ctx->setInferredSymbolType(leftExpr->getInferredSymbolType());
    ctx->setInferredDataType(leftExpr->getInferredDataType());
  } else if (isOfSymbolType(leftExpr->getInferredSymbolType(), "integer") &&
             isOfSymbolType(rightExpr->getInferredSymbolType(), "real")) {
    promoteIfNeeded(ctx, leftExpr->getInferredSymbolType(), rightExpr->getInferredSymbolType(),
                    rightExpr->getInferredDataType());
  } else if (isOfSymbolType(leftExpr->getInferredSymbolType(), "real") &&
             isOfSymbolType(rightExpr->getInferredSymbolType(), "integer")) {
    promoteIfNeeded(ctx, rightExpr->getInferredSymbolType(), leftExpr->getInferredSymbolType(),
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

  if (!isValidOp(leftExpr->getInferredSymbolType()->getName(), ctx->getBinaryOpType()))
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

  // if left expr and right expr in cond is real or int then if the operation is
  // <,>,<=,>= then set expression type to boolean
  if (isComparisonOperator(ctx->getBinaryOpType()) && areBothNumeric(leftExpr, rightExpr)) {
    auto booleanDataType = std::make_shared<types::BooleanTypeAst>(ctx->token);
    auto booleanTypeSymbol = resolvedInferredType(booleanDataType);
    ctx->setInferredSymbolType(booleanTypeSymbol);
    ctx->setInferredDataType(booleanDataType);
  }

  return {};
}
std::any ValidationWalker::visitBreak(std::shared_ptr<statements::BreakAst> ctx) {
  return AstWalker::visitBreak(ctx);
}
std::any ValidationWalker::visitContinue(std::shared_ptr<statements::ContinueAst> ctx) {
  return AstWalker::visitContinue(ctx);
}
std::any ValidationWalker::visitConditional(std::shared_ptr<statements::ConditionalAst> ctx) {
  visit(ctx->getCondition());

  if (!isOfSymbolType(ctx->getCondition()->getInferredSymbolType(), "boolean")) {
    throw TypeError(ctx->getLineNumber(), "If statement condition must be of type boolean");
  }
  visit(ctx->getThenBody());
  if (ctx->getElseBody()) {
    visit(ctx->getElseBody());
  }

  return {};
}
std::any ValidationWalker::visitInput(std::shared_ptr<statements::InputAst> ctx) {
  const auto curScope = getEnclosingFuncProcScope(ctx->getScope());
  if (curScope && curScope->getScopeType() == symTable::ScopeType::Function) {
    throw StatementError(ctx->getLineNumber(), "Input statement not allowed in functions");
  }
  return {};
}
std::any ValidationWalker::visitOutput(std::shared_ptr<statements::OutputAst> ctx) {
  const auto curScope = getEnclosingFuncProcScope(ctx->getScope());
  if (curScope && curScope->getScopeType() == symTable::ScopeType::Function) {
    throw StatementError(ctx->getLineNumber(), "Output statement not allowed in functions");
  }
  return {};
}
std::any ValidationWalker::visitProcedure(std::shared_ptr<prototypes::ProcedureAst> ctx) {
  auto proto = ctx->getProto();
  if (proto->getName() == "main") {
    if (!proto->getParams().empty())
      throw MainError(ctx->getLineNumber(), "Main cannot have any arguments");
    if (proto->getReturnType() &&
        resolvedInferredType(proto->getReturnType())->getName() != "integer")
      throw MainError(ctx->getLineNumber(), "Main needs to return an integer");
  }

  if (ctx->getProto()->getReturnType()) {
    // If procedure has a return type, check that we reach a return statement
    const auto blockAst = std::dynamic_pointer_cast<statements::BlockAst>(ctx->getBody());
    if (!hasReturnInMethod(blockAst)) {
      throw ReturnError(ctx->getLineNumber(), "Function must have a return statement");
    }
  }
  visit(ctx->getBody());
  return {};
}
std::any
ValidationWalker::visitProcedureParams(std::shared_ptr<prototypes::ProcedureParamAst> ctx) {
  return AstWalker::visitProcedureParams(ctx);
}
std::any ValidationWalker::visitProcedureCall(std::shared_ptr<statements::ProcedureCallAst> ctx) {

  const auto curScope = getEnclosingFuncProcScope(ctx->getScope());
  if (curScope && curScope->getScopeType() == symTable::ScopeType::Function) {
    throw CallError(ctx->getLineNumber(), "Procedure call inside function");
  }

  const auto methodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getSymbol());
  if (methodSymbol->getScopeType() == symTable::ScopeType::Function) {
    throw CallError(ctx->getLineNumber(), "Call statement used on non-procedure type");
  }
  // check for var parameter aliasing
  auto protoType = std::dynamic_pointer_cast<prototypes::PrototypeAst>(methodSymbol->getDef());
  checkVarArgs(protoType, ctx->getArgs(), ctx->getLineNumber());
  return {};
}
std::any ValidationWalker::visitReturn(std::shared_ptr<statements::ReturnAst> ctx) {
  visit(ctx->getExpr());
  auto curScope = ctx->getScope();

  // Recurse up till we reach MethodSymbol scope
  while (!std::dynamic_pointer_cast<symTable::MethodSymbol>(curScope)) {
    if (curScope->getScopeType() == symTable::ScopeType::Global) {
      throw StatementError(ctx->getLineNumber(), "Invalid return");
    }
    curScope = curScope->getEnclosingScope();
  }

  auto methodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(curScope);
  auto protoType = std::dynamic_pointer_cast<prototypes::PrototypeAst>(methodSymbol->getDef());

  if (ctx->getExpr()->getInferredSymbolType()->getName() !=
      methodSymbol->getReturnType()->getName()) {
    if (not(ctx->getExpr()->getInferredSymbolType()->getName() == "integer" &&
            methodSymbol->getReturnType()->getName() == "real"))
      throw TypeError(ctx->getLineNumber(), "Invalid return type");
  }

  if (ctx->getExpr()->getInferredSymbolType()->getName() == "tuple") {
    auto returnExprTuple = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(
        ctx->getExpr()->getInferredSymbolType());
    auto expectedTuple =
        std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(methodSymbol->getReturnType());
    if (!isTupleTypeMatch(expectedTuple, returnExprTuple))
      throw TypeError(ctx->getLineNumber(), "Type mismatch");
  }
  return {};
}
std::any
ValidationWalker::visitTupleElementAssign(std::shared_ptr<statements::TupleElementAssignAst> ctx) {
  const auto idSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  const auto tupleTypeSymbol =
      std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(idSymbol->getType());
  if (not idSymbol)
    throw SymbolError(ctx->getLineNumber(), "Identifier symbol is not a variable");
  if (not tupleTypeSymbol)
    throw TypeError(ctx->getLineNumber(), "Identifier symbol is not a tuple type");
  if (ctx->getFieldIndex() == 0 ||
      ctx->getFieldIndex() > tupleTypeSymbol->getResolvedTypes().size())
    throw SizeError(ctx->getLineNumber(), "Invalid tuple index");
  return {};
}
std::any
ValidationWalker::visitTupleUnpackAssign(std::shared_ptr<statements::TupleUnpackAssignAst> ctx) {
  const std::vector<std::shared_ptr<statements::AssignLeftAst>> lVals = ctx->getLVals();
  for (const auto &lVal : lVals) {
    visit(lVal);
  }
  return {};
}
std::any ValidationWalker::visitTupleAccess(std::shared_ptr<expressions::TupleAccessAst> ctx) {
  return AstWalker::visitTupleAccess(ctx);
}
std::any ValidationWalker::visitTuple(std::shared_ptr<expressions::TupleLiteralAst> ctx) {
  auto tupleType = std::make_shared<types::TupleTypeAst>(ctx->token);
  for (const auto &element : ctx->getElements()) {
    visit(element);
    tupleType->addType(element->getInferredDataType());
  }
  ctx->setInferredSymbolType(resolvedInferredType(tupleType));
  ctx->setInferredDataType(tupleType);
  return {};
}
std::any ValidationWalker::visitTupleType(std::shared_ptr<types::TupleTypeAst> ctx) {
  return AstWalker::visitTupleType(ctx);
}
std::any ValidationWalker::visitTypealias(std::shared_ptr<statements::TypealiasAst> ctx) {
  return AstWalker::visitTypealias(ctx);
}
std::any ValidationWalker::visitFunction(std::shared_ptr<prototypes::FunctionAst> ctx) {
  if (!ctx->getProto()->getReturnType()) {
    throw ReturnError(ctx->getLineNumber(), "Function must have a return type");
  }
  const auto blockAst = std::dynamic_pointer_cast<statements::BlockAst>(ctx->getBody());
  if (!hasReturnInMethod(blockAst)) {
    throw ReturnError(ctx->getLineNumber(), "Function must have a return statement");
  }

  visit(ctx->getBody());
  return {};
}
std::any ValidationWalker::visitFunctionParam(std::shared_ptr<prototypes::FunctionParamAst> ctx) {
  return AstWalker::visitFunctionParam(ctx);
}
std::any ValidationWalker::visitPrototype(std::shared_ptr<prototypes::PrototypeAst> ctx) {
  return AstWalker::visitPrototype(ctx);
}
std::any ValidationWalker::visitFuncProcCall(std::shared_ptr<expressions::FuncProcCallAst> ctx) {
  const auto methodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getSymbol());

  // Indirect way to check if symbol is a procedure
  if (methodSymbol->getScopeType() == symTable::ScopeType::Procedure) {
    // Check if we call a procedure from a function
    const auto curScope = getEnclosingFuncProcScope(ctx->getScope());
    if (curScope && curScope->getScopeType() == symTable::ScopeType::Function)
      throw CallError(ctx->getLineNumber(), "Procedure call inside function");
  }

  if (!methodSymbol->getReturnType())
    throw ReturnError(ctx->getLineNumber(), "Return type is null");
  if (inBinaryOp && methodSymbol->getScopeType() == symTable::ScopeType::Procedure)
    throw CallError(ctx->getLineNumber(), "Procedure cannot be called in a binary expression");

  const auto protoType =
      std::dynamic_pointer_cast<prototypes::PrototypeAst>(methodSymbol->getDef());

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
      const auto &param = std::dynamic_pointer_cast<prototypes::FunctionParamAst>(params[i]);
      const auto &paramVarSymbol =
          std::dynamic_pointer_cast<symTable::VariableSymbol>(param->getSymbol());

      promoteIfNeeded(arg, arg->getInferredSymbolType(), paramVarSymbol->getType(),
                      param->getParamType());

      if (arg->getInferredSymbolType()->getName() != paramVarSymbol->getType()->getName())
        throw TypeError(ctx->getLineNumber(), "Argument type mismatch");
    } else if (methodSymbol->getScopeType() == symTable::ScopeType::Procedure) {
      const auto &param = std::dynamic_pointer_cast<prototypes::ProcedureParamAst>(params[i]);
      const auto &paramVarSymbol =
          std::dynamic_pointer_cast<symTable::VariableSymbol>(param->getSymbol());
      promoteIfNeeded(arg, arg->getInferredSymbolType(), paramVarSymbol->getType(),
                      param->getParamType());

      if (arg->getInferredSymbolType()->getName() != paramVarSymbol->getType()->getName())
        throw TypeError(ctx->getLineNumber(), "Argument type mismatch");
    }
  }

  // Check for var parameter aliasing if this is a procedure
  if (methodSymbol->getScopeType() == symTable::ScopeType::Procedure) {
    checkVarArgs(protoType, args, ctx->getLineNumber());
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
std::any ValidationWalker::visitBool(std::shared_ptr<expressions::BoolLiteralAst> ctx) {
  const auto boolType = std::make_shared<types::BooleanTypeAst>(ctx->token);
  ctx->setInferredDataType(boolType);
  ctx->setInferredSymbolType(resolvedInferredType(boolType));
  return {};
}
std::any ValidationWalker::visitCast(std::shared_ptr<expressions::CastAst> ctx) {
  visit(ctx->getExpression());
  if (ctx->getExpression()->getInferredDataType()->getNodeType() == NodeType::TupleType) {
    const auto targetTupleTypeSymbol =
        std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(ctx->getResolvedTargetType());
    const auto curTupleTypeSymbol = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(
        ctx->getExpression()->getInferredSymbolType());
    const auto targetSubTypes = targetTupleTypeSymbol->getResolvedTypes();
    const auto curSubTypes = curTupleTypeSymbol->getResolvedTypes();
    if (curSubTypes.size() != targetSubTypes.size())
      throw SizeError(ctx->getLineNumber(), "Tuple sizes do not match");
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
std::any ValidationWalker::visitChar(std::shared_ptr<expressions::CharLiteralAst> ctx) {
  const auto charType = std::make_shared<types::CharacterTypeAst>(ctx->token);
  ctx->setInferredDataType(charType);
  ctx->setInferredSymbolType(resolvedInferredType(charType));
  return {};
}
std::any ValidationWalker::visitIdentifier(std::shared_ptr<expressions::IdentifierAst> ctx) {
  const auto dataTypeSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  auto astNode = dataTypeSymbol->getDef();

  if (astNode->getNodeType() == NodeType::Declaration) {
    auto dataType = std::dynamic_pointer_cast<statements::DeclarationAst>(astNode)->getType();
    ctx->setInferredDataType(dataType);
    ctx->setInferredSymbolType(resolvedInferredType(dataType));
  } else if (astNode->getNodeType() == NodeType::FunctionParam) {
    auto dataType =
        std::dynamic_pointer_cast<prototypes::FunctionParamAst>(astNode)->getParamType();
    ctx->setInferredDataType(dataType);
    ctx->setInferredSymbolType(resolvedInferredType(dataType));
  } else if (astNode->getNodeType() == NodeType::ProcedureParam) {
    auto dataType =
        std::dynamic_pointer_cast<prototypes::ProcedureParamAst>(astNode)->getParamType();
    ctx->setInferredDataType(dataType);
    ctx->setInferredSymbolType(resolvedInferredType(dataType));
  }

  return {};
}
std::any ValidationWalker::visitIdentifierLeft(std::shared_ptr<statements::IdentifierLeftAst> ctx) {
  if (not std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol()))
    throw SymbolError(ctx->getLineNumber(), "Identifier symbol is not a variable");
  return {};
}
std::any ValidationWalker::visitInteger(std::shared_ptr<expressions::IntegerLiteralAst> ctx) {
  auto intType = std::make_shared<types::IntegerTypeAst>(ctx->token);
  ctx->setInferredDataType(intType);
  ctx->setInferredSymbolType(resolvedInferredType(intType));
  return {};
}
std::any ValidationWalker::visitReal(std::shared_ptr<expressions::RealLiteralAst> ctx) {
  auto realType = std::make_shared<types::RealTypeAst>(ctx->token);
  ctx->setInferredDataType(realType);
  ctx->setInferredSymbolType(resolvedInferredType(realType));
  return {};
}
std::any ValidationWalker::visitUnary(std::shared_ptr<expressions::UnaryAst> ctx) {
  visit(ctx->getExpression());

  auto exprType = ctx->getExpression()->getInferredSymbolType();
  auto opType = ctx->getUnaryOpType();
  if (opType == expressions::UnaryOpType::NOT) {
    if (!isOfSymbolType(exprType, "boolean")) {
      throw TypeError(ctx->getLineNumber(), "not operator can only be applied to boolean type");
    }
  } else if (opType == expressions::UnaryOpType::MINUS ||
             opType == expressions::UnaryOpType::PLUS) {
    if (!isOfSymbolType(exprType, "integer") && !isOfSymbolType(exprType, "real")) {
      throw TypeError(ctx->getLineNumber(),
                      "Unary +/- can only be applied to numeric types (integer or real)");
    }
  }

  ctx->setInferredSymbolType(ctx->getExpression()->getInferredSymbolType());
  ctx->setInferredDataType(ctx->getExpression()->getInferredDataType());
  return {};
}
std::any ValidationWalker::visitLoop(std::shared_ptr<statements::LoopAst> ctx) {

  if (ctx->getCondition()) {
    visit(ctx->getCondition());
    if (!isOfSymbolType(ctx->getCondition()->getInferredSymbolType(), "boolean")) {
      throw TypeError(ctx->getLineNumber(), "Loop condition must be of type boolean");
    }
  }

  visit(ctx->getBody());
  return {};
}
std::any ValidationWalker::visitIteratorLoop(std::shared_ptr<statements::IteratorLoopAst> ctx) {
  return AstWalker::visitIteratorLoop(ctx);
}
} // namespace gazprea::ast::walkers