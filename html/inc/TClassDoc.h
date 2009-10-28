// @(#)root/html:$Id$
// Author: Axel Naumann 2009-10-26

/*************************************************************************
 * Copyright (C) 1995-2009, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TClassDoc
#define ROOT_TClassDoc

////////////////////////////////////////////////////////////////////////////
//                                                                        //
// TClassDoc                                                              //
//                                                                        //
// Documentation for a clas / struct / union / enum / namespace           //
//                                                                        //
////////////////////////////////////////////////////////////////////////////

#ifndef ROOT_TDocumented
#include "TDocumented.h"
#endif
#ifndef ROOT_TDocStringTable
#include "TDocStringTable.h"
#endif
#ifndef ROOT_TList
#include "TList.h"
#endif

namespace Doc {
class TDataDoc;
class TFunctionDoc;
class TTypedefDoc;
class TModuleDoc;

class TClassDoc: public TDocumented {
public:

   // Kind of C++ record this TClassDoc is describing
   enum EKind {
      kClass = BIT(16), // it's a class
      kStruct, // it's a struct
      kUnion, // it's a union
      kEnum, // it's an enum
      kNamespace, // it's a namespace
      kTypedef, // it's a typedef to a class - should that be here?
      kUnknownKind, // not set
      kNumKinds = kUnknownKind, // number of kinds
      kKindBitMask = BIT(16) | BIT(17) | BIT(18)
   };

   TClassDoc() {}
   TClassDoc(const char* name, TClass* cl, const char* module);

   virtual ~TClassDoc();

   const TString& GetModuleName() const { return fModule; }
   const TCollection* GetMembers() const { return &fMembers; }
   const TCollection* GetTypes() const { return &fTypes; }
   const TCollection* GetSeeAlso() const { return &fSeeAlso; }
   EKind  GetKind() const { return (EKind) (TestBits(kKindBitMask) - BIT(16)); }

   const char* GetURL() const { return fHtmlFileName; }
   const char* GetHtmlFileName() const { return fHtmlFileName; }
   Int_t Compare(const TObject* obj) const;

   TDataDoc*     AddDataMember(const char* name, const char* type);
   TFunctionDoc* AddFunctionMember(const char* name, const char* type, const char* signature);
   TTypedefDoc*  AddTypedef(const char* name, const TDocString& underlying);
   void          AddSeeAlso(TDocumented* seealso);
   void          SetHtmlFileName(const char* htmlfilename) { fHtmlFileName = htmlfilename; }

   Bool_t        HaveSource() { return !fSrcFiles.IsEmpty(); }
   TCollection&  GetFiles() { return fSrcFiles; }

   void          SetSelected(Bool_t sel = kTRUE) { fSelected = sel; }
   Bool_t        IsSelected() const { return kTRUE; }

private:
   TDocString fShortDoc; // short documentation
   TString    fModule; // module that this class belongs to
   EKind fKind; // kind of element

   TDocStringTable fRefTypes; // strings referenced by the struct and its members
   TDocStringTable fRefFiles; // files containing the struct and its members
   TList fMembers; // data (TDataDoc) of function (TFunctionDoc) members
   TList fTypes; // types contained in this class, list of TTypedefDoc
   TList fSeeAlso; // types referenced by this class and typedefs of this class; list of TObjString

   mutable TString fHtmlFileName; //! cached name of the HTML doc file
   TList fSrcFiles; //! collection of TFileSysEntry (decl, impl) associated with this class
   Bool_t fSelected; //! whether selected for documentation output

   ClassDef(TClassDoc, 1); // Documentation for a class
};
} // namespace Doc

#endif // ROOT_TClassDoc

