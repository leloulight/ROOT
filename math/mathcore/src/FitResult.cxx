// @(#)root/mathcore:$Id$
// Author: L. Moneta Wed Aug 30 11:05:34 2006

/**********************************************************************
 *                                                                    *
 * Copyright (c) 2006  LCG ROOT Math Team, CERN/PH-SFT                *
 *                                                                    *
 *                                                                    *
 **********************************************************************/

// Implementation file for class FitResult

#include "Fit/FitResult.h"

#include "Fit/FitConfig.h"

#include "Fit/BinData.h"

#include "Math/Minimizer.h"

#include "Math/IParamFunction.h"
#include "Math/OneDimFunctionAdapter.h"

#include "Math/DistFunc.h"

#include "TMath.h"  
#include "Math/RichardsonDerivator.h"
#include "Math/Error.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <iomanip>

namespace ROOT { 

   namespace Fit { 


FitResult::FitResult() : 
   fValid(false), fNormalized(false), fNFree(0), fNdf(0), fNCalls(0), 
   fStatus(-1), fVal(0), fEdm(0), fChi2(-1), fFitFunc(0)
{
   // Default constructor implementation.
}

FitResult::FitResult(ROOT::Math::Minimizer & min, const FitConfig & fconfig, const IModelFunction * func,  bool isValid,  unsigned int sizeOfData, bool binnedFit, const  ROOT::Math::IMultiGenFunction * chi2func, unsigned int ncalls ) : 
   fValid(isValid),
   fNormalized(false),
   fNFree(min.NFree() ),
   fNdf(0),
   fNCalls(min.NCalls()),
   fStatus(min.Status() ),
   fVal (min.MinValue()),  
   fEdm (min.Edm()), 
   fChi2(-1),
   fFitFunc(0), 
   fParams(std::vector<double>( min.NDim() ) )
{

   // set minimizer type 
   fMinimType = fconfig.MinimizerType();

   // append algorithm name for minimizer that support it  
   if ( (fMinimType.find("Fumili") == std::string::npos) &&
        (fMinimType.find("GSLMultiFit") == std::string::npos) 
      ) { 
      if (fconfig.MinimizerAlgoType() != "") fMinimType += " / " + fconfig.MinimizerAlgoType(); 
   }

   // replace ncalls if minimizer does not support it (they are taken then from the FitMethodFunction)
   if (fNCalls == 0) fNCalls = ncalls;

   // Constructor from a minimizer, fill the data. ModelFunction  is passed as non const 
   // since it will be managed by the FitResult
   const unsigned int npar = fParams.size();
   if (npar == 0) return;

   if (min.X() ) std::copy(min.X(), min.X() + npar, fParams.begin());
   else { 
      // case minimizer does not provide minimum values (it failed) take from configuration
      for (unsigned int i = 0; i < npar; ++i ) {
         fParams[i] = ( fconfig.ParSettings(i).Value() );
      }
   }

   if (sizeOfData >  min.NFree() ) fNdf = sizeOfData - min.NFree(); 


   // set right parameters in function (in case minimizer did not do before)
   // do also when fit is not valid
   if (func ) { 
      fFitFunc = dynamic_cast<IModelFunction *>( func->Clone() ); 
      assert(fFitFunc);
      fFitFunc->SetParameters(&fParams.front());
   }
   else { 
      // when no fFitFunc is present take parameters from FitConfig
      fParNames.reserve( npar );
      for (unsigned int i = 0; i < npar; ++i ) {
         fParNames.push_back( fconfig.ParSettings(i).Name() );
      }
   }

   if (min.Errors() != 0) 
      fErrors = std::vector<double>(min.Errors(), min.Errors() + npar ) ; 

   // check for fixed or limited parameters
   for (unsigned int ipar = 0; ipar < npar; ++ipar) { 
      const ParameterSettings & par = fconfig.ParSettings(ipar); 
      if (par.IsFixed() ) fFixedParams.push_back(ipar); 
      if (par.IsBound() ) fBoundParams.push_back(ipar); 
   } 

   if (binnedFit) { 
      if (chi2func == 0) 
         fChi2 = fVal;
      else { 
         // compute chi2 equivalent for likelihood fits
         fChi2 = (*chi2func)(&fParams[0]); 
      }
   }
      
   // fill error matrix
   // cov matrix rank 
   if (fValid) { 

      unsigned int r = npar * (  npar + 1 )/2;  
      fCovMatrix.reserve(r);
      for (unsigned int i = 0; i < npar; ++i) 
         for (unsigned int j = 0; j <= i; ++j)
            fCovMatrix.push_back(min.CovMatrix(i,j) );
      
      assert (fCovMatrix.size() == r ); 

      // minos errors 
      if (fconfig.MinosErrors()) { 
         const std::vector<unsigned int> & ipars = fconfig.MinosParams(); 
         unsigned int n = (ipars.size() > 0) ? ipars.size() : npar; 
         for (unsigned int i = 0; i < n; ++i) {
          double elow, eup;
          unsigned int index = (ipars.size() > 0) ? ipars[i] : i; 
          bool ret = min.GetMinosError(index, elow, eup);
          if (ret) SetMinosError(index, elow, eup); 
         }
      }

      // globalCC
      fGlobalCC.reserve(npar);
      for (unsigned int i = 0; i < npar; ++i) { 
         double globcc = min.GlobalCC(i); 
         if (globcc < 0) break; // it is not supported by that minimizer
         fGlobalCC.push_back(globcc); 
      }
      
   }

}

FitResult::~FitResult() { 
   // destructor. FitResult manages the fit Function pointer
   if (fFitFunc) delete fFitFunc;   
}

FitResult::FitResult(const FitResult &rhs) { 
   // Implementation of copy constructor
   (*this) = rhs; 
}

FitResult & FitResult::operator = (const FitResult &rhs) { 
   // Implementation of assignment operator.
   if (this == &rhs) return *this;  // time saving self-test

   // Manages the fitted function 
   if (fFitFunc) delete fFitFunc;
   fFitFunc = 0; 
   if (rhs.fFitFunc != 0 ) {
      fFitFunc = dynamic_cast<IModelFunction *>( (rhs.fFitFunc)->Clone() ); 
      assert(fFitFunc != 0); 
   }

   // copy all other data members 
   fValid = rhs.fValid; 
   fNormalized = rhs.fNormalized;
   fNFree = rhs.fNFree; 
   fNdf = rhs.fNdf; 
   fNCalls = rhs.fNCalls; 
   fStatus = rhs.fStatus; 
   fVal = rhs.fVal;  
   fEdm = rhs.fEdm; 
   fChi2 = rhs.fChi2;

   fFixedParams = rhs.fFixedParams;
   fBoundParams = rhs.fBoundParams;
   fParams = rhs.fParams; 
   fErrors = rhs.fErrors; 
   fCovMatrix = rhs.fCovMatrix; 
   fGlobalCC = rhs.fGlobalCC;
   fMinosErrors = rhs.fMinosErrors; 

   fMinimType = rhs.fMinimType;
   fParNames = rhs.fParNames; 
   
   return *this; 

}  

bool FitResult::Update(const ROOT::Math::Minimizer & min, bool isValid, unsigned int ncalls) { 
   // update fit result with new status from minimizer 
   // nclass if is not zero is added to the total function calls

   const unsigned int npar = fParams.size();
   if (min.NDim() != npar ) { 
      MATH_ERROR_MSG("FitResult::Update","Wrong minimizer status ");
      return false; 
   }
   if (min.X() == 0 ) { 
      MATH_ERROR_MSG("FitResult::Update","Invalid minimizer status ");
      return false; 
   }
   //fNFree = min.NFree(); 
   if (fNFree != min.NFree() ) { 
      MATH_ERROR_MSG("FitResult::Update","Configuration has changed  ");
      return false; 
   }

   fValid = isValid; 
   // update minimum value
   fVal = min.MinValue(); 
   fEdm = min.Edm(); 
   fStatus = min.Status(); 

   // update number of function calls
   if ( min.NCalls() > 0)   fNCalls += min.NCalls();
   else fNCalls += ncalls;

   // copy parameter value and errors 
   std::copy(min.X(), min.X() + npar, fParams.begin());

   if (min.Errors() != 0)  std::copy(min.Errors(), min.Errors() + npar, fErrors.begin() ) ; 

   // set parameters  in fit model function 
   if (fFitFunc) fFitFunc->SetParameters(&fParams.front());
   
   if (fValid) { 

      // update error matrix
      unsigned int r = npar * (  npar + 1 )/2;  
      if (fCovMatrix.size() != r) fCovMatrix.resize(r);
      unsigned int l = 0; 
      for (unsigned int i = 0; i < npar; ++i) {
         for (unsigned int j = 0; j <= i; ++j)  
            fCovMatrix[l++] = min.CovMatrix(i,j);
      }
               
      // update global CC       
      if (fGlobalCC.size() != npar) fGlobalCC.resize(npar);
      for (unsigned int i = 0; i < npar; ++i) { 
         double globcc = min.GlobalCC(i); 
         if (globcc < 0) { 
            break; // it is not supported by that minimizer
            fGlobalCC.clear(); 
         }
         fGlobalCC[i] = globcc; 
      }
    
   }
   return true;
}
 
void FitResult::NormalizeErrors() { 
   // normalize errors and covariance matrix according to chi2 value
   if (fNdf == 0 || fChi2 <= 0) return; 
   double s2 = fChi2/fNdf; 
   double s = std::sqrt(fChi2/fNdf); 
   for (unsigned int i = 0; i < fErrors.size() ; ++i) 
      fErrors[i] *= s; 
   for (unsigned int i = 0; i < fCovMatrix.size() ; ++i) 
      fCovMatrix[i] *= s2; 

   fNormalized = true; 
} 


double FitResult::Prob() const { 
   // fit probability
   return ROOT::Math::chisquared_cdf_c(fChi2, static_cast<double>(fNdf) ); 
}

double FitResult::LowerError(unsigned int i) const { 
   // return lower Minos error for parameter i 
   //  return the parabolic error if Minos error has not been calculated for the parameter i 
   std::map<unsigned int, std::pair<double,double> >::const_iterator itr = fMinosErrors.find(i); 
   return ( itr != fMinosErrors.end() ) ? itr->second.first : Error(i) ;  
}

double FitResult::UpperError(unsigned int i) const { 
   // return upper Minos error for parameter i
   //  return the parabolic error if Minos error has not been calculated for the parameter i 
   std::map<unsigned int, std::pair<double,double> >::const_iterator itr = fMinosErrors.find(i); 
   return ( itr != fMinosErrors.end() ) ? itr->second.second : Error(i) ;  
}

void FitResult::SetMinosError(unsigned int i, double elow, double eup) { 
   // set the Minos error for parameter i 
   fMinosErrors[i] = std::make_pair(elow,eup);
}

int FitResult::Index(const std::string & name) const { 
   // find index for given parameter name
   if (! fFitFunc) return -1;
   unsigned int npar = fParams.size(); 
   for (unsigned int i = 0; i < npar; ++i) 
      if ( fFitFunc->ParameterName(i) == name) return i; 
   
   return -1; // case name is not found
} 

bool FitResult::IsParameterBound(unsigned int ipar) const { 
   for (unsigned int i = 0; i < fBoundParams.size() ; ++i) 
      if ( fBoundParams[i] == ipar) return true; 
   return false; 
}

bool FitResult::IsParameterFixed(unsigned int ipar) const { 
   for (unsigned int i = 0; i < fFixedParams.size() ; ++i) 
      if ( fFixedParams[i] == ipar) return true; 
   return false; 
}

std::string FitResult::GetParameterName(unsigned int ipar) const {
   if (fFitFunc) return fFitFunc->ParameterName(ipar); 
   else if (ipar < fParNames.size() ) return fParNames[ipar];
   return "param_" + ROOT::Math::Util::ToString(ipar);
}

void FitResult::Print(std::ostream & os, bool doCovMatrix) const { 
   // print the result in the given stream 
   // need to add minos errors , globalCC, etc..
   if (!fValid) { 
      os << "\n****************************************\n";
      os << "            Invalid FitResult            ";
   }
   
   os << "\n****************************************\n";
   //os << "            FitResult                   \n\n";
   os << "Minimizer is " << fMinimType << std::endl;
   unsigned int npar = fParams.size(); 
   if (npar == 0) { 
      std::cout << "Error: FitResult is empty !" << std::endl;
      return;
   }
   const unsigned int nw = 25; 
   if (fVal != fChi2 || fChi2 < 0) 
      os << std::setw(nw) << std::left << "LogLikelihood" << " =\t" << fVal << std::endl;
   if (fChi2 >= 0) os << std::setw(nw) << std::left <<  "Chi2"  << " =\t" << fChi2<< std::endl;
   os << std::setw(nw) << std::left << "NDf"    << " =\t" << fNdf << std::endl; 
   if (fMinimType.find("Linear") == std::string::npos) {  // no need to print this for linear fits
      os << std::setw(nw) << std::left << "Edm"    << " =\t" << fEdm << std::endl; 
      os << std::setw(nw) << std::left << "NCalls" << " =\t" << fNCalls << std::endl; 
   }
   for (unsigned int i = 0; i < npar; ++i) { 
      os << std::setw(nw) << std::left << GetParameterName(i); 
      os << " =\t" << std::setw(12) << fParams[i]; 
      if (IsParameterFixed(i) ) 
         os << " \t(fixed)";
      else {
         if (fErrors.size() != 0)
            os << " \t+/-\t" << std::setw(12) << fErrors[i]; 
         if (IsParameterBound(i) ) 
            os << " \t (limited)"; 
      }
      os << std::endl; 
   }

   if (doCovMatrix) PrintCovMatrix(os); 
}

void FitResult::PrintCovMatrix(std::ostream &os) const { 
   // print the covariance and correlation matrix 
   if (!fValid) return;
   if (fCovMatrix.size() == 0) return; 
//   os << "****************************************\n";
   os << "\n            Covariance Matrix            \n\n";
   unsigned int npar = fParams.size(); 
   const int kPrec = 5; 
   const int kWidth = 10; 
   const int parw = 12; 
   const int matw = kWidth+4;
   os << std::setw(parw) << " " << "\t"; 
   for (unsigned int i = 0; i < npar; ++i) {
      if (!IsParameterFixed(i) ) { 
         os << std::setw(matw)  << GetParameterName(i) ;
      }
   }
   os << std::endl;   
   for (unsigned int i = 0; i < npar; ++i) {
      if (!IsParameterFixed(i) ) { 
         os << std::setw(parw) << std::left << GetParameterName(i) << "\t";
         for (unsigned int j = 0; j < npar; ++j) {
            if (!IsParameterFixed(j) ) { 
               os.precision(kPrec); os.width(kWidth);  os << std::setw(matw) << CovMatrix(i,j); 
            }
         }
         os << std::endl;
      }
   }
//   os << "****************************************\n";
   os << "\n            Correlation Matrix         \n\n";
   os << std::setw(parw) << " " << "\t"; 
   for (unsigned int i = 0; i < npar; ++i) {
      if (!IsParameterFixed(i) ) { 
         os << std::setw(matw)  << GetParameterName(i) ;
      }
   }
   os << std::endl;   
   for (unsigned int i = 0; i < npar; ++i) {
      if (!IsParameterFixed(i) ) { 
         os << std::setw(parw) << std::left << GetParameterName(i) << "\t";
         for (unsigned int j = 0; j < npar; ++j) {
            if (!IsParameterFixed(j) ) {
               os.precision(kPrec); os.width(kWidth);  os << std::setw(matw) << Correlation(i,j); 
            }
         }
         os << std::endl;
      }
   }
}

void FitResult::GetConfidenceIntervals(unsigned int n, unsigned int stride1, unsigned int stride2, const double * x, double * ci, double cl ) const {     
   // stride1 stride in coordinate  stride2 stride in dimension space
   // i.e. i-th point in k-dimension is x[ stride1 * i + stride2 * k]
   // compute the confidence interval of the fit on the given data points
   // the dimension of the data points must match the dimension of the fit function
   // confidence intervals are returned in array ci

   // use student quantile
   //double t = - TMath::StudentQuantile((1.-cl)/2, f->GetNDF()); 
   double t = TMath::StudentQuantile(0.5 + cl/2, fNdf); 
   double chidf = TMath::Sqrt(fChi2/fNdf);

   if (!fFitFunc) {
      MATH_ERROR_MSG("FitResult::GetConfidenceIntervals","cannot compute Confidence Intervals without fitter function");
      return;
   }

   unsigned int ndim = fFitFunc->NDim(); 
   unsigned int npar = fFitFunc->NPar(); 

   std::vector<double> xpoint(ndim); 
   std::vector<double> grad(npar); 
   std::vector<double> vsum(npar); 

   // loop on the points
   for (unsigned int ipoint = 0; ipoint < n; ++ipoint) { 

      for (unsigned int kdim = 0; kdim < ndim; ++kdim) { 
         unsigned int i = ipoint * stride1 + kdim * stride2; 
         assert(i < ndim*n); 
         xpoint[kdim] = x[ipoint * stride1 + kdim * stride2]; 
      }

      // calculate gradient of fitted function w.r.t the parameters

      // check first if fFitFunction provides parameter gradient or not 
      
      // does not provide gradient
      // t.b.d : skip calculation for fixed parameters
      ROOT::Math::RichardsonDerivator d; 
      for (unsigned int ipar = 0; ipar < npar; ++ipar) { 
         ROOT::Math::OneDimParamFunctionAdapter<const ROOT::Math::IParamMultiFunction &> fadapter(*fFitFunc,&xpoint.front(),&fParams.front(),ipar);
         d.SetFunction(fadapter); 
         grad[ipar] = d(fParams[ipar] ); // evaluate df/dp
      }

      // multiply covariance matrix with gradient
      vsum.assign(npar,0.0);
      for (unsigned int ipar = 0; ipar < npar; ++ipar) { 
         for (unsigned int jpar = 0; jpar < npar; ++jpar) {
             vsum[ipar] += CovMatrix(ipar,jpar) * grad[jpar]; 
         }
      }
      // multiply gradient by vsum
      double r2 = 0; 
      for (unsigned int ipar = 0; ipar < npar; ++ipar) { 
         r2 += grad[ipar] * vsum[ipar]; 
      }
      double r = std::sqrt(r2); 
      ci[ipoint] = r * t * chidf; 
   }
}

void FitResult::GetConfidenceIntervals(const BinData & data, double * ci, double cl ) const { 
   // implement confidence intervals from a given bin data sets
   // currently copy the data from Bindata. 
   // could implement otherwise directly
   unsigned int ndim = data.NDim(); 
   unsigned int np = data.NPoints(); 
   std::vector<double> xdata( ndim * np ); 
   for (unsigned int i = 0; i < np ; ++i) { 
      const double * x = data.Coords(i); 
      std::vector<double>::iterator itr = xdata.begin()+ ndim * i;
      std::copy(x,x+ndim,itr);
   }
   // points are arraned as x0,y0,z0, ....xN,yN,zN  (stride1=ndim, stride2=1)
   GetConfidenceIntervals(np,ndim,1,&xdata.front(),ci,cl);
}

   } // end namespace Fit

} // end namespace ROOT

