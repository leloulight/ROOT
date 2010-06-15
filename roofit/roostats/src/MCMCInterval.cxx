// @(#)root/roostats:$Id$
// Authors: Kevin Belasco        17/06/2009
// Authors: Kyle Cranmer         17/06/2009
/*************************************************************************
 * Copyright (C) 1995-2008, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//_________________________________________________
/*
BEGIN_HTML
<p>
MCMCInterval is a concrete implementation of the RooStats::ConfInterval
interface.  It takes as input Markov Chain of data points in the parameter
space generated by Monte Carlo using the Metropolis algorithm.  From the Markov
Chain, the confidence interval can be determined in two ways:
</p>

<p>
Using a Kernel-Estimated PDF: (not the default method)
</p>
<p>
A RooNDKeysPdf is constructed from the data set using adaptive kernel width.
With this RooNDKeysPdf F, we then integrate over the most likely domain in the
parameter space (tallest points in the posterior RooNDKeysPdf) until the target
confidence level is reached within an acceptable neighborhood as defined by
SetEpsilon(). More specifically: we calculate the following for different
cutoff values C until we reach the target confidence level: \int_{ F >= C } F
d{normset}.
Important note: this is not the default method because of a bug in constructing
the RooNDKeysPdf from a weighted data set.  Configure to use this method by
calling SetUseKeys(true), and the data set will be interpreted without weights.
</p>

<p>
Using a binned data set: (the default method)
</p>
This is the binned analog of the continuous integrative method that uses the
kernel-estimated PDF.  The points in the Markov Chain are put into a binned
data set and the interval is then calculated by adding the heights of the bins
in decreasing order until the desired level of confidence has been reached.
Note that this means the actual confidence level is >= the confidence level
prescribed by the client (unless the user calls SetHistStrict(kFALSE)).  This
method is the default but may not remain as such in future releases, so you may
wish to explicitly configure to use this method by calling SetUseKeys(false)
</p>

<p>
These are not the only ways for the confidence interval to be determined, and
other possibilities are being considered being added, especially for the
1-dimensional case.
</p>

<p>
One can ask an MCMCInterval for the lower and upper limits on a specific
parameter of interest in the interval.  Note that this works better for some
distributions (ones with exactly one local maximum) than others, and sometimes
has little value.
</p>
END_HTML
*/
//_________________________________________________

#ifndef ROOT_Rtypes
#include "Rtypes.h"
#endif

#ifndef ROOT_TMath
#include "TMath.h"
#endif

#ifndef RooStats_MCMCInterval
#include "RooStats/MCMCInterval.h"
#endif
#ifndef ROOSTATS_MarkovChain
#include "RooStats/MarkovChain.h"
#endif
#ifndef RooStats_Heaviside
#include "RooStats/Heaviside.h"
#endif
#ifndef ROO_DATA_HIST
#include "RooDataHist.h"
#endif
#ifndef ROO_KEYS_PDF
#include "RooNDKeysPdf.h"
#endif
#ifndef ROO_PRODUCT
#include "RooProduct.h"
#endif
#ifndef RooStats_RooStatsUtils
#include "RooStats/RooStatsUtils.h"
#endif
#ifndef ROO_REAL_VAR
#include "RooRealVar.h"
#endif
#ifndef ROO_ARG_LIST
#include "RooArgList.h"
#endif
#ifndef ROOT_TIterator
#include "TIterator.h"
#endif
#ifndef ROOT_TH1
#include "TH1.h"
#endif
#ifndef ROOT_TH1F
#include "TH1F.h"
#endif
#ifndef ROOT_TH2F
#include "TH2F.h"
#endif
#ifndef ROOT_TH3F
#include "TH3F.h"
#endif
#ifndef ROO_MSG_SERVICE
#include "RooMsgService.h"
#endif
#ifndef ROO_GLOBAL_FUNC
#include "RooGlobalFunc.h"
#endif
#ifndef ROOT_TObject
#include "TObject.h"
#endif
#ifndef ROOT_THnSparse
#include "THnSparse.h"
#endif
//#ifndef ROOT_TFile
//#include "TFile.h"
//#endif

#include <cstdlib>
#include <string>
#include <algorithm>

ClassImp(RooStats::MCMCInterval);

using namespace RooFit;
using namespace RooStats;
using namespace std;

static const Double_t DEFAULT_EPSILON = 0.01;
static const Double_t DEFAULT_DELTA   = 10e-6;

MCMCInterval::MCMCInterval(const char* name)
   : ConfInterval(name)
{
   fConfidenceLevel = 0.0;
   fHistConfLevel = 0.0;
   fKeysConfLevel = 0.0;
   fFull = 0.0;
   fChain = NULL;
   fAxes = NULL;
   fDataHist = NULL;
   fSparseHist = NULL;
   fKeysPdf = NULL;
   fProduct = NULL;
   fHeaviside = NULL;
   fKeysDataHist = NULL;
   fCutoffVar = NULL;
   fHist = NULL;
   fNumBurnInSteps = 0;
   fHistCutoff = -1;
   fKeysCutoff = -1;
   fDimension = 1;
   fUseKeys = kFALSE;
   fUseSparseHist = kFALSE;
   fIsHistStrict = kTRUE;
   fEpsilon = DEFAULT_EPSILON;
   fDelta = DEFAULT_DELTA;
   fIntervalType = kShortest;
}

