/*****************************************************************************
 * Project: BaBar detector at the SLAC PEP-II B-factory
 * Package: RooFitTools
 *    File: $Id: RooUnblindCPAsymVar.rdl,v 1.10 2001/11/20 04:00:55 verkerke Exp $
 * Authors:
 *   WV, Wouter Verkerke, University of California Santa Barbara, verkerke@slac.stanford.edu
 * History:
 *   05-Mar-2001 WV Created initial version
 *
 * Copyright (C) 2001 University of California
 *****************************************************************************/
#ifndef ROO_UNBLIND_CPASYM_VAR
#define ROO_UNBLIND_CPASYM_VAR

#include "RooFitCore/RooAbsHiddenReal.hh"
#include "RooFitCore/RooAbsCategory.hh"
#include "RooFitCore/RooRealProxy.hh"
#include "RooFitCore/RooCategoryProxy.hh"
#include "RooFitModels/RooBlindTools.hh"

class RooCategory ;

class RooUnblindCPAsymVar : public RooAbsHiddenReal {
public:
  // Constructors, assignment etc
  RooUnblindCPAsymVar() ;
  RooUnblindCPAsymVar(const char *name, const char *title, 
			const char *blindString, RooAbsReal& cpasym);
  RooUnblindCPAsymVar(const char *name, const char *title, 
		      const char *blindString, RooAbsReal& cpasym, RooAbsCategory& blindState);
  RooUnblindCPAsymVar(const RooUnblindCPAsymVar& other, const char* name=0);
  virtual TObject* clone(const char* newname) const { return new RooUnblindCPAsymVar(*this,newname); }  
  virtual ~RooUnblindCPAsymVar();

protected:

  // Function evaluation
  virtual Double_t evaluate() const ;

  RooRealProxy _asym ;
  RooBlindTools _blindEngine ;

  ClassDef(RooUnblindCPAsymVar,1) // CP-Asymmetry unblinding transformation
};

#endif
