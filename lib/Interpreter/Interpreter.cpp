//------------------------------------------------------------------------------
// CLING - the C++ LLVM-based InterpreterG :)
// version: $Id$
// author:  Lukasz Janyst <ljanyst@cern.ch>
//------------------------------------------------------------------------------

#include "cling/Interpreter/Interpreter.h"

#include "cling/Interpreter/CIBuilder.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/DeclGroup.h"
#include "clang/AST/Decl.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/CodeGen/ModuleBuilder.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/Utils.h"
#include "clang/Lex/HeaderSearch.h"
#include "clang/Lex/Pragma.h"
#include "clang/Lex/Preprocessor.h"
#include "llvm/Constants.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "Visitors.h"
#include "ClangUtils.h"
#include "ExecutionContext.h"
#include "IncrementalASTParser.h"
#include "InputValidator.h"
#include "ASTTransformVisitor.h"

#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

static const char* fake_argv[] = { "clang", "-x", "c++", "-D__CLING__", "-I.", 0 };
static const int fake_argc = (sizeof(fake_argv) / sizeof(const char*)) - 1;

namespace {
  static
  llvm::sys::Path
  findDynamicLibrary(const std::string& filename,
                     bool addPrefix = true,
                     bool addSuffix = true)
  {
    // Check wether filename is a dynamic library, either through absolute path
    // or in one of the system library paths.
    {
      llvm::sys::Path FullPath(filename);
      if (FullPath.isDynamicLibrary())
        return FullPath;
    }
    
    std::vector<llvm::sys::Path> LibPaths;
    llvm::sys::Path::GetSystemLibraryPaths(LibPaths);
    for (unsigned i = 0; i < LibPaths.size(); ++i) {
      llvm::sys::Path FullPath(LibPaths[i]);
      FullPath.appendComponent(filename);
      if (FullPath.isDynamicLibrary())
        return FullPath;
    }
    
    if (addPrefix) {
      static const std::string prefix("lib");
      llvm::sys::Path found = findDynamicLibrary(prefix + filename, false, addSuffix);
      if (found.isDynamicLibrary())
        return found;
    }
    
    if (addSuffix) {
      llvm::sys::Path found
      = findDynamicLibrary(filename + llvm::sys::Path::GetDLLSuffix().str(),
                           false, false);
      if (found.isDynamicLibrary())
        return found;
    }
    
    return llvm::sys::Path();
  }
  
}

namespace cling {

  //
  //  Interpreter
  //
  
  //---------------------------------------------------------------------------
  // Constructor
  //---------------------------------------------------------------------------
  Interpreter::Interpreter(const char* llvmdir /*= 0*/):
  m_CIBuilder(0),
  m_UniqueCounter(0),
  m_printAST(false),
  m_LastDump(0)
  {
    m_PragmaHandler = new clang::PragmaNamespace("cling");

    m_CIBuilder.reset(new CIBuilder(fake_argc, fake_argv, llvmdir));

    m_IncrASTParser.reset(new IncrementalASTParser(createCI(),
                                                   maybeGenerateASTPrinter(),
                                                   &getPragmaHandler()));
    m_ExecutionContext.reset(new ExecutionContext(*this));
    
    m_InputValidator.reset(new InputValidator(createCI()));

    m_ValuePrintStream.reset(new llvm::raw_os_ostream(std::cout));
    
    // Allow the interpreter to find itself.
    // OBJ first: if it exists it should be more up to date
    AddIncludePath(CLING_SRCDIR_INCL);
    AddIncludePath(CLING_INSTDIR_INCL);

    compileString(""); // Consume initialization.

    std::stringstream sstr;
    sstr << "#include <stdio.h>\n"
    << "#define __STDC_LIMIT_MACROS\n"
    << "#define __STDC_CONSTANT_MACROS\n"
    << "#include \"cling/Interpreter/Interpreter.h\"\n"
    << "#include \"cling/Interpreter/ValuePrinter.h\"\n";
    // Would like
    // namespace cling {Interpreter* gCling = (Interpreter*)0x875478643;"
    // but we can't handle namespaced decls yet :-(
    // sstr << "namespace cling {Interpreter* gCling = (Interpreter*)" << (void*) this << "; (void) gCling; \n"
    sstr << "cling::Interpreter* gCling = (cling::Interpreter*)"
         << (const void*) this << ";";
    compileString(sstr.str());
  }
  
