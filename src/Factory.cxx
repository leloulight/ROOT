// @(#)root/tmva $Id: Factory.cxx,v 1.67 2006/09/29 23:27:15 andreas.hoecker Exp $   
// Author: Andreas Hoecker, Joerg Stelzer, Helge Voss, Kai Voss 

/**********************************************************************************
 * Project: TMVA - a Root-integrated toolkit for multivariate data analysis       *
 * Package: TMVA                                                                  *
 * Class  : TMVA::Factory                                                         *
 * Web    : http://tmva.sourceforge.net                                           *
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
 * (http://tmva.sourceforge.net/LICENSE)                                          *
 **********************************************************************************/

//_______________________________________________________________________
//                                                                      
// This is the main MVA steering class: it creates all MVA methods,     
// and guides them through the training, testing and evaluation         
// phases. It also manages multiple MVA handling in case of distinct    
// phase space requirements (cuts).                                     
//_______________________________________________________________________

#include "Riostream.h"
#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TString.h"
#include "TLeaf.h"
#include "TEventList.h"
#include "TH1.h"
#include "TH2F.h"
#include "TText.h"
#include "TTreeFormula.h"
#include "TStyle.h"
#include "TMatrixF.h"
#include "TMatrixDSym.h"
#include "TPaletteAxis.h"

#ifndef ROOT_TMVA_Factory
#include "TMVA/Factory.h"
#endif
#ifndef ROOT_TMVA_Tools
#include "TMVA/Tools.h"
#endif
#ifndef ROOT_TMVA_Ranking
#include "TMVA/Ranking.h"
#endif
#ifndef ROOT_TMVA_DataSet
#include "TMVA/DataSet.h"
#endif
#ifndef ROOT_TMVA_Methods
#include "TMVA/Methods.h"
#endif


const Bool_t DEBUG_TMVA_Factory = kFALSE;

const int MinNoTrainingEvents = 10;
const int MinNoTestEvents     = 1;
const long int basketsize     = 1280000;

const TString BCwhite__f  ( "\033[1;37m" );
const TString BCred__f    ( "\033[31m"   );
const TString BCblue__f   ( "\033[34m"   );
const TString BCblue__b   ( "\033[44m"   );
const TString BCred__b    ( "\033[1;41m" );
const TString EC__        ( "\033[0m"    );
const TString BClblue__b  ( "\033[1;44m" );

using namespace std;

ClassImp(TMVA::Factory)

//_______________________________________________________________________
TMVA::Factory::Factory( TString jobName, TFile* theTargetFile, TString theOption )
   : fDataSet         ( new DataSet ),
     fTargetFile      ( theTargetFile ),
     fOptions         ( theOption ),
     fVerbose         ( kTRUE ),
     fMultipleMVAs    ( kFALSE ),
     fMultipleStoredOptions( kFALSE )
{  
   cout  << "Factory Constructor" << endl;
   // standard constructor
   //   jobname       : this name will appear in all weight file names produced by the MVAs
   //   theTargetFile : output ROOT file; the test tree and all evaluation plots 
   //                   will be stored here
   //   theOption     : option string; the currently only option is "V" for verbose

   // histograms are not automatically associated with the current
   // directory and hence don't go out of scope when closing the file
   // TH1::AddDirectory(kFALSE);

   fJobName = jobName;
   this->Greeting("Color");
   // interpret option string 
   // at present, only verbose option defined
   TString s = fOptions;
   s.ToUpper();
   if (s.Contains("V")) fVerbose = kTRUE;
   else                 fVerbose = kFALSE;

   Data().SetBaseRootDir(fTargetFile);
   Data().SetLocalRootDir(fTargetFile);
   Data().SetVerbose(Verbose());
}

//_______________________________________________________________________
TMVA::Factory::Factory( TFile* theTargetFile)
   : fDataSet         ( new DataSet ),
     fTargetFile      ( theTargetFile ),
     fOptions         ( ""),
     fMultipleMVAs    (kFALSE),
     fMultipleStoredOptions(kFALSE)
{  
   // depreciated constructor

   fJobName = "";
   this->Greeting("Color");
   // interpret option string 
   // at present, only verbose option defined
   TString s = fOptions;
   s.ToUpper();
   if (s.Contains("V")) fVerbose = kTRUE;
   else                 fVerbose = kFALSE;
  
   Data().SetBaseRootDir(fTargetFile);
   Data().SetLocalRootDir(0);
}

//_______________________________________________________________________
void TMVA::Factory::Greeting(TString op) 
{
   // print greeting message
   op.ToUpper();
   if (op.Contains("COLOR") || op.Contains("COLOUR") ) {
      cout << "--- " << GetName() << ": " << BCred__f 
           << "_______________________________________" << EC__ << endl;
      cout << "--- " << GetName() << ": " << BCblue__f
           << BCred__b << BCwhite__f << " // " << EC__
           << BCwhite__f << BClblue__b 
           << "|\\  /|| \\  //  /\\\\\\\\\\\\\\\\\\\\\\\\ \\ \\ \\ " << EC__ << endl;
      cout << "--- " << GetName() << ": "<< BCblue__f
           << BCred__b << BCwhite__f << "//  " << EC__
           << BCwhite__f << BClblue__b 
           << "| \\/ ||  \\//  /--\\\\\\\\\\\\\\\\\\\\\\\\ \\ \\ \\" << EC__ << endl;
   }
   else {
      cout << "--- " << GetName() << ": " 
           << "_______________________________________" << endl;
      cout << "--- " << GetName() << ":  // "
           << "|\\  /|| \\  //  /\\\\\\\\\\\\\\\\\\\\\\\\ \\ \\ \\ " << endl;
      cout << "--- " << GetName() << ": //  " 
           << "| \\/ ||  \\//  /--\\\\\\\\\\\\\\\\\\\\\\\\ \\ \\ \\" << endl;
   }  
}

