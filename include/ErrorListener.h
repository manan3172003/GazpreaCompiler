#pragma once
#include <BaseErrorListener.h>

namespace gazprea {
class ErrorListener final : public antlr4::BaseErrorListener {
  void syntaxError(antlr4::Recognizer *recognizer, antlr4::Token *offendingSymbol, size_t line,
                   size_t charPositionInLine, const std::string &msg,
                   std::exception_ptr e) override;
};
} // namespace gazprea