  //---------------------------------------------------------------------------
  // Destructor
  //---------------------------------------------------------------------------
  Interpreter::~Interpreter()
  {
    //delete m_prev_module;
    //m_prev_module = 0; // Don't do this, the engine does it.
  }

  const char* Interpreter::getVersion() const {
    return "$Id$";
  }
   
  void Interpreter::AddIncludePath(const char *incpath)
  {
    // Add the given path to the list of directories in which the interpreter
    // looks for include files. Only one path item can be specified at a
    // time, i.e. "path1:path2" is not supported.
      
    clang::CompilerInstance* CI = getCI();
    clang::HeaderSearchOptions& headerOpts = CI->getHeaderSearchOpts();
    const bool IsUserSupplied = false;
    const bool IsFramework = false;
    const bool IsSysRootRelative = true;
    headerOpts.AddPath (incpath, clang::frontend::Angled, IsUserSupplied, IsFramework, IsSysRootRelative);
      
    clang::Preprocessor& PP = CI->getPreprocessor();
    clang::ApplyHeaderSearchOptions(PP.getHeaderSearchInfo(), headerOpts,
                                    PP.getLangOptions(),
                                    PP.getTargetInfo().getTriple());
      
  }
   
  clang::CompilerInstance*
  Interpreter::createCI() const
  {
    return m_CIBuilder->createCI();
  }
  
  clang::CompilerInstance*
  Interpreter::getCI() const
  {
    return m_IncrASTParser->getCI();
  }
  
  int
  Interpreter::processLine(const std::string& input_line)
  {
    //
    //  Transform the input line to implement cint
    //  command line semantics (declarations are global),
    //  and compile to produce a module.
    //
    
    std::string wrapped;
    std::string stmtFunc;
    if (strncmp(input_line.c_str(),"#include ",strlen("#include ")) != 0) {
      //
      //  Wrap input into a function along with
      //  the saved global declarations.
      //
      InputValidator::Result ValidatorResult = m_InputValidator->validate(input_line);
      if (ValidatorResult != InputValidator::kValid) {
          fprintf(stderr, "Bad input, dude! That's a code %d\n", ValidatorResult);
        return 0;
      }

      createWrappedSrc(input_line, wrapped, stmtFunc);
      if (!wrapped.size()) {
         return 0;
      }
    } else {
      wrapped = input_line;
    }
    
    //
    // Start the code generation on the old AST:
    //
    if (!m_ExecutionContext->startCodegen(m_IncrASTParser->getCI(),
                                          "Interpreter::processLine() input")) {
      fprintf(stderr, "Module creation failed!\n");
      return 0;
    }

    //
    //  Send the wrapped code through the
    //  frontend to produce a translation unit.
    //
    clang::CompilerInstance* CI = compileString(wrapped);
    if (!CI) {
      return 0;
    }
    // Note: We have a valid compiler instance at this point.
    clang::TranslationUnitDecl* tu =
      CI->getASTContext().getTranslationUnitDecl();
    if (!tu) { // Parse failed, return.
      fprintf(stderr, "Wrapped parse failed, no translation unit!\n");
      return 0;
    }
    //
    //  Send the translation unit through the
    //  llvm code generator to make a module.
    //
    if (!m_ExecutionContext->getModuleFromCodegen()) {
      fprintf(stderr, "Module creation failed!\n");
      return 0;
    }
    //
    //  Run it using the JIT.
    //
    if (!stmtFunc.empty())
      m_ExecutionContext->executeFunction(stmtFunc);
    return 1;
  }
  