//_______________________________________________________________________
TMVA::Factory::~Factory( void )
{
   // default destructor
   //
   // *** segmentation fault occurs when deleting this object :-( ***
   //   fTrainingTree->Delete();
   //
   // *** cannot delete: need to clarify ownership :-( ***
   //   fSignalTree->Delete();
   //   fBackgTree->Delete();
   this->DeleteAllMethods();
}

//_______________________________________________________________________
void TMVA::Factory::DeleteAllMethods( void )
{
   // delete methods
   vector<TMVA::IMethod*>::iterator itrMethod = fMethods.begin();
   for (; itrMethod != fMethods.end(); itrMethod++) {
      if (Verbose())
         cout << "--- " << GetName() << " <verbose>: delete method: " 
              << (*itrMethod)->GetName() << endl;    
      delete (*itrMethod);
   }
   fMethods.clear();
}

//_______________________________________________________________________
void TMVA::Factory::SetInputVariables( vector<TString>* theVariables ) 
{ 
   // sanity check
   if (theVariables->size() == 0) {
      cout << "--- " << GetName() << "::SetInputVariables: fatal error: "
           << "vector of input variables is empty ==> abort" << endl;
      exit(1);
   }
   for (UInt_t i=0; i<theVariables->size(); i++) Data().AddVariable((*theVariables)[i]);
}

//_______________________________________________________________________
Bool_t TMVA::Factory::SetInputTrees(TTree* signal, TTree* background, 
                                    Double_t signalWeight, Double_t backgroundWeight)
{
   // define the input trees for signal and background; no cuts are applied
   SetSignalTree(signal, signalWeight);
   SetBackgroundTree(background, backgroundWeight);
   return kTRUE;
}

//_______________________________________________________________________
Bool_t TMVA::Factory::SetInputTrees(TTree* inputTree, TCut SigCut, TCut BgCut)
{
   // define the input trees for signal and background from single input tree,
   // containing both signal and background events distinguished by the type 
   // identifiers: SigCut and BgCut
   TTree * signalTree = inputTree->CloneTree(0);
   TTree * backgTree  = inputTree->CloneTree(0);
   
   TIter next_branch1( signalTree->GetListOfBranches() );
   while (TBranch *branch = (TBranch*)next_branch1())
      branch->SetBasketSize(basketsize);

   TIter next_branch2( backgTree->GetListOfBranches() );
   while (TBranch *branch = (TBranch*)next_branch2())
      branch->SetBasketSize(basketsize);

   inputTree->Draw(">>signalList",SigCut,"goff");
   TEventList *signalList = (TEventList*)gDirectory->Get("signalList");

   inputTree->Draw(">>backgList",BgCut,"goff");    
   TEventList *backgList = (TEventList*)gDirectory->Get("backgList");

   if (backgList->GetN() == inputTree->GetEntries()) {
      TCut bgcut= !SigCut;
      inputTree->Draw(">>backgList",bgcut,"goff");    
      backgList = (TEventList*)gDirectory->Get("backgList");
   }
   signalList->Print();
   backgList->Print();
  
   for (Int_t i=0;i<inputTree->GetEntries(); i++) {
      inputTree->GetEntry(i);
      if ((backgList->Contains(i)) && (signalList->Contains(i))) {
         cout << "--- " << GetName() << ": WARNING  Event "<<i
              <<" is selected for signal and background sample! Skip it!"<<endl;
         continue;
      }
      if (signalList->Contains(i)) signalTree->Fill();
      if (backgList->Contains(i) ) backgTree->Fill();
   }

   signalTree->ResetBranchAddresses();
   backgTree->ResetBranchAddresses();


   Data().AddSignalTree(signalTree, 1.0);
   Data().AddBackgroundTree(backgTree, 1.0);

   delete signalList;
   delete  backgList;
   return kTRUE;    
}

//_______________________________________________________________________
Bool_t TMVA::Factory::SetInputTrees( TString datFileS, TString datFileB, 
                                     Double_t signalWeight, Double_t backgroundWeight )
{
   // create trees from these ascii files
   TTree* signalTree = new TTree( "TreeS", "Tree (S)" );
   TTree* backgTree  = new TTree( "TreeB", "Tree (B)" );
  
   signalTree->ReadFile( datFileS );
   backgTree->ReadFile( datFileB );
  
   ifstream in; 
   in.open(datFileS);
   if (!in.good()) {
      cout << "--- " << GetName() << ": Error: could not open file: " << datFileS << endl;
      return kFALSE;
   }
   in.close();
   in.open(datFileB);
   if (!in.good()) {
      cout << "--- " << GetName() << ": Error: could not open file: " << datFileB << endl;
      return kFALSE;
   }
   in.close();
    
   signalTree->Write();
   backgTree ->Write();

   SetSignalTree    (signalTree, signalWeight);
   SetBackgroundTree(backgTree,  backgroundWeight);

   return kTRUE;
}

