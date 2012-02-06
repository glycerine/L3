/*   # Compile
// old   g++ -g -O3 toy.cpp `llvm-config --cppflags --ldflags --libs core` -o toy

    g++ -g toy.cpp `llvm-config --cppflags --ldflags --libs core jit native` -rdynamic -O3 -o toy
   # Run

   # Run
   ./toy

Here is the code:
*/

// To build this:
// See example below.

#include "llvm/DerivedTypes.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetSelect.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Support/IRBuilder.h"
#include <cstdio>
#include <string>
#include <map>
#include <vector>
//using namespace llvm;

// globals

extern llvm::FunctionPassManager* TheFPM;
extern llvm::ExecutionEngine*     TheExecutionEngine;

// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.
enum Token {
  tok_eof = -1,

  // commands
  tok_def = -2, tok_extern = -3,

  // primary
  tok_identifier = -4, tok_number = -5
};

extern std::string IdentifierStr;  // Filled in if tok_identifier
extern double NumVal;              // Filled in if tok_number

/// gettok - Return the next token from standard input.
 int gettok();

//===----------------------------------------------------------------------===//
// Abstract Syntax Tree (aka Parse Tree)
//===----------------------------------------------------------------------===//

/// ExprAST - Base class for all expression nodes.
class ExprAST {
public:
  virtual ~ExprAST() {}
  virtual llvm::Value *Codegen() = 0;
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
  double Val;
public:
  NumberExprAST(double val) : Val(val) {}
  virtual llvm::Value *Codegen();
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
  std::string Name;
public:
  VariableExprAST(const std::string &name) : Name(name) {}
  virtual llvm::Value *Codegen();
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
  char Op;
  ExprAST *LHS, *RHS;
public:
  BinaryExprAST(char op, ExprAST *lhs, ExprAST *rhs) 
    : Op(op), LHS(lhs), RHS(rhs) {}
  virtual llvm::Value *Codegen();
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
  std::string Callee;
  std::vector<ExprAST*> Args;
public:
  CallExprAST(const std::string &callee, std::vector<ExprAST*> &args)
    : Callee(callee), Args(args) {}
  virtual llvm::Value *Codegen();
};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;
public:
  PrototypeAST(const std::string &name, const std::vector<std::string> &args)
    : Name(name), Args(args) {}
  
  //  llvm::Function *Codegen();
  llvm::Function *Codegen(llvm::Function*& preexistingPrototype /*out*/);
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST {
  PrototypeAST *Proto;
  ExprAST *Body;
public:
  FunctionAST(PrototypeAST *proto, ExprAST *body)
    : Proto(proto), Body(body) {}
  
  llvm::Function *Codegen();
};

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.
extern int CurTok;
 int getNextToken();

/// BinopPrecedence - This holds the precedence for each binary operator that is
/// defined.
extern std::map<char, int> BinopPrecedence;

/// GetTokPrecedence - Get the precedence of the pending binary operator token.
 int GetTokPrecedence();

/// Error* - These are little helper functions for error handling.
ExprAST *Error(const char *Str);
PrototypeAST *ErrorP(const char *Str);
FunctionAST *ErrorF(const char *Str);

 ExprAST *ParseExpression();

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
 ExprAST *ParseIdentifierExpr();

/// numberexpr ::= number
 ExprAST *ParseNumberExpr();

/// parenexpr ::= '(' expression ')'
 ExprAST *ParseParenExpr();

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
 ExprAST *ParsePrimary();

/// binoprhs
///   ::= ('+' primary)*
 ExprAST *ParseBinOpRHS(int ExprPrec, ExprAST *LHS);

/// expression
///   ::= primary binoprhs
///
 ExprAST *ParseExpression();

/// prototype
///   ::= id '(' id* ')'
 PrototypeAST *ParsePrototype();

/// definition ::= 'def' prototype expression
 FunctionAST *ParseDefinition();

/// toplevelexpr ::= expression
 FunctionAST *ParseTopLevelExpr();

/// external ::= 'extern' prototype
 PrototypeAST *ParseExtern();

//===----------------------------------------------------------------------===//
// Code Generation
//===----------------------------------------------------------------------===//

extern llvm::Module *TheModule;
extern llvm::IRBuilder<> Builder;
extern std::map<std::string, llvm::Value*> NamedValues;

llvm::Value *ErrorV(const char *Str);


//===----------------------------------------------------------------------===//
// Top-Level parsing and JIT Driver
//===----------------------------------------------------------------------===//

 void HandleDefinition();
 void HandleExtern();
 void HandleTopLevelExpression();

/// top ::= definition | external | expression | ';'
 void MainLoop();

//===----------------------------------------------------------------------===//
// "Library" functions that can be "extern'd" from user code.
//===----------------------------------------------------------------------===//

/// putchard - putchar that takes a double and returns 0.
extern "C" 
double putchard(double X);

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

int compiler_init();

void compiler_teardown();
