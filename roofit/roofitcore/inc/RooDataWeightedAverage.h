/*****************************************************************************
 * Project: RooFit                                                           *
 * Package: RooFitCore                                                       *
 *    File: $Id$
 * Authors:                                                                  *
 *   WV, Wouter Verkerke, UC Santa Barbara, verkerke@slac.stanford.edu       *
 *   DK, David Kirkby,    UC Irvine,         dkirkby@uci.edu                 *
 *                                                                           *
 * Copyright (c) 2000-2005, Regents of the University of California          *
 *                          and Stanford University. All rights reserved.    *
 *                                                                           *
 * Redistribution and use in source and binary forms,                        *
 * with or without modification, are permitted according to the terms        *
 * listed in LICENSE (http://roofit.sourceforge.net/license.txt)             *
 *****************************************************************************/
#ifndef ROO_NLL_VAR
#define ROO_NLL_VAR

#include "RooAbsOptTestStatistic.h"
#include "RooCmdArg.h"

class RooDataWeightedAverage : public RooAbsOptTestStatistic {
public:

  // Constructors, assignment etc
  RooDataWeightedAverage() {} ;  

  RooDataWeightedAverage(const char *name, const char *title, RooAbsReal& real, RooAbsData& data, 
			 Int_t nCPU=1, Bool_t interleave=kFALSE, Bool_t showProgress=kFALSE, Bool_t verbose=kTRUE) ;

  RooDataWeightedAverage(const RooDataWeightedAverage& other, const char* name=0);
  virtual TObject* clone(const char* newname) const { return new RooDataWeightedAverage(*this,newname); }

  virtual RooAbsTestStatistic* create(const char *name, const char *title, RooAbsReal& real, RooAbsData& data,
				      const RooArgSet& /*projDeps*/, const char* /*rangeName*/=0, const char* /*addCoefRangeName*/=0, 
				      Int_t nCPU=1, Bool_t interleave=kFALSE, Bool_t verbose=kTRUE, Bool_t /*splitCutRange*/=kFALSE) {
    return new RooDataWeightedAverage(name,title,real,data,nCPU,interleave,verbose) ;
  }

  virtual Double_t globalNormalization() const ;

  virtual ~RooDataWeightedAverage();


protected:

  Double_t _sumWeight ;
  Bool_t _showProgress ;
  virtual Double_t evaluatePartition(Int_t firstEvent, Int_t lastEvent, Int_t stepSize) const ;
  
  ClassDef(RooDataWeightedAverage,1) // Abstract real-valued variable
};

#endif
