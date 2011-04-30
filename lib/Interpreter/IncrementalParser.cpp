//------------------------------------------------------------------------------
// CLING - the C++ LLVM-based InterpreterG :)
// version: $Id$
// author:  Axel Naumann <axel@cern.ch>
//------------------------------------------------------------------------------

#include "IncrementalParser.h"
#include "cling/Interpreter/CIFactory.h"

#include "llvm/Support/MemoryBuffer.h"

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclGroup.h"
#include "clang/Parse/Parser.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/Pragma.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/SemaConsumer.h"

#include "cling/Interpreter/Diagnostics.h"
#include "cling/Interpreter/Interpreter.h"
#include "DynamicLookup.h"
#include "ChainedASTConsumer.h"

#include <stdio.h>
#include <sstream>

namespace cling {
  class MutableMemoryBuffer: public llvm::MemoryBuffer {
    std::string m_FileID;
    size_t m_Alloc;
  protected:
    void maybeRealloc(llvm::StringRef code, size_t oldlen) {
      size_t applen = code.size();
      char* B = 0;
      if (oldlen) {
        B = const_cast<char*>(getBufferStart());
        assert(!B[oldlen] && "old buffer is not 0 terminated!");
        // B + oldlen points to trailing '\0'
      }
      size_t newlen = oldlen + applen + 1;
      if (newlen > m_Alloc) {
        if (m_Alloc) {
           fprintf(stderr, "MutableMemoryBuffer reallocation doesn't work (Preprocessor isn't told about the new location, should instead just add a new memory buffer)!\n");
        }
        m_Alloc += 64*1024;
        B = (char*)realloc(B, m_Alloc);
      }
      memcpy(B + oldlen, code.data(), applen);
      B[newlen - 1] = 0;
      init(B, B + newlen - 1, /*RequireNullTerminator=*/ true);
    }
    
  public:
    MutableMemoryBuffer(llvm::StringRef Code, llvm::StringRef Name)
      : MemoryBuffer(), m_FileID(Name), m_Alloc(0) {
      maybeRealloc(Code, 0);
    }
    
    virtual ~MutableMemoryBuffer() {
      free((void*)getBufferStart());
    }
    
    void append(llvm::StringRef code) {
      assert(getBufferSize() && "buffer is empty!");
      maybeRealloc(code, getBufferSize());
    }
 
    virtual const char *getBufferIdentifier() const {
      return m_FileID.c_str();
    }

    virtual BufferKind getBufferKind () const {
      return MemoryBuffer_Malloc;
    }
  };

  
  IncrementalParser::IncrementalParser(Interpreter* interp,
                                       clang::PragmaNamespace* Pragma,
                                       const char* llvmdir):
    m_Enabled(true),
    m_Consumer(0), 
    m_LastTopLevelDecl(0),
    m_FirstTopLevelDecl(0) 
  {
    //m_CIFactory.reset(new CIFactory(0, 0, llvmdir));
    m_MemoryBuffer.push_back(new MutableMemoryBuffer("//cling!\n", "CLING") );
    clang::CompilerInstance* CI = CIFactory::createCI(m_MemoryBuffer[0], Pragma, llvmdir);
    assert(CI && "CompilerInstance is (null)!");
    m_CI.reset(CI);

    m_MBFileID = CI->getSourceManager().getMainFileID();
    //CI->getSourceManager().getBuffer(m_MBFileID, clang::SourceLocation()); // do we need it?


    if (CI->getSourceManager().getMainFileID().isInvalid()) {
      fprintf(stderr, "Interpreter::compileString: Failed to create main "
              "file id!\n");
      return;
    }
    
    m_Consumer = dyn_cast<ChainedASTConsumer>(&CI->getASTConsumer());
    assert(m_Consumer && "Expected ChainedASTConsumer!");

    // Initialize the parser.
    m_Parser.reset(new clang::Parser(CI->getPreprocessor(), CI->getSema()));
    CI->getPreprocessor().EnterMainSourceFile();
    m_Parser->Initialize();
    
    //if (clang::SemaConsumer *SC = dyn_cast<clang::SemaConsumer>(m_Consumer))
    //  SC->InitializeSema(CI->getSema()); // do we really need this? We know 
    // that we will have ChainedASTConsumer, which is initialized in createCI
    
    // Create the visitor that will transform all dependents that are left.
    m_Transformer.reset(new DynamicExprTransformer(interp, &CI->getSema()));
    // Attach the DynamicIDHandler if enabled
    if (m_Enabled)
      getTransformer()->AttachDynIDHandler();
  }
  
  IncrementalParser::~IncrementalParser() {}
  
  void IncrementalParser::Initialize() {
    
    parse(""); // Consume initialization.
    // Set up common declarations which are going to be available
    // only at runtime
    // Make surethat the universe won't be included to compile time by using
    // -D __CLING__ as CompilerInstance's arguments
    parse("#include \"cling/Interpreter/RuntimeUniverse.h\"");
    
    // Attach the dynamic lookup
    getTransformer()->Initialize();
  }
  
