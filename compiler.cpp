//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
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

#include "compiler.h"
using namespace llvm;

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
using namespace llvm;


/////////////////////// end of includes

// globals

 FunctionPassManager* TheFPM = 0;
 ExecutionEngine*     TheExecutionEngine = 0;

//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.

 std::string IdentifierStr;  // Filled in if tok_identifier
 double NumVal;              // Filled in if tok_number


/// gettok - Return the next token from standard input.
 int gettok() {
   int LastChar = ' ';

  // Skip any whitespace.
  while (isspace(LastChar))
    LastChar = getchar();

  if (isalpha(LastChar)) { // identifier: [a-zA-Z][a-zA-Z0-9]*
    IdentifierStr = LastChar;
    while (isalnum((LastChar = getchar())))
      IdentifierStr += LastChar;

    if (IdentifierStr == "def") return tok_def;
    if (IdentifierStr == "extern") return tok_extern;
    return tok_identifier;
  }

  if (isdigit(LastChar) || LastChar == '.') {   // Number: [0-9.]+
    std::string NumStr;
    do {
      NumStr += LastChar;
      LastChar = getchar();
    } while (isdigit(LastChar) || LastChar == '.');

    NumVal = strtod(NumStr.c_str(), 0);
    return tok_number;
  }

  if (LastChar == '#') {
    // Comment until end of line.
    do LastChar = getchar();
    while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');
    
    if (LastChar != EOF)
      return gettok();
  }
  
  // Check for end of file.  Don't eat the EOF.
  if (LastChar == EOF)
    return tok_eof;

  // Otherwise, just return the character as its ascii value.
  int ThisChar = LastChar;
  LastChar = getchar();
  return ThisChar;
}

//===----------------------------------------------------------------------===//
// Abstract Syntax Tree (aka Parse Tree)
//===----------------------------------------------------------------------===//


//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.
 int CurTok;
 int getNextToken() {
  return CurTok = gettok();
}

/// BinopPrecedence - This holds the precedence for each binary operator that is
/// defined.
 std::map<char, int> BinopPrecedence;

/// GetTokPrecedence - Get the precedence of the pending binary operator token.
 int GetTokPrecedence() {
  if (!isascii(CurTok))
    return -1;
  
  // Make sure it's a declared binop.
  int TokPrec = BinopPrecedence[CurTok];
  if (TokPrec <= 0) return -1;
  return TokPrec;
}

/// Error* - These are little helper functions for error handling.
ExprAST *Error(const char *Str) { fprintf(stderr, "Error: %s\n", Str);return 0;}
PrototypeAST *ErrorP(const char *Str) { Error(Str); return 0; }
FunctionAST *ErrorF(const char *Str) { Error(Str); return 0; }

 ExprAST *ParseExpression();

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
 ExprAST *ParseIdentifierExpr() {
  std::string IdName = IdentifierStr;
  
  getNextToken();  // eat identifier.
  
  if (CurTok != '(') // Simple variable ref.
    return new VariableExprAST(IdName);
  
  // Call.
  getNextToken();  // eat (
  std::vector<ExprAST*> Args;
  if (CurTok != ')') {
    while (1) {
      ExprAST *Arg = ParseExpression();
      if (!Arg) return 0;
      Args.push_back(Arg);

      if (CurTok == ')') break;

      if (CurTok != ',')
        return Error("Expected ')' or ',' in argument list");
      getNextToken();
    }
  }

  // Eat the ')'.
  getNextToken();
  
  return new CallExprAST(IdName, Args);
}

/// numberexpr ::= number
 ExprAST *ParseNumberExpr() {
  ExprAST *Result = new NumberExprAST(NumVal);
  getNextToken(); // consume the number
  return Result;
}

