// @(#)root/roostats:$Id: FeldmanCousins.h 26805 2009-01-13 17:45:57Z cranmer $
// Author: Kyle Cranmer   January 2009

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
The FeldmanCousins class (like the Feldman-Cousins technique) is essentially a specific configuration
 of the more general NeymanConstruction.  It is a concrete implementation of the IntervalCalculator interface that, which uses the NeymanConstruction in a particular way.  As the name suggests, it returns a ConfidenceInterval.  In particular, it produces a RooStats::PointSetInterval, which is a concrete implementation of the ConfInterval interface.  
</p>
<p>
The Neyman Construction is not a uniquely defined statistical technique, it requires that one specify an ordering rule 
or ordering principle, which is usually incoded by choosing a specific test statistic and limits of integration 
(corresponding to upper/lower/central limits).  As a result, this class must be configured with the corresponding
information before it can produce an interval.  
</p>
<p>In the case of the Feldman-Cousins approach, the ordering principle is the likelihood ratio -- motivated
by the Neyman-Pearson lemma.  When nuisance parameters are involved, the profile likelihood ratio is the natural generalization.  One may either choose to perform the construction over the full space of the nuisance parameters, or restrict the nusiance parameters to their conditional MLE (eg. profiled values). 
</p>
END_HTML
*/
//

#ifndef RooStats_FeldmanCousins
#include "RooStats/FeldmanCousins.h"
#endif

#ifndef RooStats_RooStatsUtils
#include "RooStats/RooStatsUtils.h"
#endif

#ifndef RooStats_PointSetInterval
#include "RooStats/PointSetInterval.h"
#endif

#include "RooStats/SamplingDistribution.h"

#include "RooStats/ProfileLikelihoodTestStat.h"
#include "RooStats/NeymanConstruction.h"

#include "RooDataSet.h"
#include "RooGlobalFunc.h"
#include "TFile.h"
#include "TTree.h"

ClassImp(RooStats::FeldmanCousins) ;

using namespace RooFit;
using namespace RooStats;


//_______________________________________________________
FeldmanCousins::FeldmanCousins() {
   // default constructor
  fWS = new RooWorkspace();
  fOwnsWorkspace = true;
  fDataName = "";
  fPdfName = "";
  fAdaptiveSampling=false;
  fPointsToTest = 0;
  fNbins = 10;
  fFluctuateData=true;
}

//_______________________________________________________
void FeldmanCousins::CreateTestStatSampler() const{
  // specify the Test Statistic and create a ToyMC test statistic sampler

  // get ingredients
  RooAbsPdf* pdf   = fWS->pdf(fPdfName);
  RooAbsData* data = fWS->data(fDataName);
  if (data && pdf ) {

    // get parameters (params of interest + nuisance)
    RooArgSet* parameters = pdf->getParameters(data);
    //RooArgSet* parameters = fPOI;

    // use the profile likelihood ratio as the test statistic
    ProfileLikelihoodTestStat* testStatistic = new ProfileLikelihoodTestStat(*pdf);
  
    // create the ToyMC test statistic sampler
    fTestStatSampler = new ToyMCSampler(*testStatistic) ;
    fTestStatSampler->SetPdf(*pdf);
    fTestStatSampler->SetParameters(*parameters);
    fTestStatSampler->SetNEventsPerToy(data->numEntries());
    fTestStatSampler->SetNToys((int) 50./fSize); // adjust nToys so that at least 50 events outside acceptance region
    fTestStatSampler->SetExtended(fFluctuateData);

    if(!fAdaptiveSampling){
      cout << "ntoys per point = " << (int) 50./fSize << endl;
    } else{
      cout << "ntoys per point: adaptive" << endl;
    }
    cout << "nevents per toy taken from expectation" << endl;
  }
}

//_______________________________________________________
void FeldmanCousins::CreateParameterPoints() const{
  // specify the parameter points to perform the construction.
  // allow ability to profile on some nuisance paramters

  // get ingredients
  RooAbsPdf* pdf   = fWS->pdf(fPdfName);
  RooAbsData* data = fWS->data(fDataName);
  if (data && pdf ){

    // get parameters (params of interest + nuisance)
    RooArgSet* parameters = pdf->getParameters(data);

    TIter it = parameters->createIterator();
    RooRealVar *myarg; 
    while ((myarg = (RooRealVar *)it.Next())) { 
      if(!myarg) continue;
      myarg->setBins(fNbins);
    }

    fPointsToTest= new RooDataHist("parameterScan", "", *parameters);
    cout << "# points to test = " << fPointsToTest->numEntries() << endl;
  }
}


//_______________________________________________________
ConfInterval* FeldmanCousins::GetInterval() const {
  // Main interface to get a RooStats::ConfInterval.  
  // It constructs a RooStats::PointSetInterval.

  // local variables
  RooAbsData* data = fWS->data(fDataName);
  if(!data) {
    cout << "Data is not set, FeldmanCousins not initialized" << endl;
    return 0;
  }
  
  // create the test statistic sampler (private data member fTestStatSampler)
  this->CreateTestStatSampler();

  // create paramter points to perform construction (private data member fPointsToTest)
  this->CreateParameterPoints();

  // Create a Neyman Construction
  RooStats::NeymanConstruction nc;
  // configure it
  nc.SetTestStatSampler(*fTestStatSampler);
  nc.SetTestSize(fSize); // set size of test
  nc.SetParameterPointsToTest( *fPointsToTest );
  nc.SetLeftSideTailFraction(0.); // part of definition of Feldman-Cousins
  nc.SetData(*data);
  nc.UseAdaptiveSampling(fAdaptiveSampling);

  fConfBelt = nc.GetConfidenceBelt();
  // use it
  return nc.GetInterval();
}