MCMCInterval::MCMCInterval(const char* name,
        const RooArgSet& parameters, MarkovChain& chain) : ConfInterval(name)
{
   fNumBurnInSteps = 0;
   fConfidenceLevel = 0.0;
   fHistConfLevel = 0.0;
   fKeysConfLevel = 0.0;
   fFull = 0.0;
   fAxes = NULL;
   fChain = &chain;
   fDataHist = NULL;
   fSparseHist = NULL;
   fKeysPdf = NULL;
   fProduct = NULL;
   fHeaviside = NULL;
   fKeysDataHist = NULL;
   fCutoffVar = NULL;
   fHist = NULL;
   fHistCutoff = -1;
   fKeysCutoff = -1;
   fUseKeys = kFALSE;
   fUseSparseHist = kFALSE;
   fIsHistStrict = kTRUE;
   fEpsilon = DEFAULT_EPSILON;
   SetParameters(parameters);
   fDelta = DEFAULT_DELTA;
   fIntervalType = kShortest;
}

MCMCInterval::~MCMCInterval()
{
   // destructor
   delete[] fAxes;
   delete fHist;
   delete fChain;
   // kbelasco: check here for memory management errors
   delete fDataHist;
   delete fSparseHist;
   delete fKeysPdf;
   delete fProduct;
   delete fHeaviside;
   delete fKeysDataHist;
   delete fCutoffVar;
}

struct CompareDataHistBins {
   CompareDataHistBins(RooDataHist* hist) : fDataHist(hist) {}
   bool operator() (Int_t bin1 , Int_t bin2) { 
      fDataHist->get(bin1);
      Double_t n1 = fDataHist->weight();
      fDataHist->get(bin2);
      Double_t n2 = fDataHist->weight();
      return (n1 < n2);
   }
   RooDataHist* fDataHist; 
};

struct CompareSparseHistBins {
   CompareSparseHistBins(THnSparse* hist) : fSparseHist(hist) {}
   bool operator() (Long_t bin1, Long_t bin2) { 
      Double_t n1 = fSparseHist->GetBinContent(bin1);
      Double_t n2 = fSparseHist->GetBinContent(bin2);
      return (n1 < n2);
   }
   THnSparse* fSparseHist; 
};

Bool_t MCMCInterval::IsInInterval(const RooArgSet& point) const 
{
   if (fUseKeys) {
      // evaluate keyspdf at point and return whether >= cutoff
      // kbelasco: is this right?
      RooStats::SetParameters(&point, const_cast<RooArgSet *>(&fParameters) );
      return fKeysPdf->getVal(&fParameters) >= fKeysCutoff;
   } else {
      if (fUseSparseHist) {
         Long_t bin;
         // kbelasco: consider making x static
         Double_t* x = new Double_t[fDimension];
         for (Int_t i = 0; i < fDimension; i++)
            x[i] = fAxes[i]->getVal();
         bin = fSparseHist->GetBin(x, kFALSE);
         Double_t weight = fSparseHist->GetBinContent((Long64_t)bin);
         delete[] x;
         return (weight >= (Double_t)fHistCutoff);
      } else {
         Int_t bin;
         bin = fDataHist->getIndex(point);
         fDataHist->get(bin);
         return (fDataHist->weight() >= (Double_t)fHistCutoff);
      }
   }
}

void MCMCInterval::SetConfidenceLevel(Double_t cl)
{
   fConfidenceLevel = cl;
   DetermineInterval();
}

// kbelasco: update this or just take it out
// kbelasco: consider keeping this around but changing the implementation
// to set the number of bins for each RooRealVar and then reacreating the
// histograms
//void MCMCInterval::SetNumBins(Int_t numBins)
//{
//   if (numBins > 0) {
//      fPreferredNumBins = numBins;
//      for (Int_t d = 0; d < fDimension; d++)
//         fNumBins[d] = numBins;
//   }
//   else {
//      coutE(Eval) << "* Error in MCMCInterval::SetNumBins: " <<
//                     "Negative number of bins given: " << numBins << endl;
//      return;
//   }
//
//   // If the histogram already exists, recreate it with the new bin numbers
//   if (fHist != NULL)
//      CreateHist();
//}

void MCMCInterval::SetAxes(RooArgList& axes)
{
   Int_t size = axes.getSize();
   if (size != fDimension) {
      coutE(InputArguments) << "* Error in MCMCInterval::SetAxes: " <<
                               "number of variables in axes (" << size <<
                               ") doesn't match number of parameters ("
                               << fDimension << ")" << endl;
      return;
   }
   for (Int_t i = 0; i < size; i++)
      fAxes[i] = (RooRealVar*)axes.at(i);
}