  std::string Interpreter::createUniqueName()
  {
    // Create an unique name
    
    std::ostringstream swrappername;
    swrappername << "__cling_Un1Qu3" << m_UniqueCounter++;
    return swrappername.str();
  }
  
  
  
  void
  Interpreter::createWrappedSrc(const std::string& src, std::string& wrapped,
                                std::string& stmtFunc)
  {
    bool haveStatements = false;
    stmtFunc = createUniqueName();
    std::string stmtVsDeclFunc = stmtFunc + "_stmt_vs_decl";
    std::vector<clang::Stmt*> stmts;
    clang::CompilerInstance* CI = 0;
    bool haveSemicolon = false;
    {
      size_t endsrc = src.length();
      while (endsrc && isspace(src[endsrc - 1])) --endsrc;
      haveSemicolon = src[endsrc - 1] == ';';

      std::string nonTUsrc = "void " + stmtVsDeclFunc + "() {\n" + src + ";}";
      // Create an ASTConsumer for this frontend run which
      // will produce a list of statements seen.
      StmtSplitter splitter(stmts);
      FunctionBodyConsumer* consumer =
        new FunctionBodyConsumer(splitter, stmtVsDeclFunc.c_str());

      clang::Diagnostic& Diag = m_IncrASTParser->getCI()->getDiagnostics();
      bool prevDiagSupp = Diag.getSuppressAllDiagnostics();
      Diag.setSuppressAllDiagnostics(true);
      // fprintf(stderr,"nonTUsrc=%s\n",nonTUsrc.c_str());
      CI = m_IncrASTParser->parse(nonTUsrc, consumer);
      Diag.setSuppressAllDiagnostics(prevDiagSupp);

      if (!CI) {
        wrapped.clear();
        return;
      }
    }
    
    //
    //  Rewrite the source code to support cint command
    //  line semantics.  We must move variable declarations
    //  to the global namespace and change the code so that
    //  the new global variables are used.
    //
    std::string wrapped_globals;
    std::string wrapped_stmts;
    std::string final_stmt; // last statement for value printer
    {
      clang::SourceManager& SM = CI->getSourceManager();
      const clang::LangOptions& LO = CI->getLangOpts();
      std::vector<clang::Stmt*>::iterator stmt_iter = stmts.begin();
      std::vector<clang::Stmt*>::iterator stmt_end = stmts.end();

      MapTy& Map = m_IncrASTParser->getTransformer()->getSubstSymbolMap(); // delayed id substitutions

      for (; stmt_iter != stmt_end; ++stmt_iter) {
        clang::Stmt* cur_stmt = *stmt_iter;
        
        if (dyn_cast<clang::NullStmt>(cur_stmt)) continue;

        if (!final_stmt.empty()) {
           wrapped_stmts.append(final_stmt + '\n');
        }

        //const llvm::MemoryBuffer* MB = SM.getBuffer(SM.getFileID(cur_stmt->getLocStart()));
        //const llvm::MemoryBuffer* MB = SM.getBuffer(SM.getMainFileID());
        const llvm::MemoryBuffer* MB = (const llvm::MemoryBuffer*)m_IncrASTParser->getCurBuffer();
        const char* buffer = MB->getBufferStart();
        std::string stmt_string;
        {
          std::pair<unsigned, unsigned> r = getStmtRangeWithSemicolon(cur_stmt, SM, LO);
          if (r.first == 0 && r.second == 0) {
             if (Stmt *S = Map.lookup(cur_stmt)) {
                r = getStmtRangeWithSemicolon(S, SM, LO);
             }
             else {
                fprintf(stderr, "%s", "Cannot find source for statement!\n ");
                cur_stmt->dump();
             }
          }
          //else
          stmt_string = std::string(buffer + r.first, r.second - r.first);
          //fprintf(stderr, "stmt: %s\n", stmt_string.c_str());
        }
        //
        //  Handle expression statements.
        //
        {
          const clang::Expr* expr = dyn_cast<clang::Expr>(cur_stmt);
          if (expr) {
            //fprintf(stderr, "have expr stmt.\n");
             final_stmt = stmt_string;
             if (Map.lookup(cur_stmt)) {
                final_stmt = m_IncrASTParser->getTransformer()->ToString(cur_stmt);
             }             

             continue;
          }
        }
        //
        //  Handle everything that is not a declaration statement.
        //
        const clang::DeclStmt* DS = dyn_cast<clang::DeclStmt>(cur_stmt);
        if (!DS) {
          //fprintf(stderr, "not expr, not declaration.\n");
          final_stmt = stmt_string;
          continue;
        }
        //
        //  Loop over each declarator in the declaration statement.
        //
        clang::DeclStmt::const_decl_iterator D = DS->decl_begin();
        clang::DeclStmt::const_decl_iterator E = DS->decl_end();
        for (; D != E; ++D) {
          //
          //  Handle everything that is not a variable declarator.
          //
          const clang::VarDecl* VD = dyn_cast<clang::VarDecl>(*D);
          if (!VD) {
            if (DS->isSingleDecl()) {
              //fprintf(stderr, "decl, not var decl, single decl.\n");
              wrapped_globals.append(stmt_string + '\n');
              continue;
            }
            //fprintf(stderr, "decl, not var decl, not single decl.\n");
            clang::SourceLocation SLoc =
            SM.getInstantiationLoc((*D)->getLocStart());
            clang::SourceLocation ELoc =
            SM.getInstantiationLoc((*D)->getLocEnd());
            std::pair<unsigned, unsigned> r =
            getRangeWithSemicolon(SLoc, ELoc, SM, LO);
            std::string decl = std::string(buffer + r.first, r.second - r.first);
            wrapped_globals.append(decl + ";\n");
            continue;
          }
          //
          //  Handle a variable declarator.
          //
          std::string decl = VD->getNameAsString();
          // FIXME: Probably should not remove the qualifiers!
          VD->getType().getUnqualifiedType().
          getAsStringInternal(decl, clang::PrintingPolicy(LO));
          const clang::Expr* I = VD->getInit();
          //
          //  Handle variable declarators with no initializer
          //  or with an initializer that is a constructor call.
          //
          if (!I || dyn_cast<clang::CXXConstructExpr>(I)) {
            if (!I) {
              //fprintf(stderr, "var decl, no init.\n");
            }
            else {
              //fprintf(stderr, "var decl, init is constructor.\n");
            }
            wrapped_globals.append(decl + ";\n"); // FIXME: wrong for constructor
            continue;
          }
          //
          //  Handle variable declarators with a constant initializer.
          //
          if (I->isConstantInitializer(CI->getASTContext(), false)) {
            //fprintf(stderr, "var decl, init is const.\n");
            std::pair<unsigned, unsigned> r = getStmtRange(I, SM, LO);
            wrapped_globals.append(decl + " = " +
                                   std::string(buffer + r.first, r.second - r.first)
                                   + ";\n");
            continue;
          }
          //
          //  Handle variable declarators whose initializer is not a list.
          //
          const clang::InitListExpr* ILE = dyn_cast<clang::InitListExpr>(I);
          if (!ILE) {
            //fprintf(stderr, "var decl, init is not list.\n");
            std::pair<unsigned, unsigned> r = getStmtRange(I, SM, LO);
            final_stmt = std::string(VD->getName())  + " = " +
               std::string(buffer + r.first, r.second - r.first) + ";";
            wrapped_globals.append(decl + ";\n");
            continue;
          }
          //
          //  Handle variable declarators with an initializer list.
          //
          //fprintf(stderr, "var decl, init is list.\n");
          unsigned numInits = ILE->getNumInits();
          for (unsigned j = 0; j < numInits; ++j) {
            std::string stmt;
            llvm::raw_string_ostream stm(stmt);
            stm << VD->getNameAsString() << "[" << j << "] = ";
            std::pair<unsigned, unsigned> r =
            getStmtRange(ILE->getInit(j), SM, LO);
            stm << std::string(buffer + r.first, r.second - r.first) << ";\n";
            final_stmt = stm.str();
          }
          wrapped_globals.append(decl + ";\n");
        }
      }
      // clear the DenseMap
      Map.clear();

      haveStatements = !final_stmt.empty();
      if (haveStatements) {
            std::stringstream sstr_stmt;
            sstr_stmt << "extern \"C\" void " << stmtFunc << "() {\n"
                      << wrapped_stmts;
         if (!haveSemicolon) {
            sstr_stmt << "cling::ValuePrinter(((cling::Interpreter*)"
                      << (void*)this << ")->getValuePrinterStream(),"
                      << final_stmt << ");}\n";
         } else {
            sstr_stmt << final_stmt << ";}\n";
         }
         wrapped_stmts = sstr_stmt.str();
      } else {
        stmtFunc = "";
      }
    }
    //
    //fprintf(stderr, "wrapped_globals:\n%s\n", wrapped_globals.c_str());
    //fprintf(stderr, "wrapped_stmts:\n%s\n", wrapped_stmts.c_str());
    wrapped += wrapped_globals + wrapped_stmts;

    //CI->clearOutputFiles(/*EraseFiles=*/CI->getDiagnostics().getNumErrors());
    //CI->getDiagnosticClient().EndSourceFile();
    unsigned err_count = CI->getDiagnosticClient().getNumErrors();
    // reset diag client
    if (err_count) {
      wrapped.clear();
      return;
    }
  }
  
