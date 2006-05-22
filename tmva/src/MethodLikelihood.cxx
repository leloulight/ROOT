// @(#)root/tmva $Id: MethodLikelihood.cxx,v 1.6 2006/05/22 10:33:48 andreas.hoecker Exp $ 
// Author: Andreas Hoecker, Joerg Stelzer, Helge Voss, Kai Voss 

/**********************************************************************************
 * Project: TMVA - a Root-integrated toolkit for multivariate Data analysis       *
 * Package: TMVA                                                                  *
 * Class  : TMVA::MethodLikelihood                                                *
 *                                                                                *
 * Description:                                                                   *
 *      Implementation (see header for description)                               *
 *                                                                                *
 * Authors (alphabetical):                                                        *
 *      Andreas Hoecker <Andreas.Hocker@cern.ch> - CERN, Switzerland              *
 *      Xavier Prudent  <prudent@lapp.in2p3.fr>  - LAPP, France                   *
 *      Helge Voss      <Helge.Voss@cern.ch>     - MPI-KP Heidelberg, Germany     *
 *      Kai Voss        <Kai.Voss@cern.ch>       - U. of Victoria, Canada         *
 *                                                                                *
 * Copyright (c) 2005:                                                            *
 *      CERN, Switzerland,                                                        * 
 *      U. of Victoria, Canada,                                                   * 
 *      MPI-KP Heidelberg, Germany,                                               * 
 *      LAPP, Annecy, France                                                      *
 *                                                                                *
 * Redistribution and use in source and binary forms, with or without             *
 * modification, are permitted according to the terms listed in LICENSE           *
 * (http://mva.sourceforge.net/license.txt)                                       *
 *                                                                                *
 **********************************************************************************/

//_______________________________________________________________________
//Begin_Html
/*                                                                      
Likelihood analysis ("non-parametric approach")                      

<p>
Also implemented is a "diagonalized likelihood approach",            
which improves over the uncorrelated likelihood ansatz by            
transforming linearly the input variables into a diagonal space,     
using the square-root of the covariance matrix                       

<p>                                                                  
The method of maximum likelihood is the most straightforward, and 
certainly among the most elegant multivariate analyser approaches.
We define the likelihood ratio, <i>R<sub>L</sub></i>, for event 
<i>i</i>, by:<br>
<center>
<img vspace=6 src="img/tmva_likratio.gif" align="bottom" > 
</center>
Here the signal and background likelihoods, <i>L<sub>S</sub></i>, 
<i>L<sub>B</sub></i>, are products of the corresponding probability 
densities, <i>p<sub>S</sub></i>, <i>p<sub>B</sub></i>, of the 
<i>N</i><sub>var</sub> discriminating variables used in the MVA: <br>
<center>
<img vspace=6 src="img/tmva_lik.gif" align="bottom" > 
</center>
and accordingly for L<sub>B</sub>.
In practise, TMVA uses polynomial splines to estimate the probability 
density functions (PDF) obtained from the distributions of the 
training variables.<br>

<p>
Note that in TMVA the output of the likelihood ratio is transformed
by<br> 
<center>
<img vspace=6 src="img/tmva_likratio_trans.gif" align="bottom"/> 
</center>
to avoid the occurrence of heavy peaks at <i>R<sub>L</sub></i>=0,1.   

<b>Decorrelated (or "diagonalized") Likelihood</b>

<p>
The biggest drawback of the Likelihood approach is that it assumes
that the discriminant variables are uncorrelated. If it were the case,
it can be proven that the discrimination obtained by the above likelihood
ratio is optimal, ie, no other method can beat it. However, in most 
practical applications of MVAs correlations are present. <br><p></p>

<p>
Linear correlations, measured from the training sample, can be taken 
into account in a straightforward manner through the square-root
of the covariance matrix. The square-root of a matrix
<i>C</i> is the matrix <i>C&prime;</i> that multiplied with itself
yields <i>C</i>: <i>C</i>=<i>C&prime;C&prime;</i>. We compute the 
square-root matrix (SQM) by means of diagonalising (<i>D</i>) the 
covariance matrix: <br>
<center>
<img vspace=6 src="img/tmva_sqm.gif" align="bottom" > 
</center>
and the linear transformation of the linearly correlated into the 
uncorrelated variables space is then given by multiplying the measured
variable tuple by the inverse of the SQM. Note that these transformations
are performed for both signal and background separately, since the
correlation pattern is not the same in the two samples.

<p>
The above diagonalisation is complete for linearly correlated,
Gaussian distributed variables only. In real-world examples this 
is not often the case, so that only little additional information
may be recovered by the diagonalisation procedure. In these cases,
non-linear methods must be applied.
*/
//End_Html
//_______________________________________________________________________