void MCMCInterval::CreateKeysPdf()
{
   // kbelasco: check here for memory leak.  does RooNDKeysPdf use
   // the RooArgList passed to it or does it make a clone?
   // also check for memory leak from chain, does RooNDKeysPdf clone that?
   if (fAxes == NULL || fParameters.getSize() == 0) {
      coutE(InputArguments) << "Error in MCMCInterval::CreateKeysPdf: "
         << "parameters have not been set." << endl;
      return;
   }

   if (fNumBurnInSteps >= fChain->Size()) {
      coutE(InputArguments) <<
         "MCMCInterval::CreateKeysPdf: creation of Keys PDF failed: " <<
         "Number of burn-in steps (num steps to ignore) >= number of steps " <<
         "in Markov chain." << endl;
      // kbelasco: add protections to prevent creating when
      // fNumBurnInSteps >= fChain->Size()
   }

   RooDataSet* chain = fChain->GetAsDataSet(SelectVars(fParameters),
         EventRange(fNumBurnInSteps, fChain->Size()));
   RooArgList* paramsList = new RooArgList();
   for (Int_t i = 0; i < fDimension; i++)
      paramsList->add(*fAxes[i]);

   // kbelasco: check for memory leaks with chain.  who owns it? does
   // RooNDKeysPdf take ownership?
   fKeysPdf = new RooNDKeysPdf("keysPDF", "Keys PDF", *paramsList, *chain, "a");
   //fKeysPdf = new RooNDKeysPdf("keysPDF", "Keys PDF", *paramsList, *chain, "a");
   fCutoffVar = new RooRealVar("cutoff", "cutoff", 0);
   fHeaviside = new Heaviside("heaviside", "Heaviside", *fKeysPdf, *fCutoffVar);
   fProduct = new RooProduct("product", "Keys PDF & Heaviside Product",
                                        RooArgSet(*fKeysPdf, *fHeaviside));
}

void MCMCInterval::CreateHist()
{
   if (fAxes == NULL || fChain == NULL) {
      coutE(Eval) << "* Error in MCMCInterval::CreateHist(): " <<
                     "Crucial data member was NULL." << endl;
      coutE(Eval) << "Make sure to fully construct/initialize." << endl;
      return;
   }
   if (fHist == NULL)
      delete fHist;

   if (fDimension == 1)
      fHist = new TH1F("posterior", "MCMC Posterior Histogram",
            fAxes[0]->numBins(), fAxes[0]->getMin(), fAxes[0]->getMax());

   else if (fDimension == 2)
      fHist = new TH2F("posterior", "MCMC Posterior Histogram",
            fAxes[0]->numBins(), fAxes[0]->getMin(), fAxes[0]->getMax(),
            fAxes[1]->numBins(), fAxes[1]->getMin(), fAxes[1]->getMax());

   else if (fDimension == 3) 
      fHist = new TH3F("posterior", "MCMC Posterior Histogram",
            fAxes[0]->numBins(), fAxes[0]->getMin(), fAxes[0]->getMax(),
            fAxes[1]->numBins(), fAxes[1]->getMin(), fAxes[1]->getMax(),
            fAxes[2]->numBins(), fAxes[2]->getMin(), fAxes[2]->getMax());

   else {
      coutE(Eval) << "* Error in MCMCInterval::CreateHist() : " <<
                     "TH1* couldn't handle dimension: " << fDimension << endl;
      return;
   }

   if (fNumBurnInSteps >= fChain->Size()) {
      coutE(InputArguments) <<
         "MCMCInterval::CreateHist: creation of histogram failed: " <<
         "Number of burn-in steps (num steps to ignore) >= number of steps " <<
         "in Markov chain." << endl;
      // kbelasco: consider adding protections to prevent creating when
      // fNumBurnInSteps >= fChain->Size()
      // but here it might be ok because we know it will be empty
   }

   // Fill histogram
   Int_t size = fChain->Size();
   const RooArgSet* entry;
   for (Int_t i = fNumBurnInSteps; i < size; i++) {
      entry = fChain->Get(i);
      if (fDimension == 1)
         ((TH1F*)fHist)->Fill(entry->getRealValue(fAxes[0]->GetName()),
                              fChain->Weight());
      else if (fDimension == 2)
         ((TH2F*)fHist)->Fill(entry->getRealValue(fAxes[0]->GetName()),
                              entry->getRealValue(fAxes[1]->GetName()),
                              fChain->Weight());
      else
         ((TH3F*)fHist)->Fill(entry->getRealValue(fAxes[0]->GetName()),
                              entry->getRealValue(fAxes[1]->GetName()),
                              entry->getRealValue(fAxes[2]->GetName()),
                              fChain->Weight());
   }
}

