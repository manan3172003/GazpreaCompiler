#include "CompileTimeExceptions.h"
#include "ast/RootAst.h"
#include "ast/expressions/ArrayLiteralAst.h"
#include "ast/expressions/BinaryAst.h"
#include "ast/expressions/BoolLiteralAst.h"
#include "ast/expressions/CastAst.h"
#include "ast/expressions/CharLiteralAst.h"
#include "ast/expressions/FuncProcCallAst.h"
#include "ast/expressions/IdentifierAst.h"
#include "ast/expressions/IntegerLiteralAst.h"
#include "ast/expressions/RealLiteralAst.h"
#include "ast/expressions/TupleAccessAst.h"
#include "ast/expressions/TupleLiteralAst.h"
#include "ast/expressions/UnaryAst.h"
#include "ast/prototypes/FunctionAst.h"
#include "ast/prototypes/FunctionParamAst.h"
#include "ast/prototypes/ProcedureAst.h"
#include "ast/prototypes/ProcedureParamAst.h"
#include "ast/statements/AssignmentAst.h"
#include "ast/statements/BlockAst.h"
#include "ast/statements/BreakAst.h"
#include "ast/statements/ConditionalAst.h"
#include "ast/statements/ContinueAst.h"
#include "ast/statements/DeclarationAst.h"
#include "ast/statements/IdentifierLeftAst.h"
#include "ast/statements/InputAst.h"
#include "ast/statements/IteratorLoopAst.h"
#include "ast/statements/LoopAst.h"
#include "ast/statements/OutputAst.h"
#include "ast/statements/ProcedureCallAst.h"
#include "ast/statements/ReturnAst.h"
#include "ast/statements/TupleElementAssignAst.h"
#include "ast/statements/TupleUnpackAssignAst.h"
#include "ast/statements/TypealiasAst.h"
#include "ast/types/AliasTypeAst.h"
#include "ast/types/ArrayTypeAst.h"
#include "ast/types/BooleanTypeAst.h"
#include "ast/types/CharacterTypeAst.h"
#include "ast/types/IntegerTypeAst.h"
#include "ast/types/RealTypeAst.h"
#include "ast/types/TupleTypeAst.h"
#include <ast/walkers/AstBuilder.h>