#include "TMatrixD.h"
#include "TVector.h"
#include "TObjString.h"
#include "TFile.h"
#include "TKey.h"

#ifndef ROOT_TMVA_MethodLikelihood
#include "TMVA/MethodLikelihood.h"
#endif
#ifndef ROOT_TMVA_Tools
#include "TMVA/Tools.h"
#endif


namespace TMVA {
  const int MethodLikelihood_max_nvar__=200;
  const Bool_t Transform_Likelihood_Output_=kTRUE;
  const int NmaxVar=200;
}

ClassImp(TMVA::MethodLikelihood)
 
//_______________________________________________________________________
TMVA::MethodLikelihood::MethodLikelihood( TString jobName, vector<TString>* theVariables,  
                                              TTree* theTree, TString theOption, 
                                              TDirectory* theTargetDir )
  : TMVA::MethodBase( jobName, theVariables, theTree, theOption, theTargetDir )
{
  // standard constructor
  //
  // MethodLikelihood options:
  // format and syntax of option string: "Spline2:0:25:D"
  //
  // where:
  //  Splinei [i=1,2,3,5] - which spline is used for smoothing the pdfs
  //                   0  - how often the input histos are smoothed
  //                   25 - average num of events per PDF bin to trigger warning
  //                   D  - use square-root-matrix to decorrelate variable space 
  // 
  InitLik();
  
  if (fOptions.Sizeof()<2) {
    fOptions="Spline2:0:25";
    cout << "--- " << GetName() << ": using default options: " << fOptions << endl;
  }

  // initialize 
  fNsmooth = 5;

  // sanity check
  if (fNvar > TMVA::MethodLikelihood_max_nvar__) {
    cout << "--- " << GetName() << ": too many input variables ==> abort" << endl;
    exit(1);
  }

  // default settings (should be defined in fOptions string)
  TList*  list  = TMVA::Tools::ParseFormatLine( fOptions );

  if (list->GetSize() > 0) {
    TString s = ((TObjString*)list->At(0))->GetString();
    if       (s.Contains("Spline2")) fSmoothMethod = TMVA::PDF::kSpline2;
    else  if (s.Contains("Spline3")) fSmoothMethod = TMVA::PDF::kSpline3;      
    else  if (s.Contains("Spline5")) fSmoothMethod = TMVA::PDF::kSpline3;
    else {
      cout  << "--- " << GetName() << ": WARNING unknown Spline type! Choose Spline2" << endl;
      fSmoothMethod = TMVA::PDF::kSpline2;
    }
  }

  if (list->GetSize() > 1) 
    fNsmooth = atoi( ((TObjString*)list->At(1))->GetString() ) ;
  else                     
    fNsmooth = 0;

  if (list->GetSize() > 2) {
    fAverageEvtPerBin = atoi( ((TObjString*)list->At(2))->GetString() ) ;
    if (fAverageEvtPerBin < 1) fAverageEvtPerBin = 25;
  }
  else 
    fAverageEvtPerBin = 25;

  fDecorrVarSpace = kFALSE;
  if (list->GetSize() > 3) {
    TString s = ((TObjString*)list->At(3))->GetString();
    s.ToUpper();
    if (s == "D" ) {
      fMethodName += "D";
      fTestvar    = fTestvarPrefix+GetMethodName();
      cout << "--- " << GetName() << ": decorrelate variable space" << endl;
      fDecorrVarSpace = kTRUE;    
    }
  }  

  cout << "--- " << GetName() << ": smooth input histos "<<fNsmooth<<" times; ";
  if (fSmoothMethod == TMVA::PDF::kSpline2) cout << "Spline2";
  if (fSmoothMethod == TMVA::PDF::kSpline3) cout << "Spline3";
  if (fSmoothMethod == TMVA::PDF::kSpline5) cout << "Spline5";
  cout << " for smoothing; <events> per bin " << fAverageEvtPerBin << endl;

  
  //--------------------------------------------------------------

  // note that one variable is type
  if (0 != fTrainingTree) {
    
    // trainingTree should only contain those variables that are used in the MVA
    if (fTrainingTree->GetListOfBranches()->GetEntries() - 1 != fNvar) {
      cout << "--- " << GetName() << ": Error: mismatch in number of variables" 
           << " --> abort" << endl;
      exit(1);
    }

    // count number of signal and background events
    fNevt = fTrainingTree->GetEntries();
    fNsig = 0;
    fNbgd = 0;
    for (Int_t ievt = 0; ievt < fNevt; ievt++) {
      if ((Int_t)TMVA::Tools::GetValue( fTrainingTree, ievt, "type" ) == 1) 
        ++fNsig;
      else       
        ++fNbgd;
    }        
    
    // numbers of events should match
    if (fNsig + fNbgd != fNevt) {
      cout << "--- " << GetName() << ": Error: mismatch in number of events" 
           << " --> abort" << endl;
      exit(1);
    }
    
    if (Verbose())
      cout << "--- " << GetName() << " <verbose>: num of events for training (signal, background): "
           << " (" << fNsig << ", " << fNbgd << ")" << endl;
    
    // Likelihood wants same number of events in each species
    if (fNsig != fNbgd) {
      cout << "--- " << GetName() << ":\t--------------------------------------------------"
           << endl;
      cout << "--- " << GetName() << ":\tWarning: different number of signal and background\n"
           << "--- " << GetName() << " \tevents: Likelihood training will not be optimal :-("
           << endl;
      cout << "--- " << GetName() << ":\t--------------------------------------------------"
           << endl;
    }      
  }
  else {    
    fNevt = 0;
    fNsig = 0;
    fNbgd = 0;
  }
}