void MCMCInterval::CreateSparseHist()
{
   if (fAxes == NULL || fChain == NULL) {
      coutE(Eval) << "* Error in MCMCInterval::CreateSparseHist(): " <<
                     "Crucial data member was NULL." << endl;
      coutE(Eval) << "Make sure to fully construct/initialize." << endl;
      return;
   }
   if (fSparseHist != NULL)
      delete fSparseHist;

   Double_t* min = new Double_t[fDimension];
   Double_t* max = new Double_t[fDimension];
   Int_t* bins = new Int_t[fDimension];
   for (Int_t i = 0; i < fDimension; i++) {
      min[i] = fAxes[i]->getMin();
      max[i] = fAxes[i]->getMax();
      bins[i] = fAxes[i]->numBins();
   }
   fSparseHist = new THnSparseF("posterior", "MCMC Posterior Histogram",
         fDimension, bins, min, max);
   // kbelasco: check here for memory leaks: does THnSparse copy min, max, and bins
   // or can we not delete them?
   //delete[] min;
   //delete[] max;
   //delete[] bins;

   // kbelasco: it appears we need to call Sumw2() just to get the
   // histogram to keep a running total of the weight so that Getsumw doesn't
   // just return 0
   fSparseHist->Sumw2();

   if (fNumBurnInSteps >= fChain->Size()) {
      coutE(InputArguments) <<
         "MCMCInterval::CreateSparseHist: creation of histogram failed: " <<
         "Number of burn-in steps (num steps to ignore) >= number of steps " <<
         "in Markov chain." << endl;
      // kbelasco: add protections to prevent creating when
      // fNumBurnInSteps >= fChain->Size()
   }

   // Fill histogram
   Int_t size = fChain->Size();
   const RooArgSet* entry;
   Double_t* x = new Double_t[fDimension];
   for (Int_t i = fNumBurnInSteps; i < size; i++) {
      entry = fChain->Get(i);
      for (Int_t ii = 0; ii < fDimension; ii++)
         x[ii] = entry->getRealValue(fAxes[ii]->GetName());
      fSparseHist->Fill(x, fChain->Weight());
   }
   delete[] x;
}

void MCMCInterval::CreateDataHist()
{
   if (fParameters.getSize() == 0 || fChain == NULL) {
      coutE(Eval) << "* Error in MCMCInterval::CreateDataHist(): " <<
                     "Crucial data member was NULL." << endl;
      coutE(Eval) << "Make sure to fully construct/initialize." << endl;
      return;
   }

   if (fNumBurnInSteps >= fChain->Size()) {
      coutE(InputArguments) <<
         "MCMCInterval::CreateDataHist: creation of histogram failed: " <<
         "Number of burn-in steps (num steps to ignore) >= number of steps " <<
         "in Markov chain." << endl;
      // kbelasco: add protections to prevent creating when
      // fNumBurnInSteps >= fChain->Size()
   }

   fDataHist = fChain->GetAsDataHist(SelectVars(fParameters),
         EventRange(fNumBurnInSteps, fChain->Size()));
}

void MCMCInterval::SetParameters(const RooArgSet& parameters)
{
   fParameters.removeAll();
   fParameters.add(parameters);
   fDimension = fParameters.getSize();
   if (fAxes != NULL)
      delete[] fAxes;
   fAxes = new RooRealVar*[fDimension];
   TIterator* it = fParameters.createIterator();
   Int_t n = 0;
   TObject* obj;
   while ((obj = it->Next()) != NULL) {
      if (dynamic_cast<RooRealVar*>(obj) != NULL)
         fAxes[n] = (RooRealVar*)obj;
      else
         coutE(Eval) << "* Error in MCMCInterval::SetParameters: " <<
                     obj->GetName() << " not a RooRealVar*" << std::endl;
      n++;
   }
   delete it;
}

void MCMCInterval::DetermineInterval()
{
   switch (fIntervalType) {
      case kShortest:
         DetermineShortestInterval();
         break;
      case kLower:
         DetermineLowerInterval();
         break;
      case kCentral:
         DetermineCentralInterval();
         break;
      case kUpper:
         DetermineUpperInterval();
         break;
      default:
         coutE(InputArguments) << "MCMCInterval::DetermineInterval(): " <<
            "Error: Interval type not set" << endl;
         break;
   }
}

void MCMCInterval::DetermineShortestInterval()
{
   if (fUseKeys)
      DetermineByKeys();
   else
      DetermineByHist();
}

void MCMCInterval::DetermineLowerInterval()
{
   if (fDimension != 1) {
      coutE(InputArguments) << "MCMCInterval::DetermineLowerInterval(): " <<
         "Error: Can only find a lower interval for 1-D intervals" << endl;
      return;
   }
   // kbelasco: fill in code here to find interval
}