  clang::ASTConsumer*
  Interpreter::maybeGenerateASTPrinter() const
  {
    if (m_printAST) {
      return clang::CreateASTDumper();
    }
    return new clang::ASTConsumer();
  }
  
  clang::CompilerInstance*
  Interpreter::compileString(const std::string& argCode)
  {
    return m_IncrASTParser->parse(argCode,
                                  m_ExecutionContext->getCodeGenerator());
  }
  
  clang::CompilerInstance*
  Interpreter::compileFile(const std::string& filename,
                           const std::string* trailcode /*=0*/)
  {
    std::string code;
    code += "#include \"" + filename + "\"\n";
    if (trailcode) code += *trailcode;
    return compileString(code);
  }
  
  static
  bool tryLoadSharedLib(const std::string& filename)
  {
    llvm::sys::Path DynLib = findDynamicLibrary(filename);
    if (!DynLib.isDynamicLibrary())
      return false;
    
    std::string errMsg;
    bool err =
    llvm::sys::DynamicLibrary::LoadLibraryPermanently(DynLib.str().c_str(), &errMsg);
    if (err) {
      //llvm::errs() << "Could not load shared library: " << errMsg << '\n';
      fprintf(stderr,
              "Interpreter::loadFile: Could not load shared library!\n");
      fprintf(stderr, "%s\n", errMsg.c_str());
      return false;
    }
    return true;
  }
  
