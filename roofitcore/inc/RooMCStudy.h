/*****************************************************************************
 * Project: BaBar detector at the SLAC PEP-II B-factory
 * Package: RooFitCore
 *    File: $Id: RooMCStudy.rdl,v 1.8 2002/06/13 18:16:36 verkerke Exp $
 * Authors:
 *   WV, Wouter Verkerke, UC Santa Barbara, verkerke@slac.stanford.edu
 *   DK, David Kirkby, Stanford University, kirkby@hep.stanford.edu
 * History:
 *   09-Oct-2001 WV Created initial version
 *
 * Copyright (C) 2001 University of California
 *****************************************************************************/
#ifndef ROO_MC_STUDY
#define ROO_MC_STUDY

#include "TList.h"
#include "RooFitCore/RooArgSet.hh"
class RooAbsPdf;
class RooDataSet ;
class RooAbsData ;
class RooAbsGenContext ;
class RooFitResult ;
class RooPlot ;
class RooRealVar ;

class RooMCStudy {
public:

  RooMCStudy(const RooAbsPdf& genModel, const RooAbsPdf& fitModel, 
	     const RooArgSet& dependents, const char* genOptions="",
	     const char* fitOptions="", const RooDataSet* genProtoData=0,
	     const RooArgSet& projDeps=RooArgSet()) ;
  virtual ~RooMCStudy() ;
  
  // Run methods
  Bool_t generateAndFit(Int_t nSamples, Int_t nEvtPerSample, Bool_t keepGenData=kFALSE, const char* asciiFilePat=0) ;
  Bool_t generate(Int_t nSamples, Int_t nEvtPerSample, Bool_t keepGenData=kFALSE, const char* asciiFilePat=0) ;
  Bool_t fit(Int_t nSamples, const char* asciiFilePat) ;
  Bool_t fit(Int_t nSamples, TList& dataSetList) ;
  Bool_t addFitResult(const RooFitResult& fr) ;

  // Result accessors
  const RooArgSet* fitParams(Int_t sampleNum) const ;
  const RooFitResult* fitResult(Int_t sampleNum) const ;
  const RooDataSet* genData(Int_t sampleNum) const ;
  const RooDataSet& fitParDataSet() ;

  // Plot methods
  RooPlot* plotNLL(Double_t lo, Double_t hi, Int_t nBins=100) ;
  RooPlot* plotParamOn(RooPlot* frame) ;
  RooPlot* plotParam(const RooRealVar& param) ;
  RooPlot* plotError(const RooRealVar& param, Double_t lo, Double_t hi, Int_t nbins=100) ;
  RooPlot* plotPull(const RooRealVar& param, Double_t lo=-3.0, Double_t hi=3.0, Int_t nbins=25, Bool_t fitGauss=kFALSE) ;
    
protected:

  Bool_t run(Bool_t generate, Bool_t fit, Int_t nSamples, Int_t nEvtPerSample, Bool_t keepGenData, const char* asciiFilePat) ;
  Bool_t fitSample(RooAbsData* genSample) ;
  void calcPulls() ;
    
  RooAbsPdf*        _genModel ;    // Generator model 
  RooAbsGenContext* _genContext ;  // Generator context 
  RooArgSet*        _genParams ;   // List of fit parameters
  const RooDataSet* _genProtoData ;// Generator prototype data set
  RooArgSet         _projDeps ;    // List of projected dependents in fit

  RooArgSet    _dependents ;    // List of dependents 
  RooArgSet    _allDependents ; // List of generate + prototype dependents
  RooAbsPdf*   _fitModel ;      // Fit model 
  RooArgSet*   _fitInitParams ; // List of initial values of fit parameters
  RooArgSet*   _fitParams ;     // List of fit parameters
  RooRealVar*  _nllVar ;
  
  TList       _genDataList ;    // List of generated data sample
  TList       _fitResList ;     // List of RooFitResult fit output objects
  RooDataSet* _fitParData ;     // Data set of fit parameters of each sample
  TString     _fitOptions ;     // Fit options string
  Bool_t      _extendedGen ;    // Add poisson term to number of events to generate?
  Bool_t      _binGenData ;     // Bin data between generating and fitting

  Bool_t      _canAddFitResults ; // Allow adding of external fit results?

private:
  RooMCStudy(const RooMCStudy&) ;
	
  ClassDef(RooMCStudy,0) // Monte Carlo study manager
} ;


#endif