//_______________________________________________________________________
TMVA::MethodLikelihood::MethodLikelihood( vector<TString> *theVariables, 
                                              TString theWeightFile,  
                                              TDirectory* theTargetDir )
  : TMVA::MethodBase( theVariables, theWeightFile, theTargetDir ) 
{  
  // construct likelihood references from file
  InitLik();
}

//_______________________________________________________________________
void TMVA::MethodLikelihood::InitLik( void )
{
  // default initialisation called by all constructors
  fFin        = NULL;
  fSqS        = NULL;
  fSqB        = NULL;
  fHistSig    = NULL;
  fHistBgd    = NULL; 
  fHistSig_smooth = NULL; 
  fHistBgd_smooth = NULL;
  fPDFSig     = NULL;
  fPDFBgd     = NULL;
  
  fMethodName = "Likelihood";
  fMethod     = TMVA::Types::Likelihood;
  fTestvar    = fTestvarPrefix+GetMethodName();
  fEpsilon    = 1e-5;
  fBgdPDFHist = new TList();
  fSigPDFHist = new TList();

  fNevt       = 0;
  fNsig       = 0;
  fNbgd       = 0;
  
  fHistSig        = new vector<TH1*>     ( fNvar ); 
  fHistBgd        = new vector<TH1*>     ( fNvar ); 
  fHistSig_smooth = new vector<TH1*>     ( fNvar ); 
  fHistBgd_smooth = new vector<TH1*>     ( fNvar );
  fPDFSig         = new vector<TMVA::PDF*>( fNvar );
  fPDFBgd         = new vector<TMVA::PDF*>( fNvar );
}