//_______________________________________________________________________
void TMVA::Factory::BookMultipleMVAs(TString theVariable, Int_t nbins, Double_t *array)
{
   // books multiple MVAs according to the variable, number of bins and 
   // the cut array given

   // here the mess starts!
   fMultipleMVAs          = kTRUE;
   fMultiTrain            = kFALSE;
   fMultiTest             = kFALSE; 
   fMultiEvalVar          = kFALSE;
   fMultiEval             = kFALSE; 
   fMultipleStoredOptions = kFALSE;

   // at least one sanity check:
   // this method must be called *before* any Method has been booked!
   if ( fMethods.size() > 0) {
      cout << "--- " << GetName() 
           << ": ERROR! BookMultipleMVAs must be called befor booking any Method!"<<endl;
      exit(1);
   }

   // print out
   cout << "--- " << GetName() << " : MulitCut Analysis Booked:  "
        << theVariable << " is splitted in " << nbins << " bins:" << endl;

   // check if already some bins are booked
   // if yes add the new bins to *every* existing one
   if (fMultipleMVAnames.size() >0) {
     
      // loop over existing bins and add the new ones
      // store their values in temporary opjects
      Int_t nOldBins =  fMultipleMVAnames.size();
      TString* simpleName  = new TString[nOldBins];
      TString* description = new TString[nOldBins];

      TCut oldCut[1000];
      Int_t binc=0;
      for (map<TString, std::pair<TString,TCut> >::iterator oldBin = fMultipleMVAnames.begin();
           oldBin != fMultipleMVAnames.end(); oldBin++) {
         simpleName[binc]=oldBin->first;
         description[binc]=(oldBin->second).first;
         oldCut[binc]=(oldBin->second).second;
         binc++;
      } // end of loop over existing bins

      map<TString, std::pair<TString,TCut> >::iterator startBins = fMultipleMVAnames.begin();
      map<TString, std::pair<TString,TCut> >::iterator endBins   = fMultipleMVAnames.end();
      fMultipleMVAnames.erase(startBins, endBins);

      // create new map
      for (Int_t oldbin=0; oldbin<nOldBins; oldbin++) {
         for (Int_t bin=0; bin<(nbins); bin++) {
            // prepare string for this bin
            // FIXME!!! assume at the moment that array is sorted!
            // --> fMultipleMVAnames = new TMap(nbins);
         
            // simple bin name
            TString *binMVAname = new TString( simpleName[oldbin] + "__" + theVariable + 
                                               Form("_bin_%d",(bin+1)));
            // this is the cut in human readable version
            TString *binMVAdescription = new TString( description[oldbin] + " && " 
                                                      + Form(" %g < ",array[bin])  
                                                      + theVariable  
                                                      + Form(" < %g",array[bin+1]));
            // create ROOT TCut
            TString *binMVAtmp = new TString("("+ theVariable +
                                             Form(" > %g ",array[bin]) +
                                             ") && (" +
                                             theVariable + Form(" < %g",array[bin+1]) +")");
         
            TCut *binMVACut = new TCut(binMVAtmp->Data());
                 
            // fill all three into the map
            fMultipleMVAnames[*binMVAname] = std::pair<TString,TCut>(*binMVAdescription, *binMVACut + oldCut[oldbin]);

            if (Verbose()) cout << "--- " <<  GetName() <<": "
                                <<binMVAname->Data()
                                <<"  ["<< binMVAdescription->Data() << "]  "<<endl;
            delete binMVAname;
            delete binMVAdescription;
            delete binMVAtmp;
            delete binMVACut;
         }// end of loop over bins
      } // end of loop over oldbins

      delete[] simpleName;
      delete[] description;

   } 
   else { // this is the first time BookMultipleMVAs is being called
      for (Int_t bin=0; bin<(nbins); bin++) {
         // prepare string for this bin
         // FIXME!!! assume at the moment that array is sorted!
         //    fMultipleMVAnames = new TMap(nbins);
       
         // simple bin name
         TString *binMVAname = new TString( theVariable + 
                                            Form("_bin_%d",(bin+1)));
         // this is the cut in human readable version
         TString *binMVAdescription = new TString( Form("%g < ",array[bin]) + 
                                                   theVariable + 
                                                   Form(" < %g",array[bin+1]));
         // create ROOT TCut
         TString *binMVAtmp = new TString("("+ theVariable +
                                          Form(" > %g ",array[bin]) +
                                          ") && (" +
                                          theVariable + Form(" < %g",array[bin+1]) +")");
       
         TCut *binMVACut = new TCut(binMVAtmp->Data());
       
         // fill all three into the map
         fMultipleMVAnames[*binMVAname] =  std::pair<TString,TCut>(*binMVAdescription, *binMVACut);
         if (Verbose()) cout << "--- " <<  GetName() <<": "
                             <<binMVAname->Data()
                             <<"  ["<< binMVAdescription->Data() << "]  "<<endl;
         delete binMVAname;
         delete binMVAdescription;
         delete binMVAtmp;
         delete binMVACut;
      } // end of loop over bins
   } // end of if (fMultipleMVAnames.size() >0)

}

