#include "ast/RootAst.h"
#include "ast/expressions/IntegerAst.h"
#include "ast/statements/DeclarationAst.h"

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
  return visit(ctx->dec_stat());
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
  return GazpreaBaseVisitor::visitFunction_stat(ctx);
}
std::any
AstBuilder::visitFunction_params(GazpreaParser::Function_paramsContext *ctx) {
  return GazpreaBaseVisitor::visitFunction_params(ctx);
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
  return GazpreaBaseVisitor::visitReturn_stat(ctx);
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
  return GazpreaBaseVisitor::visitBlock_stat(ctx);
}
std::any AstBuilder::visitAssign_stat(GazpreaParser::Assign_statContext *ctx) {
  return GazpreaBaseVisitor::visitAssign_stat(ctx);
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
  declAst->expr = std::any_cast<std::shared_ptr<expressions::ExpressionAst>>(
      visit(ctx->expr()));
  return std::static_pointer_cast<Ast>(declAst);
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
  return GazpreaBaseVisitor::visitUnaryExpr(ctx);
}
std::any
AstBuilder::visitFloatLiteral(GazpreaParser::FloatLiteralContext *ctx) {
  return GazpreaBaseVisitor::visitFloatLiteral(ctx);
}
std::any AstBuilder::visitAppendExpr(GazpreaParser::AppendExprContext *ctx) {
  return GazpreaBaseVisitor::visitAppendExpr(ctx);
}
std::any
AstBuilder::visitTupleAccessExpr(GazpreaParser::TupleAccessExprContext *ctx) {
  return GazpreaBaseVisitor::visitTupleAccessExpr(ctx);
}
std::any AstBuilder::visitIdentifier(GazpreaParser::IdentifierContext *ctx) {
  return GazpreaBaseVisitor::visitIdentifier(ctx);
}
std::any AstBuilder::visitAddSubExpr(GazpreaParser::AddSubExprContext *ctx) {
  return GazpreaBaseVisitor::visitAddSubExpr(ctx);
}
std::any AstBuilder::visitIntLiteral(GazpreaParser::IntLiteralContext *ctx) {
  auto intAst = std::make_shared<expressions::IntegerAst>(
      ctx->getStart(), std::stoi(ctx->INT_LIT()->getText()));
  return std::static_pointer_cast<expressions::ExpressionAst>(intAst);
}
std::any AstBuilder::visitScientificFloatLiteral(
    GazpreaParser::ScientificFloatLiteralContext *ctx) {
  return GazpreaBaseVisitor::visitScientificFloatLiteral(ctx);
}
std::any
AstBuilder::visitDotFloatLiteral(GazpreaParser::DotFloatLiteralContext *ctx) {
  return GazpreaBaseVisitor::visitDotFloatLiteral(ctx);
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
  return GazpreaBaseVisitor::visitFloatDotLiteral(ctx);
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