  clang::CompilerInstance*
   IncrementalParser::parse(llvm::StringRef src)
  {
    // Add src to the memory buffer, parse it, and add it to
    // the AST. Returns the CompilerInstance (and thus the AST).
    // Diagnostics are reset for each call of parse: they are only covering
    // src.
    
    clang::Preprocessor& PP = m_CI->getPreprocessor();
    m_CI->getDiagnosticClient().BeginSourceFile(m_CI->getLangOpts(), &PP);
    
    if (src.size()) {
      std::ostringstream source_name;
      source_name << "input_line_" << (m_MemoryBuffer.size()+1);
      m_MemoryBuffer.push_back( new MutableMemoryBuffer("//cling!\n", source_name.str()) );
      MutableMemoryBuffer *currentBuffer = m_MemoryBuffer.back();
      currentBuffer->append(src);
      clang::FileID FID = m_CI->getSourceManager().createFileIDForMemBuffer(currentBuffer);
      
      PP.EnterSourceFile(FID, 0, clang::SourceLocation());     
      
      clang::Token &tok = const_cast<clang::Token&>(m_Parser->getCurToken());
      tok.setKind(clang::tok::semi);
    }
    
    DiagnosticPrinter* DC = reinterpret_cast<DiagnosticPrinter*>(&m_CI->getDiagnosticClient());
    DC->resetCounts();
    m_CI->getDiagnostics().Reset();
    
    clang::ASTConsumer* Consumer = &m_CI->getASTConsumer();
    clang::Parser::DeclGroupPtrTy ADecl;
    
    bool atEOF = false;
    if (m_Parser->getCurToken().is(clang::tok::eof)) {
      atEOF = true;
    }
    else {
      atEOF = m_Parser->ParseTopLevelDecl(ADecl);
    }
    while (!atEOF) {
      // Not end of file.
      // If we got a null return and something *was* parsed, ignore it.  This
      // is due to a top-level semicolon, an action override, or a parse error
      // skipping something.
      if (ADecl) {
        clang::DeclGroupRef DGR = ADecl.getAsVal<clang::DeclGroupRef>();
        for (clang::DeclGroupRef::iterator i=DGR.begin(); i< DGR.end(); ++i) {
          if (!m_FirstTopLevelDecl)
            m_FirstTopLevelDecl = *i;
          
          m_LastTopLevelDecl = *i;
          if (m_Enabled)
            getTransformer()->Visit(m_LastTopLevelDecl);
        } 
        Consumer->HandleTopLevelDecl(DGR);
      } // ADecl
      if (m_Parser->getCurToken().is(clang::tok::eof)) {
        atEOF = true;
      }
      else {
        atEOF = m_Parser->ParseTopLevelDecl(ADecl);
      }
    };
    
    getCI()->getSema().PerformPendingInstantiations();
    
    // Process any TopLevelDecls generated by #pragma weak.
    for (llvm::SmallVector<clang::Decl*,2>::iterator
           I = getCI()->getSema().WeakTopLevelDecls().begin(),
           E = getCI()->getSema().WeakTopLevelDecls().end(); I != E; ++I) {
      Consumer->HandleTopLevelDecl(clang::DeclGroupRef(*I));
    }
    
    clang::ASTContext *Ctx = &m_CI->getASTContext();
    Consumer->HandleTranslationUnit(*Ctx);
    
    DC->EndSourceFile();
    unsigned err_count = DC->getNumErrors();
    if (err_count) {
      fprintf(stderr, "IncrementalParser::parse(): Parse failed!\n");
      emptyLastFunction();
      return 0;
    }
    return m_CI.get();
  }

  void IncrementalParser::setEnabled(bool value) {
    m_Enabled = value;
    if (m_Enabled)
      getTransformer()->AttachDynIDHandler();
    else
      getTransformer()->DetachDynIDHandler();    
  }

   
  void IncrementalParser::emptyLastFunction() {
    // Given a broken AST (e.g. due to a syntax error),
    // replace the last function's body by a null statement.
    
    // Note: this does not touch the identifier table.
    clang::ASTContext& Ctx = m_CI->getASTContext();
    clang::FunctionDecl* F = dyn_cast<clang::FunctionDecl>(m_LastTopLevelDecl);
    if (F && F->getBody()) {
      clang::NullStmt* NStmt = new (Ctx) clang::NullStmt(clang::SourceLocation());
      F->setBody(NStmt);
    }
  } 
  
  void IncrementalParser::addConsumer(clang::ASTConsumer* consumer) {
    m_Consumer->Consumers.push_back(consumer);
    consumer->Initialize(getCI()->getSema().getASTContext());
  }
  
  void IncrementalParser::removeConsumer(clang::ASTConsumer* consumer) {
    for (unsigned int i = 0; i != m_Consumer->Consumers.size(); ++i) {
      if (m_Consumer->Consumers[i] == consumer) {
        m_Consumer->Consumers.erase(m_Consumer->Consumers.begin() + i);
        break;
      }         
    }
  }  
} // namespace cling
