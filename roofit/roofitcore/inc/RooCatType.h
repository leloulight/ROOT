/*****************************************************************************
 * Project: RooFit                                                           *
 * Package: RooFitCore                                                       *
 *    File: $Id: RooCatType.h,v 1.20 2007/05/11 09:11:30 verkerke Exp $
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
#ifndef ROO_CAT_TYPE
#define ROO_CAT_TYPE

#include "Riosfwd.h"
#include "TObject.h"
#include "RooPrintable.h"

class RooCatType : public TObject, public RooPrintable {
public:
  inline RooCatType() : TObject(), RooPrintable() { _value = 0 ; _label[0] = 0 ; } 
  inline RooCatType(const char* name, Int_t value) : TObject(), RooPrintable(), _value(value) { SetName(name) ; } 
  inline RooCatType(const RooCatType& other) : TObject(other), RooPrintable(other), _value(other._value) { strcpy(_label,other._label) ;} ;
  virtual ~RooCatType() {} ;
  virtual TObject* Clone(const char*) const { return new RooCatType(*this); }
  virtual const Text_t* GetName() const { return _label ; }
  virtual void SetName(const Text_t* name) ;

  inline RooCatType& operator=(const RooCatType& other) { 
    if (&other==this) return *this ;
    SetName(other.GetName()) ; 
    _value = other._value ; 
    return *this ; } 

  inline Bool_t operator==(const RooCatType& other) {
    return ( _value==other._value && !strcmp(_label,other._label)) ;
  }

  inline Bool_t operator==(Int_t index) { return (_value==index) ; }

  Bool_t operator==(const char* label) { return !strcmp(_label,label) ; }

  inline Int_t getVal() const { return _value ; }
  void setVal(Int_t newValue) { _value = newValue ; }

  virtual void printName(ostream& os) const ;
  virtual void printTitle(ostream& os) const ;
  virtual void printClassName(ostream& os) const ;
  virtual void printValue(ostream& os) const ;
  
  inline virtual void Print(Option_t *options= 0) const {
    printStream(defaultPrintStream(),defaultPrintContents(options),defaultPrintStyle(options));
  }

protected:
  friend class RooAbsCategory ;
  Int_t _value ;
  char _label[256] ;
	
  ClassDef(RooCatType,1) // Category state, (name,index) pair
} ;


#endif