  int
  Interpreter::loadFile(const std::string& filename,
                        const std::string* trailcode /*=0*/,
                        bool allowSharedLib /*=true*/)
  {
    if (allowSharedLib && tryLoadSharedLib(filename))
      return 0;
    
    if (!m_ExecutionContext->startCodegen(m_IncrASTParser->getCI(), filename)) {
      fprintf(stderr, "Error: could not compile prompt history!\n");
      return 1;
    }    
    
    clang::CompilerInstance* CI = compileFile(filename, trailcode);
    if (!CI) {
      return 1;
    }
    
    if (!m_ExecutionContext->getModuleFromCodegen()) {
      fprintf(stderr, "Error: Backend did not create a module!\n");
      return 1;
    }
    return 0;
  }
  
  int
  Interpreter::executeFile(const std::string& filename)
  {
    std::string::size_type pos = filename.find_last_of('/');
    if (pos == std::string::npos) {
      pos = 0;
    }
    else {
      ++pos;
    }
    
    // Note: We are assuming the filename does not end in slash here.
    std::string funcname(filename, pos);
    std::string::size_type endFileName = std::string::npos;
    
    std::string args("()"); // arguments with enclosing '(', ')'
    pos = funcname.find_first_of('(');
    if (pos != std::string::npos) {
      std::string::size_type posParamsEnd = funcname.find_last_of(')');
      if (posParamsEnd != std::string::npos) {
        args = funcname.substr(pos, posParamsEnd - pos + 1);
        endFileName = filename.find_first_of('(');
      }
    }
    
    //fprintf(stderr, "funcname: %s\n", funcname.c_str());
    pos = funcname.find_last_of('.');
    if (pos != std::string::npos) {
      funcname.erase(pos);
      //fprintf(stderr, "funcname: %s\n", funcname.c_str());
    }
    
    std::string func = createUniqueName();
    std::string wrapper = "extern \"C\" void " + func;
    wrapper += "(){\n" + funcname + args + ";\n}";
    int err = loadFile(filename.substr(0, endFileName), &wrapper);
    if (err) {
      return err;
    }
    m_ExecutionContext->executeFunction(func);
    return 0;
  }