//_______________________________________________________________________
TMVA::MethodLikelihood::~MethodLikelihood( void )
{
  // destructor
  if (NULL != fSqS)  delete fSqS;
  if (NULL != fSqB)  delete fSqB;
  
  if (NULL != fHistSig) delete  fHistSig;
  if (NULL != fHistBgd) delete  fHistBgd;
  if (NULL != fHistSig_smooth) delete  fHistSig_smooth;
  if (NULL != fHistBgd_smooth) delete  fHistBgd_smooth;
  if (NULL != fPDFSig)  delete  fPDFSig;
  if (NULL != fPDFBgd)  delete  fPDFBgd;

  // delete histos  
  if (NULL != fFin) { fFin->Close(); fFin->Delete(); }
  if (NULL != fFin) { fFin->Close(); fFin->Delete(); }

  delete fBgdPDFHist;
  delete fSigPDFHist;
}

//_______________________________________________________________________
void TMVA::MethodLikelihood::Train( void )
{
  // create reference distributions (PDFs) from signal and background events:
  // fill histograms and smooth them; if decorrelation is required, compute 
  // corresponding square-root matrices

  // default sanity checks
  if (!CheckSanity()) { 
    cout << "--- " << GetName() << ": Error: sanity check failed" << endl;
    exit(1);
  }

  // build square-root matrices for signal and background
  if (fDecorrVarSpace) {

    this->GetSQRMats();

    if (Verbose()) {
      cout << "--- " << GetName() 
           << " <verbose>: SQRT covariance matrix for signal: " << endl;
      fSqS->Print();
      cout << "--- " << GetName() 
           << " <verbose>: SQRT covariance matrix for background: " << endl;
      fSqB->Print();
    }
  }

  // create reference histograms
  fNbins = (Int_t)(TMath::Min(fNsig,fNbgd)/fAverageEvtPerBin);
  
  TString histTitle, histName;
  TVector* vmin = new TVector( fNvar );
  TVector* vmax = new TVector( fNvar );
  TVector* vs   = new TVector( fNvar );
  TVector* vb   = new TVector( fNvar );
  for (Int_t ivar=0; ivar<fNvar; ivar++) { (*vmin)(ivar) = 1e15; (*vmax)(ivar) = -1e15; }

  // search for kinematic borders
  for (Int_t ievt=0; ievt<fTrainingTree->GetEntries(); ievt++) {

    for (Int_t ivar=0; ivar<fNvar; ivar++) {
      Double_t x = TMVA::Tools::GetValue( fTrainingTree, ievt, (*fInputVars)[ivar] );
      (*vs)(ivar) = __N__( x, GetXminNorm( ivar ), GetXmaxNorm( ivar ) );
      (*vb)(ivar) = (*vs)(ivar);
    }

    // the minima and maxima will change after the diagonalization
    if (fDecorrVarSpace) { (*vs) *= (*fSqS); (*vb) *= (*fSqB); }

    for (Int_t ivar=0; ivar<fNvar; ivar++) {
      if ((*vs)(ivar) < (*vmin)(ivar)) (*vmin)(ivar) = (*vs)(ivar);
      if ((*vb)(ivar) < (*vmin)(ivar)) (*vmin)(ivar) = (*vb)(ivar);
      if ((*vs)(ivar) > (*vmax)(ivar)) (*vmax)(ivar) = (*vs)(ivar);
      if ((*vb)(ivar) > (*vmax)(ivar)) (*vmax)(ivar) = (*vb)(ivar);
    }      
  }
  if (Verbose()) {
    cout << "--- " << GetName() << " <verbose>: variable minima and maxima: " << endl;
    vmin->Print();
    vmax->Print();
  }
  
  for (Int_t ivar=0; ivar<fNvar; ivar++) {     
    // for signal events
    histTitle         = (*fInputVars)[ivar] + " signal training";
    histName          = (*fInputVars)[ivar] + "_sig";
    TH1F *htemp    = new TH1F( histName, histTitle, fNbins, (*vmin)(ivar), (*vmax)(ivar) );
    (*fHistSig)[ivar]   = htemp;

    // for background events
    histTitle         = (*fInputVars)[ivar] + " background training";
    histName          = (*fInputVars)[ivar] + "_bgd";
    TH1F *htemp2 = new TH1F( histName, histTitle, fNbins, (*vmin)(ivar), (*vmax)(ivar) );
    (*fHistBgd)[ivar]   = htemp2;    
  }

  delete vmin;
  delete vmax;
  delete vs;
  delete vb;
  
  // ----- fill the reference histograms

  // event loop
  TVector* v = new TVector( fNvar );
  for (Int_t ievt=0; ievt<fTrainingTree->GetEntries(); ievt++) {

    // fill variable vector
    for (Int_t ivar=0; ivar<fNvar; ivar++) {
      Double_t x = TMVA::Tools::GetValue( fTrainingTree, ievt, (*fInputVars)[ivar] );
      (*v)(ivar) = __N__( x, GetXminNorm( ivar ), GetXmaxNorm( ivar ) );
    }

    // compute diagonalized vector
    if ((Int_t)TMVA::Tools::GetValue( fTrainingTree, ievt, "type") == 1) {
      if (fDecorrVarSpace) (*v) *= (*fSqS);
      for (Int_t ivar=0; ivar<fNvar; ivar++) (*fHistSig)[ivar]->Fill( (Float_t)(*v)(ivar) );
    }
    else {
      if (fDecorrVarSpace) (*v) *= (*fSqB);
      for (Int_t ivar=0; ivar<fNvar; ivar++) (*fHistBgd)[ivar]->Fill( (Float_t)(*v)(ivar) );
    }
  }
  delete v;

  // apply smoothing, and create PDFs
  for (Int_t ivar=0; ivar<fNvar; ivar++) { 

    // signal
    (*fHistSig_smooth)[ivar] = (TH1D*)(*fHistSig)[ivar]->Clone();
    histTitle =  (*fInputVars)[ivar] + " signal training  smoothed ";
    histTitle += fNsmooth;
    histTitle += " times";
    histName  = (*fInputVars)[ivar] + "_sig_smooth";
    (*fHistSig_smooth)[ivar]->SetName(histName);
    (*fHistSig_smooth)[ivar]->SetTitle(histTitle);
    if (((*fHistSig_smooth)[ivar]->GetNbinsX() >2 ) && (fNsmooth >0) ){
      (*fHistSig_smooth)[ivar]->Smooth(fNsmooth);
    }
    else {
      if (((*fHistSig_smooth)[ivar]->GetNbinsX() <=2 ) )
      cout << "--- " << GetName() << ": WARNING histogram "<< (*fHistSig_smooth)[ivar]->GetName()
           <<" has not enough ("<<(*fHistSig_smooth)[ivar]->GetNbinsX()
           <<") bins for for smoothing "<<endl;
    }
    (*fPDFSig)[ivar] =  new TMVA::PDF( (*fHistSig_smooth)[ivar], fSmoothMethod );

    // background
    (*fHistBgd_smooth)[ivar] = (TH1D*)(*fHistBgd)[ivar]->Clone();
    histTitle  = (*fInputVars)[ivar] + " background training  smoothed ";
    histTitle += fNsmooth;
    histTitle += " times";
    histName   = (*fInputVars)[ivar] + "_bgd_smooth";
    (*fHistBgd_smooth)[ivar]->SetName(histName);
    (*fHistBgd_smooth)[ivar]->SetTitle(histTitle);
    if (((*fHistBgd_smooth)[ivar]->GetNbinsX() >2) && (fNsmooth >0) ){
      (*fHistBgd_smooth)[ivar]->Smooth(fNsmooth);
    }
    else{
      if (((*fHistBgd_smooth)[ivar]->GetNbinsX() <=2 ) )
      cout << "--- " << GetName() << ": WARNING histogram "<< (*fHistBgd_smooth)[ivar]->GetName()
           <<" has not enough ("<<(*fHistBgd_smooth)[ivar]->GetNbinsX()
           <<") bins for for smoothing "<<endl;
    }
    (*fPDFBgd)[ivar] =  new TMVA::PDF( (*fHistBgd_smooth)[ivar], fSmoothMethod );
  }    

  // write weights to file
  WriteWeightsToFile();

  // the reference histograms to file
  WriteHistosToFile();
}