//_______________________________________________________________________
void TMVA::Factory::PrepareTrainingAndTestTree( TCut cut, Int_t Ntrain, Int_t Ntest, TString TreeName )
{ 
   // possible user settings for Ntrain and Ntest:
   //   ------------------------------------------------------
   //   |              |              |        |             |
   //   ------------------------------------------------------
   //                                                        # input signal events
   //                                          # input signal events after cuts
   //   ------------------------------------------------------
   //   |              |              |             |       |
   //   ------------------------------------------------------
   //    \/  \/                      # input bg events
   //                                               # input bg events after cuts
   //      Ntrain/2       Ntest/2                         
   //
   // definitions:
   //
   //         nsigTot = all signal events
   //         nbkgTot = all bkg events
   //         nTot    = nsigTot + nbkgTot
   //         i.g.: nsigTot != nbkgTot
   //         N:M     = use M events after event N (distinct event sample)
   //                   (read as: "from event N to event M")
   //
   // assumptions:
   //
   //         a) equal number of signal and background events is used for training
   //         b) any numbers of signal and background events are used for testing
   //         c) an explicit syntax can violate a)
   //
   // cases (in order of importance)
   //
   // 1)
   //      user gives         : N1
   //      PrepareTree does   : nsig_train=nbkg_train=min(N1,nsigTot,nbkgTot)
   //                           nsig_test =nsig_train:nsigTot, nbkg_test =nsig_train:nbkgTot
   //      -> give warning if nsig_test<=0 || nbkg_test<=0
   //
   // 2)
   //      user gives         : N1, N2
   //      PrepareTree does   : nsig_train=nbkg_train=min(N1,nsigTot,nbkgTot)
   //                           nsig_test =nsig_train:min(N2,nsigTot-nsig_train),
   //                           nbkg_test =nsig_train:min(N2,nbkgTot-nbkg_train)
   //      -> give warning if nsig(bkg)_train != N1, or
   //                      if nsig_test<N2 || nbkg_test<N2
   //
   // 3)
   //      user gives         : -1
   //      PrepareTree does   : nsig_train=nbkg_train=min(nsigTot,nbkgTot)
   //                           nsig_test =nsigTot, nbkg_test=nbkgTot
   //      -> give warning that same samples are used for testing and training
   //
   // 4)
   //      user gives         : -1, -1
   //      PrepareTree does   : nsig_train=nsigTot, nbkg_train=nbkgTot
   //                           nsig_test =nsigTot, nbkg_test =nbkgTot
   //      -> give warning that same samples are used for testing and training,
   //         and, if nsig_train != nbkg_train, that an unequal number of 
   //         signal and background events are used in training
   //                          
   // ------------------------------------------------------------------------
   // Give in any case the number of signal and background events that are
   // used for testing and training, and tell whether there are overlaps between
   // the samples.
   // ------------------------------------------------------------------------
   // 
   if(fMultipleMVAs) Data().SetMultiCut(cut);
   else              Data().SetCut(cut);

   Data().PrepareForTrainingAndTesting(Ntrain, Ntest, TreeName);
}

//_______________________________________________________________________
void TMVA::Factory::SetSignalTree(TTree* signal, Double_t weight)
{
   // number of signal events (used to compute significance)
   Data().ClearSignalTreeList();
   Data().AddSignalTree(signal, weight);
}

//_______________________________________________________________________
void TMVA::Factory::SetBackgroundTree(TTree* background, Double_t weight)
{
   // number of background events (used to compute significance)
   Data().ClearBackgroundTreeList();
   Data().AddBackgroundTree(background, weight);
}

//_______________________________________________________________________
Bool_t TMVA::Factory::BookMethod( TString theMethodName, TString methodTitle, TString theOption ) 
{
   // booking via name; the names are translated into enums and the 
   // corresponding overloaded BookMethod is called

   if (fMultipleMVAs && !fMultipleStoredOptions ) {
      cout << "--- " << GetName() << ": store method " << methodTitle
           << "  and its options for multiple MVAs" << endl;
    
      cout << "--- " << GetName() << ": multiple cuts are currently not supported :-("
           << " ... this will be fixed soon, promised ! ==> abort" << endl;
      exit(1);

      //      fMultipleMVAMethodOptions[theMethodTitle] = std::pair<TString,TString>(theOption, "");
      return kTRUE;
   }

   if (theMethodName != "Variable") 
      cout << "--- " << GetName() << ": create method: " << theMethodName << endl;

   return BookMethod( TMVA::gTypes.GetMethodType( theMethodName ), methodTitle, theOption );

   return kFALSE;
}

