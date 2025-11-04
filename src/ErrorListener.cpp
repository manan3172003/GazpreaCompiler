#include "CompileTimeExceptions.h"
#include "GazpreaLexer.h"

#include <ErrorListener.h>
#include <Parser.h>
#include <Token.h>

namespace gazprea {
void ErrorListener::syntaxError(antlr4::Recognizer *recognizer, antlr4::Token *offendingSymbol,
                                size_t line, size_t charPositionInLine, const std::string &msg,
                                std::exception_ptr e) {
  // Check for common syntax errors based on message patterns
  if (msg.find("missing ';'") != std::string::npos) {
    throw SyntaxError(line, "Missing semicolon ';' - expected ';' at the end of statement");
  }
  if (msg.find("extraneous input") != std::string::npos) {
    std::string tokenText = offendingSymbol ? offendingSymbol->getText() : "unknown";
    if (tokenText == "<EOF>") {
      throw SyntaxError(
          line,
          "Unexpected end of file - mismatched braces or parentheses detected earlier in the file");
    }
    throw SyntaxError(line, "Unexpected token '" + tokenText +
                                "' - found extra input that doesn't belong here");
  }
  if (msg.find("mismatched input") != std::string::npos) {
    std::string tokenText = offendingSymbol ? offendingSymbol->getText() : "unknown";
    throw SyntaxError(line, "Invalid syntax - unexpected syntax near '" + tokenText + "'");
  }
  if (msg.find("no viable alternative") != std::string::npos) {
    std::string tokenText = offendingSymbol ? offendingSymbol->getText() : "unknown";
    throw SyntaxError(line, "Invalid syntax at '" + tokenText + "' - unable to parse statement");
  }

  const std::vector<std::string> rule_stack =
      static_cast<antlr4::Parser *>(recognizer)->getRuleInvocationStack();

  switch (offendingSymbol->getType()) {
  case GazpreaLexer::FUNCTION: {
    // Check if function was declared somewhere except global scope
    for (const auto &rule : rule_stack) {
      if (rule == "procedure_stat" || rule == "block_stat" || rule == "function_stat") {
        throw StatementError(line, "Function declarations are only allowed in global scope");
      }
    }
    break;
  }
  case GazpreaLexer::PROCEDURE: {
    // Check if procedure was declared somewhere except global scope
    for (const auto &rule : rule_stack) {
      if (rule == "procedure_stat" || rule == "block_stat" || rule == "function_stat") {
        throw StatementError(line, "Procedure declarations are only allowed in global scope");
      }
    }
    break;
  }
  // TODO: Implement syntax error checks for other statements
  case GazpreaLexer::TYPEALIAS: {
    // Check if type alias was declared somewhere except global scope
    for (const auto &rule : rule_stack) {
      if (rule == "procedure_stat" || rule == "block_stat" || rule == "function_stat") {
        throw StatementError(line, "Type alias declarations are only allowed in global scope");
      }
    }
    break;
  }

  case GazpreaLexer::RETURN: {
    // Check if return is used outside function/procedure
    bool inFuncProc = false;
    for (const auto &rule : rule_stack) {
      if (rule == "procedure_stat" || rule == "function_stat") {
        inFuncProc = true;
        break;
      }
    }
    if (!inFuncProc) {
      throw StatementError(line, "Return statement used outside of function or procedure");
    }
    break;
  }

  default:;
  }

  throw SyntaxError(line, msg);
}
} // namespace gazprea