//--------------------------------------------------------------------*- C++ -*-
// CLING - the C++ LLVM-based InterpreterG :)
// version: $Id$
// author:  Vassil Vassilev <vasil.georgiev.vasilev@cern.ch>
//------------------------------------------------------------------------------

#ifndef CLING_DYNAMIC_LOOKUP_H
#define CLING_DYNAMIC_LOOKUP_H

#include "clang/AST/StmtVisitor.h"
#include "clang/Sema/ExternalSemaSource.h"
#include "clang/Sema/Ownership.h"
#include "clang/Sema/SemaConsumer.h"

#include "llvm/ADT/OwningPtr.h"
#include "llvm/Support/Casting.h"


namespace clang {
  class Sema;
}

namespace cling {

  class InterpreterCallbacks;

  /// \brief Provides last chance of recovery for clang semantic analysis.
  /// When the compiler doesn't find the symbol in its symbol table it asks
  /// its ExternalSemaSource to look for the symbol.
  ///
  /// In contrast to the compiler point of view, where these symbols must be
  /// errors, the interpreter's point of view these symbols are to be
  /// evaluated at runtime. For that reason the interpreter marks all unknown
  /// by the compiler symbols to be with delayed lookup (evaluation).
  /// One have to be carefull in the cases, in which the compiler expects that
  /// the lookup will fail!
  class DynamicIDHandler : public clang::ExternalSemaSource {
  public:
    InterpreterCallbacks* Callbacks;
    DynamicIDHandler(clang::Sema* Sema);
    ~DynamicIDHandler();

    /// \brief Provides last resort lookup for failed unqualified lookups
    ///
    /// If there is failed lookup, tell sema to create an artificial declaration
    /// which is of dependent type. So the lookup result is marked as dependent
    /// and the diagnostics are suppressed. After that is's an interpreter's
    /// responsibility to fix all these fake declarations and lookups.
    /// It is done by the DynamicExprTransformer.
    ///
    /// @param[out] R The recovered symbol.
    /// @param[in] S The scope in which the lookup failed.
    virtual bool LookupUnqualified(clang::LookupResult& R, clang::Scope* S);

  private:
    clang::Sema* m_Sema;
    clang::ASTContext& m_Context;

    /// \brief Checks whether the failed lookup is not expected from the
    /// compiler to fail.
    ///
    /// @param[out] R The symbol to be checked.
    /// @param[in] S The scope, where the lookup failed.
    bool IsDynamicLookup (clang::LookupResult& R, clang::Scope* S);

  };
} // end namespace cling

namespace cling {
  typedef llvm::SmallVector<clang::Stmt*, 2> ASTNodes;

  /// \brief Helper structure that allows children of a node to delegate how
  /// to be replaced from their parent. For example a child can return more than
  /// one replacement node.
  class ASTNodeInfo {
  private:
    ASTNodes Nodes;
    bool forReplacement;
  public:
    ASTNodeInfo() : forReplacement(false){}
    ASTNodeInfo(clang::Stmt* S, bool needed) : forReplacement(needed) {
      Nodes.push_back(S);
    }

    bool isForReplacement() { return forReplacement; }
    void setForReplacement(bool val = true) { forReplacement = val; }
    bool hasSingleNode() { return Nodes.size() == 1; }
    clang::Stmt* getAsSingleNode() {
      assert(hasSingleNode() && "There is more than one node!");
      return Nodes[0];
    }
    ASTNodes& getNodes() { return Nodes; }
    void addNode(clang::Stmt* Node) { Nodes.push_back(Node); }
    template <typename T> T* getAs() {
      return llvm::dyn_cast<T>(getAsSingleNode());
    }
    template <typename T> T* castTo() {
      T* Result = llvm::dyn_cast<T>(getAsSingleNode());
      assert(Result && "Cannot cast to type!");
      return Result;
    }
  };
} //end namespace cling

namespace cling {
  class Interpreter;
  class DynamicIDHandler;

  typedef llvm::DenseMap<clang::Stmt*, clang::Stmt*> MapTy;