//_______________________________________________________________________
Bool_t TMVA::Factory::BookMethod( TMVA::Types::MVA theMethod, TString methodTitle, TString theOption ) 
{
   // books MVA method; the option configuration string is custom for each MVA
   // the TString field "theNameAppendix" serves to define (and distringuish) 
   // several instances of a given MVA, eg, when one wants to compare the 
   // performance of various configurations

   TMVA::IMethod *method = 0;

   // initialize methods
   switch(theMethod) {
   case TMVA::Types::Cuts:       
      method = new TMVA::MethodCuts      ( fJobName, methodTitle, Data(), theOption ); break;
   case TMVA::Types::Fisher:     
      method = new TMVA::MethodFisher    ( fJobName, methodTitle, Data(), theOption ); break;
   case TMVA::Types::MLP:        
      method = new TMVA::MethodMLP       ( fJobName, methodTitle, Data(), theOption ); break;
   case TMVA::Types::TMlpANN:    
      method = new TMVA::MethodTMlpANN   ( fJobName, methodTitle, Data(), theOption ); break;
   case TMVA::Types::CFMlpANN:   
      method = new TMVA::MethodCFMlpANN  ( fJobName, methodTitle, Data(), theOption ); break;
   case TMVA::Types::Likelihood: 
      method = new TMVA::MethodLikelihood( fJobName, methodTitle, Data(), theOption ); break;
   case TMVA::Types::Variable:   
      method = new TMVA::MethodVariable  ( fJobName, methodTitle, Data(), theOption ); break;
   case TMVA::Types::HMatrix:    
      method = new TMVA::MethodHMatrix   ( fJobName, methodTitle, Data(), theOption ); break;
   case TMVA::Types::PDERS:      
      method = new TMVA::MethodPDERS     ( fJobName, methodTitle, Data(), theOption ); break;
   case TMVA::Types::BDT:        
      method = new TMVA::MethodBDT       ( fJobName, methodTitle, Data(), theOption ); break;
   case TMVA::Types::SVM:        
      method = new TMVA::MethodSVM       ( fJobName, methodTitle, Data(), theOption ); break;
   case TMVA::Types::RuleFit:    
      method = new TMVA::MethodRuleFit   ( fJobName, methodTitle, Data(), theOption ); break;
   default:
      cout << "--- " << GetName() << ": Error: method: " 
           << theMethod << " does not exist ==> abort" << endl;
      exit(1);
   }

   fMethods.push_back( method );

   return kTRUE;
}

//_______________________________________________________________________
Bool_t TMVA::Factory::BookMethod( TMVA::Types::MVA theMethod, TString methodTitle, TString methodOption,
                                  TMVA::Types::MVA theCommittee, TString committeeOption ) 
{
   // books MVA method; the option configuration string is custom for each MVA
   // the TString field "theNameAppendix" serves to define (and distringuish) 
   // several instances of a given MVA, eg, when one wants to compare the 
   // performance of various configurations

   TMVA::IMethod *method = 0;

   // initialize methods
   switch(theMethod) {
   case TMVA::Types::Committee:    
      method = new TMVA::MethodCommittee( fJobName, methodTitle, Data(), methodOption, theCommittee, committeeOption ); break;
   default:
      cout << "--- " << GetName() << ": Error: method: " 
           << theMethod << " does not exist ==> abort" << endl;
      exit(1);
   }

   fMethods.push_back( method );

   return kTRUE;
}

//_______________________________________________________________________
TMVA::IMethod* TMVA::Factory::GetMVA( TString method )
{
   // returns pointer to MVA that corresponds to "method"
   vector<TMVA::IMethod*>::iterator itrMethod    = fMethods.begin();
   vector<TMVA::IMethod*>::iterator itrMethodEnd = fMethods.end();
   for (; itrMethod != itrMethodEnd; itrMethod++) {
      TMVA::IMethod* mva = (*itrMethod);    
      if ( (mva->GetMethodName()).Contains(method)) return mva;
   }
   return 0;
}

//_______________________________________________________________________
void TMVA::Factory::TrainAllMethods( void ) 
{  
   // iterates over all MVAs that have been booked, and calls their training methods

   // if multiple  MVAs 
   if (fMultipleMVAs && !fMultipleStoredOptions ) {
      cout << "--- " << GetName() << ": TrainAllMethods will be called for multiple MVAs " << endl;
      fMultiTrain=kTRUE;
      return;
   }
  
   // iterate over methods and train
   vector<TMVA::IMethod*>::iterator itrMethod    = fMethods.begin();
   vector<TMVA::IMethod*>::iterator itrMethodEnd = fMethods.end();
   for (; itrMethod != itrMethodEnd; itrMethod++) {
      if (Data().GetTrainingTree()->GetEntries() >= MinNoTrainingEvents) {
         cout << "--- " << GetName() << ": train method: " 
              << (*itrMethod)->GetMethodTitle() << endl;
         ((MethodBase*)(*itrMethod))->TrainMethod();
      }
      else {
         cout << "--- " << GetName() 
              << ": WARNING method "<< (*itrMethod)->GetMethodName() 
              << " not trained (training tree has less entries ["<< Data().GetTrainingTree()->GetEntries() 
              <<"] than required ["<<MinNoTrainingEvents<<"]"<<endl; 
      }
   }

   // variable ranking 
   cout << "--- " << endl;
   cout << "--- " << GetName() << ": begin ranking of input variables:" << endl;
   for (itrMethod = fMethods.begin(); itrMethod != itrMethodEnd; itrMethod++) {
      if (Data().GetTrainingTree()->GetEntries() >= MinNoTrainingEvents) {

         // create and print ranking
         const Ranking* ranking = (*itrMethod)->CreateRanking();
         if (ranking != 0) ranking->Print();
         else cout << "--- " << GetName() << ": no variable ranking supplied by method: " 
                   << ((TMVA::IMethod*)*itrMethod)->GetMethodTitle() << endl;
      }
   }
   cout << "--- " << endl;
}