void MCMCInterval::DetermineCentralInterval()
{
   if (fDimension != 1) {
      coutE(InputArguments) << "MCMCInterval::DetermineCentralInterval(): " <<
         "Error: Can only find a central interval for 1-D intervals" << endl;
      return;
   }
   // kbelasco: fill in code here to find interval
}

void MCMCInterval::DetermineUpperInterval()
{
   if (fDimension != 1) {
      coutE(InputArguments) << "MCMCInterval::DetermineUpperInterval(): " <<
         "Error: Can only find a upper interval for 1-D intervals" << endl;
      return;
   }
   // kbelasco: fill in code here to find interval
}

void MCMCInterval::DetermineByKeys()
{
   if (fKeysPdf == NULL)
      CreateKeysPdf();
   // now we have a keys pdf of the posterior

   Double_t cutoff = 0.0;
   fCutoffVar->setVal(cutoff);
   RooAbsReal* integral = fProduct->createIntegral(fParameters, NormSet(fParameters));
   Double_t full = integral->getVal(fParameters);
   fFull = full;
   delete integral;
   if (full < 0.98) {
      coutW(Eval) << "Warning: Integral of Keys PDF came out to " << full
         << " intead of expected value 1.  Will continue using this "
         << "factor to normalize further integrals of this PDF." << endl;
   }

   // kbelasco: Is there a better way to set the search range?
   // from 0 to max value of Keys
   // kbelasco: how to get max value?
   //Double_t max = product.maxVal(product.getMaxVal(fParameters));

   Double_t volume = 1.0;
   TIterator* it = fParameters.createIterator();
   RooRealVar* var;
   while ((var = (RooRealVar*)it->Next()) != NULL)
      volume *= (var->getMax() - var->getMin());
   delete it;

   Double_t topCutoff = full / volume;
   Double_t bottomCutoff = topCutoff;
   Double_t confLevel = CalcConfLevel(topCutoff, full);
   if (AcceptableConfLevel(confLevel)) {
      fKeysConfLevel = confLevel;
      fKeysCutoff = topCutoff;
      return;
   }
   Bool_t changed = kFALSE;
   // find high end of range
   while (confLevel > fConfidenceLevel) {
      topCutoff *= 2.0;
      confLevel = CalcConfLevel(topCutoff, full);
      if (AcceptableConfLevel(confLevel)) {
         fKeysConfLevel = confLevel;
         fKeysCutoff = topCutoff;
         return;
      }
      changed = kTRUE;
   }
   if (changed) {
      bottomCutoff = topCutoff / 2.0;
   } else {
      changed = kFALSE;
      bottomCutoff /= 2.0;
      confLevel = CalcConfLevel(bottomCutoff, full);
      if (AcceptableConfLevel(confLevel)) {
         fKeysConfLevel = confLevel;
         fKeysCutoff = bottomCutoff;
         return;
      }
      while (confLevel < fConfidenceLevel) {
         bottomCutoff /= 2.0;
         confLevel = CalcConfLevel(bottomCutoff, full);
         if (AcceptableConfLevel(confLevel)) {
            fKeysConfLevel = confLevel;
            fKeysCutoff = bottomCutoff;
            return;
         }
         changed = kTRUE;
      }
      if (changed) {
         topCutoff = bottomCutoff * 2.0;
      }
   }

   printf("range set: [%g, %g]\n", bottomCutoff, topCutoff);

   cutoff = (topCutoff + bottomCutoff) / 2.0;
   confLevel = CalcConfLevel(cutoff, full);

   // need to use WithinDeltaFraction() because sometimes the integrating the
   // posterior in this binary search seems to not have enough granularity to
   // find an acceptable conf level (small no. of strange cases).
   // WithinDeltaFraction causes the search to terminate when
   // topCutoff is essentially equal to bottomCutoff (compared to the magnitude
   // of their mean).
   while (!AcceptableConfLevel(confLevel) &&
          !WithinDeltaFraction(topCutoff, bottomCutoff)) {
      if (confLevel > fConfidenceLevel)
         bottomCutoff = cutoff;
      else if (confLevel < fConfidenceLevel)
         topCutoff = cutoff;
      cutoff = (topCutoff + bottomCutoff) / 2.0;
      printf("[%g, %g]\n", bottomCutoff, topCutoff);
      confLevel = CalcConfLevel(cutoff, full);
   }

   fKeysCutoff = cutoff;
   fKeysConfLevel = confLevel;
}

void MCMCInterval::DetermineByHist()
{
   if (fUseSparseHist)
      DetermineBySparseHist();
   else
      DetermineByDataHist();
}