//_______________________________________________________________________
Double_t TMVA::MethodLikelihood::GetMvaValue( TMVA::Event *e )
{
  // returns the likelihood estimator for signal

  // fill a new Likelihood branch into the testTree
  Int_t    ivar;
  Double_t myMVA;
  TH1D     *hist;
  
  Double_t ps = 1;
  Double_t pb = 1;
  
  // retrieve variables, and transform, if required
  TVector vs( fNvar );
  TVector vb( fNvar );
  for (ivar=0; ivar<fNvar; ivar++) {
    vs(ivar) = __N__( e->GetData(ivar), GetXminNorm( ivar ), GetXmaxNorm( ivar ) );
    vb(ivar) = vs(ivar);
  }
  if (fDecorrVarSpace) { vs *= (*fSqS); vb *= (*fSqB); }

  // compute the likelihood (signal)
  TIter next1(fSigPDFHist);
  for (ivar=0; ivar<fNvar; ivar++) {
    
    Double_t x = vs(ivar);
    
    next1.Reset();
    while ((hist = (TH1D*)next1())) {
      TString varname;
      for (Int_t i=0; i<(((TString)hist->GetName()).Index("_sig_")); i++)
        varname.Append( ( (TString)hist->GetName())(i) );
      if (varname == (*fInputVars)[ivar]) break;        
    }
    
    // interpolate linearly between adjacent bins
    Int_t bin     = hist->FindBin(x);
    Int_t nextbin = bin;
    if ((x > hist->GetBinCenter(bin) && bin != hist->GetNbinsX()) || bin == 1) 
      nextbin++;
    else
      nextbin--;  
    
    Double_t dx   = hist->GetBinCenter(bin)  - hist->GetBinCenter(nextbin);
    Double_t dy   = hist->GetBinContent(bin) - hist->GetBinContent(nextbin);
    Double_t like = hist->GetBinContent(bin) + (x - hist->GetBinCenter(bin)) * dy/dx;
    ps *= max(like, fEpsilon);
  }     
  
  // compute the likelihood (background)
  TIter next2(fBgdPDFHist);
  for (ivar=0; ivar<fNvar; ivar++) {
    
    Double_t x = vb(ivar);
    
    next2.Reset();
    while ((hist = (TH1D*)next2())) {
      TString varname;
      for (Int_t i=0; i<(((TString)hist->GetName()).Index("_bgd_")); i++)
        varname.Append( ( (TString)hist->GetName())(i) );
      if (varname == (*fInputVars)[ivar]) break;        
    }
    
    // interpolate linearly between adjacent bins
    Int_t bin     = hist->FindBin(x);
    Int_t nextbin = bin;
    if ((x > hist->GetBinCenter(bin) && bin != hist->GetNbinsX()) || bin == 1) 
      nextbin++;
    else
      nextbin--;  
    
    Double_t dx   = hist->GetBinCenter(bin)  - hist->GetBinCenter(nextbin);
    Double_t dy   = hist->GetBinContent(bin) - hist->GetBinContent(nextbin);
    Double_t like = hist->GetBinContent(bin) + (x - hist->GetBinCenter(bin)) * dy/dx;
    pb *= max(like, fEpsilon);
  }
  
  // the likelihood
  if (Transform_Likelihood_Output_) {
    // inverse Fermi function
    Double_t r   = ps/(pb+ps);
    Double_t tau = 15.0;
    myMVA = - log(1.0/r - 1.0)/tau;
  }
  else myMVA = ps/(pb+ps);
  
  return myMVA;
}

