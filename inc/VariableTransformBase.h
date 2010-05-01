// @(#)root/tmva $Id$
// Author: Andreas Hoecker, Peter Speckmayer,Joerg Stelzer, Helge Voss

/**********************************************************************************
 * Project: TMVA - a Root-integrated toolkit for multivariate data analysis       *
 * Package: TMVA                                                                  *
 * Class  : VariableTransformBase                                                 *
 * Web    : http://tmva.sourceforge.net                                           *
 *                                                                                *
 * Description:                                                                   *
 *      Pre-transformation of input variables (base class)                        *
 *                                                                                *
 * Authors (alphabetical):                                                        *
 *      Andreas Hoecker <Andreas.Hocker@cern.ch> - CERN, Switzerland              *
 *      Peter Speckmayer <Peter.Speckmayer@cern.ch> - CERN, Switzerland           *
 *      Joerg Stelzer   <Joerg.Stelzer@cern.ch>  - CERN, Switzerland              *
 *      Helge Voss      <Helge.Voss@cern.ch>     - MPI-K Heidelberg, Germany      *
 *                                                                                *
 * Copyright (c) 2005:                                                            *
 *      CERN, Switzerland                                                         *
 *      U. of Victoria, Canada                                                    *
 *      MPI-K Heidelberg, Germany                                                 *
 *                                                                                *
 * Redistribution and use in source and binary forms, with or without             *
 * modification, are permitted according to the terms listed in LICENSE           *
 * (http://tmva.sourceforge.net/LICENSE)                                          *
 **********************************************************************************/

#ifndef ROOT_TMVA_VariableTransformBase
#define ROOT_TMVA_VariableTransformBase

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// VariableTransformBase                                                //
//                                                                      //
// Linear interpolation class                                           //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include <vector>

#ifndef ROOT_TH1
#include "TH1.h"
#endif
#ifndef ROOT_TDirectory
#include "TDirectory.h"
#endif
#ifndef ROOT_TString
#include "TString.h"
#endif

#ifndef ROOT_TMVA_Types
#include "TMVA/Types.h"
#endif
#ifndef ROOT_TMVA_Event
#include "TMVA/Event.h"
#endif
#ifndef ROOT_TMVA_VariableInfo
#include "TMVA/VariableInfo.h"
#endif
#ifndef ROOT_TMVA_DataSetInfo
#include "TMVA/DataSetInfo.h"
#endif

namespace TMVA {

   class VariableTransformBase : public TObject {

   public:

      typedef std::vector<std::pair<Char_t,UInt_t> > VectorOfCharAndInt;
      typedef VectorOfCharAndInt::iterator       ItVarTypeIdx;
      typedef VectorOfCharAndInt::const_iterator ItVarTypeIdxConst;

      VariableTransformBase( DataSetInfo& dsi, Types::EVariableTransform tf, const TString& trfName );
      virtual ~VariableTransformBase( void );

      virtual void         Initialize() = 0;
      virtual Bool_t       PrepareTransformation( const std::vector<Event*>&  ) = 0;
      virtual const Event* Transform       ( const Event* const, Int_t cls ) const = 0;
      virtual const Event* InverseTransform( const Event* const, Int_t cls ) const = 0;

      // accessors
      void   SetEnabled  ( Bool_t e ) { fEnabled = e; }
      void   SetNormalise( Bool_t n ) { fNormalise = n; }
      Bool_t IsEnabled()    const { return fEnabled; }
      Bool_t IsCreated()    const { return fCreated; }
      Bool_t IsNormalised() const { return fNormalise; }

      // variable selection
      virtual void           SelectInput( const TString& inputVariables  );
      virtual void           GetInput ( const Event* event, std::vector<Float_t>& input  ) const;
      virtual void           SetOutput( Event* event, std::vector<Float_t>& output ) const;

      void SetUseSignalTransform( Bool_t e=kTRUE) { fUseSignalTransform = e; }
      Bool_t UseSignalTransform() const { return fUseSignalTransform; }

      virtual const char* GetName() const { return fTransformName.Data(); }
      TString GetShortName() const { TString a(fTransformName); a.ReplaceAll("Transform",""); return a; }

      virtual void WriteTransformationToStream ( std::ostream& o ) const = 0;
      virtual void ReadTransformationFromStream( std::istream& istr, const TString& classname="" ) = 0;

      virtual void AttachXMLTo(void* parent);
      virtual void ReadFromXML( void* trfnode );

      Types::EVariableTransform GetVariableTransform() const { return fVariableTransform; }

      // writer of function code
      virtual void MakeFunction( std::ostream& fout, const TString& fncName, Int_t part,
                                 UInt_t trCounter, Int_t cls );

      // provides string vector giving explicit transformation
      virtual std::vector<TString>* GetTransformationStrings( Int_t cls ) const;
      virtual void PrintTransformation( ostream & ) {}

      const std::vector<TMVA::VariableInfo>& Variables() const { return fVariables; }
      const std::vector<TMVA::VariableInfo>& Targets()   const { return fTargets;   }

      MsgLogger& Log() const { return *fLogger; }

      void SetTMVAVersion(TMVAVersion_t v) { fTMVAVersion = v; }

   protected:

      void CalcNorm( const std::vector<Event*>& );

      void SetCreated( Bool_t c = kTRUE ) { fCreated = c; }
      void SetNVariables( UInt_t i )      { fNVars = i; }
      void SetName( const TString& c )    { fTransformName = c; }

      UInt_t GetNVariables()  const { return fDsi.GetNVariables();  }
      UInt_t GetNTargets()    const { return fDsi.GetNTargets();    }
      UInt_t GetNSpectators() const { return fDsi.GetNSpectators(); }

      DataSetInfo& fDsi;

      std::vector<TMVA::VariableInfo>& Variables() { return fVariables; }
      std::vector<TMVA::VariableInfo>& Targets() { return fTargets; }
      Int_t GetNClasses() const { return fDsi.GetNClasses(); }


      mutable Event*           fTransformedEvent;     // holds the current transformed event
      mutable Event*           fBackTransformedEvent; // holds the current back-transformed event

      // variable selection
      VectorOfCharAndInt               fGet;           // get variables/targets/spectators


   private:

      Types::EVariableTransform fVariableTransform;  // Decorrelation, PCA, etc.

      void UpdateNorm( Int_t ivar, Double_t x );

      Bool_t                           fUseSignalTransform; // true if transformation bases on signal data
      Bool_t                           fEnabled;            // has been enabled
      Bool_t                           fCreated;            // has been created
      Bool_t                           fNormalise;          // normalise input variables
      UInt_t                           fNVars;              // number of variables
      TString                          fTransformName;      // name of transformation
      std::vector<TMVA::VariableInfo>  fVariables;          // event variables [saved to weight file]
      std::vector<TMVA::VariableInfo>  fTargets;            // event targets [saved to weight file --> TODO ]


   protected:

      TMVAVersion_t                    fTMVAVersion;

      mutable MsgLogger* fLogger;                     //! message logger

      ClassDef(VariableTransformBase,0)   //  Base class for variable transformations
   };

} // namespace TMVA

#endif
