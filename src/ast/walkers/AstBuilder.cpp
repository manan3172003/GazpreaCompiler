#include "ast/RootAst.h"
#include "ast/expressions/IdentifierAst.h"
#include "ast/expressions/IntegerAst.h"
#include "ast/expressions/RealAst.h"
#include "ast/expressions/UnaryAst.h"
#include "ast/prototypes/FunctionAst.h"
#include "ast/prototypes/FunctionParamAst.h"
#include "ast/statements/AssignmentAst.h"
#include "ast/statements/BlockAst.h"
#include "ast/statements/DeclarationAst.h"
#include "ast/statements/ReturnAst.h"

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
    return visit(ctx->dec_stat());
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

std::any
AstBuilder::visitTypealias_stat(GazpreaParser::Typealias_statContext *ctx) {
  return GazpreaBaseVisitor::visitTypealias_stat(ctx);
}
std::any AstBuilder::visitStat(GazpreaParser::StatContext *ctx) {
  return GazpreaBaseVisitor::visitStat(ctx);
}
std::any
AstBuilder::visitProcedure_stat(GazpreaParser::Procedure_statContext *ctx) {
  return GazpreaBaseVisitor::visitProcedure_stat(ctx);
}
std::any
AstBuilder::visitProcedure_params(GazpreaParser::Procedure_paramsContext *ctx) {
  return GazpreaBaseVisitor::visitProcedure_params(ctx);
}
std::any AstBuilder::visitProcedure_call_stat(
    GazpreaParser::Procedure_call_statContext *ctx) {
  return GazpreaBaseVisitor::visitProcedure_call_stat(ctx);
}
std::any
AstBuilder::visitFunction_stat(GazpreaParser::Function_statContext *ctx) {
  const auto protoAst =
      std::make_shared<prototypes::PrototypeAst>(ctx->getStart());
  protoAst->setName(ctx->ID()->getText());
  if (ctx->function_params()) {
    // Handle function parameters
    protoAst->setArgs(std::any_cast<std::vector<std::shared_ptr<Ast>>>(
        visit(ctx->function_params())));
  }
  protoAst->setType(ctx->type()->getText());

  const auto functionAst =
      std::make_shared<prototypes::FunctionAst>(ctx->getStart());
  functionAst->setProto(protoAst);

  if (ctx->expr()) {
    // Handle single expression function body
    const auto returnAst =
        std::make_shared<statements::ReturnAst>(ctx->getStart());
    returnAst->setExpr(
        std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(
            visit(ctx->expr())));
    const auto blockAst =
        std::make_shared<statements::BlockAst>(ctx->getStart());
    blockAst->addChildren(returnAst);
    functionAst->setBody(
        std::static_pointer_cast<statements::StatementAst>(blockAst));
  } else if (ctx->block_stat()) {
    // Handle block function body
    const auto bodyAst =
        std::any_cast<std::shared_ptr<statements::StatementAst>>(
            visit(ctx->block_stat()));
    functionAst->setBody(bodyAst);
  } else {
    // Forward declaration without body
    functionAst->setBody(nullptr);
  }
  return std::static_pointer_cast<Ast>(functionAst);
}
std::any
AstBuilder::visitFunction_params(GazpreaParser::Function_paramsContext *ctx) {
  auto params = std::vector<std::shared_ptr<Ast>>{};
  for (const auto &param : ctx->function_param()) {
    params.push_back(std::any_cast<std::shared_ptr<Ast>>(visit(param)));
  }
  return params;
}