namespace gazprea::ast::walkers {

std::any AstBuilder::visitFile(GazpreaParser::FileContext *ctx) {
  auto root = std::make_shared<RootAst>(ctx->getStart());
  for (const auto child : ctx->global_stat()) {
    root->addChild(std::any_cast<std::shared_ptr<Ast>>(visit(child)));
  }
  return root;
}
std::any AstBuilder::visitGlobal_stat(GazpreaParser::Global_statContext *ctx) {
  if (ctx->dec_stat()) {
    // Cast StatementAst to Ast for global context
    const auto stmt =
        std::any_cast<std::shared_ptr<statements::StatementAst>>(visit(ctx->dec_stat()));
    return std::static_pointer_cast<Ast>(stmt);
  }
  if (ctx->typealias_stat()) {
    return visit(ctx->typealias_stat());
  }
  if (ctx->procedure_stat()) {
    return visit(ctx->procedure_stat());
  }
  if (ctx->function_stat()) {
    return visit(ctx->function_stat());
  }
  return {};
}

std::any AstBuilder::visitTypealias_stat(GazpreaParser::Typealias_statContext *ctx) {
  auto typealiasAst = std::make_shared<statements::TypealiasAst>(ctx->getStart());
  typealiasAst->setAlias(ctx->ID()->getText());
  auto type = std::any_cast<std::shared_ptr<types::DataTypeAst>>(visit(ctx->type()));
  typealiasAst->setType(type);

  return std::static_pointer_cast<Ast>(typealiasAst);
}
std::any AstBuilder::visitStat(GazpreaParser::StatContext *ctx) {
  if (ctx->BREAK()) {
    auto breakAst = std::make_shared<statements::BreakAst>(ctx->getStart());
    return std::static_pointer_cast<statements::StatementAst>(breakAst);
  } else if (ctx->CONTINUE()) {
    auto continueAst = std::make_shared<statements::ContinueAst>(ctx->getStart());
    return std::static_pointer_cast<statements::StatementAst>(continueAst);
  }
  return GazpreaBaseVisitor::visitStat(ctx);
}
std::any AstBuilder::visitProcedure_stat(GazpreaParser::Procedure_statContext *ctx) {
  const auto protoAst = std::make_shared<prototypes::PrototypeAst>(ctx->getStart());
  protoAst->setName(ctx->ID()->getText());
  protoAst->setProtoType(symTable::ScopeType::Procedure);
  if (ctx->procedure_params()) {
    // Handle Procedure parameters
    protoAst->setParams(
        std::any_cast<std::vector<std::shared_ptr<Ast>>>(visit(ctx->procedure_params())));
  }
  if (ctx->type())
    protoAst->setReturnType(makeType(ctx->type(), ctx->getStart()));

  const auto procAst = std::make_shared<prototypes::ProcedureAst>(ctx->getStart());
  procAst->setProto(protoAst);

  if (ctx->block_stat()) {
    // Handle block Procedure body
    const auto bodyAst =
        std::any_cast<std::shared_ptr<statements::StatementAst>>(visit(ctx->block_stat()));
    procAst->setBody(bodyAst);
  }
  return std::static_pointer_cast<Ast>(procAst);
}
std::any AstBuilder::visitProcedure_params(GazpreaParser::Procedure_paramsContext *ctx) {
  auto params = std::vector<std::shared_ptr<Ast>>{};
  for (const auto &param : ctx->procedure_param()) {
    params.push_back(std::any_cast<std::shared_ptr<Ast>>(visit(param)));
  }
  return params;
}
std::any AstBuilder::visitProcedure_param(GazpreaParser::Procedure_paramContext *ctx) {
  const auto paramAst = std::make_shared<prototypes::ProcedureParamAst>(ctx->getStart());
  if (ctx->qualifier() && ctx->qualifier()->VAR()) {
    paramAst->setQualifier(Qualifier::Var);
  } else {
    paramAst->setQualifier(Qualifier::Const); // Default qualifier
  }
  paramAst->setParamType(makeType(ctx->type(), ctx->getStart()));
  if (ctx->ID()) {
    paramAst->setName(ctx->ID()->getText());
  }
  // Otherwise, parameter has no name (forward declaration)

  return std::static_pointer_cast<Ast>(paramAst);
}
std::any AstBuilder::visitProcedure_call_stat(GazpreaParser::Procedure_call_statContext *ctx) {
  const auto procCallAst = std::make_shared<statements::ProcedureCallAst>(ctx->getStart());
  procCallAst->setName(ctx->ID()->getText());
  if (ctx->args()) {
    procCallAst->setArgs(
        std::any_cast<std::vector<std::shared_ptr<expressions::ArgAst>>>(visit(ctx->args())));
  }
  return std::static_pointer_cast<statements::StatementAst>(procCallAst);
}
std::any AstBuilder::visitFunction_stat(GazpreaParser::Function_statContext *ctx) {
  const auto protoAst = std::make_shared<prototypes::PrototypeAst>(ctx->getStart());
  protoAst->setName(ctx->ID()->getText());
  protoAst->setProtoType(symTable::ScopeType::Function);
  if (ctx->function_params()) {
    // Handle function parameters
    protoAst->setParams(
        std::any_cast<std::vector<std::shared_ptr<Ast>>>(visit(ctx->function_params())));
  }
  protoAst->setReturnType(makeType(ctx->type(), ctx->getStart()));

  const auto functionAst = std::make_shared<prototypes::FunctionAst>(ctx->getStart());
  functionAst->setProto(protoAst);

  if (ctx->expr()) {
    // Handle single expression function body
    const auto returnAst = std::make_shared<statements::ReturnAst>(ctx->getStart());
    returnAst->setExpr(
        std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(visit(ctx->expr())));
    const auto blockAst = std::make_shared<statements::BlockAst>(ctx->getStart());
    blockAst->addChildren(returnAst);
    functionAst->setBody(std::static_pointer_cast<statements::StatementAst>(blockAst));
  } else if (ctx->block_stat()) {
    // Handle block function body
    const auto bodyAst =
        std::any_cast<std::shared_ptr<statements::StatementAst>>(visit(ctx->block_stat()));
    functionAst->setBody(bodyAst);
  } else {
    // Forward declaration without body
    functionAst->setBody(nullptr);
  }
  return std::static_pointer_cast<Ast>(functionAst);
}
std::any AstBuilder::visitFunction_params(GazpreaParser::Function_paramsContext *ctx) {
  auto params = std::vector<std::shared_ptr<Ast>>{};
  for (const auto &param : ctx->function_param()) {
    params.push_back(std::any_cast<std::shared_ptr<Ast>>(visit(param)));
  }
  return params;
}

std::any AstBuilder::visitFunction_param(GazpreaParser::Function_paramContext *ctx) {
  const auto paramAst = std::make_shared<prototypes::FunctionParamAst>(ctx->getStart());
  paramAst->setQualifier(Qualifier::Const);
  paramAst->setParamType(makeType(ctx->type(), ctx->getStart()));
  if (ctx->ID()) {
    paramAst->setName(ctx->ID()->getText());
  }
  // Otherwise, parameter has no name (forward declaration)

  return std::static_pointer_cast<Ast>(paramAst);
}
std::any AstBuilder::visitArgs(GazpreaParser::ArgsContext *ctx) {
  auto args = std::vector<std::shared_ptr<expressions::ArgAst>>{};
  for (const auto &argCtx : ctx->expr()) {
    const auto argAst = std::make_shared<expressions::ArgAst>(argCtx->getStart());
    argAst->setExpr(std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(visit(argCtx)));
    args.push_back(argAst);
  }
  return args;
}
std::any AstBuilder::visitOutput_stat(GazpreaParser::Output_statContext *ctx) {
  auto outputAst = std::make_shared<statements::OutputAst>(ctx->getStart());
  auto expr = std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(visit(ctx->expr()));
  outputAst->setExpression(expr);
  return std::static_pointer_cast<statements::StatementAst>(outputAst);
}
std::any AstBuilder::visitInput_stat(GazpreaParser::Input_statContext *ctx) {
  throw SyntaxError(ctx->getStart()->getLine(), "Input statement not allowed");
  auto lVal = std::any_cast<std::shared_ptr<statements::AssignLeftAst>>(visit(ctx->assign_left()));

  auto inputAst = std::make_shared<statements::InputAst>(ctx->getStart());
  inputAst->setLVal(lVal);

  return std::static_pointer_cast<statements::StatementAst>(inputAst);
}
std::any AstBuilder::visitReturn_stat(GazpreaParser::Return_statContext *ctx) {
  const auto returnAst = std::make_shared<statements::ReturnAst>(ctx->getStart());
  if (ctx->expr()) {
    returnAst->setExpr(
        std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(visit(ctx->expr())));
  }
  return std::static_pointer_cast<statements::StatementAst>(returnAst);
}
std::any AstBuilder::visitIf_stat(GazpreaParser::If_statContext *ctx) {
  const auto ifAst = std::make_shared<statements::ConditionalAst>(ctx->getStart());
  ifAst->setCondition(
      std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(visit(ctx->expr())));
  auto thenStmt = std::any_cast<std::shared_ptr<statements::StatementAst>>(visit(ctx->stat()));
  if (thenStmt->getNodeType() == NodeType::Block) {
    ifAst->setThenBody(std::static_pointer_cast<statements::BlockAst>(thenStmt));
  } else {
    auto blockAst = std::make_shared<statements::BlockAst>(ctx->getStart());
    blockAst->addChildren(thenStmt);
    ifAst->setThenBody(blockAst);
  }
  if (ctx->else_stat()) {
    auto elseStmt =
        std::any_cast<std::shared_ptr<statements::StatementAst>>(visit(ctx->else_stat()));
    if (elseStmt->getNodeType() == NodeType::Block) {
      ifAst->setElseBody(std::static_pointer_cast<statements::BlockAst>(elseStmt));
    } else {
      auto blockAst = std::make_shared<statements::BlockAst>(ctx->getStart());
      blockAst->addChildren(elseStmt);
      ifAst->setElseBody(blockAst);
    }
  }
  return std::static_pointer_cast<statements::StatementAst>(ifAst);
}
std::any AstBuilder::visitElse_stat(GazpreaParser::Else_statContext *ctx) {
  return visit(ctx->stat());
}

std::any AstBuilder::visitInfiniteLoop(GazpreaParser::InfiniteLoopContext *ctx) {
  auto loopAst = std::make_shared<statements::LoopAst>(ctx->getStart());
  loopAst->setIsInfinite(true);
  auto stmt = std::any_cast<std::shared_ptr<statements::StatementAst>>(visit(ctx->stat()));
  if (stmt->getNodeType() == NodeType::Block) {
    loopAst->setBody(std::static_pointer_cast<statements::BlockAst>(stmt));
  } else {
    auto blockAst = std::make_shared<statements::BlockAst>(ctx->getStart());
    blockAst->addChildren(stmt);
    loopAst->setBody(blockAst);
  }
  return std::static_pointer_cast<statements::StatementAst>(loopAst);
}

std::any AstBuilder::visitPrePredicatedLoop(GazpreaParser::PrePredicatedLoopContext *ctx) {
  auto loopAst = std::make_shared<statements::LoopAst>(ctx->getStart());
  loopAst->setCondition(
      std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(visit(ctx->expr())));
  loopAst->setIsPostPredicated(false);
  auto stmt = std::any_cast<std::shared_ptr<statements::StatementAst>>(visit(ctx->stat()));
  if (stmt->getNodeType() == NodeType::Block) {
    loopAst->setBody(std::static_pointer_cast<statements::BlockAst>(stmt));
  } else {
    auto blockAst = std::make_shared<statements::BlockAst>(ctx->getStart());
    blockAst->addChildren(stmt);
    loopAst->setBody(blockAst);
  }
  return std::static_pointer_cast<statements::StatementAst>(loopAst);
}

std::any AstBuilder::visitPostPredicatedLoop(GazpreaParser::PostPredicatedLoopContext *ctx) {
  auto loopAst = std::make_shared<statements::LoopAst>(ctx->getStart());
  loopAst->setCondition(
      std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(visit(ctx->expr())));
  loopAst->setIsPostPredicated(true);
  auto stmt = std::any_cast<std::shared_ptr<statements::StatementAst>>(visit(ctx->stat()));
  if (stmt->getNodeType() == NodeType::Block) {
    loopAst->setBody(std::static_pointer_cast<statements::BlockAst>(stmt));
  } else {
    auto blockAst = std::make_shared<statements::BlockAst>(ctx->getStart());
    blockAst->addChildren(stmt);
    loopAst->setBody(blockAst);
  }
  return std::static_pointer_cast<statements::StatementAst>(loopAst);
}

std::any AstBuilder::visitIterativeLoop(GazpreaParser::IterativeLoopContext *ctx) {
  auto loopAst = std::make_shared<statements::IteratorLoopAst>(ctx->getStart());

  loopAst->setIteratorName(ctx->ID()->getText());
  loopAst->setDomainExpr(
      std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(visit(ctx->expr())));
  auto stmt = std::any_cast<std::shared_ptr<statements::StatementAst>>(visit(ctx->stat()));
  if (stmt->getNodeType() == NodeType::Block) {
    loopAst->setBody(std::static_pointer_cast<statements::BlockAst>(stmt));
  } else {
    auto blockAst = std::make_shared<statements::BlockAst>(ctx->getStart());
    blockAst->addChildren(stmt);
    loopAst->setBody(blockAst);
  }
  return std::static_pointer_cast<statements::StatementAst>(loopAst);
}

std::any AstBuilder::visitBlock_stat(GazpreaParser::Block_statContext *ctx) {
  const auto blockAst = std::make_shared<statements::BlockAst>(ctx->getStart());
  for (const auto child : ctx->stat()) {
    auto statementAst = std::any_cast<std::shared_ptr<statements::StatementAst>>(visit(child));
    blockAst->addChildren(statementAst);
  }
  return std::static_pointer_cast<statements::StatementAst>(blockAst);
}
std::any AstBuilder::visitSingularAssign(GazpreaParser::SingularAssignContext *ctx) {
  auto expr = std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(visit(ctx->expr()));
  auto lVal = std::any_cast<std::shared_ptr<statements::AssignLeftAst>>(visit(ctx->assign_left()));

  auto assignAst = std::make_shared<statements::AssignmentAst>(ctx->getStart());
  assignAst->setExpr(expr);
  assignAst->setLVal(lVal);

  return std::static_pointer_cast<statements::StatementAst>(assignAst);
}
std::any AstBuilder::visitTupleUnpackAssign(GazpreaParser::TupleUnpackAssignContext *ctx) {
  auto expr = std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(visit(ctx->expr()));
  auto lVal = std::make_shared<statements::TupleUnpackAssignAst>(ctx->getStart());
  for (const auto lhs : ctx->assign_left()) {
    auto subLVal = std::any_cast<std::shared_ptr<statements::AssignLeftAst>>(visit(lhs));
    lVal->addSubLVal(subLVal);
  }

  auto assignAst = std::make_shared<statements::AssignmentAst>(ctx->getStart());
  assignAst->setExpr(expr);
  assignAst->setLVal(lVal);

  return std::static_pointer_cast<statements::StatementAst>(assignAst);
}
std::any AstBuilder::visitIdLVal(GazpreaParser::IdLValContext *ctx) {
  auto lVal = std::make_shared<statements::IdentifierLeftAst>(ctx->getStart());
  lVal->setName(ctx->ID()->getText());
  return std::static_pointer_cast<statements::AssignLeftAst>(lVal);
}
std::any AstBuilder::visitTupleElementLVal(GazpreaParser::TupleElementLValContext *ctx) {
  auto lVal = std::make_shared<statements::TupleElementAssignAst>(ctx->getStart());

  // visit and create lVal ast
  std::string accessToken = ctx->TUPLE_ACCESS()->getText();
  size_t pos = accessToken.find('.');
  lVal->setTupleName(accessToken.substr(0, pos));
  lVal->setFieldIndex(getInt(accessToken.substr(pos + 1), ctx->getStart()->getLine()));

  return std::static_pointer_cast<statements::AssignLeftAst>(lVal);
}
std::any AstBuilder::visitDec_stat(GazpreaParser::Dec_statContext *ctx) {
  auto declAst = std::make_shared<statements::DeclarationAst>(ctx->getStart());
  if (ctx->qualifier()) {
    if (ctx->qualifier()->CONST())
      declAst->setQualifier(Qualifier::Const);
    else if (ctx->qualifier()->VAR())
      declAst->setQualifier(Qualifier::Var);
  } else
    declAst->setQualifier(Qualifier::Const);
  if (ctx->type()) {
    auto type = std::any_cast<std::shared_ptr<types::DataTypeAst>>(visit(ctx->type()));
    declAst->setType(type);
  } else {
    declAst->setType(nullptr);
  }

  declAst->setName(ctx->ID()->getText());

  if (ctx->expr()) {
    declAst->setExpr(
        std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(visit(ctx->expr())));
  }

  return std::static_pointer_cast<statements::StatementAst>(declAst);
}
std::any AstBuilder::visitTuple_type(GazpreaParser::Tuple_typeContext *ctx) {
  auto tupleType = std::make_shared<types::TupleTypeAst>(ctx->getStart());
  for (auto const type : ctx->type_list()->type()) {
    tupleType->addType(makeType(type, ctx->getStart()));
  }
  return std::static_pointer_cast<types::DataTypeAst>(tupleType);
}
std::any AstBuilder::visitType_list(GazpreaParser::Type_listContext *ctx) {
  return GazpreaBaseVisitor::visitType_list(ctx);
}
std::any AstBuilder::visitQualifier(GazpreaParser::QualifierContext *ctx) {
  return GazpreaBaseVisitor::visitQualifier(ctx);
}
std::any AstBuilder::visitPowerExpr(GazpreaParser::PowerExprContext *ctx) {
  return createBinaryExpr(ctx->expr(0), "^", ctx->expr(1), ctx->getStart());
}
std::any AstBuilder::visitCastExpr(GazpreaParser::CastExprContext *ctx) {
  const auto castAst = std::make_shared<expressions::CastAst>(ctx->getStart());
  castAst->setType(makeType(ctx->type(), ctx->getStart()));
  castAst->setExpression(
      std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(visit(ctx->expr())));
  return std::static_pointer_cast<expressions::ExpressionAst>(castAst);
}
std::any AstBuilder::visitLogicalExpr(GazpreaParser::LogicalExprContext *ctx) {
  return createBinaryExpr(ctx->expr(0), ctx->op->getText(), ctx->expr(1), ctx->getStart());
}

std::any AstBuilder::visitBoolLiteral(GazpreaParser::BoolLiteralContext *ctx) {
  const auto boolAst = std::make_shared<expressions::BoolLiteralAst>(ctx->getStart());
  if (ctx->TRUE()) {
    boolAst->setValue(true);
  } else {
    boolAst->setValue(false);
  }
  return std::static_pointer_cast<expressions::ExpressionAst>(boolAst);
}
std::any AstBuilder::visitParenExpr(GazpreaParser::ParenExprContext *ctx) {
  return visit(ctx->expr());
}
std::any AstBuilder::visitUnaryExpr(GazpreaParser::UnaryExprContext *ctx) {
  auto unaryExpression = std::make_shared<expressions::UnaryAst>(ctx->getStart());
  auto childExpression =
      std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(visit(ctx->expr()));

  expressions::UnaryOpType opType;
  if (ctx->op->getText() == "+")
    opType = expressions::UnaryOpType::PLUS;
  else if (ctx->op->getText() == "-")
    opType = expressions::UnaryOpType::MINUS;
  else
    opType = expressions::UnaryOpType::NOT;

  unaryExpression->setUnaryOpType(opType);
  unaryExpression->setExpression(childExpression);

  return std::static_pointer_cast<expressions::ExpressionAst>(unaryExpression);
}
std::any AstBuilder::visitFloatLiteral(GazpreaParser::FloatLiteralContext *ctx) {
  return visit(ctx->float_lit());
}
std::any AstBuilder::visitAppendExpr(GazpreaParser::AppendExprContext *ctx) {
  return GazpreaBaseVisitor::visitAppendExpr(ctx);
}
std::any AstBuilder::visitTupleAccessExpr(GazpreaParser::TupleAccessExprContext *ctx) {
  const auto tupleExpression = std::make_shared<expressions::TupleAccessAst>(ctx->getStart());
  std::string accessToken = ctx->TUPLE_ACCESS()->getText();
  size_t pos = accessToken.find('.');
  tupleExpression->setTupleName(accessToken.substr(0, pos));
  tupleExpression->setFieldIndex(getInt(accessToken.substr(pos + 1), ctx->getStart()->getLine()));
  return std::static_pointer_cast<expressions::ExpressionAst>(tupleExpression);
}
std::any AstBuilder::visitIdentifier(GazpreaParser::IdentifierContext *ctx) {
  const auto idAst = std::make_shared<expressions::IdentifierAst>(ctx->getStart());
  idAst->setName(ctx->ID()->getText());
  return std::static_pointer_cast<expressions::ExpressionAst>(idAst);
}
std::any AstBuilder::visitAddSubExpr(GazpreaParser::AddSubExprContext *ctx) {
  return createBinaryExpr(ctx->expr(0), ctx->op->getText(), ctx->expr(1), ctx->getStart());
}
std::any AstBuilder::visitIntLiteral(GazpreaParser::IntLiteralContext *ctx) {
  const auto intAst = std::make_shared<expressions::IntegerLiteralAst>(
      ctx->getStart(), getInt(ctx->INT_LIT()->getText(), ctx->getStart()->getLine()));
  return std::static_pointer_cast<expressions::ExpressionAst>(intAst);
}
std::any
AstBuilder::visitScientificFloatLiteral(GazpreaParser::ScientificFloatLiteralContext *ctx) {
  return visit(ctx->float_parse());
}
std::any AstBuilder::visitByExpr(GazpreaParser::ByExprContext *ctx) {
  return GazpreaBaseVisitor::visitByExpr(ctx);
}
std::any AstBuilder::visitCharLiteral(GazpreaParser::CharLiteralContext *ctx) {
  const auto charAst = std::make_shared<expressions::CharLiteralAst>(ctx->getStart());
  std::string sanitizedLiteral =
      ctx->CHAR_LIT()->getText().substr(1, ctx->CHAR_LIT()->getText().size() - 2);
  char charLiteral = convertStringToChar(sanitizedLiteral, ctx->getStart()->getLine());
  charAst->setValue(charLiteral);
  return std::static_pointer_cast<expressions::ExpressionAst>(charAst);
}
std::any AstBuilder::visitRelationalExpr(GazpreaParser::RelationalExprContext *ctx) {
  return createBinaryExpr(ctx->expr(0), ctx->op->getText(), ctx->expr(1), ctx->getStart());
}

std::any AstBuilder::visitScientificFloat(GazpreaParser::ScientificFloatContext *ctx) {
  std::shared_ptr<expressions::ExpressionAst> exprAst;
  if (ctx->float_lit()) {
    exprAst = std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(visit(ctx->float_lit()));
  } else if (ctx->dot_float()) {
    exprAst = std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(visit(ctx->dot_float()));
  } else if (ctx->float_dot()) {
    exprAst = std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(visit(ctx->float_dot()));
  }

  auto realAst = std::static_pointer_cast<expressions::RealLiteralAst>(exprAst);

  realAst->realValue =
      realAst->realValue * getFloat("1" + ctx->EXPONENT()->getText(), ctx->getStart()->getLine());
  return std::static_pointer_cast<expressions::ExpressionAst>(realAst);
}
std::any
AstBuilder::visitScientificFloatNoDecimal(GazpreaParser::ScientificFloatNoDecimalContext *ctx) {
  auto realAst = std::make_shared<expressions::RealLiteralAst>(
      ctx->getStart(),
      getFloat(ctx->INT_LIT()->getText() + ctx->EXPONENT()->getText(), ctx->getStart()->getLine()));
  return std::static_pointer_cast<expressions::ExpressionAst>(realAst);
}
std::any AstBuilder::visitDotFloat(GazpreaParser::DotFloatContext *ctx) {
  return visit(ctx->dot_float());
}
std::any AstBuilder::visitFloatDot(GazpreaParser::FloatDotContext *ctx) {
  return visit(ctx->float_dot());
}
std::any AstBuilder::visitDot_float(GazpreaParser::Dot_floatContext *ctx) {
  auto realAst = std::make_shared<expressions::RealLiteralAst>(
      ctx->getStart(), getFloat("0." + ctx->INT_LIT()->getText(), ctx->getStart()->getLine()));
  return std::static_pointer_cast<expressions::ExpressionAst>(realAst);
}
std::any AstBuilder::visitFloat_dot(GazpreaParser::Float_dotContext *ctx) {
  auto realAst = std::make_shared<expressions::RealLiteralAst>(
      ctx->getStart(), getFloat(ctx->INT_LIT()->getText(), ctx->getStart()->getLine()));
  return std::static_pointer_cast<expressions::ExpressionAst>(realAst);
}
std::any AstBuilder::visitFloat_lit(GazpreaParser::Float_litContext *ctx) {
  auto realAst = std::make_shared<expressions::RealLiteralAst>(
      ctx->getStart(), getFloat(ctx->INT_LIT(0)->getText() + "." + ctx->INT_LIT(1)->getText(),
                                ctx->getStart()->getLine()));
  return std::static_pointer_cast<expressions::ExpressionAst>(realAst);
}
std::any AstBuilder::visitTupleLiteral(GazpreaParser::TupleLiteralContext *ctx) {
  return visit(ctx->tuple_lit());
}
std::any AstBuilder::visitMulDivRemExpr(GazpreaParser::MulDivRemExprContext *ctx) {
  return createBinaryExpr(ctx->expr(0), ctx->op->getText(), ctx->expr(1), ctx->getStart());
}
std::any AstBuilder::visitStringLiteral(GazpreaParser::StringLiteralContext *ctx) {
  throw TypeError(
      ctx->getStart()->getLine(),
      "String literals are not supported - use single quotes '' for character literals");
}
std::any AstBuilder::visitFuncProcExpr(GazpreaParser::FuncProcExprContext *ctx) {
  const auto fpCallAst = std::make_shared<expressions::FuncProcCallAst>(ctx->getStart());
  fpCallAst->setName(ctx->ID()->getText());
  if (ctx->args()) {
    fpCallAst->setArgs(
        std::any_cast<std::vector<std::shared_ptr<expressions::ArgAst>>>(visit(ctx->args())));
  }
  return std::static_pointer_cast<expressions::ExpressionAst>(fpCallAst);
}
std::any AstBuilder::visitEqualityExpr(GazpreaParser::EqualityExprContext *ctx) {
  return createBinaryExpr(ctx->expr(0), ctx->op->getText(), ctx->expr(1), ctx->getStart());
}
std::any AstBuilder::visitAndExpr(GazpreaParser::AndExprContext *ctx) {
  return createBinaryExpr(ctx->expr(0), "and", ctx->expr(1), ctx->getStart());
}
std::any AstBuilder::visitTuple_lit(GazpreaParser::Tuple_litContext *ctx) {
  auto tupleLiteral = std::make_shared<expressions::TupleLiteralAst>(ctx->getStart());
  for (auto const element : ctx->tuple_elements()->expr()) {
    auto const expr = std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(visit(element));
    tupleLiteral->addElement(expr);
  }

  return std::static_pointer_cast<expressions::ExpressionAst>(tupleLiteral);
}

std::shared_ptr<types::DataTypeAst> AstBuilder::makeType(GazpreaParser::TypeContext *typeContext,
                                                         antlr4::Token *token) {
  auto type = std::any_cast<std::shared_ptr<types::DataTypeAst>>(visit(typeContext));
  return type;
}
std::any AstBuilder::visitBuilt_in_stat(GazpreaParser::Built_in_statContext *ctx) {
  return GazpreaBaseVisitor::visitBuilt_in_stat(ctx);
}
std::any AstBuilder::visitStructFieldLVal(GazpreaParser::StructFieldLValContext *ctx) {
  return GazpreaBaseVisitor::visitStructFieldLVal(ctx);
}
std::any AstBuilder::visitArrayElementLVal(GazpreaParser::ArrayElementLValContext *ctx) {
  return GazpreaBaseVisitor::visitArrayElementLVal(ctx);
}
std::any
AstBuilder::visitTwoDimArrayElementLVal(GazpreaParser::TwoDimArrayElementLValContext *ctx) {
  return GazpreaBaseVisitor::visitTwoDimArrayElementLVal(ctx);
}
std::any AstBuilder::visitField_list(GazpreaParser::Field_listContext *ctx) {
  return GazpreaBaseVisitor::visitField_list(ctx);
}
std::any AstBuilder::visitField(GazpreaParser::FieldContext *ctx) {
  return GazpreaBaseVisitor::visitField(ctx);
}
std::any AstBuilder::visitVectorType(GazpreaParser::VectorTypeContext *ctx) {
  return GazpreaBaseVisitor::visitVectorType(ctx);
}
std::any AstBuilder::visitTwoDimArray(GazpreaParser::TwoDimArrayContext *ctx) {
  auto arrayTypeAst = std::make_shared<types::ArrayTypeAst>(ctx->getStart());
  arrayTypeAst->setType(std::any_cast<std::shared_ptr<types::DataTypeAst>>(visit(ctx->type())));
  auto sizeExpr1 = std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(visit(ctx->expr(0)));
  auto sizeExpr2 = std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(visit(ctx->expr(1)));
  arrayTypeAst->pushSize(sizeExpr1);
  arrayTypeAst->pushSize(sizeExpr2);
  return std::static_pointer_cast<types::DataTypeAst>(arrayTypeAst);
}
std::any AstBuilder::visitTwoDimArrayAlt(GazpreaParser::TwoDimArrayAltContext *ctx) {
  auto arrayTypeAst = std::make_shared<types::ArrayTypeAst>(ctx->getStart());
  arrayTypeAst->setType(std::any_cast<std::shared_ptr<types::DataTypeAst>>(visit(ctx->type())));
  auto sizeExpr1 = std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(visit(ctx->expr(0)));
  auto sizeExpr2 = std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(visit(ctx->expr(1)));
  arrayTypeAst->pushSize(sizeExpr1);
  arrayTypeAst->pushSize(sizeExpr2);
  return std::static_pointer_cast<types::DataTypeAst>(arrayTypeAst);
}
std::any AstBuilder::visitOneDimArray(GazpreaParser::OneDimArrayContext *ctx) {
  auto arrayTypeAst = std::make_shared<types::ArrayTypeAst>(ctx->getStart());
  arrayTypeAst->setType(std::any_cast<std::shared_ptr<types::DataTypeAst>>(visit(ctx->type())));
  auto sizeExpr = std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(visit(ctx->expr()));
  arrayTypeAst->pushSize(sizeExpr);
  return std::static_pointer_cast<types::DataTypeAst>(arrayTypeAst);
}
std::any AstBuilder::visitArrayLiteral(GazpreaParser::ArrayLiteralContext *ctx) {
  return visit(ctx->array_lit());
}
std::any AstBuilder::visitMatrixLiteral(GazpreaParser::MatrixLiteralContext *ctx) {
  return GazpreaBaseVisitor::visitMatrixLiteral(ctx);
}
std::any AstBuilder::visitFormatExpr(GazpreaParser::FormatExprContext *ctx) {
  return GazpreaBaseVisitor::visitFormatExpr(ctx);
}
std::any AstBuilder::visitStructAccessExpr(GazpreaParser::StructAccessExprContext *ctx) {
  return GazpreaBaseVisitor::visitStructAccessExpr(ctx);
}
std::any AstBuilder::visitLengthExpr(GazpreaParser::LengthExprContext *ctx) {
  return GazpreaBaseVisitor::visitLengthExpr(ctx);
}
std::any AstBuilder::visitReverseExpr(GazpreaParser::ReverseExprContext *ctx) {
  return GazpreaBaseVisitor::visitReverseExpr(ctx);
}
std::any AstBuilder::visitArrayAccessExpr(GazpreaParser::ArrayAccessExprContext *ctx) {
  return GazpreaBaseVisitor::visitArrayAccessExpr(ctx);
}
std::any AstBuilder::visitGeneratorExpr(GazpreaParser::GeneratorExprContext *ctx) {
  return GazpreaBaseVisitor::visitGeneratorExpr(ctx);
}
std::any AstBuilder::visitBuiltinFuncExpr(GazpreaParser::BuiltinFuncExprContext *ctx) {
  return GazpreaBaseVisitor::visitBuiltinFuncExpr(ctx);
}
std::any AstBuilder::visitDstarExpr(GazpreaParser::DstarExprContext *ctx) {
  return GazpreaBaseVisitor::visitDstarExpr(ctx);
}
std::any AstBuilder::visitShapeExpr(GazpreaParser::ShapeExprContext *ctx) {
  return GazpreaBaseVisitor::visitShapeExpr(ctx);
}
std::any AstBuilder::visitSliceRangeExpr(GazpreaParser::SliceRangeExprContext *ctx) {
  return GazpreaBaseVisitor::visitSliceRangeExpr(ctx);
}
std::any AstBuilder::visitSliceEndExpr(GazpreaParser::SliceEndExprContext *ctx) {
  return GazpreaBaseVisitor::visitSliceEndExpr(ctx);
}
std::any AstBuilder::visitSliceStartExpr(GazpreaParser::SliceStartExprContext *ctx) {
  return GazpreaBaseVisitor::visitSliceStartExpr(ctx);
}
std::any AstBuilder::visitSliceAllExpr(GazpreaParser::SliceAllExprContext *ctx) {
  return GazpreaBaseVisitor::visitSliceAllExpr(ctx);
}
std::any AstBuilder::visitSingleIndexExpr(GazpreaParser::SingleIndexExprContext *ctx) {
  return GazpreaBaseVisitor::visitSingleIndexExpr(ctx);
}
std::any AstBuilder::visitArray_lit(GazpreaParser::Array_litContext *ctx) {
  auto arrayLiteralAst = std::make_shared<expressions::ArrayLiteralAst>(ctx->getStart());
  for (auto child : ctx->array_elements()->children) {
    if (child->getText() != ",")
      arrayLiteralAst->addElement(
          std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(visit(child)));
  }
  return std::static_pointer_cast<expressions::ExpressionAst>(arrayLiteralAst);
}
std::any AstBuilder::visitMatrix_lit(GazpreaParser::Matrix_litContext *ctx) {
  return GazpreaBaseVisitor::visitMatrix_lit(ctx);
}
std::any AstBuilder::visitArray_elements(GazpreaParser::Array_elementsContext *ctx) {
  return GazpreaBaseVisitor::visitArray_elements(ctx);
}
std::any AstBuilder::visitTuple_elements(GazpreaParser::Tuple_elementsContext *ctx) {
  return GazpreaBaseVisitor::visitTuple_elements(ctx);
}

// Helper function implementations
expressions::BinaryOpType AstBuilder::stringToBinaryOpType(const std::string &op) {
  if (op == "^")
    return expressions::BinaryOpType::POWER;
  if (op == "*")
    return expressions::BinaryOpType::MULTIPLY;
  if (op == "/")
    return expressions::BinaryOpType::DIVIDE;
  if (op == "%")
    return expressions::BinaryOpType::REM;
  if (op == "+")
    return expressions::BinaryOpType::ADD;
  if (op == "-")
    return expressions::BinaryOpType::SUBTRACT;
  if (op == "<")
    return expressions::BinaryOpType::LESS_THAN;
  if (op == ">")
    return expressions::BinaryOpType::GREATER_THAN;
  if (op == "<=")
    return expressions::BinaryOpType::LESS_EQUAL;
  if (op == ">=")
    return expressions::BinaryOpType::GREATER_EQUAL;
  if (op == "==")
    return expressions::BinaryOpType::EQUAL;
  if (op == "!=")
    return expressions::BinaryOpType::NOT_EQUAL;
  if (op == "and")
    return expressions::BinaryOpType::AND;
  if (op == "or")
    return expressions::BinaryOpType::OR;
  if (op == "xor")
    return expressions::BinaryOpType::XOR;

  throw std::runtime_error("Unknown binary operator: " + op);
}

std::any AstBuilder::createBinaryExpr(antlr4::tree::ParseTree *leftCtx, const std::string &op,
                                      antlr4::tree::ParseTree *rightCtx, antlr4::Token *token) {
  auto binaryAst = std::make_shared<expressions::BinaryAst>(token);
  auto left = std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(visit(leftCtx));
  auto right = std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(visit(rightCtx));
  binaryAst->setLeft(left);
  binaryAst->setRight(right);
  binaryAst->setBinaryOpType(stringToBinaryOpType(op));

  return std::static_pointer_cast<expressions::ExpressionAst>(binaryAst);
}

std::any AstBuilder::visitVector_type(GazpreaParser::Vector_typeContext *ctx) {
  return GazpreaBaseVisitor::visitVector_type(ctx);
}
std::any AstBuilder::visitStruct_type(GazpreaParser::Struct_typeContext *ctx) {
  return GazpreaBaseVisitor::visitStruct_type(ctx);
}
std::any AstBuilder::visitCharType(GazpreaParser::CharTypeContext *ctx) {
  return std::static_pointer_cast<types::DataTypeAst>(
      std::make_shared<types::CharacterTypeAst>(ctx->getStart()));
}
std::any AstBuilder::visitBooleanType(GazpreaParser::BooleanTypeContext *ctx) {
  return std::static_pointer_cast<types::DataTypeAst>(
      std::make_shared<types::BooleanTypeAst>(ctx->getStart()));
}
std::any AstBuilder::visitRealType(GazpreaParser::RealTypeContext *ctx) {
  return std::static_pointer_cast<types::DataTypeAst>(
      std::make_shared<types::RealTypeAst>(ctx->getStart()));
}
std::any AstBuilder::visitTupType(GazpreaParser::TupTypeContext *ctx) {
  return visit(ctx->tuple_type());
}
std::any AstBuilder::visitStructType(GazpreaParser::StructTypeContext *ctx) {
  return GazpreaBaseVisitor::visitStructType(ctx);
}
std::any AstBuilder::visitIntType(GazpreaParser::IntTypeContext *ctx) {
  return std::static_pointer_cast<types::DataTypeAst>(
      std::make_shared<types::IntegerTypeAst>(ctx->getStart()));
}
std::any AstBuilder::visitStringType(GazpreaParser::StringTypeContext *ctx) {
  return GazpreaBaseVisitor::visitStringType(ctx);
}
std::any AstBuilder::visitAliasType(GazpreaParser::AliasTypeContext *ctx) {
  auto aliasType = std::make_shared<types::AliasTypeAst>(ctx->getStart());
  aliasType->setAlias(ctx->ID()->getText());
  return std::static_pointer_cast<types::DataTypeAst>(aliasType);
}

char AstBuilder::convertStringToChar(const std::string &str, int lineNumber) {
  if (str.length() == 1) {
    return str[0];
  } else if (str[0] == '\\') {
    switch (str[1]) {
    case '0':
      return '\0';
    case 'a':
      return '\a';
    case 'b':
      return '\b';
    case 'n':
      return '\n';
    case 't':
      return '\t';
    case 'r':
      return '\r';
    case '\\':
      return '\\';
    case '\'':
      return '\'';
    case '\"':
      return '\"';
    default:
      throw SyntaxError(lineNumber,
                        "Escape sequence: \\" + std::string(1, str[1]) + " not supported");
    }
  } else {
    throw SyntaxError(lineNumber, "Invalid character literal: " + str);
  }
}
int AstBuilder::getInt(std::string str, int lineNumber) {
  try {
    return std::stoi(str);
  } catch (const std::out_of_range &e) {
    throw LiteralError(lineNumber, "Integer out of bounds");
  } catch (const std::invalid_argument &e) {
    throw SyntaxError(lineNumber, "Integer value invalid");
  }
}

float AstBuilder::getFloat(std::string str, int lineNumber) {
  try {
    return std::stof(str);
  } catch (const std::out_of_range &e) {
    throw LiteralError(lineNumber, "Float out of bounds");
  } catch (const std::invalid_argument &e) {
    throw SyntaxError(lineNumber, "Float value invalid");
  }
}
} // namespace gazprea::ast::walkers