void MCMCInterval::DetermineBySparseHist()
{
   Long_t numBins;
   if (fSparseHist == NULL)
      CreateSparseHist();

   numBins = (Long_t)fSparseHist->GetNbins();

   std::vector<Long_t> bins(numBins);
   for (Int_t ibin = 0; ibin < numBins; ibin++)
      bins[ibin] = (Long_t)ibin;
   std::stable_sort(bins.begin(), bins.end(), CompareSparseHistBins(fSparseHist));

   Double_t nEntries = fSparseHist->GetSumw();
   Double_t sum = 0;
   Double_t content;
   Int_t i;
   // see above note on indexing to understand numBins - 3
   for (i = numBins - 1; i >= 0; i--) {
      content = fSparseHist->GetBinContent(bins[i]);
      if ((sum + content) / nEntries >= fConfidenceLevel) {
         fHistCutoff = content;
         if (fIsHistStrict) {
            sum += content;
            i--;
            break;
         } else {
            i++;
            break;
         }
      }
      sum += content;
   }

   if (fIsHistStrict) {
      // keep going to find the sum
      for ( ; i >= 0; i--) {
         content = fSparseHist->GetBinContent(bins[i]);
         if (content == fHistCutoff)
            sum += content;
         else
            break; // content must be < fHistCutoff
      }
   } else {
      // backtrack to find the cutoff and sum
      for ( ; i < numBins; i++) {
         content = fSparseHist->GetBinContent(bins[i]);
         if (content > fHistCutoff) {
            fHistCutoff = content;
            break;
         } else // content == fHistCutoff
            sum -= content;
         if (i == numBins - 1)
            // still haven't set fHistCutoff correctly yet, and we have no bins
            // left, so set fHistCutoff to something higher than the tallest bin
            fHistCutoff = content + 1.0;
      }
   }

   fHistConfLevel = sum / nEntries;
}

void MCMCInterval::DetermineByDataHist()
{
   Int_t numBins;
   if (fDataHist == NULL)
      CreateDataHist();

   numBins = fDataHist->numEntries();

   std::vector<Int_t> bins(numBins);
   for (Int_t ibin = 0; ibin < numBins; ibin++)
      bins[ibin] = ibin;
   std::stable_sort(bins.begin(), bins.end(), CompareDataHistBins(fDataHist));

   Double_t nEntries = fDataHist->sum(kFALSE);
   Double_t sum = 0;
   Double_t content;
   Int_t i;
   for (i = numBins - 1; i >= 0; i--) {
      fDataHist->get(bins[i]);
      content = fDataHist->weight();
      if ((sum + content) / nEntries >= fConfidenceLevel) {
         fHistCutoff = content;
         if (fIsHistStrict) {
            sum += content;
            i--;
            break;
         } else {
            i++;
            break;
         }
      }
      sum += content;
   }

   if (fIsHistStrict) {
      // keep going to find the sum
      for ( ; i >= 0; i--) {
         fDataHist->get(bins[i]);
         content = fDataHist->weight();
         if (content == fHistCutoff)
            sum += content;
         else
            break; // content must be < fHistCutoff
      }
   } else {
      // backtrack to find the cutoff and sum
      for ( ; i < numBins; i++) {
         fDataHist->get(bins[i]);
         content = fDataHist->weight();
         if (content > fHistCutoff) {
            fHistCutoff = content;
            break;
         } else // content == fHistCutoff
            sum -= content;
         if (i == numBins - 1)
            // still haven't set fHistCutoff correctly yet, and we have no bins
            // left, so set fHistCutoff to something higher than the tallest bin
            fHistCutoff = content + 1.0;
      }
   }

   fHistConfLevel = sum / nEntries;
}

Double_t MCMCInterval::GetActualConfidenceLevel()
{
   if (fUseKeys)
      return fKeysConfLevel;
   else
      return fHistConfLevel;
}

Double_t  MCMCInterval::GetSumOfWeights() const { 
   return fDataHist->sum(kFALSE);
}

Double_t MCMCInterval::LowerLimit(RooRealVar& param)
{
   if (fUseKeys)
      return LowerLimitByKeys(param);
   else
      return LowerLimitByHist(param);
}

Double_t MCMCInterval::UpperLimit(RooRealVar& param)
{
   if (fUseKeys)
      return UpperLimitByKeys(param);
   else
      return UpperLimitByHist(param);
}

// Determine the lower limit for param on this interval
// using the binned data set
Double_t MCMCInterval::LowerLimitByHist(RooRealVar& param)
{
   if (fUseSparseHist)
      return LowerLimitBySparseHist(param);
   else
      return LowerLimitByDataHist(param);
}

// Determine the upper limit for param on this interval
// using the binned data set
Double_t MCMCInterval::UpperLimitByHist(RooRealVar& param)
{
   if (fUseSparseHist)
      return UpperLimitBySparseHist(param);
   else
      return UpperLimitByDataHist(param);
}