//_______________________________________________________________________
void TMVA::Factory::TestAllMethods( void )
{
   // iterates over all MVAs that have been booked, and calls their testing methods
   cout << "--- " << GetName() << ": begin testing all methods:" << endl;

   // if multiple  MVAs 
   if (fMultipleMVAs && !fMultipleStoredOptions ) {
      cout << "--- " << GetName() << ": TestAllMethods will be called for multiple MVAs " << endl;
      fMultiTest=kTRUE;
      return;
   } 
   else if (Data().GetTrainingTree() == NULL) {
      cout << "--- "<< GetName() 
           << " you perform testing without training before, hope you  \n"
           << "--- did give a reasonable test tree and weight files " <<endl;
   } 
   else if ((Data().GetTrainingTree()->GetEntries() < MinNoTrainingEvents) && fMultipleMVAs && fMultiTrain) {
      cout << "--- "<< GetName() 
           <<" : WARNING Skip testing since training wasn't performed for this bin"<<endl;
      return;
   }

   // iterate over methods and test
   vector<TMVA::IMethod*>::iterator itrMethod    = fMethods.begin();
   vector<TMVA::IMethod*>::iterator itrMethodEnd = fMethods.end();
   for (; itrMethod != itrMethodEnd; itrMethod++) {
      cout << "--- " << GetName() << ": test method: " << (*itrMethod)->GetMethodTitle() << endl;
      (*itrMethod)->PrepareEvaluationTree(0);
      if (DEBUG_TMVA_Factory) Data().GetTestTree()->Print();
   }
}

//_______________________________________________________________________
void TMVA::Factory::EvaluateAllVariables( TString options )
{
   // iterates over all MVA input varables and evaluates them
   cout << "--- " << GetName() << ": begin evaluating all variables:" << endl;

   // if multiple  MVAs 
   if (fMultipleMVAs && !fMultipleStoredOptions ) {
      cout << "--- " << GetName() << ": EvaluateAllVariables will be called for multiple MVAs " << endl;
      fMultiEvalVar=kTRUE;
      return;
   } 
   else if (Data().GetTrainingTree() == NULL) {
      cout << "--- "<< GetName() 
           << " you perform testing without training before, hope you  \n"
           << "--- did give a reasonable test tree and weight files " <<endl;
   }
   else if ((Data().GetTrainingTree()->GetEntries() < MinNoTrainingEvents) && fMultipleMVAs && fMultiTrain) {
      cout << "--- "<< GetName() 
           <<" : WARNING Skip evaluation since training wasn't performed for this bin"<<endl;
      return;
   }

   for (UInt_t i=0; i<Data().GetNVariables(); i++) {
      TString s = Data().GetInternalVarName(i);
      if (options.Contains("V")) s += ":V";
      this->BookMethod( "Variable", s );
   }
}