//_______________________________________________________________________
void TMVA::MethodLikelihood::GetSQRMats( void ) 
{
  // compute square-root matrices for signal and background

  if (NULL != fSqS) { delete fSqS; fSqS = 0; }
  if (NULL != fSqB) { delete fSqB; fSqB = 0; }

  TMatrixDSym *covMatS = new TMatrixDSym( fNvar );
  TMatrixDSym *covMatB = new TMatrixDSym( fNvar );
  fSqS                 = new TMatrixD   ( fNvar, fNvar );
  fSqB                 = new TMatrixD   ( fNvar, fNvar );
  
  TMVA::Tools::GetCovarianceMatrix( fTrainingTree, covMatS, fInputVars, 1, kTRUE );
  TMVA::Tools::GetCovarianceMatrix( fTrainingTree, covMatB, fInputVars, 0, kTRUE );

  TMVA::Tools::GetSQRootMatrix( covMatS, fSqS );
  TMVA::Tools::GetSQRootMatrix( covMatB, fSqB );

  delete covMatS;
  delete covMatB;
}

//_______________________________________________________________________
void  TMVA::MethodLikelihood::WriteWeightsToFile( void )
{  
  // write reference PDFs to file

  TString fname = GetWeightFileName() + ".root";
  cout << "--- " << GetName() << ": creating weight file: " << fname << endl;
  TFile *fout = new TFile( fname, "RECREATE" );

  // build TList of input variables, and TVectors for min/max
  // NOTE: the latter values are mandatory for the normalisation 
  // in the reader application !!!
  TList    lvar;
  TVectorD vmin( fNvar ), vmax( fNvar );
  for (Int_t ivar=0; ivar<fNvar; ivar++) {
    lvar.Add( new TNamed( (*fInputVars)[ivar], TString() ) );
    vmin[ivar] = this->GetXminNorm( ivar );
    vmax[ivar] = this->GetXmaxNorm( ivar );
  }
  // write to file
  lvar.Write();
  vmin.Write( "vmin" );
  vmax.Write( "vmax" );
  lvar.Delete();

  // save configuration options
  // (best would be to use a TMap here, but found implementation really complicated)
  TVectorD likelOptions( 4 );
  likelOptions(0) = (Double_t)fSmoothMethod;
  likelOptions(1) = (Double_t)fNsmooth;
  likelOptions(2) = (Double_t)fAverageEvtPerBin;
  likelOptions(3) = (Double_t)fDecorrVarSpace;
  likelOptions.Write( "LikelihoodOptions" );
  
  // write out SQRT matrices if diagonalization option is set
  if (fDecorrVarSpace) {
    fSqS->Write( "sqS" );
    fSqB->Write( "sqB" );
  }

  // now write the histograms
  for(Int_t ivar=0; ivar<fNvar; ivar++){ 
    (*fPDFSig)[ivar]->GetPDFHist()->Write();
    (*fPDFBgd)[ivar]->GetPDFHist()->Write();
  }                  

  fout->Close();
  delete fout;
}
  