/// parenexpr ::= '(' expression ')'
 ExprAST *ParseParenExpr() {
  getNextToken();  // eat (.
  ExprAST *V = ParseExpression();
  if (!V) return 0;
  
  if (CurTok != ')')
    return Error("expected ')'");
  getNextToken();  // eat ).
  return V;
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
 ExprAST *ParsePrimary() {
  switch (CurTok) {
  default: return Error("unknown token when expecting an expression");
  case tok_identifier: return ParseIdentifierExpr();
  case tok_number:     return ParseNumberExpr();
  case '(':            return ParseParenExpr();
  }
}

/// binoprhs
///   ::= ('+' primary)*
 ExprAST *ParseBinOpRHS(int ExprPrec, ExprAST *LHS) {
  // If this is a binop, find its precedence.
  while (1) {
    int TokPrec = GetTokPrecedence();
    
    // If this is a binop that binds at least as tightly as the current binop,
    // consume it, otherwise we are done.
    if (TokPrec < ExprPrec)
      return LHS;
    
    // Okay, we know this is a binop.
    int BinOp = CurTok;
    getNextToken();  // eat binop
    
    // Parse the primary expression after the binary operator.
    ExprAST *RHS = ParsePrimary();
    if (!RHS) return 0;
    
    // If BinOp binds less tightly with RHS than the operator after RHS, let
    // the pending operator take RHS as its LHS.
    int NextPrec = GetTokPrecedence();
    if (TokPrec < NextPrec) {
      RHS = ParseBinOpRHS(TokPrec+1, RHS);
      if (RHS == 0) return 0;
    }
    
    // Merge LHS/RHS.
    LHS = new BinaryExprAST(BinOp, LHS, RHS);
  }
}

/// expression
///   ::= primary binoprhs
///
 ExprAST *ParseExpression() {
  ExprAST *LHS = ParsePrimary();
  if (!LHS) return 0;
  
  return ParseBinOpRHS(0, LHS);
}

/// prototype
///   ::= id '(' id* ')'
 PrototypeAST *ParsePrototype() {
  if (CurTok != tok_identifier)
    return ErrorP("Expected function name in prototype");

  std::string FnName = IdentifierStr;
  getNextToken();
  
  if (CurTok != '(')
    return ErrorP("Expected '(' in prototype");
  
  std::vector<std::string> ArgNames;
  while (getNextToken() == tok_identifier)
    ArgNames.push_back(IdentifierStr);
  if (CurTok != ')')
    return ErrorP("Expected ')' in prototype");
  
  // success.
  getNextToken();  // eat ')'.
  
  return new PrototypeAST(FnName, ArgNames);
}

/// definition ::= 'def' prototype expression
 FunctionAST *ParseDefinition() {
  getNextToken();  // eat def.
  PrototypeAST *Proto = ParsePrototype();
  if (Proto == 0) return 0;

  if (ExprAST *E = ParseExpression())
    return new FunctionAST(Proto, E);
  return 0;
}

/// toplevelexpr ::= expression
 FunctionAST *ParseTopLevelExpr() {
  if (ExprAST *E = ParseExpression()) {
    // Make an anonymous proto.
    PrototypeAST *Proto = new PrototypeAST("", std::vector<std::string>());
    return new FunctionAST(Proto, E);
  }
  return 0;
}

/// external ::= 'extern' prototype
 PrototypeAST *ParseExtern() {
  getNextToken();  // eat extern.
  return ParsePrototype();
}

//===----------------------------------------------------------------------===//
// Code Generation
//===----------------------------------------------------------------------===//

llvm::Module *TheModule;
 IRBuilder<> Builder(getGlobalContext());
 std::map<std::string, Value*> NamedValues;

Value *ErrorV(const char *Str) { Error(Str); return 0; }

Value *NumberExprAST::Codegen() {
  return ConstantFP::get(getGlobalContext(), APFloat(Val));
}

Value *VariableExprAST::Codegen() {
  // Look this variable up in the function.
  Value *V = NamedValues[Name];
  return V ? V : ErrorV("Unknown variable name");
}

Value *BinaryExprAST::Codegen() {
  Value *L = LHS->Codegen();
  Value *R = RHS->Codegen();
  if (L == 0 || R == 0) return 0;
  
  switch (Op) {
  case '+': return Builder.CreateFAdd(L, R, "addtmp");
  case '-': return Builder.CreateFSub(L, R, "subtmp");
  case '*': return Builder.CreateFMul(L, R, "multmp");
  case '<':
    L = Builder.CreateFCmpULT(L, R, "cmptmp");
    // Convert bool 0/1 to double 0.0 or 1.0
    return Builder.CreateUIToFP(L, Type::getDoubleTy(getGlobalContext()),
                                "booltmp");
  default: return ErrorV("invalid binary operator");
  }
}

Value *CallExprAST::Codegen() {
  // Look up the name in the global module table.
  Function *CalleeF = TheModule->getFunction(Callee);
  if (CalleeF == 0)
    return ErrorV("Unknown function referenced");
  
  // If argument mismatch error.
  if (CalleeF->arg_size() != Args.size())
    return ErrorV("Incorrect # arguments passed");

  std::vector<Value*> ArgsV;
  for (unsigned i = 0, e = Args.size(); i != e; ++i) {
    ArgsV.push_back(Args[i]->Codegen());
    if (ArgsV.back() == 0) return 0;
  }
  
  return Builder.CreateCall(CalleeF, ArgsV.begin(), ArgsV.end(), "calltmp");
}

Function *PrototypeAST::Codegen(Function*& preexistingPrototype /*out*/) {
  preexistingPrototype = NULL;

  // Make the function type:  double(double,double) etc.
  std::vector<const Type*> Doubles(Args.size(),
                                   Type::getDoubleTy(getGlobalContext()));
  FunctionType *FT = FunctionType::get(Type::getDoubleTy(getGlobalContext()),
                                       Doubles, false);
  
  Function *F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule);

  
  // If F conflicted, there was already something named 'Name'.  If it has a
  // body, don't allow redefinition or reextern.
  if (F->getName() != Name) {

    // Delete the one we just made and get the existing one.
    F->eraseFromParent();
    F = TheModule->getFunction(Name);

    // If F already has a body, reject this.
    if (!F->empty()) {
      ErrorF("redefinition of function");
      return 0;
    }
    
    // If F took a different number of args, reject.
    if (F->arg_size() != Args.size()) {
      ErrorF("redefinition of function with different # args");
      return 0;
    }

    // if there is a preexisting prototype only -- without a body -- and with matching args
    // then inform caller of this, so that the FunctionAST::Codegen can not accidentally
    // delete it on body compilation failure.
    preexistingPrototype = F;
    //    printf("There was a pre-existing prototype for this function-- noting this fact: %p\n",preexistingPrototype);
  }
  
  // Set names for all arguments.
  unsigned Idx = 0;
  for (Function::arg_iterator AI = F->arg_begin(); Idx != Args.size();
       ++AI, ++Idx) {
    AI->setName(Args[Idx]);
    
    // Add arguments to variable symbol table.
    NamedValues[Args[Idx]] = AI;
  }
  
  return F;
}