  /// \brief In order to implement the runtime type binding and expression
  /// evaluation we need to be able to compile code which contains unknown
  /// symbols (undefined variables, types, functions, etc.). This cannot be done
  /// by a compiler like clang, because it is not valid C++ code.
  ///
  /// DynamicExprTransformer transforms these unknown symbols into valid C++
  /// code at AST (abstract syntax tree) level. Thus it provides an opportunity
  /// their evaluation to happen at runtime. Several steps are performed:
  ///
  /// 1. Skip compiler's error diagnostics - if a compiler encounters unknown
  /// symbol, by definition, it must output an error and it mustn't produce
  /// machine code. Cling implements an extension to Clang semantic analyzer
  /// that allows the compiler to recover even an unknown symbol is encountered.
  /// For instance if the compiler sees a symbol it looks for its definition in
  /// a internal structure (symbol table) and it is not found it asks whether
  /// somebody else could provide the missing symbol. That is the place where
  /// the DynamicIDHandler, which is controlled by DynamicExprTransformer comes
  /// into play. It marks all unknown symbols as dependent as if they are
  /// templates and are going to be resolved at first instantiation, with the
  /// only difference that an instantiation never happens. The advantage is that
  /// the unknown symbols are not diagnosed but the disadvantage is that
  /// somebody needs to transform them into valid expressions with valid types.
  ///
  /// 2. Replace all dependent symbols - all artificially dependent symbols need
  /// to be replaced with appropriate valid symbols in order the compiler to
  /// produce executable machine code. The DynamicExprTransformer walks up all
  /// statements and declarations that might be possibly marked earlier as
  /// dependent and replaces them with valid expression, which preserves the
  /// meant behavior. Main implementation goal is to replace the as little
  /// as possible part of the statement. The replacement is done immediately
  /// after the expected type can be deduced.
  ///
  /// 2.1. EvaluateT - this is a templated function, which is put at the
  /// place of the dependent expression. It will be called at runtime and it
  /// will use the runtime instance of the interpreter (cling interprets itself)
  /// to evaluate the replaced expression. The template parameter of the
  /// function carries the expected expression type. If unknown symbol is
  /// encountered as a right-hand-side of an assignment one can claim that
  /// the type of the unknown expression should be compatible with the type of
  /// the left-hand-side.
  ///
  /// 2.2 LifetimeHandler - in some more complex situation in order to preserve
  /// the behavior the expression must be replaced with more complex structures.
  ///
  /// 3. Evaluate interface - this is the core function in the interpreter,
  /// which does the delayed evaluation. It uses a callback function, which
  /// should be reimplemented in the subsystem that provides the runtime types
  /// and addresses of the expressions.
  class EvaluateTSynthesizer : public clang::SemaConsumer,
                               public clang::StmtVisitor<EvaluateTSynthesizer,
                                                         ASTNodeInfo> {

  private:

    /// \brief Stores the declaration of the EvaluateT function.
    clang::FunctionDecl* m_EvalDecl;

    /// \brief Sema's external source, which provides last resort lookup.
    llvm::OwningPtr<DynamicIDHandler> m_DynIDHandler;

    /// \brief Keeps track of the replacements being made. If an AST node is
    /// changed with another it should be added to the map (newNode->oldNode).
    MapTy m_SubstSymbolMap;

    /// \brief Stores the actual declaration context, in which declarations are
    /// being visited.
    clang::DeclContext* m_CurDeclContext;

    /// \brief Stores pointer to cling, mainly used for declaration lookup.
    Interpreter* m_Interpreter;

    /// \brief Use instead of clang::SourceRange().
    clang::SourceRange m_NoRange;

    /// \brief Use instead of clang::SourceLocation() as start location.
    clang::SourceLocation m_NoSLoc;

    /// \brief Use instead of clang::SourceLocation() as end location.
    clang::SourceLocation m_NoELoc;

    /// \brief Needed for the AST transformations, owned by Sema
    clang::ASTContext* m_Context;

    /// \brief Needed for the AST transformations, owned by CompilerInstance
    clang::Sema* m_Sema;

  public:

    typedef clang::StmtVisitor<EvaluateTSynthesizer, ASTNodeInfo>
      BaseStmtVisitor;

    using BaseStmtVisitor::Visit;

    EvaluateTSynthesizer(Interpreter* interp);

    ~EvaluateTSynthesizer() { }

    void Initialize(clang::ASTContext& Ctx) { m_Context = &Ctx; }

    void InitializeSema(clang::Sema& S) { m_Sema = &S; }
    bool HandleTopLevelDecl(clang::DeclGroupRef DGR);

    MapTy& getSubstSymbolMap() { return m_SubstSymbolMap; }

    ASTNodeInfo VisitStmt(clang::Stmt* Node);
    ASTNodeInfo VisitCompoundStmt(clang::CompoundStmt* Node);
    ASTNodeInfo VisitIfStmt(clang::IfStmt* Node);
    /// \brief Transforms a declaration with initializer of dependent type.
    /// If an object on the free store is being initialized we use the
    /// EvaluateT
    /// If an object on the stack is being initialized it is transformed into
    /// reference and an object on the free store is created in order to
    /// avoid the copy constructors, which might be private
    ///
    /// For example:
    /// @code
    /// int i = 5;
    /// MyClass my(dep->Symbol(i))
    /// @endcode
    /// where dep->Symbol() is of artificially dependent type it is being
    /// transformed into:
    /// @code
    /// cling::runtime::internal::LifetimeHandler
    /// __unique("dep->Sybmol(*(int*)@)",(void*[]){&i}, DC, "MyClass");
    /// MyClass &my(*(MyClass*)__unique.getMemory());
    /// @endcode
    ///
    /// Note: here our main priority is to preserve equivalent behavior. We have
    /// to clean the heap memory afterwords.
    ///
    ASTNodeInfo VisitDeclStmt(clang::DeclStmt* Node);
    ASTNodeInfo VisitExpr(clang::Expr* Node);
    ASTNodeInfo VisitBinaryOperator(clang::BinaryOperator* Node);
    ASTNodeInfo VisitCallExpr(clang::CallExpr* E);
    ASTNodeInfo VisitDeclRefExpr(clang::DeclRefExpr* DRE);
    ASTNodeInfo VisitDependentScopeDeclRefExpr(
                                        clang::DependentScopeDeclRefExpr* Node);

    ///\brief Sets callbacks so that DynamicIDHandler can use them, when it sees
    /// the unknown symbol again at runtime. This time the implementation of the
    /// LookupObject callback should provide the actual definition of the object
    /// and the lookup may succeed.
    ///
    /// @param[in] C The concrete implementation of the callback interface.
    ///
    void SetCallbacks(InterpreterCallbacks* C) {
      m_DynIDHandler->Callbacks = C;
    }

  protected:
    /// @{
    /// @name Helpers, which simplify node replacement

    ///\brief Replaces given dependent AST node with an instantiation of
    /// EvaluateT with the deduced type.
    ///
    /// @param[in] InstTy The deduced type used to create instantiation.
    /// @param[in] SubTree The AST node or subtree, which is being replaced.
    /// @param[in] ValuePrinterReq Whether to turn on the value printing or not
    ///
    clang::Expr* SubstituteUnknownSymbol(const clang::QualType InstTy,
                                         clang::Expr* SubTree,
                                         bool ValuePrinterReq = false);

    ///\brief Builds the actual call expression, which is put in the place of
    /// the dependent AST node.
    ///
    /// @param[in] InstTy The deduced type used to create instantiation.
    /// @param[in] SubTree The AST node or subtree, which is being replaced.
    /// @param[in] CallArgs Proper arguments, which the call will use.
    ///
    clang::CallExpr* BuildEvalCallExpr(clang::QualType InstTy,
                                       clang::Expr* SubTree,
                                clang::ASTOwningVector<clang::Expr*>& CallArgs);

    ///\brief Builds the DynamicExprInfo class with proper info.
    ///
    clang::Expr* BuildDynamicExprInfo(clang::Expr* SubTree,
                                      bool ValuePrinterReq = false);

    ///\brief Creates cstyle casts a pointer expression to a given qualified
    /// type.
    ///
    clang::Expr* ConstructCStyleCasePtrExpr(clang::QualType Ty, uint64_t Ptr);

    ///\brief Creates const char* expression from given value.
    clang::Expr* ConstructConstCharPtrExpr(const char* Val);

    ///\brief Checks if the given node is marked as dependent by us.
    ///
    bool IsArtificiallyDependent(clang::Expr* Node);

    ///\brief Checks if the given declaration should be examined. It checks
    /// whether a declaration context marked as dependent contains the
    /// declaration or the declaration type is not one of those we are looking
    /// for.
    ///
    bool ShouldVisit(clang::Decl* D);

    /// \brief Gets all children of a given node.
    ///
    bool GetChildren(ASTNodes& Children, clang::Stmt* Node);
    /// @}
  };
} // end namespace cling
#endif // CLING_DYNAMIC_LOOKUP_H
