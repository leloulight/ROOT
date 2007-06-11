// @(#)root/html:$Name:  $:$Id: TClassDocOutput.h,v 1.1 2007/02/07 20:40:38 brun Exp $
// Author: Axel Naumann 2007-01-09

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TClassDocOutput
#define ROOT_TClassDocOutput

#ifndef ROOT_TDocOutput
#include "TDocOutput.h"
#endif

class TDocParser;

class TClassDocOutput: public TDocOutput {
protected:
   enum ETraverse {
      kUp, kDown, kBoth        // direction to traverse class tree in ClassHtmlTree()
   };

   Int_t          fHierarchyLines; // counter for no. lines in hierarchy
   TClass*        fCurrentClass;   // class to generate output for
   TDocParser*    fParser;         // parser we use

   void           ClassHtmlTree(std::ostream &out, TClass *classPtr, ETraverse dir=kBoth, int depth=1);
   void           ClassTree(TVirtualPad *canvas, Bool_t force=kFALSE);

   Bool_t         CreateDotClassChartIncl(const char* filename);
   Bool_t         CreateDotClassChartInh(const char* filename);
   Bool_t         CreateDotClassChartInhMem(const char* filename);
   Bool_t         CreateDotClassChartLib(const char* filename);

   Bool_t         CreateHierarchyDot();
   void           CreateSourceOutputStream(std::ostream& out, const char* extension, TString& filename);
   void           DescendHierarchy(std::ostream &out, TClass* basePtr, Int_t maxLines=0, Int_t depth=1);

   virtual void   ListFunctions(std::ostream& classFile);
   virtual void   ListDataMembers(std::ostream& classFile);

   virtual void   WriteClassDocHeader(std::ostream& classFile);
   virtual void   WriteMethod(std::ostream & out, TString& ret, 
                              TString& name, TString& params,
                              const char* file, TString& anchor,
                              TString& comment, TString& codeOneLiner,
                              TMethod* guessedMethod);
   virtual void   WriteClassDescription(std::ostream& out, const TString& description);

public:
   TClassDocOutput(THtml& html, TClass* cl);
   virtual ~TClassDocOutput();

   void           Class2Html(Bool_t force=kFALSE);
   Bool_t         ClassDotCharts(std::ostream & out);
   void           CreateClassHierarchy(std::ostream& out, const char* docFileName);

   void           MakeTree(Bool_t force = kFALSE);

   friend class TDocParser;

   ClassDef(TClassDocOutput, 0); // generates documentation web pages for a class
};

#endif // ROOT_TClassDocOutput
