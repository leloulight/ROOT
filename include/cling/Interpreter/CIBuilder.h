//--------------------------------------------------------------------*- C++ -*-
// CLING - the C++ LLVM-based InterpreterG :)
// version: $Id$
// author:  Axel Naumann <axel@cern.ch>
//------------------------------------------------------------------------------

#ifndef CLING_CIBUILDER_H
#define CLING_CIBUILDER_H

#include "clang/Frontend/CompilerInstance.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Path.h"

namespace llvm {
   class LLVMContext;
}

namespace clang {
  class PragmaHandler;
}

namespace cling {
class CIBuilder {
public:
   //---------------------------------------------------------------------
   //! Constructor
   //---------------------------------------------------------------------
   CIBuilder(int argc, const char* const *argv, const char* llvmdir = 0);
   ~CIBuilder();

   clang::CompilerInstance* createCI() const;
   llvm::LLVMContext* getLLVMContext() const { return m_llvm_context.get(); }
   llvm::sys::Path getResourcePath(){return m_resource_path;}

private:
   int         m_argc;
   const char* const *m_argv;
   llvm::sys::Path m_resource_path;
   mutable llvm::OwningPtr<llvm::LLVMContext> m_llvm_context; // We own, our types.
};
}
#endif // CLING_CIBUILDER_H