  void Interpreter::installLazyFunctionCreator(void* (*fp)(const std::string&)) {
    m_ExecutionContext->getEngine().InstallLazyFunctionCreator(fp);
  }
  

   // Implements the interpretation of the unknown symbols. 
   bool Interpreter::EvalCore(llvm::GenericValue& result, const char* expr) {
      printf("%s\n", expr);
      return 0;
   }
   
   void cling::Interpreter::dumpAST(bool showAST, int last) {
     clang::Decl* D = m_LastDump;
     int oldPolicy = m_IncrASTParser->getCI()->getASTContext().PrintingPolicy.Dump;

     if (!D && last == -1 ) {
        fprintf(stderr, "No last dump found! Assuming ALL \n");
        last = 0;
        showAST = false;        
     }

     m_IncrASTParser->getCI()->getASTContext().PrintingPolicy.Dump = showAST;

     if (last == -1) {
        while ((D = D->getNextDeclInContext())) {
           D->dump();
        }
     }
     else if (last == 0) {
        m_IncrASTParser->getCI()->getASTContext().getTranslationUnitDecl()->dump();
     } else {
        clang::Decl *FD = m_IncrASTParser->getFirstTopLevelDecl(); // First Decl to print
        clang::Decl *LD = FD;

        // FD and LD are first

        clang::Decl *NextLD = 0;
        for (int i = 1; i < last; ++i) {
           NextLD = LD->getNextDeclInContext();
           if (NextLD) {
              LD = NextLD;
           }
        }

        // LD is last Decls after FD: [FD x y z LD a b c d]

        while ((NextLD = LD->getNextDeclInContext())) {
           // LD not yet at end: move window
           FD = FD->getNextDeclInContext();
           LD = NextLD;
        }

        // Now LD is == getLastDeclinContext(), and FD is last decls before
        // LD is last Decls after FD: [x y z a FD b c LD]
        
        while (FD) {
           FD->dump();
           fprintf(stderr, "\n"); // New line for every decl
           FD = FD->getNextDeclInContext();
        }        
     }

     m_LastDump = m_IncrASTParser->getLastTopLevelDecl();     
     m_IncrASTParser->getCI()->getASTContext().PrintingPolicy.Dump = oldPolicy;
   }
  
} // namespace cling