std::any
AstBuilder::visitFunction_param(GazpreaParser::Function_paramContext *ctx) {
  const auto paramAst =
      std::make_shared<prototypes::FunctionParamAst>(ctx->getStart());
  paramAst->setType(ctx->type()->getText());
  if (ctx->ID()) {
    paramAst->setName(ctx->ID()->getText());
  }
  // Otherwise, parameter has no name (forward declaration)

  return std::static_pointer_cast<Ast>(paramAst);
}
std::any AstBuilder::visitArgs(GazpreaParser::ArgsContext *ctx) {
  return GazpreaBaseVisitor::visitArgs(ctx);
}
std::any AstBuilder::visitOutput_stat(GazpreaParser::Output_statContext *ctx) {
  return GazpreaBaseVisitor::visitOutput_stat(ctx);
}
std::any AstBuilder::visitInput_stat(GazpreaParser::Input_statContext *ctx) {
  return GazpreaBaseVisitor::visitInput_stat(ctx);
}
std::any AstBuilder::visitReturn_stat(GazpreaParser::Return_statContext *ctx) {
  const auto returnAst =
      std::make_shared<statements::ReturnAst>(ctx->getStart());
  if (ctx->expr()) {
    returnAst->setExpr(
        std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(
            visit(ctx->expr())));
  }
  return std::static_pointer_cast<statements::StatementAst>(returnAst);
}
std::any AstBuilder::visitIf_stat(GazpreaParser::If_statContext *ctx) {
  return GazpreaBaseVisitor::visitIf_stat(ctx);
}
std::any AstBuilder::visitElse_stat(GazpreaParser::Else_statContext *ctx) {
  return GazpreaBaseVisitor::visitElse_stat(ctx);
}
std::any AstBuilder::visitLoop_stat(GazpreaParser::Loop_statContext *ctx) {
  return GazpreaBaseVisitor::visitLoop_stat(ctx);
}
std::any AstBuilder::visitBlock_stat(GazpreaParser::Block_statContext *ctx) {
  const auto blockAst = std::make_shared<statements::BlockAst>(ctx->getStart());
  for (const auto child : ctx->stat()) {
    auto statementAst =
        std::any_cast<std::shared_ptr<statements::StatementAst>>(visit(child));
    blockAst->addChildren(statementAst);
  }
  return std::static_pointer_cast<statements::StatementAst>(blockAst);
}
std::any AstBuilder::visitAssign_stat(GazpreaParser::Assign_statContext *ctx) {
  auto assignAst = std::make_shared<statements::AssignmentAst>(ctx->getStart());
  if (ctx->ID()) {
    assignAst->name = ctx->ID()->getText();
  }
  assignAst->expr = std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(
      visit(ctx->expr()));
  return std::static_pointer_cast<statements::StatementAst>(assignAst);
}
std::any AstBuilder::visitDec_stat(GazpreaParser::Dec_statContext *ctx) {
  auto declAst = std::make_shared<statements::DeclarationAst>(ctx->getStart());
  if (ctx->qualifier()) {
    if (ctx->qualifier()->CONST())
      declAst->qualifier = Qualifier::Const;
    else if (ctx->qualifier()->VAR())
      declAst->qualifier = Qualifier::Var;
  } else
    declAst->qualifier = Qualifier::Const;
  declAst->type = ctx->type()->getText();
  declAst->name = ctx->ID()->getText();

  if (ctx->expr()) {
    declAst->expr = std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(
        visit(ctx->expr()));
  }

  return std::static_pointer_cast<statements::StatementAst>(declAst);
}
std::any
AstBuilder::visitTuple_dec_stat(GazpreaParser::Tuple_dec_statContext *ctx) {
  return GazpreaBaseVisitor::visitTuple_dec_stat(ctx);
}
std::any AstBuilder::visitTuple_type(GazpreaParser::Tuple_typeContext *ctx) {
  return GazpreaBaseVisitor::visitTuple_type(ctx);
}
std::any AstBuilder::visitType_list(GazpreaParser::Type_listContext *ctx) {
  return GazpreaBaseVisitor::visitType_list(ctx);
}
std::any AstBuilder::visitType(GazpreaParser::TypeContext *ctx) {
  return GazpreaBaseVisitor::visitType(ctx);
}
std::any AstBuilder::visitQualifier(GazpreaParser::QualifierContext *ctx) {
  return GazpreaBaseVisitor::visitQualifier(ctx);
}
std::any AstBuilder::visitPowerExpr(GazpreaParser::PowerExprContext *ctx) {
  return GazpreaBaseVisitor::visitPowerExpr(ctx);
}
std::any AstBuilder::visitCastExpr(GazpreaParser::CastExprContext *ctx) {
  return GazpreaBaseVisitor::visitCastExpr(ctx);
}
std::any AstBuilder::visitLogicalExpr(GazpreaParser::LogicalExprContext *ctx) {
  return GazpreaBaseVisitor::visitLogicalExpr(ctx);
}
std::any AstBuilder::visitBoolLiteral(GazpreaParser::BoolLiteralContext *ctx) {
  return GazpreaBaseVisitor::visitBoolLiteral(ctx);
}
std::any AstBuilder::visitParenExpr(GazpreaParser::ParenExprContext *ctx) {
  return GazpreaBaseVisitor::visitParenExpr(ctx);
}
std::any AstBuilder::visitUnaryExpr(GazpreaParser::UnaryExprContext *ctx) {
  auto unaryExpression =
      std::make_shared<expressions::UnaryAst>(ctx->getStart());
  auto childExpression =
      std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(
          visit(ctx->expr()));

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
std::any
AstBuilder::visitFloatLiteral(GazpreaParser::FloatLiteralContext *ctx) {
  auto realAst = std::make_shared<expressions::RealAst>(
      ctx->getStart(), std::stof(ctx->FLOAT_LIT()->getText()));
  return std::static_pointer_cast<expressions::ExpressionAst>(realAst);
}
std::any AstBuilder::visitAppendExpr(GazpreaParser::AppendExprContext *ctx) {
  return GazpreaBaseVisitor::visitAppendExpr(ctx);
}
std::any
AstBuilder::visitTupleAccessExpr(GazpreaParser::TupleAccessExprContext *ctx) {
  return GazpreaBaseVisitor::visitTupleAccessExpr(ctx);
}
std::any AstBuilder::visitIdentifier(GazpreaParser::IdentifierContext *ctx) {
  const auto idAst =
      std::make_shared<expressions::IdentifierAst>(ctx->getStart());
  idAst->setName(ctx->ID()->getText());
  return std::static_pointer_cast<expressions::ExpressionAst>(idAst);
}
std::any AstBuilder::visitAddSubExpr(GazpreaParser::AddSubExprContext *ctx) {
  return GazpreaBaseVisitor::visitAddSubExpr(ctx);
}
std::any AstBuilder::visitIntLiteral(GazpreaParser::IntLiteralContext *ctx) {
  const auto intAst = std::make_shared<expressions::IntegerAst>(
      ctx->getStart(), std::stoi(ctx->INT_LIT()->getText()));
  return std::static_pointer_cast<expressions::ExpressionAst>(intAst);
}
std::any AstBuilder::visitScientificFloatLiteral(
    GazpreaParser::ScientificFloatLiteralContext *ctx) {
  auto realAst = std::make_shared<expressions::RealAst>(
      ctx->getStart(), std::stof(ctx->SCIENTIFIC_FLOAT()->getText()));
  return std::static_pointer_cast<expressions::ExpressionAst>(realAst);
}
std::any
AstBuilder::visitDotFloatLiteral(GazpreaParser::DotFloatLiteralContext *ctx) {
  auto realAst = std::make_shared<expressions::RealAst>(
      ctx->getStart(), std::stof(ctx->DOT_FLOAT()->getText()));
  return std::static_pointer_cast<expressions::ExpressionAst>(realAst);
}
std::any AstBuilder::visitByExpr(GazpreaParser::ByExprContext *ctx) {
  return GazpreaBaseVisitor::visitByExpr(ctx);
}
std::any AstBuilder::visitCharLiteral(GazpreaParser::CharLiteralContext *ctx) {
  return GazpreaBaseVisitor::visitCharLiteral(ctx);
}
std::any
AstBuilder::visitRelationalExpr(GazpreaParser::RelationalExprContext *ctx) {
  return GazpreaBaseVisitor::visitRelationalExpr(ctx);
}
std::any
AstBuilder::visitFloatDotLiteral(GazpreaParser::FloatDotLiteralContext *ctx) {
  auto realAst = std::make_shared<expressions::RealAst>(
      ctx->getStart(), std::stof(ctx->FLOAT_DOT()->getText().substr(0, -1)));
  return std::static_pointer_cast<expressions::ExpressionAst>(realAst);
}
std::any
AstBuilder::visitTupleLiteral(GazpreaParser::TupleLiteralContext *ctx) {
  return GazpreaBaseVisitor::visitTupleLiteral(ctx);
}
std::any AstBuilder::visitMulDivRemDstarExpr(
    GazpreaParser::MulDivRemDstarExprContext *ctx) {
  return GazpreaBaseVisitor::visitMulDivRemDstarExpr(ctx);
}
std::any
AstBuilder::visitStringLiteral(GazpreaParser::StringLiteralContext *ctx) {
  return GazpreaBaseVisitor::visitStringLiteral(ctx);
}
std::any
AstBuilder::visitFuncProcExpr(GazpreaParser::FuncProcExprContext *ctx) {
  return GazpreaBaseVisitor::visitFuncProcExpr(ctx);
}
std::any AstBuilder::visitRangeExpr(GazpreaParser::RangeExprContext *ctx) {
  return GazpreaBaseVisitor::visitRangeExpr(ctx);
}
std::any
AstBuilder::visitEqualityExpr(GazpreaParser::EqualityExprContext *ctx) {
  return GazpreaBaseVisitor::visitEqualityExpr(ctx);
}
std::any AstBuilder::visitAndExpr(GazpreaParser::AndExprContext *ctx) {
  return GazpreaBaseVisitor::visitAndExpr(ctx);
}
std::any AstBuilder::visitTuple_lit(GazpreaParser::Tuple_litContext *ctx) {
  return GazpreaBaseVisitor::visitTuple_lit(ctx);
}
} // namespace gazprea::ast::walkers