Function *FunctionAST::Codegen() {
  //printf("starting FunctionAST::Codegen()\n");

  NamedValues.clear();

  // was there an pre-existing and matching (in terms of # args and without a body) prototype? 
  // If so, then we'll find that preexistingPrototype == TheFunction, otherwise preexistingPrototype
  // will stay NULL.
  Function *preexistingPrototype = NULL;
  Function *TheFunction = Proto->Codegen(preexistingPrototype);
  //printf("preexistingPrototype after codegen for FunctionAST is: %p\n",preexistingPrototype);
  if (TheFunction == 0)
    return 0;
  
  // Create a new basic block to start insertion into.
  BasicBlock *BB = BasicBlock::Create(getGlobalContext(), "entry", TheFunction);
  Builder.SetInsertPoint(BB);
  
  if (Value *RetVal = Body->Codegen()) {
    // Finish off the function.
    Builder.CreateRet(RetVal);

    // Validate the generated code, checking for consistency.
    verifyFunction(*TheFunction);

    // Optimize the function.
    TheFPM->run(*TheFunction);

    return TheFunction;
  }
  
  // Error reading body, remove function.
  if (!preexistingPrototype) {
    printf("erasing failed function compilation from parent because no pre-existing prototype!\n");
    TheFunction->eraseFromParent();
  }
  return 0;
}