//_______________________________________________________________________
void TMVA::Factory::EvaluateAllMethods( void )
{
   // iterates over all MVAs that have been booked, and calls their evaluation methods

   // if multiple  MVAs 
   if (fMultipleMVAs && !fMultipleStoredOptions ) {
      cout << "--- " << GetName() << ": EvaluateAllMethods will be called for multiple MVAs " << endl;
      fMultiEval=kTRUE;
      return;
   } 
   else if (Data().GetTrainingTree() == NULL) {
      cout << "--- "<< GetName() 
           << " you perform testing without training before, hope you  \n"
           << "--- did give a reasonable test tree and weight files " <<endl;
   }
   else if ((Data().GetTrainingTree()->GetEntries() < MinNoTrainingEvents) && fMultipleMVAs && fMultiTrain) {
      cout << "--- "<< GetName() <<" : WARNING Skip evaluation since training wasn't performed"<<endl;
      return;
   }

   // although equal, we now want to seperate the outpuf for the variables
   // and the real methods
   Int_t    isel; //will be 0 for a Method; 1 for a Variable
   Int_t nmeth_used[2] = {0,0}; //0 Method; 1 Variable

   vector< vector<TString> > mname(2);
   vector< vector<Double_t> > sig(2),sep(2),eff01(2),eff10(2),eff30(2),mutr(2);
   vector< vector<Double_t> > trainEff01(2), trainEff10(2), trainEff30(2);

   // iterate over methods and evaluate
   vector<TMVA::IMethod*>::iterator itrMethod    = fMethods.begin();
   vector<TMVA::IMethod*>::iterator itrMethodEnd = fMethods.end();
   for (; itrMethod != itrMethodEnd; itrMethod++) {
      cout << "--- " << GetName() << ": evaluate method: " << (*itrMethod)->GetMethodTitle() << endl;
      isel=0; if ((*itrMethod)->GetMethodName().Contains("Variable")) isel=1;

      // perform the evaluation
      (*itrMethod)->TestInit(0);
      // do the job
      if ((*itrMethod)->IsOK()) (*itrMethod)->Test(0);
      if ((*itrMethod)->IsOK()) {
         mname[isel].push_back( (*itrMethod)->GetMethodTitle() );
         sig[isel].push_back  ( (*itrMethod)->GetSignificance() );
         sep[isel].push_back  ( (*itrMethod)->GetSeparation() );
         TTree * testTree = ((MethodBase*)(*itrMethod))->Data().GetTestTree();
         eff01[isel].push_back( (*itrMethod)->GetEfficiency("Efficiency:0.01", testTree)  );
         eff10[isel].push_back( (*itrMethod)->GetEfficiency("Efficiency:0.10", testTree)  );
         eff30[isel].push_back( (*itrMethod)->GetEfficiency("Efficiency:0.30", testTree)  );

         trainEff01[isel].push_back( (*itrMethod)->GetTrainingEfficiency("Efficiency:0.01")  ); // the first pass takes longer
         trainEff10[isel].push_back( (*itrMethod)->GetTrainingEfficiency("Efficiency:0.10")  );
         trainEff30[isel].push_back( (*itrMethod)->GetTrainingEfficiency("Efficiency:0.30")  );

         mutr[isel].push_back ( (*itrMethod)->GetmuTransform(testTree) );
         nmeth_used[isel]++;
         (*itrMethod)->WriteEvaluationHistosToFile( Data().BaseRootDir() );
      }
      else {
         cout << "--- " << GetName() << ": Warning: " << (*itrMethod)->GetName() 
              << " returned isOK flag: " 
              << (*itrMethod)->IsOK() << endl;
      }
   }

   // now sort the variables according to the best 'eff at Beff=0.10'
   for (Int_t k=0; k<2; k++) {
      vector< vector<Double_t> > vtemp;
      vtemp.push_back( eff10[k] ); // this is the vector that is ranked
      vtemp.push_back( eff01[k] );
      vtemp.push_back( eff30[k] );
      vtemp.push_back( trainEff10[k] );
      vtemp.push_back( trainEff01[k] );
      vtemp.push_back( trainEff30[k] );
      vtemp.push_back( sig[k]   );
      vtemp.push_back( sep[k]   );
      vtemp.push_back( mutr[k]  ); 
      vector<TString> vtemps = mname[k];
      TMVA::Tools::UsefulSortDescending( vtemp, &vtemps );
      eff10[k] = vtemp[0];
      eff01[k] = vtemp[1];
      eff30[k] = vtemp[2];
      trainEff10[k] = vtemp[3];
      trainEff01[k] = vtemp[4];
      trainEff30[k] = vtemp[5];
      sig[k]   = vtemp[6];
      sep[k]   = vtemp[7];
      mutr[k]  = vtemp[8];
      mname[k] = vtemps;
   }
   cout << "--- " << endl;
   cout << "--- " << GetName() << ": Evaluation results ranked by best 'signal eff @B=0.10'" << endl;
   cout << "---------------------------------------------------------------------------"   << endl;
   cout << "--- MVA              Signal efficiency:         Signifi- Sepa-    mu-Trans-"   << endl;
   cout << "--- Methods:         @B=0.01  @B=0.10  @B=0.30  cance:   ration:  form:"       << endl;
   cout << "---------------------------------------------------------------------------"   << endl;
   for (Int_t k=0; k<2; k++) {
      if (k == 1 && nmeth_used[k] > 0 && !fMultipleMVAs) {
         cout << "---------------------------------------------------------------------------" << endl;
         cout << "--- Input Variables: " << endl
              << "---------------------------------------------------------------------------" << endl;
      }
      for (Int_t i=0; i<nmeth_used[k]; i++) {
         if (k == 1) mname[k][i].ReplaceAll( "Variable_", "" );
         printf("--- %-15s: %1.3f    %1.3f    %1.3f    %1.3f    %1.3f    %1.3f \n",
                (const char*)mname[k][i], 
                eff01[k][i], eff10[k][i], eff30[k][i], sig[k][i], sep[k][i], mutr[k][i] );
      }
   }
   cout << "---------------------------------------------------------------------------" << endl;
   cout << "--- " << endl;
   cout << "--- " << GetName() << ": Testing efficiency compared to training efficiency " << endl;
   cout << "---                --> checks for overtraining" << endl;
   cout << "---------------------------------------------------------------------------"   << endl;
   cout << "--- MVA           Signal efficiency: from test sample (from traing sample) "   << endl;
   cout << "--- Methods:         @B=0.01             @B=0.10            @B=0.30   "   << endl;
   cout << "---------------------------------------------------------------------------"   << endl;
   for (Int_t k=0; k<2; k++) {
      if (k == 1 && nmeth_used[k] > 0 && !fMultipleMVAs) {
         cout << "---------------------------------------------------------------------------" << endl;
         cout << "--- Input Variables: " << endl
              << "---------------------------------------------------------------------------" << endl;
      }
      for (Int_t i=0; i<nmeth_used[k]; i++) {
         if (k == 1) mname[k][i].ReplaceAll( "Variable_", "" );
         printf("--- %-15s: %1.3f (%1.3f)       %1.3f (%1.3f)      %1.3f (%1.3f) \n",
                (const char*)mname[k][i], 
                eff01[k][i],trainEff01[k][i],
                eff10[k][i],trainEff10[k][i],
                eff30[k][i],trainEff30[k][i]
		);
      }
   }
   cout << "---------------------------------------------------------------------------" << endl;
   cout << "--- " << endl;  
   cout << "--- " << GetName() << ": Write Test Tree '"<< Data().GetTestTree()->GetName()<<"' to file" << endl;
   Data().BaseRootDir()->cd();
   Data().GetTestTree()->Write("",TObject::kOverwrite);
}


