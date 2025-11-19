#pragma once
#include "GazpreaBaseVisitor.h"
#include "ast/expressions/BinaryAst.h"
#include "ast/expressions/ExpressionAst.h"

namespace gazprea::ast::walkers {

class AstBuilder : public GazpreaBaseVisitor {
  std::shared_ptr<types::DataTypeAst> makeType(GazpreaParser::TypeContext *typeContext,
                                               antlr4::Token *token);

public:
  std::any visitFile(GazpreaParser::FileContext *ctx) override;
  std::any visitGlobal_stat(GazpreaParser::Global_statContext *ctx) override;
  std::any visitTypealias_stat(GazpreaParser::Typealias_statContext *ctx) override;
  std::any visitStat(GazpreaParser::StatContext *ctx) override;
  std::any visitProcedure_stat(GazpreaParser::Procedure_statContext *ctx) override;
  std::any visitProcedure_params(GazpreaParser::Procedure_paramsContext *ctx) override;
  std::any visitProcedure_param(GazpreaParser::Procedure_paramContext *ctx) override;
  std::any visitProcedure_call_stat(GazpreaParser::Procedure_call_statContext *ctx) override;
  std::any visitFunction_stat(GazpreaParser::Function_statContext *ctx) override;
  std::any visitFunction_params(GazpreaParser::Function_paramsContext *ctx) override;
  std::any visitFunction_param(GazpreaParser::Function_paramContext *ctx) override;
  std::any visitArgs(GazpreaParser::ArgsContext *ctx) override;
  std::any visitOutput_stat(GazpreaParser::Output_statContext *ctx) override;
  std::any visitInput_stat(GazpreaParser::Input_statContext *ctx) override;
  std::any visitReturn_stat(GazpreaParser::Return_statContext *ctx) override;
  std::any visitIf_stat(GazpreaParser::If_statContext *ctx) override;
  std::any visitElse_stat(GazpreaParser::Else_statContext *ctx) override;
  std::any visitInfiniteLoop(GazpreaParser::InfiniteLoopContext *ctx) override;
  std::any visitPrePredicatedLoop(GazpreaParser::PrePredicatedLoopContext *ctx) override;
  std::any visitPostPredicatedLoop(GazpreaParser::PostPredicatedLoopContext *ctx) override;
  std::any visitSingularAssign(GazpreaParser::SingularAssignContext *ctx) override;
  std::any visitTupleUnpackAssign(GazpreaParser::TupleUnpackAssignContext *ctx) override;
  std::any visitIdLVal(GazpreaParser::IdLValContext *ctx) override;
  std::any visitTupleElementLVal(GazpreaParser::TupleElementLValContext *ctx) override;
  std::any visitBlock_stat(GazpreaParser::Block_statContext *ctx) override;
  std::any visitIterativeLoop(GazpreaParser::IterativeLoopContext *ctx) override;
  std::any visitDec_stat(GazpreaParser::Dec_statContext *ctx) override;
  std::any visitTuple_type(GazpreaParser::Tuple_typeContext *ctx) override;
  std::any visitType_list(GazpreaParser::Type_listContext *ctx) override;
  std::any visitQualifier(GazpreaParser::QualifierContext *ctx) override;
  std::any visitPowerExpr(GazpreaParser::PowerExprContext *ctx) override;
  std::any visitCastExpr(GazpreaParser::CastExprContext *ctx) override;
  std::any visitLogicalExpr(GazpreaParser::LogicalExprContext *ctx) override;
  std::any visitBoolLiteral(GazpreaParser::BoolLiteralContext *ctx) override;
  std::any visitParenExpr(GazpreaParser::ParenExprContext *ctx) override;
  std::any visitUnaryExpr(GazpreaParser::UnaryExprContext *ctx) override;
  std::any visitFloatLiteral(GazpreaParser::FloatLiteralContext *ctx) override;
  std::any visitAppendExpr(GazpreaParser::AppendExprContext *ctx) override;
  std::any visitTupleAccessExpr(GazpreaParser::TupleAccessExprContext *ctx) override;
  std::any visitIdentifier(GazpreaParser::IdentifierContext *ctx) override;
  std::any visitAddSubExpr(GazpreaParser::AddSubExprContext *ctx) override;
  std::any visitIntLiteral(GazpreaParser::IntLiteralContext *ctx) override;
  std::any visitScientificFloatLiteral(GazpreaParser::ScientificFloatLiteralContext *ctx) override;
  std::any visitByExpr(GazpreaParser::ByExprContext *ctx) override;
  std::any visitCharLiteral(GazpreaParser::CharLiteralContext *ctx) override;
  std::any visitRelationalExpr(GazpreaParser::RelationalExprContext *ctx) override;
  std::any visitTupleLiteral(GazpreaParser::TupleLiteralContext *ctx) override;
  std::any visitStringLiteral(GazpreaParser::StringLiteralContext *ctx) override;
  std::any visitFuncProcExpr(GazpreaParser::FuncProcExprContext *ctx) override;
  std::any visitEqualityExpr(GazpreaParser::EqualityExprContext *ctx) override;
  std::any visitAndExpr(GazpreaParser::AndExprContext *ctx) override;
  std::any visitTuple_lit(GazpreaParser::Tuple_litContext *ctx) override;

