#include "ast/expressions/StructFuncCallRouterAst.h"

namespace gazprea::ast::expressions {

void StructFuncCallRouterAst::setIsStruct(bool isStructCall) { isStruct = isStructCall; }
void StructFuncCallRouterAst::setCallName(const std::string &callId) { callName = callId; }
void StructFuncCallRouterAst::setFuncProcCallAst(
    const std::shared_ptr<FuncProcCallAst> &createdFuncProcCallAst) {
  callName = createdFuncProcCallAst->getName();
  isStruct = false;

  funcProcCallAst = createdFuncProcCallAst;
  structLiteralAst = nullptr;
}
void StructFuncCallRouterAst::setStructLiteralAst(
    const std::shared_ptr<StructLiteralAst> &createdStructLiteralAst) {
  isStruct = true;
  structLiteralAst = createdStructLiteralAst;
  funcProcCallAst = nullptr;
}

bool StructFuncCallRouterAst::getIsStruct() const { return isStruct; }
std::string StructFuncCallRouterAst::getCallName() const { return callName; }
std::shared_ptr<FuncProcCallAst> StructFuncCallRouterAst::getFuncProcCallAst() const {
  return funcProcCallAst;
}
std::shared_ptr<StructLiteralAst> StructFuncCallRouterAst::getStructLiteralAst() const {
  return structLiteralAst;
}

NodeType StructFuncCallRouterAst::getNodeType() const { return NodeType::StructFuncCallRouter; }
std::string StructFuncCallRouterAst::toStringTree(std::string prefix) const {
  if (isStruct)
    return structLiteralAst->toStringTree(prefix);
  return funcProcCallAst->toStringTree(prefix);
}
bool StructFuncCallRouterAst::isLValue() {
  if (isStruct)
    return structLiteralAst->isLValue();

  return funcProcCallAst->isLValue();
}

} // namespace gazprea::ast::expressions