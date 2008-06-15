/*****************************************************************************
 * Project: RooFit                                                           *
 * Package: RooFitCore                                                       *
 * @(#)root/roofitcore:$Id$
 * Authors:                                                                  *
 *   WV, Wouter Verkerke, UC Santa Barbara, verkerke@slac.stanford.edu       *
 *   DK, David Kirkby,    UC Irvine,         dkirkby@uci.edu                 *
 *                                                                           *
 * Copyright (c) 2000-2005, Regents of the University of California          *
 *                          and Stanford University. All rights reserved.    *
 *                                                                           *
 * Redistribution and use in source and binary forms,                        *
 * with or without modification, are permitted according to the terms        *
 * listed in LICENSE (http://roofit.sourceforge.net/license.txt)             *
 *****************************************************************************/

//////////////////////////////////////////////////////////////////////////////
//
// BEGIN_HTML
// RooRealProxy is the concrete proxy for RooAbsReal objects
// A RooRealProxy is the general mechanism to store references
// to RooAbsReals inside a RooAbsArg
//
// RooRealProxy provides a cast operator to Double_t, allowing
// the proxy to functions a Double_t on the right hand side of expressions.
// END_HTML
//

#include "RooFit.h"
#include "Riostream.h"

#include "TClass.h"
#include "RooRealProxy.h"
#include "RooRealVar.h"

ClassImp(RooRealProxy)
;


//_____________________________________________________________________________
RooRealProxy::RooRealProxy(const char* inName, const char* desc, RooAbsArg* owner, RooAbsReal& ref,
			   Bool_t valueServer, Bool_t shapeServer, Bool_t ownArg) : 
  RooArgProxy(inName, desc, owner,ref, valueServer, shapeServer, ownArg)
{
  // Constructor with owner and proxied real-valued object
}



//_____________________________________________________________________________
RooRealProxy::RooRealProxy(const char* inName, RooAbsArg* owner, const RooRealProxy& other) : 
  RooArgProxy(inName, owner, other) 
{
  // Copy constructor 
}



//_____________________________________________________________________________
RooRealProxy::~RooRealProxy() 
{
  // Destructor
}



//_____________________________________________________________________________
RooAbsRealLValue* RooRealProxy::lvptr() const 
{
  // Assert that the held arg is an LValue
  RooAbsRealLValue* Lvptr = (RooAbsRealLValue*)dynamic_cast<const RooAbsRealLValue*>(_arg) ;
  if (!Lvptr) {
    cout << "RooRealProxy(" << name() << ")::INTERNAL error, expected " << _arg->GetName() << " to be an lvalue" << endl ;
    assert(0) ;
  }
  return Lvptr ;
}

