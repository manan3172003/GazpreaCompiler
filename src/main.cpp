#include "GazpreaLexer.h"
#include "GazpreaParser.h"

#include "ANTLRFileStream.h"
#include "CommonTokenStream.h"
#include "tree/ParseTree.h"
#include "tree/ParseTreeWalker.h"

#include "BackEnd.h"
#include "ast/RootAst.h"
#include "ast/statements/DeclarationAst.h"
#include "ast/walkers/AstBuilder.h"
#include "ast/walkers/DefineWalker.h"
#include "ast/walkers/ResolveWalker.h"
#include "ast/walkers/TypeWalker.h"

#include <fstream>
#include <iostream>

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cout << "Missing required argument.\n"
              << "Required arguments: <input file path> <output file path>\n";
    return 1;
  }

  // Open the file then parse and lex it.
  antlr4::ANTLRFileStream afs;
  afs.loadFromFile(argv[1]);
  gazprea::GazpreaLexer lexer(&afs);
  antlr4::CommonTokenStream tokens(&lexer);
  gazprea::GazpreaParser parser(&tokens);

  antlr4::tree::ParseTree *tree = parser.file();

  gazprea::ast::walkers::AstBuilder astBuilder;
  auto rootAst = std::any_cast<std::shared_ptr<gazprea::ast::RootAst>>(
      astBuilder.visit(tree));

  std::cout << rootAst->toStringTree("") << std::endl;

  auto symTab = std::make_shared<gazprea::symTable::SymbolTable>();
  gazprea::ast::walkers::DefineWalker defineWalker(symTab);
  defineWalker.visit(rootAst);

  std::cout << rootAst->toStringTree("") << std::endl;

  gazprea::ast::walkers::ResolveWalker resolveWalker(symTab);
  resolveWalker.visit(rootAst);

  std::cout << rootAst->toStringTree("") << std::endl;

  gazprea::ast::walkers::TypeWalker typeInferPromoWalker(symTab);
  typeInferPromoWalker.visit(rootAst);

  std::ofstream os(argv[2]);
  // BackEnd backend;
  // backend.emitModule();
  // backend.lowerDialects();
  // backend.dumpLLVM(os);

  return 0;
}
