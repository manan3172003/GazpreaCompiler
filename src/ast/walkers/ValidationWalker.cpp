#include "CompileTimeExceptions.h"
#include "ast/types/ArrayTypeAst.h"
#include "ast/types/BooleanTypeAst.h"
#include "ast/types/CharacterTypeAst.h"
#include "ast/types/RealTypeAst.h"
#include "symTable/MethodSymbol.h"
#include "symTable/StructTypeSymbol.h"
#include "symTable/TupleTypeSymbol.h"
#include "symTable/VariableSymbol.h"
#include "symTable/VectorTypeSymbol.h"
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
  inAssignment = true;
  visit(ctx->getExpr());
  inAssignment = false;

  if (ctx->getLVal()->getNodeType() == NodeType::IdentifierLeft) {
    const auto idAssignStat =
        std::dynamic_pointer_cast<statements::IdentifierLeftAst>(ctx->getLVal());
    const auto varSymbol =
        std::dynamic_pointer_cast<symTable::VariableSymbol>(idAssignStat->getSymbol());
    if (varSymbol) {
      ensureArrayLiteralType(ctx->getExpr(), varSymbol->getType());
    }
  }

  const auto exprTypeSymbol = ctx->getExpr()->getInferredSymbolType();
  if (ctx->getLVal()->getNodeType() == NodeType::IdentifierLeft) {
    const auto idAssignStat =
        std::dynamic_pointer_cast<statements::IdentifierLeftAst>(ctx->getLVal());
    validateVariableAssignmentTypes(idAssignStat, exprTypeSymbol);
  } else if (ctx->getLVal()->getNodeType() == NodeType::TupleElementAssign) {
    const auto tupleElementAssignStat =
        std::dynamic_pointer_cast<statements::TupleElementAssignAst>(ctx->getLVal());
    validateTupleElementAssignmentTypes(tupleElementAssignStat, exprTypeSymbol);
  } else if (ctx->getLVal()->getNodeType() == NodeType::StructElementAssign) {
    const auto structElementAssignStat =
        std::dynamic_pointer_cast<statements::StructElementAssignAst>(ctx->getLVal());
    validateStructElementAssignmentTypes(structElementAssignStat, exprTypeSymbol);
  } else if (ctx->getLVal()->getNodeType() == NodeType::TupleUnpackAssign) {
    const auto tupleUnpackAssignStat =
        std::dynamic_pointer_cast<statements::TupleUnpackAssignAst>(ctx->getLVal());
    validateTupleUnpackAssignmentTypes(tupleUnpackAssignStat, exprTypeSymbol);
  } // TODO: add more assignment types here in part 2
  return {};
}
std::any ValidationWalker::visitDeclaration(std::shared_ptr<statements::DeclarationAst> ctx) {
  if (ctx->getType())
    visit(ctx->getType());
  const auto variableSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());

  // We're going to have an expression since we'll set defaults in AstBuilder
  inAssignment = true;
  visit(ctx->getExpr());
  inAssignment = false;
  if (not ctx->getType()) {
    // setting inferred type here
    variableSymbol->setType(ctx->getExpr()->getInferredSymbolType());
    ctx->setType(ctx->getExpr()->getInferredDataType());
    ctx->getType()->setSymbol(
        std::dynamic_pointer_cast<symTable::Symbol>(ctx->getExpr()->getInferredSymbolType()));
  }
  // type check
  const auto declarationType =
      std::dynamic_pointer_cast<symTable::Type>(ctx->getType()->getSymbol());
  ensureArrayLiteralType(ctx->getExpr(), declarationType);
  const auto expressionType = ctx->getExpr()->getInferredSymbolType();
  if (not typesMatch(declarationType, expressionType))
    throw TypeError(ctx->getLineNumber(), "Type mismatch");

  // do not need to check qualifier
  // var can be assigned to const, const can be assigned to var

  // Infer vector sizes from RHS expression
  if (isOfSymbolType(declarationType, "vector")) {
    auto vectorDeclarationType =
        std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(declarationType);
    if (vectorDeclarationType) {
      if (const auto literal =
              std::dynamic_pointer_cast<expressions::ArrayLiteralAst>(ctx->getExpr())) {
      }
      inferVectorSize(vectorDeclarationType, ctx->getExpr());
      variableSymbol->setType(vectorDeclarationType);
    }
  }

  // Check if global declaration - must have only literals
  if (ctx->getScope() && ctx->getScope()->getScopeType() == symTable::ScopeType::Global) {
    if (!isLiteralExpression(ctx->getExpr())) {
      throw GlobalError(ctx->getLineNumber(), "Global declarations can only have literal values");
    }
    if (ctx->getQualifier() == Qualifier::Var)
      throw TypeError(ctx->getLineNumber(), "Cannot declare a variable as var in Global Scope");
  }
  return {};
}
std::any ValidationWalker::visitBlock(std::shared_ptr<statements::BlockAst> ctx) {
  bool visitedAllDeclarations = false;
  for (const auto &child : ctx->getChildren()) {
    if (not(std::dynamic_pointer_cast<statements::DeclarationAst>(child) ||
            std::dynamic_pointer_cast<statements::StructDeclarationAst>(child)))
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
  auto curScope = ctx->getScope();
  while (curScope && curScope->getScopeType() != symTable::ScopeType::Loop)
    curScope = curScope->getEnclosingScope();
  if (!curScope)
    throw StatementError(ctx->getLineNumber(), "Break statement must be in loop statement");
  return {};
}
std::any ValidationWalker::visitContinue(std::shared_ptr<statements::ContinueAst> ctx) {
  auto curScope = ctx->getScope();
  while (curScope && curScope->getScopeType() != symTable::ScopeType::Loop)
    curScope = curScope->getEnclosingScope();
  if (!curScope)
    throw StatementError(ctx->getLineNumber(), "Continue statement must be in loop statement");
  return {};
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
  visit(ctx->getExpression());
  const auto typeName = ctx->getExpression()->getInferredSymbolType()->getName();
  if (typeName == "tuple" || typeName == "struct")
    throw TypeError(ctx->getLineNumber(), "Cannot print this datatype");
  return {};
}
std::any ValidationWalker::visitProcedure(std::shared_ptr<prototypes::ProcedureAst> ctx) {
  visit(ctx->getProto());
  auto proto = ctx->getProto();
  if (proto->getName() == "main") {
    if (!proto->getParams().empty())
      throw MainError(ctx->getLineNumber(), "Main cannot have any arguments");
    if (not proto->getReturnType())
      throw MainError(ctx->getLineNumber(), "Main needs to return an integer");
    if (proto->getReturnType() &&
        resolvedInferredType(proto->getReturnType())->getName() != "integer")
      throw MainError(ctx->getLineNumber(), "Main needs to return an integer");
  }

  if (ctx->getProto()->getReturnType()) {
    // If procedure has a return type, check that we reach a return statement
    const auto blockAst = std::dynamic_pointer_cast<statements::BlockAst>(ctx->getBody());
    if (blockAst && !hasReturnInMethod(blockAst)) {
      throw ReturnError(ctx->getLineNumber(), "Function must have a return statement");
    }
  }
  if (not ctx->getBody()) {
    const auto methodSymbol = symTab->getGlobalScope()->getSymbol(ctx->getProto()->getName());
    const auto procedureDeclaration =
        std::dynamic_pointer_cast<prototypes::ProcedureAst>(methodSymbol->getDef());
    if (not procedureDeclaration->getBody())
      throw DefinitionError(procedureDeclaration->getLineNumber(),
                            "Procedure does not have a definition");
  }
  if (ctx->getBody())
    visit(ctx->getBody());
  return {};
}
std::any
ValidationWalker::visitProcedureParams(std::shared_ptr<prototypes::ProcedureParamAst> ctx) {
  visit(ctx->getParamType());
  return {};
}
std::any ValidationWalker::visitProcedureCall(std::shared_ptr<statements::ProcedureCallAst> ctx) {
  auto methodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getSymbol());
  if (not methodSymbol)
    throw CallError(ctx->getLineNumber(), "Call statement used on non-procedure type");

  // need to re-resolve to get defined function and setSymbol to latest symbol with body defined
  methodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(
      symTab->getGlobalScope()->resolveSymbol(methodSymbol->getName()));
  ctx->setSymbol(methodSymbol);

  // Cannot call a function using call
  if (methodSymbol->getScopeType() == symTable::ScopeType::Function) {
    throw CallError(ctx->getLineNumber(), "Call statement used on non-procedure type");
  }

  // Get procedure prototype
  auto procedureDecl = std::dynamic_pointer_cast<prototypes::ProcedureAst>(methodSymbol->getDef());
  auto protoType = procedureDecl->getProto();

  // cannot call main
  if (protoType->getName() == "main")
    throw CallError(ctx->getLineNumber(), "Cannot call main function");

  // Validate arguments against parameters
  const auto params = protoType->getParams();
  const auto args = ctx->getArgs();
  validateArgs(params, args, methodSymbol->getScopeType(), ctx->getLineNumber());

  // Cannot call a procedure inside a function
  const auto curScope = getEnclosingFuncProcScope(ctx->getScope());
  if (curScope && curScope->getScopeType() == symTable::ScopeType::Function) {
    throw CallError(ctx->getLineNumber(), "Procedure call inside function");
  }

  return {};
}
std::any ValidationWalker::visitReturn(std::shared_ptr<statements::ReturnAst> ctx) {
  auto curScope = ctx->getScope();

  // Recurse up till we reach MethodSymbol scope
  while (!std::dynamic_pointer_cast<symTable::MethodSymbol>(curScope)) {
    if (curScope->getScopeType() == symTable::ScopeType::Global) {
      throw StatementError(ctx->getLineNumber(), "Invalid return");
    }
    curScope = curScope->getEnclosingScope();
  }
  if (curScope->getScopeType() == symTable::ScopeType::Function && not ctx->getExpr())
    throw TypeError(ctx->getLineNumber(), "Cannot have empty return in a function");

  if (ctx->getExpr())
    visit(ctx->getExpr());

  const auto methodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(curScope);
  const auto methodReturnType = methodSymbol->getReturnType();
  const auto statReturnType = ctx->getExpr() ? ctx->getExpr()->getInferredSymbolType() : nullptr;

  if (not methodReturnType && statReturnType)
    throw TypeError(ctx->getLineNumber(), "Return type mismatch");
  if (methodReturnType && not statReturnType)
    throw TypeError(ctx->getLineNumber(), "Return type mismatch");
  if (methodReturnType && statReturnType && not typesMatch(methodReturnType, statReturnType))
    throw TypeError(ctx->getLineNumber(), "Return type mismatch");
  return {};
}
std::any
ValidationWalker::visitTupleElementAssign(std::shared_ptr<statements::TupleElementAssignAst> ctx) {
  const auto idSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  if (not idSymbol)
    throw SymbolError(ctx->getLineNumber(), "Identifier symbol is not a variable");
  const auto tupleTypeSymbol =
      std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(idSymbol->getType());
  if (not tupleTypeSymbol)
    throw TypeError(ctx->getLineNumber(), "Identifier symbol is not a tuple type");
  if (ctx->getFieldIndex() == 0 ||
      ctx->getFieldIndex() > static_cast<int>(tupleTypeSymbol->getResolvedTypes().size()))
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
  // call helper to validate inferred types
  validateTupleAccessInferredTypes(ctx);

  const auto tupleSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  const auto tupleTypeSymbol =
      std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(tupleSymbol->getType());
  const int32_t fieldIndex = ctx->getFieldIndex();
  const auto elementType = tupleTypeSymbol->getResolvedTypes()[fieldIndex - 1];

  std::shared_ptr<types::DataTypeAst> dataType;
  const std::string typeName = elementType->getName();

  if (typeName == "integer")
    dataType = std::make_shared<types::IntegerTypeAst>(ctx->token);
  else if (typeName == "real")
    dataType = std::make_shared<types::RealTypeAst>(ctx->token);
  else if (typeName == "character")
    dataType = std::make_shared<types::CharacterTypeAst>(ctx->token);
  else if (typeName == "boolean")
    dataType = std::make_shared<types::BooleanTypeAst>(ctx->token);
  // TODO: introduce part 2 types
  else
    throw TypeError(ctx->getLineNumber(), "Type mismatch");

  ctx->setInferredSymbolType(elementType);
  ctx->setInferredDataType(dataType);

  return {};
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
std::any ValidationWalker::visitStruct(std::shared_ptr<expressions::StructLiteralAst> ctx) {
  const auto typeSymbol = ctx->getScope()->resolveType(ctx->getStructTypeName());
  const auto structTypeSymbol = std::dynamic_pointer_cast<symTable::StructTypeSymbol>(typeSymbol);
  const auto structTypeAst =
      std::dynamic_pointer_cast<types::StructTypeAst>(structTypeSymbol->getDef());

  const auto literalElementValues = ctx->getElements();
  const auto symbolElementTypes = structTypeSymbol->getResolvedTypes();

  // number of arguments must be the same
  if (literalElementValues.size() != symbolElementTypes.size())
    throw TypeError(ctx->getLineNumber(), "Incorrect struct member list");

  for (size_t i = 0; i < literalElementValues.size(); ++i) {
    const auto &literalElementValue = literalElementValues[i];
    const auto &symbolElementType = symbolElementTypes[i];
    visit(literalElementValue);

    if (not typesMatch(symbolElementType, literalElementValue->getInferredSymbolType()))
      throw TypeError(ctx->getLineNumber(), "Incorrect struct member list");

    if (symbolElementType->getName() == "tuple" &&
        literalElementValue->getInferredSymbolType()->getName() == "tuple")
      throw TypeError(ctx->getLineNumber(), "Incorrect struct member list");
  }

  ctx->setInferredSymbolType(structTypeSymbol);
  ctx->setInferredDataType(structTypeAst);
  return AstWalker::visitStruct(ctx);
}
std::any
ValidationWalker::visitStructDeclaration(std::shared_ptr<statements::StructDeclarationAst> ctx) {
  visit(ctx->getType());
  return {};
}
std::any ValidationWalker::visitStructElementAssign(
    std::shared_ptr<statements::StructElementAssignAst> ctx) {
  const auto idSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  if (not idSymbol)
    throw SymbolError(ctx->getLineNumber(), "Identifier symbol is not a variable");

  const auto structTypeSymbol =
      std::dynamic_pointer_cast<symTable::StructTypeSymbol>(idSymbol->getType());
  if (not structTypeSymbol)
    throw TypeError(ctx->getLineNumber(), "Identifier symbol is not a struct type");

  if (not structTypeSymbol->elementNameExist(ctx->getElementName()))
    throw TypeError(ctx->getLineNumber(), "Invalid struct element");
  return {};
}
std::any ValidationWalker::visitStructAccess(std::shared_ptr<expressions::StructAccessAst> ctx) {
  const auto idSymbol = std::dynamic_pointer_cast<symTable::VariableSymbol>(ctx->getSymbol());
  if (not idSymbol)
    throw SymbolError(ctx->getLineNumber(), "Identifier symbol is not a variable");

  const auto structTypeSymbol =
      std::dynamic_pointer_cast<symTable::StructTypeSymbol>(idSymbol->getType());
  if (not structTypeSymbol)
    throw TypeError(ctx->getLineNumber(), "Identifier symbol is not a struct type");

  if (not structTypeSymbol->elementNameExist(ctx->getElementName()))
    throw TypeError(ctx->getLineNumber(), "Invalid struct element");

  const auto elementSymbolType = structTypeSymbol->getResolvedType(ctx->getElementName());
  const auto elementDataType = structTypeSymbol->getUnresolvedType(ctx->getElementName());

  ctx->setInferredSymbolType(elementSymbolType);
  ctx->setInferredDataType(elementDataType);

  return {};
}
std::any ValidationWalker::visitStructType(std::shared_ptr<types::StructTypeAst> ctx) {
  for (const auto &elementAst : ctx->getTypes())
    visit(elementAst);

  if (const auto scope = std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getScope()))
    throw DefinitionError(ctx->getLineNumber(),
                          "Cannot define a struct in a Function/Procedure parameters");

  const auto typeSymbol = std::dynamic_pointer_cast<symTable::StructTypeSymbol>(ctx->getSymbol());
  for (const auto &resolvedType : typeSymbol->getResolvedTypes()) {
    if (std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(resolvedType) ||
        std::dynamic_pointer_cast<symTable::StructTypeSymbol>(resolvedType)) {
      throw DefinitionError(ctx->getLineNumber(), "Invalid struct declaration");
    }
  }
  return {};
}
std::any ValidationWalker::visitTupleType(std::shared_ptr<types::TupleTypeAst> ctx) {
  const auto typeSymbol = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(ctx->getSymbol());
  for (const auto &elementAst : ctx->getTypes())
    visit(elementAst);
  for (const auto &resolvedType : typeSymbol->getResolvedTypes()) {
    if (std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(resolvedType) ||
        std::dynamic_pointer_cast<symTable::StructTypeSymbol>(resolvedType)) {
      throw DefinitionError(ctx->getLineNumber(), "Invalid tuple declaration");
    }
  }
  return {};
}
std::any ValidationWalker::visitTypealias(std::shared_ptr<statements::TypealiasAst> ctx) {
  visit(ctx->getType());
  return {};
}
std::any ValidationWalker::visitFunction(std::shared_ptr<prototypes::FunctionAst> ctx) {
  visit(ctx->getProto());
  if (!ctx->getProto()->getReturnType()) {
    throw ReturnError(ctx->getLineNumber(), "Function must have a return type");
  }
  const auto blockAst = std::dynamic_pointer_cast<statements::BlockAst>(ctx->getBody());
  if (blockAst && !hasReturnInMethod(blockAst)) {
    throw ReturnError(ctx->getLineNumber(), "Function must have a return statement");
  }

  if (not ctx->getBody()) {
    const auto methodSymbol = symTab->getGlobalScope()->getSymbol(ctx->getProto()->getName());
    const auto procedureDeclaration =
        std::dynamic_pointer_cast<prototypes::FunctionAst>(methodSymbol->getDef());
    if (not procedureDeclaration->getBody())
      throw DefinitionError(procedureDeclaration->getLineNumber(),
                            "Function does not have a definition");
  }

  if (ctx->getBody())
    visit(ctx->getBody());
  return {};
}
std::any ValidationWalker::visitFunctionParam(std::shared_ptr<prototypes::FunctionParamAst> ctx) {
  visit(ctx->getParamType());
  return {};
}
std::any ValidationWalker::visitPrototype(std::shared_ptr<prototypes::PrototypeAst> ctx) {
  for (const auto &param : ctx->getParams())
    visit(param);
  if (ctx->getReturnType())
    visit(ctx->getReturnType());
  return {};
}
std::any ValidationWalker::visitStructFuncCallRouter(
    std::shared_ptr<expressions::StructFuncCallRouterAst> ctx) {
  if (ctx->getIsStruct()) {
    const auto structLiteralAst = ctx->getStructLiteralAst();
    visit(structLiteralAst);
    ctx->setSymbol(structLiteralAst->getSymbol());
    ctx->setScope(structLiteralAst->getScope());
    ctx->setInferredDataType(structLiteralAst->getInferredDataType());
    ctx->setInferredSymbolType(structLiteralAst->getInferredSymbolType());
  } else {
    const auto fpCallAst = ctx->getFuncProcCallAst();
    visit(fpCallAst);
    ctx->setSymbol(fpCallAst->getSymbol());
    ctx->setScope(fpCallAst->getScope());
    ctx->setInferredDataType(fpCallAst->getInferredDataType());
    ctx->setInferredSymbolType(fpCallAst->getInferredSymbolType());
  }

  return {};
}
std::any ValidationWalker::visitFuncProcCall(std::shared_ptr<expressions::FuncProcCallAst> ctx) {
  auto methodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getSymbol());
  if (not methodSymbol)
    throw CallError(ctx->getLineNumber(), "Can only call functions/procedures");

  // need to re-resolve to get defined function and setSymbol to latest symbol with body defined
  methodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(
      symTab->getGlobalScope()->resolveSymbol(methodSymbol->getName()));
  ctx->setSymbol(methodSymbol);

  // Get method prototype
  std::shared_ptr<prototypes::PrototypeAst> protoType;
  if (methodSymbol->getScopeType() == symTable::ScopeType::Procedure) {
    const auto procedureDecl =
        std::dynamic_pointer_cast<prototypes::ProcedureAst>(methodSymbol->getDef());
    protoType = procedureDecl->getProto();
  } else if (methodSymbol->getScopeType() == symTable::ScopeType::Function) {
    const auto functionDecl =
        std::dynamic_pointer_cast<prototypes::FunctionAst>(methodSymbol->getDef());
    protoType = functionDecl->getProto();
  }

  // Validate arguments against parameters
  const auto params = protoType->getParams();
  const auto args = ctx->getArgs();
  validateArgs(params, args, methodSymbol->getScopeType(), ctx->getLineNumber());

  // Indirect way to check if symbol is a procedure
  if (methodSymbol->getScopeType() == symTable::ScopeType::Procedure) {
    // Throw error if procedure called outside an assignment/declaration or procedure call
    if (not inAssignment)
      throw CallError(ctx->getLineNumber(), "Cannot call procedure here");

    // Check if we call a procedure from a function
    const auto curScope = getEnclosingFuncProcScope(ctx->getScope());
    if (curScope && curScope->getScopeType() == symTable::ScopeType::Function)
      throw CallError(ctx->getLineNumber(), "Procedure call inside function");
  }

  // Cannot call a void procedure in an expression
  if (!methodSymbol->getReturnType())
    throw ReturnError(ctx->getLineNumber(), "Return type is null");

  // Cannot call a procedure in a binary expression
  if (inBinaryOp && methodSymbol->getScopeType() == symTable::ScopeType::Procedure)
    throw CallError(ctx->getLineNumber(), "Procedure cannot be called in a binary expression");

  // Cannot call main
  if (protoType->getName() == "main")
    throw CallError(ctx->getLineNumber(), "Cannot call main function");

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
  visit(ctx->getTargetType());
  const auto exprType = ctx->getExpression()->getInferredSymbolType();
  const auto targetType = ctx->getResolvedTargetType();

  if (isTuple(exprType) && isTuple(targetType)) {
    const auto targetTupleTypeSymbol =
        std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(targetType);
    const auto curTupleTypeSymbol = std::dynamic_pointer_cast<symTable::TupleTypeSymbol>(exprType);
    const auto targetSubTypes = targetTupleTypeSymbol->getResolvedTypes();
    const auto curSubTypes = curTupleTypeSymbol->getResolvedTypes();

    if (curSubTypes.size() != targetSubTypes.size())
      throw SizeError(ctx->getLineNumber(), "Tuple sizes do not match");
    for (size_t i = 0; i < curSubTypes.size(); i++) {
      if (not utils::isPromotable(curSubTypes[i]->getName(), targetSubTypes[i]->getName())) {
        throw TypeError(ctx->getLineNumber(), "Tuple sub type not promotable");
      }
    }
  } else if (isScalar(exprType) && isScalar(targetType)) {
    // check scalar is promotable
    const auto promoteTo = ctx->getResolvedTargetType()->getName();
    const auto promoteFrom = ctx->getExpression()->getInferredSymbolType()->getName();
    if (not utils::isPromotable(promoteFrom, promoteTo)) {
      throw TypeError(ctx->getLineNumber(), "Type not promotable");
    }
  } else {
    throw TypeError(ctx->getLineNumber(), "Illegal cast");
  }
  ctx->setInferredSymbolType(ctx->getResolvedTargetType());
  ctx->setInferredDataType(ctx->getTargetType());
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
    // Use the variable symbol's type directly to preserve inferred sizes
    ctx->setInferredSymbolType(dataTypeSymbol->getType());
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
std::any ValidationWalker::visitArray(std::shared_ptr<expressions::ArrayLiteralAst> ctx) {
  auto arrayType = std::make_shared<types::ArrayTypeAst>(ctx->token);
  const auto elements = ctx->getElements();

  if (elements.empty()) {
    ctx->setInferredDataType(arrayType);
    // ctx->setInferredSymbolType(resolvedInferredType(arrayType));
    return {};
  }

  // first pass: find if it's a two-dimensional array and check for >2 dimensions
  visit(elements[0]);
  bool is2DArray = false;

  if (elements[0]->getInferredDataType()->getNodeType() == NodeType::ArrayType) {
    is2DArray = true;
  }

  // second pass: elements must all be the same dimension &
  // third pass: infer type by explicitly checking subtypes
  std::shared_ptr<symTable::Type> expectedSubtype = nullptr;
  std::shared_ptr<types::DataTypeAst> inferredElementType = nullptr;

  for (size_t i = 0; i < elements.size(); i++) {
    auto element = elements[i];

    if (i > 0) {
      visit(element);
    }

    // dimension consistency
    bool isElementArray = (element->getInferredDataType()->getNodeType() == NodeType::ArrayType);
    if (isElementArray != is2DArray) {
      throw TypeError(ctx->getLineNumber(), "Array elements must have the same dimension");
    }

    std::shared_ptr<symTable::Type> currentSubtype;
    if (is2DArray) {
      auto elementArrayType =
          std::dynamic_pointer_cast<symTable::ArrayTypeSymbol>(element->getInferredSymbolType());
      if (elementArrayType && elementArrayType->getType()) {
        currentSubtype = elementArrayType->getType();
      }
    } else {
      currentSubtype = element->getInferredSymbolType();
    }

    if (expectedSubtype != nullptr && currentSubtype != nullptr) {
      if (not(typesMatch(currentSubtype, expectedSubtype) ||
              typesMatch(expectedSubtype, currentSubtype))) {
        throw TypeError(ctx->getLineNumber(), "Type mismatch in Array");
      }

      if (expectedSubtype->getName().find("real") != std::string::npos) {
        continue;
      }
    }
    expectedSubtype = currentSubtype;
    inferredElementType = element->getInferredDataType();
  }

  arrayType->setType(inferredElementType);
  ctx->setInferredDataType(arrayType);
  ctx->setInferredSymbolType(resolvedInferredType(arrayType));
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

std::any ValidationWalker::visitArrayType(std::shared_ptr<types::ArrayTypeAst> ctx) {
  if (ctx->getType()) {
    visit(ctx->getType());
  }

  for (const auto &sizeExpr : ctx->getSizes()) {
    if (sizeExpr) {
      visit(sizeExpr);
    }
  }

  return {};
}

std::any ValidationWalker::visitVectorType(std::shared_ptr<types::VectorTypeAst> ctx) {
  if (ctx->getElementType()) {
    visit(ctx->getElementType());
  }
  return {};
}
std::any ValidationWalker::visitLenMemberFunc(std::shared_ptr<statements::LenMemberFuncAst> ctx) {
  visit(ctx->getLeft());
  auto vectorTypeSym = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(
      ctx->getLeft()->getInferredSymbolType());
  if (!vectorTypeSym) {
    throw TypeError(ctx->getLineNumber(), "len can only be used on vector types");
  }
  auto intType = std::make_shared<types::IntegerTypeAst>(ctx->token);
  ctx->setInferredDataType(intType);
  ctx->setInferredSymbolType(resolvedInferredType(intType));
  return {};
}
std::any
ValidationWalker::visitAppendMemberFunc(std::shared_ptr<statements::AppendMemberFuncAst> ctx) {
  visit(ctx->getLeft());
  auto vectorTypeSym = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(
      ctx->getLeft()->getInferredSymbolType());
  if (!vectorTypeSym) {
    throw TypeError(ctx->getLineNumber(), "append can only be used on vector types");
  }
  for (const auto &arg : ctx->getArgs()) {
    visit(arg);
  }
  ctx->setInferredSymbolType(ctx->getLeft()->getInferredSymbolType());
  ctx->setInferredDataType(ctx->getLeft()->getInferredDataType());
  return {};
}
std::any ValidationWalker::visitPushMemberFunc(std::shared_ptr<statements::PushMemberFuncAst> ctx) {
  visit(ctx->getLeft());
  auto vectorTypeSym = std::dynamic_pointer_cast<symTable::VectorTypeSymbol>(
      ctx->getLeft()->getInferredSymbolType());
  if (!vectorTypeSym) {
    throw TypeError(ctx->getLineNumber(), "push can only be used on vector types");
  }
  auto elementType = vectorTypeSym->getType();
  for (const auto &arg : ctx->getArgs()) {
    visit(arg);
    auto argType =
        arg->getExpr() ? arg->getExpr()->getInferredSymbolType() : arg->getInferredSymbolType();
    if (!elementType || !argType || argType->getName() != elementType->getName()) {
      throw TypeError(arg->getLineNumber(), "push argument type mismatch");
    }
  }
  ctx->setInferredSymbolType(ctx->getLeft()->getInferredSymbolType());
  ctx->setInferredDataType(ctx->getLeft()->getInferredDataType());
  return {};
}
std::any
ValidationWalker::visitConcatMemberFunc(std::shared_ptr<statements::ConcatMemberFuncAst> ctx) {
  visit(ctx->getLeft());
  for (const auto &arg : ctx->getArgs()) {
    visit(arg);
  }
  ctx->setInferredSymbolType(ctx->getLeft()->getInferredSymbolType());
  ctx->setInferredDataType(ctx->getLeft()->getInferredDataType());
  return {};
}
std::any
ValidationWalker::visitLengthBuiltinFunc(std::shared_ptr<expressions::LengthBuiltinFuncAst> ctx) {
  auto methodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getSymbol());
  visit(ctx->arg);
  const auto argType = ctx->arg->getInferredSymbolType();
  if (!argType ||
      (argType->getName().substr(0, 5) != "array" && argType->getName().substr(0, 6) != "vector")) {
    throw CallError(ctx->getLineNumber(), "length builtin must be called on arrays or vectors");
  }
  ctx->setInferredSymbolType(methodSymbol->getReturnType());
  ctx->setInferredDataType(std::make_shared<types::IntegerTypeAst>(ctx->token));
  return {};
}
std::any
ValidationWalker::visitShapeBuiltinFunc(std::shared_ptr<expressions::ShapeBuiltinFuncAst> ctx) {
  auto methodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getSymbol());
  visit(ctx->arg);
  const auto argType = ctx->arg->getInferredSymbolType();
  if (!argType ||
      (argType->getName().substr(0, 5) != "array" && argType->getName().substr(0, 6) != "vector")) {
    throw CallError(ctx->getLineNumber(), "shape builtin must be called on arrays or vectors");
  }
  ctx->setInferredSymbolType(methodSymbol->getReturnType());
  ctx->setInferredDataType(std::make_shared<types::ArrayTypeAst>(ctx->token));
  return {};
}
std::any
ValidationWalker::visitReverseBuiltinFunc(std::shared_ptr<expressions::ReverseBuiltinFuncAst> ctx) {
  auto methodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getSymbol());
  visit(ctx->arg);
  const auto argType = ctx->arg->getInferredSymbolType();
  if (!argType ||
      (argType->getName().substr(0, 5) != "array" && argType->getName().substr(0, 6) != "vector")) {
    throw CallError(ctx->getLineNumber(), "reverse builtin must be called on arrays or vectors");
  }
  ctx->setInferredSymbolType(methodSymbol->getReturnType());
  if (ctx->arg->getNodeType() == NodeType::ArrayLiteral) {
    ctx->setInferredDataType(std::make_shared<types::ArrayTypeAst>(ctx->token));
  } else {
    ctx->setInferredDataType(std::make_shared<types::VectorTypeAst>(ctx->token));
  }
  // TODO: add strings?
  return {};
}
std::any
ValidationWalker::visitFormatBuiltinFunc(std::shared_ptr<expressions::FormatBuiltinFuncAst> ctx) {
  auto methodSymbol = std::dynamic_pointer_cast<symTable::MethodSymbol>(ctx->getSymbol());
  visit(ctx->arg);
  const auto argType = ctx->arg->getInferredSymbolType();
  if (!argType ||
      (argType->getName().substr(0, 5) != "array" && argType->getName().substr(0, 6) != "vector")) {
    throw CallError(ctx->getLineNumber(), "format builtin must be called on arrays or vectors");
  }
  ctx->setInferredSymbolType(methodSymbol->getReturnType());
  // TODO: Update it for strings
  ctx->setInferredDataType(std::make_shared<types::IntegerTypeAst>(ctx->token));
  return {};
}
} // namespace gazprea::ast::walkers