//_______________________________________________________________________
void  TMVA::MethodLikelihood::ReadWeightsFromFile( void )
{
  // read reference PDFs from file

  TString fname = GetWeightFileName();
  if (!fname.EndsWith( ".root" )) fname += ".root";

  cout << "--- " << GetName() << ": reading weight file: " << fname << endl;
  fFin = new TFile(fname);

  // build TList of input variables, and TVectors for min/max
  // NOTE: the latter values are mandatory for the normalisation 
  // in the reader application 
  TList lvar;
  for (Int_t ivar=0; ivar<fNvar; ivar++) {    
    // read variable names
    TNamed t;
    t.Read( (*fInputVars)[ivar] );

    // sanity check
    if (t.GetName() != (*fInputVars)[ivar]) {
      cout << "--- " << GetName() << ": Error while reading weight file; "
           << "unknown variable: " << t.GetName() << " at position: " << ivar << ". "
           << "Expected variable: " << (*fInputVars)[ivar] << " ==> abort" << endl;
      exit(1);
    }
  }

  // read vectors
  TVectorD vmin( fNvar ), vmax( fNvar );
  // unfortunatly the more elegant vmin/max.Read( "vmin/max" ) crash in ROOT <= V4.04.02
  TVectorD *tmp = (TVectorD*)fFin->Get( "vmin" );
  vmin = *tmp;
  tmp  = (TVectorD*)fFin->Get( "vmax" );
  vmax = *tmp;

  // initialize min/max
  for (Int_t ivar=0; ivar<fNvar; ivar++) {    
    this->SetXminNorm( ivar, vmin[ivar] );
    this->SetXmaxNorm( ivar, vmax[ivar] );
  }

  // save configuration options
  // (best would be to use a TMap here, but found implementation really complicated)
  TVectorD* likelOptions = (TVectorD*)fFin->Get( "LikelihoodOptions" );
  fSmoothMethod        = (TMVA::PDF::SmoothMethod)(Int_t)(*likelOptions)(0);
  fNsmooth             = (Int_t)(*likelOptions)(1);
  fAverageEvtPerBin    = (Int_t)(*likelOptions)(2);
  fDecorrVarSpace      = (Bool_t)(*likelOptions)(3);

  // read in SQRT matrices if diagonalization option is set
  if (fDecorrVarSpace) {
    if (NULL != fSqS) { delete fSqS; fSqS = 0; }
    if (NULL != fSqB) { delete fSqB; fSqB = 0; }

    fSqS = new TMatrixD( fNvar, fNvar );
    fSqB = new TMatrixD( fNvar, fNvar );
    
    fSqS->Read( "sqS" );
    fSqB->Read( "sqB" );    
  }

  // now read the histograms
  fSigPDFHist = new TList();
  fBgdPDFHist = new TList();
  
  TIter next(fFin->GetListOfKeys());
  TKey *key;
  while ((key = (TKey*)next())) {
    TClass *cl = gROOT->GetClass(key->GetClassName());
    if (!cl->InheritsFrom("TH1D")) continue;
    TH1D *h = (TH1D*)key->ReadObj();
    TString hname= h->GetName();
    if      (hname.Contains("_sig_")) fSigPDFHist->Add(h);
    else if (hname.Contains("_bgd_")) fBgdPDFHist->Add(h);
  }  
}

