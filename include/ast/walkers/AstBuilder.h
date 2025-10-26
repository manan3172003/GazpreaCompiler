#pragma once
#include "GazpreaBaseVisitor.h"
#include "ast/expressions/BinaryAst.h"
#include "ast/expressions/ExpressionAst.h"

namespace gazprea::ast::walkers {

class AstBuilder : public GazpreaBaseVisitor {
private:
  static std::shared_ptr<types::DataTypeAst>
  makeType(GazpreaParser::TypeContext *typeContext, antlr4::Token *token);

public:
  std::any visitFile(GazpreaParser::FileContext *ctx) override;
  std::any visitGlobal_stat(GazpreaParser::Global_statContext *ctx) override;
  std::any
  visitTypealias_stat(GazpreaParser::Typealias_statContext *ctx) override;
  std::any visitStat(GazpreaParser::StatContext *ctx) override;
  std::any
  visitProcedure_stat(GazpreaParser::Procedure_statContext *ctx) override;
  std::any
  visitProcedure_params(GazpreaParser::Procedure_paramsContext *ctx) override;
  std::any
  visitProcedure_param(GazpreaParser::Procedure_paramContext *ctx) override;
  std::any visitProcedure_call_stat(
      GazpreaParser::Procedure_call_statContext *ctx) override;
  std::any
  visitFunction_stat(GazpreaParser::Function_statContext *ctx) override;
  std::any
  visitFunction_params(GazpreaParser::Function_paramsContext *ctx) override;
  std::any
  visitFunction_param(GazpreaParser::Function_paramContext *ctx) override;
  std::any visitArgs(GazpreaParser::ArgsContext *ctx) override;
  std::any visitOutput_stat(GazpreaParser::Output_statContext *ctx) override;
  std::any visitInput_stat(GazpreaParser::Input_statContext *ctx) override;
  std::any visitReturn_stat(GazpreaParser::Return_statContext *ctx) override;
  std::any visitIf_stat(GazpreaParser::If_statContext *ctx) override;
  std::any visitElse_stat(GazpreaParser::Else_statContext *ctx) override;
  std::any visitInfiniteLoop(GazpreaParser::InfiniteLoopContext *ctx) override;
  std::any
  visitPrePredicatedLoop(GazpreaParser::PrePredicatedLoopContext *ctx) override;
  std::any visitPostPredicatedLoop(
      GazpreaParser::PostPredicatedLoopContext *ctx) override;
  std::any
  visitIterativeLoop(GazpreaParser::IterativeLoopContext *ctx) override;
  std::any visitBlock_stat(GazpreaParser::Block_statContext *ctx) override;
  std::any visitAssign_stat(GazpreaParser::Assign_statContext *ctx) override;
  std::any visitDec_stat(GazpreaParser::Dec_statContext *ctx) override;
  std::any
  visitTuple_dec_stat(GazpreaParser::Tuple_dec_statContext *ctx) override;
  std::any visitTuple_type(GazpreaParser::Tuple_typeContext *ctx) override;
  std::any visitType_list(GazpreaParser::Type_listContext *ctx) override;
  std::any visitType(GazpreaParser::TypeContext *ctx) override;
  std::any visitQualifier(GazpreaParser::QualifierContext *ctx) override;
  std::any visitPowerExpr(GazpreaParser::PowerExprContext *ctx) override;
  std::any visitCastExpr(GazpreaParser::CastExprContext *ctx) override;
  std::any visitLogicalExpr(GazpreaParser::LogicalExprContext *ctx) override;
  std::any visitBoolLiteral(GazpreaParser::BoolLiteralContext *ctx) override;
  std::any visitParenExpr(GazpreaParser::ParenExprContext *ctx) override;
  std::any visitUnaryExpr(GazpreaParser::UnaryExprContext *ctx) override;
  std::any visitFloatLiteral(GazpreaParser::FloatLiteralContext *ctx) override;
  std::any visitAppendExpr(GazpreaParser::AppendExprContext *ctx) override;
  std::any
  visitTupleAccessExpr(GazpreaParser::TupleAccessExprContext *ctx) override;
  std::any visitIdentifier(GazpreaParser::IdentifierContext *ctx) override;
  std::any visitAddSubExpr(GazpreaParser::AddSubExprContext *ctx) override;
  std::any visitIntLiteral(GazpreaParser::IntLiteralContext *ctx) override;
  std::any visitScientificFloatLiteral(
      GazpreaParser::ScientificFloatLiteralContext *ctx) override;
  std::any
  visitDotFloatLiteral(GazpreaParser::DotFloatLiteralContext *ctx) override;
  std::any visitByExpr(GazpreaParser::ByExprContext *ctx) override;
  std::any visitCharLiteral(GazpreaParser::CharLiteralContext *ctx) override;
  std::any
  visitRelationalExpr(GazpreaParser::RelationalExprContext *ctx) override;
  std::any
  visitFloatDotLiteral(GazpreaParser::FloatDotLiteralContext *ctx) override;
  std::any visitTupleLiteral(GazpreaParser::TupleLiteralContext *ctx) override;
  std::any visitMulDivRemDstarExpr(
      GazpreaParser::MulDivRemDstarExprContext *ctx) override;
  std::any
  visitStringLiteral(GazpreaParser::StringLiteralContext *ctx) override;
  std::any visitFuncProcExpr(GazpreaParser::FuncProcExprContext *ctx) override;
  std::any visitRangeExpr(GazpreaParser::RangeExprContext *ctx) override;
  std::any visitEqualityExpr(GazpreaParser::EqualityExprContext *ctx) override;
  std::any visitAndExpr(GazpreaParser::AndExprContext *ctx) override;
  std::any visitTuple_lit(GazpreaParser::Tuple_litContext *ctx) override;

private:
  expressions::BinaryOpType stringToBinaryOpType(const std::string &op);
  std::any createBinaryExpr(antlr4::tree::ParseTree *leftCtx,
                            const std::string &op,
                            antlr4::tree::ParseTree *rightCtx,
                            antlr4::Token *token);
};
} // namespace gazprea::ast::walkers