  // Part 2 refactor
  std::any visitBuilt_in_stat(GazpreaParser::Built_in_statContext *ctx) override;
  std::any visitStructFieldLVal(GazpreaParser::StructFieldLValContext *ctx) override;
  std::any visitArrayElementLVal(GazpreaParser::ArrayElementLValContext *ctx) override;
  std::any visitTwoDimArrayElementLVal(GazpreaParser::TwoDimArrayElementLValContext *ctx) override;
  std::any visitVector_type(GazpreaParser::Vector_typeContext *ctx) override;
  std::any visitStruct_type(GazpreaParser::Struct_typeContext *ctx) override;
  std::any visitField_list(GazpreaParser::Field_listContext *ctx) override;
  std::any visitField(GazpreaParser::FieldContext *ctx) override;
  std::any visitCharType(GazpreaParser::CharTypeContext *ctx) override;
  std::any visitBooleanType(GazpreaParser::BooleanTypeContext *ctx) override;
  std::any visitRealType(GazpreaParser::RealTypeContext *ctx) override;
  std::any visitVectorType(GazpreaParser::VectorTypeContext *ctx) override;
  std::any visitTupType(GazpreaParser::TupTypeContext *ctx) override;
  std::any visitTwoDimArray(GazpreaParser::TwoDimArrayContext *ctx) override;
  std::any visitStructType(GazpreaParser::StructTypeContext *ctx) override;
  std::any visitTwoDimArrayAlt(GazpreaParser::TwoDimArrayAltContext *ctx) override;
  std::any visitIntType(GazpreaParser::IntTypeContext *ctx) override;
  std::any visitStringType(GazpreaParser::StringTypeContext *ctx) override;
  std::any visitAliasType(GazpreaParser::AliasTypeContext *ctx) override;
  std::any visitOneDimArray(GazpreaParser::OneDimArrayContext *ctx) override;
  std::any visitArrayLiteral(GazpreaParser::ArrayLiteralContext *ctx) override;
  std::any visitMatrixLiteral(GazpreaParser::MatrixLiteralContext *ctx) override;
  std::any visitFormatExpr(GazpreaParser::FormatExprContext *ctx) override;
  std::any visitStructAccessExpr(GazpreaParser::StructAccessExprContext *ctx) override;
  std::any visitMulDivRemExpr(GazpreaParser::MulDivRemExprContext *ctx) override;
  std::any visitLengthExpr(GazpreaParser::LengthExprContext *ctx) override;
  std::any visitReverseExpr(GazpreaParser::ReverseExprContext *ctx) override;
  std::any visitArrayAccessExpr(GazpreaParser::ArrayAccessExprContext *ctx) override;
  std::any visitGeneratorExpr(GazpreaParser::GeneratorExprContext *ctx) override;
  std::any visitBuiltinFuncExpr(GazpreaParser::BuiltinFuncExprContext *ctx) override;
  std::any visitDstarExpr(GazpreaParser::DstarExprContext *ctx) override;
  std::any visitShapeExpr(GazpreaParser::ShapeExprContext *ctx) override;
  std::any visitSliceRangeExpr(GazpreaParser::SliceRangeExprContext *ctx) override;
  std::any visitSliceEndExpr(GazpreaParser::SliceEndExprContext *ctx) override;
  std::any visitSliceStartExpr(GazpreaParser::SliceStartExprContext *ctx) override;
  std::any visitSliceAllExpr(GazpreaParser::SliceAllExprContext *ctx) override;
  std::any visitSingleIndexExpr(GazpreaParser::SingleIndexExprContext *ctx) override;
  std::any visitArray_lit(GazpreaParser::Array_litContext *ctx) override;
  std::any visitMatrix_lit(GazpreaParser::Matrix_litContext *ctx) override;
  std::any visitArray_elements(GazpreaParser::Array_elementsContext *ctx) override;
  std::any visitTuple_elements(GazpreaParser::Tuple_elementsContext *ctx) override;
  std::any visitScientificFloat(GazpreaParser::ScientificFloatContext *ctx) override;
  std::any
  visitScientificFloatNoDecimal(GazpreaParser::ScientificFloatNoDecimalContext *ctx) override;
  std::any visitDotFloat(GazpreaParser::DotFloatContext *ctx) override;
  std::any visitFloatDot(GazpreaParser::FloatDotContext *ctx) override;
  std::any visitDot_float(GazpreaParser::Dot_floatContext *ctx) override;
  std::any visitFloat_dot(GazpreaParser::Float_dotContext *ctx) override;
  std::any visitFloat_lit(GazpreaParser::Float_litContext *ctx) override;

private:
  static expressions::BinaryOpType stringToBinaryOpType(const std::string &op);
  std::any createBinaryExpr(antlr4::tree::ParseTree *leftCtx, const std::string &op,
                            antlr4::tree::ParseTree *rightCtx, antlr4::Token *token);
  char convertStringToChar(const std::string &str, int lineNumber);
  int getInt(std::string str, int lineNumber);
  float getFloat(std::string str, int lineNumber);
};
} // namespace gazprea::ast::walkers