//_______________________________________________________________________
void TMVA::Factory::ProcessMultipleMVA( void )
{
   // multiple MVAs in different phase space regions are trained and tested

   if (fMultipleMVAs) {
      // assume that we have booked all method:
      // all other methods know that they are called from this method!
      fMultipleStoredOptions=kTRUE;

      // loop over bins:
      for (map<TString, std::pair<TString,TCut> >::iterator bin = fMultipleMVAnames.begin();
           bin != fMultipleMVAnames.end(); bin++) {


         cout << "---------------------------------------------------------------------------"   << endl;  
         cout << "--- " << GetName() << ": Process Bin "<< bin->first<< endl;
         cout << "---                 with cut ["<< (bin->second).first <<"]"<< endl;
         cout << "---------------------------------------------------------------------------"   << endl;      

         TString binName( "multicutTMVA::" +  bin->first );
         fLocalTDir = fTargetFile->mkdir( binName, (bin->second).first);    
         fLocalTDir->cd();


         // prepare trees for this bin
         //         this->PrepareTrainingAndTestTree( ((bin->second).second)+fMultiCut, fMultiNtrain, fMultiNtest );
         Data().PrepareForTrainingAndTesting( fMultiNtrain, fMultiNtest );

         // reset list of methods: 
         if (Verbose()) cout << "--- " << GetName() << " <verbose>: delete previous methods" << endl;
         this->DeleteAllMethods();

         // loop over stored methods
         for (map<TString, std::pair<TString,TString> >::iterator method = fMultipleMVAMethodOptions.begin();
              method != fMultipleMVAMethodOptions.end(); method++) {
        
            // book methods
            this->BookMethod(method->first, (method->second).first, (method->second).second ) ;
         } // end of loop over methods

         // set weigt file dir: SetWeightFileDir
         // iterate over methods and test      
         vector<TMVA::IMethod*>::iterator itrMethod2    = fMethods.begin();
         vector<TMVA::IMethod*>::iterator itrMethod2End = fMethods.end();
         for (; itrMethod2 != itrMethod2End; itrMethod2++) {
            TString binDir( "weights/" + bin->first );
            (*itrMethod2)->SetWeightFileDir(binDir);
         }
      
         if (Verbose()) cout << "--- " << GetName() << " <verbose>: booked " << fMethods.size() 
                             << " methods" << endl;
      
         if (fMultiTrain) this->TrainAllMethods();
         if (fMultiTest)  this->TestAllMethods();
         if (fMultiEval) {
            this->EvaluateAllMethods();
        
            //check if fTestTree contains MVA variables
            Bool_t hasMVA=kFALSE;
            TIter next_branch1( Data().GetTestTree()->GetListOfBranches() );
            while (TBranch *branch = (TBranch*)next_branch1()) {
               if (((TString)branch->GetName()).Contains("MVA_")) hasMVA=kTRUE;
            } // end of loop over fTestTree branches
            
            if (hasMVA) {

               if (Data().GetMultiCutTestTree() == NULL)
                  Data().SetMultiCutTestTree(Data().GetTestTree()->CloneTree(0));

               Data().GetMultiCutTestTree()->CopyEntries(Data().GetTestTree());

            } //end of if (fTesttree has MVA branches

         } // end of if (fMultiEval) 
      } // end loop over bins
      // write global tree to top directory of the file

      fTargetFile->cd();
      if (Data().GetMultiCutTestTree() != NULL) Data().GetMultiCutTestTree()->Write();
      if (DEBUG_TMVA_Factory) Data().GetMultiCutTestTree()->Print();

      // Evaluate MVA methods globally for all multiCut Bins

      // reset list of methods: 
      if (Verbose()) cout << "--- " << GetName() << " <verbose>: delete previous methods" << endl;
      this->DeleteAllMethods();

      // evaluate the combined TestTree
      cout << "--- " << endl;
      cout << "-------------------------------------------------------------------" << endl;
      cout << "--- " << GetName() << ": Combined Overall Evaluation:" << endl;
      cout << "-------------------------------------------------------------------" << endl;
      cout << "--- " << "           "<<endl;

      TMVA::IMethod *method = 0;
      TIter next_branch1( Data().GetTestTree()->GetListOfBranches() );
      while (TBranch *branch = (TBranch*)next_branch1()) {
         TLeaf *leaf = branch->GetLeaf(branch->GetName());
         if (((TString)branch->GetName()).Contains("TMVA::")) {
            method = new TMVA::MethodVariable  ( fJobName, 
                                                 (TString)leaf->GetName(),  
                                                 Data(), 
                                                 (TString)leaf->GetName(), 
                                                 fTargetFile );   
            fMethods.push_back( method );
         } // is MVA variable
      }

      fLocalTDir = fTargetFile;
      this->EvaluateAllMethods();
      
      // this is save:
      fMultipleStoredOptions = kFALSE;
   } 
   else {
      cout << "--- " << GetName() << ":ERROR!!! ProcessMultipleMVA without bin definitions!"<<endl;
      cout << " Call BookMultipleMVAs in prior!" << endl;
      exit(1);
   }   
   // plot input variables in global tree
   
   if (Data().GetMultiCutTestTree() != NULL) {
      Data().PlotVariables( "MultiCutTestTree", "multicut_input_variables", Types::kNone );
      Data().PlotVariables( "MultiCutTestTree", "multicut_decorrelated_input_variables", Types::kDecorrelated );
      Data().PlotVariables( "MultiCutTestTree", "multicut_PCA_input_variables", Types::kPCA );
   }
}
