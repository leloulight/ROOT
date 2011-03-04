//------------------------------------------------------------------------------
// CLING - the C++ LLVM-based InterpreterG :)
// version: $Id: DynamicLookup.h 36608 2010-11-11 18:21:02Z vvassilev $
// author:  Vassil Vassilev <vasil.georgiev.vasilev@cern.ch>
//------------------------------------------------------------------------------

#ifndef CLING_DYNAMIC_LOOKUP_H
#define CLING_DYNAMIC_LOOKUP_H

#include "llvm/ADT/OwningPtr.h"

#include "clang/AST/StmtVisitor.h"
#include "clang/AST/DeclVisitor.h"
#include "clang/Sema/Ownership.h"
#include "clang/Sema/ExternalSemaSource.h"

namespace clang {
  class Sema;
  
}

namespace cling {
  class DynamicIDHandler : public clang::ExternalSemaSource {
  public:
    DynamicIDHandler(clang::Sema* Sema) : m_Sema(Sema){}
    ~DynamicIDHandler();
    
    // Override this to provide last resort lookup for failed unqualified lookups
    virtual bool LookupUnqualified(clang::LookupResult &R, clang::Scope *S);
    //Remove the fake dependent declarations
    void RemoveFakeDecls();
  private:
    llvm::SmallVector<clang::Decl*, 8> m_FakeDecls;
    clang::Sema* m_Sema;
  };
} // end namespace cling

namespace cling {
  class Interpreter;
  class DynamicIDHandler;
  class EvalInfo;
  
  typedef llvm::DenseMap<clang::Stmt*, clang::Stmt*> MapTy;
  
  // Ideally the visitor should traverse the dependent nodes, which actially are 
  // the language extensions. For example File::Open("MyFile"); h->Draw() is not valid C++ call
  // if h->Draw() is defined in MyFile. In that case we need to skip Sema diagnostics, so the 
  // h->Draw() is marked as dependent node. That requires the DynamicExprTransformer to find all
  // dependent nodes and escape them to the interpreter, using pre-defined Eval function.
  class DynamicExprTransformer : public clang::DeclVisitor<DynamicExprTransformer>,
                                 public clang::StmtVisitor<DynamicExprTransformer, EvalInfo> {
    
  private: // members
    clang::FunctionDecl* m_EvalDecl;
    llvm::OwningPtr<DynamicIDHandler> m_DynIDHandler;
    MapTy m_SubstSymbolMap;
    /* 
       Specifies the unknown symbol surrounding
       Example: int a; ...; h->Draw(a); -> Eval(gCling, "*(int*)@", {&a});
       m_EvalExpressionBuf holds the types of the variables.
       m_Environment holds the refs from which runtime addresses are built.
    */
    std::string m_EvalExpressionBuf;
    llvm::SmallVector<clang::DeclRefExpr*, 64> m_Environment;
    clang::DeclContext* m_CurDeclContext; // We need it for Evaluate()
    clang::QualType m_DeclContextType; // Used for building Eval args
  public: // members
    clang::Sema* m_Sema;
    
  public: // types
    
    typedef clang::DeclVisitor<DynamicExprTransformer> BaseDeclVisitor;
    typedef clang::StmtVisitor<DynamicExprTransformer, EvalInfo> BaseStmtVisitor;
    
    using BaseStmtVisitor::Visit;
    
  public:
    
    //Constructors
    DynamicExprTransformer();      
    DynamicExprTransformer(clang::Sema* Sema);
    
    // Destructors
    ~DynamicExprTransformer() { }
    
    void Initialize();
    clang::FunctionDecl *getEvalDecl() { return m_EvalDecl; }
    void setEvalDecl(clang::FunctionDecl *FDecl) { if (!m_EvalDecl) m_EvalDecl = FDecl; }
    MapTy &getSubstSymbolMap() { return m_SubstSymbolMap; }
    
    // DeclVisitor      
    void Visit(clang::Decl *D);
    void VisitFunctionDecl(clang::FunctionDecl *D);
    void VisitTemplateDecl(clang::TemplateDecl *D); 
    void VisitDecl(clang::Decl *D);
    void VisitDeclContext(clang::DeclContext *DC);
    
    // StmtVisitor
    EvalInfo VisitStmt(clang::Stmt *Node);
    EvalInfo VisitExpr(clang::Expr *Node);
    EvalInfo VisitCallExpr(clang::CallExpr *E);
    EvalInfo VisitDeclRefExpr(clang::DeclRefExpr *DRE);
    EvalInfo VisitDependentScopeDeclRefExpr(clang::DependentScopeDeclRefExpr *Node);
    
    // EvalBuilder
    clang::Expr *SubstituteUnknownSymbol(const clang::QualType InstTy, clang::Expr *SubTree);
    clang::CallExpr *BuildEvalCallExpr(clang::QualType type, clang::Expr *SubTree, clang::ASTOwningVector<clang::Expr*> &CallArgs);
    void BuildEvalEnvironment(clang::Expr *SubTree);
    void BuildEvalArgs(clang::ASTOwningVector<clang::Expr*> &Result);
    clang::Expr *BuildEvalArg0(clang::ASTContext &C);
    clang::Expr *BuildEvalArg1(clang::ASTContext &C);
    clang::Expr *BuildEvalArg2(clang::ASTContext &C);
    
    // Helper
    bool IsArtificiallyDependent(clang::Expr *Node);
    bool ShouldVisit(clang::Decl *D);      
  };
} // end namespace cling
#endif // CLING_DYNAMIC_LOOKUP_H
