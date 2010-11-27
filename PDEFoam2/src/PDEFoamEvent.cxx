
/**********************************************************************************
 * Project: TMVA - a Root-integrated toolkit for multivariate data analysis       *
 * Package: TMVA                                                                  *
 * Classes: PDEFoamEvent                                                          *
 * Web    : http://tmva.sourceforge.net                                           *
 *                                                                                *
 * Description:                                                                   *
 *      Implementations                                                           *
 *                                                                                *
 * Authors (alphabetical):                                                        *
 *      Tancredi Carli   - CERN, Switzerland                                      *
 *      Dominik Dannheim - CERN, Switzerland                                      *
 *      S. Jadach        - Institute of Nuclear Physics, Cracow, Poland           *
 *      Alexander Voigt  - CERN, Switzerland                                      *
 *      Peter Speckmayer - CERN, Switzerland                                      *
 *                                                                                *
 * Copyright (c) 2008:                                                            *
 *      CERN, Switzerland                                                         *
 *      MPI-K Heidelberg, Germany                                                 *
 *                                                                                *
 * Redistribution and use in source and binary forms, with or without             *
 * modification, are permitted according to the terms listed in LICENSE           *
 * (http://tmva.sourceforge.net/LICENSE)                                          *
 **********************************************************************************/

//_____________________________________________________________________
//
// Implementation of PDEFoamEvent
//
// The PDEFoamEvent method is an
// extension of the PDERS method, which uses self-adapting binning to
// divide the multi-dimensional phase space in a finite number of
// hyper-rectangles (boxes).
//
// For a given number of boxes, the binning algorithm adjusts the size
// and position of the boxes inside the multidimensional phase space,
// minimizing the variance of the signal and background densities inside
// the boxes. The binned density information is stored in binary trees,
// allowing for a very fast and memory-efficient classification of
// events.
//
// The implementation of the PDEFoamEvent is based on the monte-carlo
// integration package PDEFoamEvent included in the analysis package ROOT.
//_____________________________________________________________________


#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cassert>
#include <climits>

#include "TMath.h"

#ifndef ROOT_TMVA_PDEFoamEvent
#include "TMVA/PDEFoamEvent.h"
#endif

ClassImp(TMVA::PDEFoamEvent)

using namespace std;

//_____________________________________________________________________
TMVA::PDEFoamEvent::PDEFoamEvent() 
   : PDEFoam()
{
   // Default constructor for streamer, user should not use it.
}

//_____________________________________________________________________
TMVA::PDEFoamEvent::PDEFoamEvent(const TString& Name)
   : PDEFoam(Name)
{}

//_____________________________________________________________________
TMVA::PDEFoamEvent::PDEFoamEvent(const PDEFoamEvent &From)
   : PDEFoam(From)
{
   // Copy Constructor  NOT IMPLEMENTED (NEVER USED)
   Log() << kFATAL << "COPY CONSTRUCTOR NOT IMPLEMENTED" << Endl;
}

//_____________________________________________________________________
void TMVA::PDEFoamEvent::FillFoamCells(const Event* ev)
{
   // This function fills an event weight into the PDEFoam.  Cell
   // element 0 is filled with the weight, and element 1 is filled
   // with the squared weight.

   Float_t weight = fFillFoamWithOrigWeights ? ev->GetOriginalWeight() : ev->GetWeight();

   // find corresponding foam cell
   std::vector<Float_t> values  = ev->GetValues();
   std::vector<Float_t> tvalues = VarTransform(values);
   PDEFoamCell *cell = FindCell(tvalues);

   // 0. Element: Number of events
   // 1. Element: RMS
   SetCellElement(cell, 0, GetCellElement(cell, 0) + weight);
   SetCellElement(cell, 1, GetCellElement(cell, 1) + weight*weight);
}

//_____________________________________________________________________
Double_t TMVA::PDEFoamEvent::GetCellValue( PDEFoamCell* cell, ECellValue cv,
					   Int_t idim1, Int_t idim2 )
{
   // Return the discriminant projected onto the dimensions 'dim1',
   // 'dim2'.

   // calculate the projected discriminant
   if (cv == kValue) {

      // get cell position and dimesions
      PDEFoamVect  cellPosi(GetTotDim()), cellSize(GetTotDim());
      cell->GetHcub(cellPosi,cellSize);

      // calculate projected area of cell
      const Double_t area = cellSize[idim1] * cellSize[idim2];
      // calculate projected area of whole foam
      const Double_t foam_area = (fXmax[idim1]-fXmin[idim1])*(fXmax[idim2]-fXmin[idim2]);
      if (area<1e-20){
         Log() << kWARNING << "<Project2>: Warning, cell volume too small --> skiping cell!" << Endl;
         return 0;
      }

      // calc cell entries per projected cell area
      return GetCellValue(cell, kValue)/(area*foam_area);
   } else {
      return GetCellValue(cell, cv);
   }
}