//_______________________________________________________________________
void  TMVA::MethodLikelihood::WriteHistosToFile( void )
{
  // write histograms and PDFs to file for monitoring purposes

  cout << "--- " << GetName() << ": write " << GetName() 
       << " special histos to file: " << fBaseDir->GetPath() << endl;
  
  gDirectory->GetListOfKeys()->Print();
  fLocalTDir = fBaseDir->mkdir(GetName()+GetMethodName());
  fLocalTDir->cd();
  for (Int_t ivar=0; ivar<fNvar; ivar++) { 
    fLocalTDir->cd();
    (*fHistSig)[ivar]->Write();    
    (*fHistBgd)[ivar]->Write();
    (*fHistSig_smooth)[ivar]->Write();    
    (*fHistBgd_smooth)[ivar]->Write();
    (*fPDFSig)[ivar]->GetPDFHist()->Write();
    (*fPDFBgd)[ivar]->GetPDFHist()->Write();

    // add special plots to check the smoothing in the GetVal method
    Float_t xmin=((*fPDFSig)[ivar]->GetPDFHist()->GetXaxis())->GetXmin();
    Float_t xmax=((*fPDFSig)[ivar]->GetPDFHist()->GetXaxis())->GetXmax();
    TH1D* mm = new TH1D( (*fInputVars)[ivar]+"_additional_check",
                         (*fInputVars)[ivar]+"_additional_check", 15000, xmin, xmax );
    Double_t intBin = (xmax-xmin)/15000;
    for (Int_t bin=0; bin < 15000; bin++) {
      Double_t x = (bin + 0.5)*intBin + xmin;
      mm->SetBinContent(bin+1 ,(*fPDFSig)[ivar]->GetVal(x));
    }
    mm->Write();
    // delete mm;    
  }                  
}
