// @(#)root/cintex:$Name:  $:$Id: CINTFunctionBuilder.h,v 1.4 2005/12/12 09:12:27 roiser Exp $
// Author: Pere Mato 2005

// Copyright CERN, CH-1211 Geneva 23, 2004-2005, All rights reserved.
//
// Permission to use, copy, modify, and distribute this software for any
// purpose is hereby granted without fee, provided that this copyright and
// permissions notice appear in all copies and derivatives.
//
// This software is provided "as is" without express or implied warranty.

#ifndef ROOT_Cintex_CINTFunctionBuilder
#define ROOT_Cintex_CINTFunctionBuilder

#include "Reflex/Type.h"
#include "CINTdefs.h"

namespace ROOT {
   namespace Cintex {

      class CINTFunctionBuilder {
      public:
         CINTFunctionBuilder(const ROOT::Reflex::Member& m);
         ~CINTFunctionBuilder();
         void Setup(void);
         static void Setup(const ROOT::Reflex::Member&);
      private:
         const ROOT::Reflex::Member&  fFunction;
      };

   }
}

#endif // ROOT_Cintex_CINTFunctionBuilder
