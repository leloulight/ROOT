/*****************************************************************************
 * Project: BaBar detector at the SLAC PEP-II B-factory
 * Package: RooFitTools
 *    File: $Id: RooFormula.rdl,v 1.19 2001/09/17 18:48:14 verkerke Exp $
 * Authors:
 *   WV, Wouter Verkerke, University of California Santa Barbara, verkerke@slac.stanford.edu
 * History:
 *   05-Mar-2001 WV Created initial version
 *
 * Copyright (C) 2001 University of California
 *****************************************************************************/
#ifndef ROO_FORMULA
#define ROO_FORMULA

#include "Rtypes.h"
#include "TFormula.h"
#include "TObjArray.h"
#include "RooFitCore/RooAbsReal.hh"
#include "RooFitCore/RooArgSet.hh"
#include "RooFitCore/RooPrintable.hh"

class RooFormula : public TFormula, public RooPrintable {
public:
  // Constructors etc.
  RooFormula() ;
  RooFormula(const char* name, const char* formula, const RooArgSet& varList);
  RooFormula(const RooFormula& other, const char* name=0) ;
  RooFormula& operator=(const RooFormula& other) ;
  virtual TObject* Clone(const char* newName=0) const { return new RooFormula(*this) ; }
  virtual ~RooFormula();
	
  // Dependent management
  RooArgSet& actualDependents() const ;
  Bool_t changeDependents(const RooAbsCollection& newDeps, Bool_t mustReplaceAll=kFALSE) ;

  inline RooAbsArg* getParameter(const char* name) const { return (RooAbsArg*) _useList.FindObject(name) ; }
  inline RooAbsArg* getParameter(Int_t index) const { return (RooAbsArg*) _origList.At(index) ; }

  // Function value accessor
  inline Bool_t ok() { return _isOK ; }
  Double_t eval(const RooArgSet* nset=0) ;

  // Debugging
  void dump() ;
  Bool_t reCompile(const char* newFormula) ;

  // Printing interface (human readable)
  virtual void printToStream(ostream& os, PrintOption opt= Standard, TString indent= "") const;
  inline virtual void Print(Option_t *options= 0) const {
    printToStream(defaultStream(),parseOptions(options));
  }

protected:
  
  void initCopy(const RooFormula& other) ;

  // Interface to TFormula engine
  Int_t DefinedVariable(TString &name) ;
  Double_t DefinedValue(Int_t code) ;

  RooArgSet* _nset ;
  Bool_t    _isOK ;
  TList     _origList ;      //! Original list of dependents
  TObjArray _useList ;       //! List of actual dependents 
  mutable RooArgSet _actual; //! Set of actual dependents
  TObjArray _labelList ;     //  List of label names for category objects  

  ClassDef(RooFormula,1)     // TFormula derived class interfacing with RooAbsArg objects
};

#endif
