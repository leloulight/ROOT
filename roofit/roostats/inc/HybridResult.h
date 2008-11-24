// @(#)root/roostats:$Id$

/*************************************************************************
 * Project: RooStats                                                     *
 * Package: RooFit/RooStats                                              *
 * Authors:                                                              *
 *   Kyle Cranmer, Lorenzo Moneta, Gregory Schott, Wouter Verkerke       *
 *************************************************************************
 * Copyright (C) 1995-2008, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOSTATS_HybridResult
#define ROOSTATS_HybridResult

#ifndef ROOSTATS_HypoTestResult
#include "RooStats/HypoTestResult.h"
#endif

namespace RooStats {

   class HybridPlot; 

   class HybridResult : public TNamed /* , public HypoTestResult*/ {  /// TO DO: inheritance

   public:

      /// Constructor for HybridResult
      HybridResult(const char *name,const char *title,std::vector<double>& testStat_sb_vals,
                   std::vector<double>& testStat_b_vals);

      HybridResult(const char *name,const char *title);

     /// Default constructor for HybridResult
      HybridResult();

      /// Destructor of HybridResult
      virtual ~HybridResult();

      void SetDataTestStatistics(double testStat_data_val);

      /// TO DO: use from HypoTestResult
      double CLb();
      double CLsplusb();
      double CLs();

      void Add(HybridResult* other);
      HybridPlot* GetPlot(const char* name,const char* title, int n_bins);
      void PrintMore(const char* options);

      /// Get test statistics values for the sb model
      std::vector<double> GetTestStat_sb(){return fTestStat_sb;}

      /// Get test statistics values for the b model
      std::vector<double> GetTestStat_b(){return fTestStat_b;}

      /// Get test statistics value on data
      double GetTestStat_data(){return fTestStat_data;}

   private:
      std::vector<double> fTestStat_b; // vector of results for B-only toy-MC
      std::vector<double> fTestStat_sb; // vector of results for S+B toy-MC
      double fTestStat_data; // results (test statistics) evaluated for data

   protected:

      ClassDef(HybridResult,1)  // Class containing the results of the HybridCalculator
   };
}

#endif