//===----------------------------------------------------------------------===//
// Top-Level parsing and JIT Driver
//===----------------------------------------------------------------------===//

 void HandleDefinition() {
  if (FunctionAST *F = ParseDefinition()) {
    if (Function *LF = F->Codegen()) {
      fprintf(stderr, "Read function definition:");
      LF->dump();
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

 void HandleExtern() {
  if (PrototypeAST *P = ParseExtern()) {
    Function* dontCare = 0;
    if (Function *F = P->Codegen(dontCare)) {
      fprintf(stderr, "Read extern: ");
      F->dump();
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}


 void HandleTopLevelExpression() {
  // Evaluate a top-level expression into an anonymous function.
  if (FunctionAST *F = ParseTopLevelExpr()) {
    if (Function *LF = F->Codegen()) {

      // JIT the function, returning a function pointer.
      void *FPtr = TheExecutionEngine->getPointerToFunction(LF);
      
      // Cast it to the right type (takes no arguments, returns a double) so we
      // can call it as a native function.
      double (*FP)() = (double (*)())(intptr_t)FPtr;
      fprintf(stderr, "Evaluated to %f\n", FP());
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}



/// top ::= definition | external | expression | ';'
 void MainLoop() {
  while (1) {
    fprintf(stderr, "ready> ");
    switch (CurTok) {
    case tok_eof:    return;
    case ';':        getNextToken(); break;  // ignore top-level semicolons.
    case tok_def:    HandleDefinition(); break;
    case tok_extern: HandleExtern(); break;
    default:         HandleTopLevelExpression(); break;
    }
  }
}

//===----------------------------------------------------------------------===//
// "Library" functions that can be "extern'd" from user code.
//===----------------------------------------------------------------------===//

/// putchard - putchar that takes a double and returns 0.
extern "C" 
double putchard(double X) {
  putchar((char)X);
  return 0;
}

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//


int compiler_init() {
  InitializeNativeTarget();
  LLVMContext &Context = getGlobalContext();

  // Install standard binary operators.
  // 1 is lowest precedence.
  BinopPrecedence['<'] = 10;
  BinopPrecedence['+'] = 20;
  BinopPrecedence['-'] = 20;
  BinopPrecedence['*'] = 40;  // highest.

  // Prime the first token.
  //  fprintf(stderr, "ready> ");
  //getNextToken();

  // Make the module, which holds all the code.
  TheModule = new Module("my cool jit", Context);

  // Create the JIT.  This takes ownership of the module.
  std::string ErrStr;
  llvm::EngineBuilder TheEngineBuilder = EngineBuilder(TheModule);
  TheEngineBuilder.setErrorStr(&ErrStr);
  TheExecutionEngine = TheEngineBuilder.create();
  if (!TheExecutionEngine) {
    fprintf(stderr, "Could not create ExecutionEngine: %s\n", ErrStr.c_str());
    assert(0);
    exit(1);
  }

  // Set the global so the code gen can use this.
  //
  TheFPM = new FunctionPassManager(TheModule);
  FunctionPassManager& OurFPM = *TheFPM;

  // Set up the optimizer pipeline.  Start with registering info about how the
  // target lays out data structures.
  OurFPM.add(new TargetData(*TheExecutionEngine->getTargetData()));
  // Do simple "peephole" optimizations and bit-twiddling optzns.
  OurFPM.add(createInstructionCombiningPass());
  // Reassociate expressions.
  OurFPM.add(createReassociatePass());
  // Eliminate Common SubExpressions.
  OurFPM.add(createGVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  OurFPM.add(createCFGSimplificationPass());

  OurFPM.doInitialization();


  // Run the main "interpreter loop" now.
  //  MainLoop();


  // Print out all of the generated code.
  //  TheModule->dump();

  return 0;
}

// cleanup
void compiler_teardown() {

    if (TheExecutionEngine) {
        delete TheExecutionEngine;
    }

    //    if (TheModule) {
    //        delete TheModule;
    //    }

    if (TheFPM) {
        delete TheFPM;
    }

}