// Determine the lower limit for param on this interval
// using the binned data set
Double_t MCMCInterval::LowerLimitBySparseHist(RooRealVar& param)
{
   if (fDimension != 1) {
      coutE(InputArguments) << "In MCMCInterval::LowerLimitBySparseHist: "
         << "Sorry, will not compute lower limit unless dimension == 1" << endl;
      return param.getMin();
   }
   if (fHistCutoff < 0)
      DetermineBySparseHist(); // this initializes fSparseHist

   for (Int_t d = 0; d < fDimension; d++) {
      if (strcmp(fAxes[d]->GetName(), param.GetName()) == 0) {
         Long_t numBins = (Long_t)fSparseHist->GetNbins();
         Double_t lowerLimit = param.getMax();
         Double_t val;
         Int_t coord;
         for (Long_t i = 0; i < numBins; i++) {
            if (fSparseHist->GetBinContent(i, &coord) >= fHistCutoff) {
               val = fSparseHist->GetAxis(d)->GetBinCenter(coord);
               if (val < lowerLimit)
                  lowerLimit = val;
            }
         }
         return lowerLimit;
      }
   }
   return param.getMin();
}

// Determine the lower limit for param on this interval
// using the binned data set
Double_t MCMCInterval::LowerLimitByDataHist(RooRealVar& param)
{
   if (fHistCutoff < 0)
      DetermineByDataHist(); // this initializes fDataHist

   for (Int_t d = 0; d < fDimension; d++) {
      if (strcmp(fAxes[d]->GetName(), param.GetName()) == 0) {
         Int_t numBins = fDataHist->numEntries();
         Double_t lowerLimit = param.getMax();
         Double_t val;
         for (Int_t i = 0; i < numBins; i++) {
            fDataHist->get(i);
            if (fDataHist->weight() >= fHistCutoff) {
               val = fDataHist->get()->getRealValue(param.GetName());
               if (val < lowerLimit)
                  lowerLimit = val;
            }
         }
         return lowerLimit;
      }
   }
   return param.getMin();
}

// Determine the upper limit for param on this interval
// using the binned data set
Double_t MCMCInterval::UpperLimitBySparseHist(RooRealVar& param)
{
   if (fDimension != 1) {
      coutE(InputArguments) << "In MCMCInterval::UpperLimitBySparseHist: "
         << "Sorry, will not compute upper limit unless dimension == 1" << endl;
      return param.getMax();
   }
   if (fHistCutoff < 0)
      DetermineBySparseHist(); // this initializes fSparseHist

   for (Int_t d = 0; d < fDimension; d++) {
      if (strcmp(fAxes[d]->GetName(), param.GetName()) == 0) {
         Long_t numBins = (Long_t)fSparseHist->GetNbins();
         Double_t upperLimit = param.getMin();
         Double_t val;
         Int_t coord;
         for (Long_t i = 0; i < numBins; i++) {
            if (fSparseHist->GetBinContent(i, &coord) >= fHistCutoff) {
               val = fSparseHist->GetAxis(d)->GetBinCenter(coord);
               if (val > upperLimit)
                  upperLimit = val;
            }
         }
         return upperLimit;
      }
   }
   return param.getMax();
}

// Determine the upper limit for param on this interval
// using the binned data set
Double_t MCMCInterval::UpperLimitByDataHist(RooRealVar& param)
{
   if (fHistCutoff < 0)
      DetermineByDataHist(); // this initializes fDataHist

   for (Int_t d = 0; d < fDimension; d++) {
      if (strcmp(fAxes[d]->GetName(), param.GetName()) == 0) {
         Int_t numBins = fDataHist->numEntries();
         Double_t upperLimit = param.getMin();
         Double_t val;
         for (Int_t i = 0; i < numBins; i++) {
            fDataHist->get(i);
            if (fDataHist->weight() >= fHistCutoff) {
               val = fDataHist->get()->getRealValue(param.GetName());
               if (val > upperLimit)
                  upperLimit = val;
            }
         }
         return upperLimit;
      }
   }
   return param.getMax();
}

// Determine the lower limit for param on this interval
// using the keys pdf
Double_t MCMCInterval::LowerLimitByKeys(RooRealVar& param)
{
   if (fKeysCutoff < 0)
      DetermineByKeys();

   if (fKeysDataHist == NULL)
      CreateKeysDataHist();

   if (fKeysDataHist == NULL) {
      // if its still NULL, there was an error in making it
      coutE(Eval) << "in MCMCInterval::LowerLimitByKeys(): "
                  << "Could not fill RooDataHist from RooProduct.  "
                  << "Returning param.getMin()" << endl;
      return param.getMin();
   }

   for (Int_t d = 0; d < fDimension; d++) {
      if (strcmp(fAxes[d]->GetName(), param.GetName()) == 0) {
         Int_t numBins = fKeysDataHist->numEntries();
         Double_t lowerLimit = param.getMax();
         Double_t val;
         for (Int_t i = 0; i < numBins; i++) {
            fKeysDataHist->get(i);
            if (fKeysDataHist->weight() >= fKeysCutoff) {
               val = fKeysDataHist->get()->getRealValue(param.GetName());
               if (val < lowerLimit)
                  lowerLimit = val;
            }
         }
         return lowerLimit;
      }
   }
   return param.getMin();
}

