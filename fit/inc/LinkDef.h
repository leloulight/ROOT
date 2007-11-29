// @(#)root/minuit2:$Id: LinkDef.h,v 1.3 2005/12/09 09:49:00 moneta Exp $
// Author: L. Moneta    10/2005  

/**********************************************************************
 *                                                                    *
 * Copyright (c) 2005 ROOT Foundation,  CERN/PH-SFT                   *
 *                                                                    *
 **********************************************************************/


#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;


//#pragma link C++ class ROOT::Fit::BinPoint;
#pragma link C++ class ROOT::Fit::DataRange;
#pragma link C++ class ROOT::Fit::DataOptions;
//#pragma link C++ class ROOT::Fit::DataVector<ROOT::Fit::BinPoint>;
#pragma link C++ class ROOT::Fit::Fitter;
#pragma link C++ class ROOT::Fit::FitConfig+;
#pragma link C++ class ROOT::Fit::BinData+;
#pragma link C++ class ROOT::Fit::UnBinData+;
#pragma link C++ class ROOT::Fit::FitResult+;
#pragma link C++ class ROOT::Fit::ParameterSettings+;

#ifndef _WIN32
#pragma link C++ class ROOT::Fit::Chi2FCN<ROOT::Math::IBaseFunctionMultiDim>-;
#pragma link C++ class ROOT::Fit::Chi2FCN<ROOT::Math::IGradientFunctionMultiDim>-;
#pragma link C++ class ROOT::Fit::LogLikelihoodFCN<ROOT::Math::IBaseFunctionMultiDim>-;
#pragma link C++ class ROOT::Fit::LogLikelihoodFCN<ROOT::Math::IGradientFunctionMultiDim>-;
#pragma link C++ class ROOT::Fit::PoissonLikelihoodFCN<ROOT::Math::IBaseFunctionMultiDim>-;
#pragma link C++ class ROOT::Fit::PoissonLikelihoodFCN<ROOT::Math::IGradientFunctionMultiDim>-;
#endif


#pragma link C++ class ROOT::Math::WrappedTF1;
#pragma link C++ class ROOT::Math::WrappedMultiTF1;

//#pragma extra_include "Math/IParamFunction.h"

//fitter template functions
#pragma link C++ function ROOT::Fit::Fitter::Fit(const ROOT::Fit::BinData &, const ROOT::Math::IParametricFunctionMultiDim&);
#pragma link C++ function ROOT::Fit::Fitter::Fit(const ROOT::Fit::UnBinData &,const ROOT::Math::IParametricFunctionMultiDim &);
#pragma link C++ function ROOT::Fit::Fitter::LikelihoodFit(const ROOT::Fit::BinData &,const ROOT::Math::IParametricFunctionMultiDim&);
#pragma link C++ function ROOT::Fit::Fitter::LikelihoodFit(const ROOT::Fit::UnBinData &,const ROOT::Math::IParametricFunctionMultiDim&);
#pragma link C++ function ROOT::Fit::Fitter::LikelihoodFit(const ROOT::Fit::BinData &);
#pragma link C++ function ROOT::Fit::Fitter::LikelihoodFit(const ROOT::Fit::UnBinData &);



#pragma link C++ namespace ROOT::Fit;


#endif