// Determine the upper limit for param on this interval
// using the keys pdf
Double_t MCMCInterval::UpperLimitByKeys(RooRealVar& param)
{
   if (fKeysCutoff < 0)
      DetermineByKeys();

   if (fKeysDataHist == NULL)
      CreateKeysDataHist();

   if (fKeysDataHist == NULL) {
      // if its still NULL, there was an error in making it
      coutE(Eval) << "in MCMCInterval::UpperLimitByKeys(): "
                  << "Could not fill RooDataHist from RooProduct.  "
                  << "Returning param.getMax()" << endl;
      return param.getMax();
   }

   for (Int_t d = 0; d < fDimension; d++) {
      if (strcmp(fAxes[d]->GetName(), param.GetName()) == 0) {
         Int_t numBins = fKeysDataHist->numEntries();
         Double_t upperLimit = param.getMin();
         Double_t val;
         for (Int_t i = 0; i < numBins; i++) {
            fKeysDataHist->get(i);
            if (fKeysDataHist->weight() >= fKeysCutoff) {
               val = fKeysDataHist->get()->getRealValue(param.GetName());
               if (val > upperLimit)
                  upperLimit = val;
            }
         }
         return upperLimit;
      }
   }
   return param.getMax();
}

Double_t MCMCInterval::GetHistCutoff()
{
   if (fHistCutoff < 0)
      DetermineByHist();

   return fHistCutoff;
}

Double_t MCMCInterval::GetKeysPdfCutoff()
{
   if (fKeysCutoff < 0)
      DetermineByKeys();

   return fKeysCutoff / fFull;
}

Double_t MCMCInterval::CalcConfLevel(Double_t cutoff, Double_t full)
{
   RooAbsReal* integral;
   Double_t confLevel;
   fCutoffVar->setVal(cutoff);
   integral = fProduct->createIntegral(fParameters, NormSet(fParameters));
   confLevel = integral->getVal(fParameters) / full;
   coutI(Eval) << "cutoff = " << cutoff << ", conf = " << confLevel << endl;
   cout << "tmp: cutoff = " << cutoff << ", conf = " << confLevel << endl;
   delete integral;
   return confLevel;
}

TH1* MCMCInterval::GetPosteriorHist()
{
  if(fConfidenceLevel == 0)
     coutE(InputArguments) << "Error in MCMCInterval::GetPosteriorHist: "
                           << "confidence level not set " << endl;
  if (fHist == NULL)
     CreateHist();
  return (TH1*) fHist->Clone("MCMCposterior_hist");
}

RooNDKeysPdf* MCMCInterval::GetPosteriorKeysPdf()
{
   if (fConfidenceLevel == 0)
      coutE(InputArguments) << "Error in MCMCInterval::GetPosteriorKeysPdf: "
                            << "confidence level not set " << endl;
   if (fKeysPdf == NULL)
      CreateKeysPdf();
   return (RooNDKeysPdf*) fKeysPdf->Clone("MCMCPosterior_keys");
}

RooProduct* MCMCInterval::GetPosteriorKeysProduct()
{
   if (fConfidenceLevel == 0)
      coutE(InputArguments) << "Error in MCMCInterval::GetPosteriorKeysProduct: "
                            << "confidence level not set " << endl;
   if (fKeysPdf == NULL)
      CreateKeysPdf();
   return (RooProduct*) fProduct->Clone("MCMCPosterior_keysproduct");
}

RooArgSet* MCMCInterval::GetParameters() const
{  
   // returns list of parameters
   return new RooArgSet(fParameters);
}

Bool_t MCMCInterval::AcceptableConfLevel(Double_t confLevel)
{
   return (TMath::Abs(confLevel - fConfidenceLevel) < fEpsilon);
}

Bool_t MCMCInterval::WithinDeltaFraction(Double_t a, Double_t b)
{
   return (TMath::Abs(a - b) < TMath::Abs(fDelta * (a + b)/2));
}

void MCMCInterval::CreateKeysDataHist()
{
   if (fProduct == NULL)
      DetermineByKeys();

   fKeysDataHist = new RooDataHist("_productDataHist",
         "Keys PDF & Heaviside Product Data Hist", fParameters);
   fKeysDataHist = fProduct->fillDataHist(fKeysDataHist, &fParameters, 1.);
}

Bool_t MCMCInterval::CheckParameters(const RooArgSet& parameterPoint) const
{  
   // check that the parameters are correct

   if (parameterPoint.getSize() != fParameters.getSize() ) {
     coutE(Eval) << "MCMCInterval: size is wrong, parameters don't match" << std::endl;
     return kFALSE;
   }
   if ( ! parameterPoint.equals( fParameters ) ) {
     coutE(Eval) << "MCMCInterval: size is ok, but parameters don't match" << std::endl;
     return kFALSE;
   }
   return kTRUE;
}
