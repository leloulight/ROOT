// @(#)root/memstat:$Id: TMemStat.cxx 32678 2010-03-18 16:32:00Z anar $
// Author: Anar Manafov (A.Manafov@gsi.de) 2008-03-02

/*************************************************************************
* Copyright (C) 1995-2010, Rene Brun and Fons Rademakers.               *
* All rights reserved.                                                  *
*                                                                       *
* For the licensing terms see $ROOTSYS/LICENSE.                         *
* For the list of contributors see $ROOTSYS/README/CREDITS.             *
*************************************************************************/
#include "TDirectory.h"

#include "TMemStat.h"
#include "TMemStatBacktrace.h"
#include "TMemStatMng.h"
#include "TMemStatHelpers.h"

ClassImp(TMemStat)

using namespace std;
using namespace memstat;

_INIT_TOP_STACK;

//______________________________________________________________________________
TMemStat::TMemStat(Option_t* option): fIsActive(kFALSE)
{
   // Supported options:
   //    "gnubuiltin" - if declared, then MemStat will use gcc build-in function,
   //                      otherwise glibc backtrace will be used
   //
   // Note: Currently MemStat uses a hard-coded output file name (for writing) = "memstat.root";

   // It marks the highest used stack address.
   _GET_CALLER_FRAME_ADDR;

   //preserve context. When exiting will restore the current directory
   TDirectory::TContext context(gDirectory);

   Bool_t useBuiltin = kTRUE;
   // Define string in a scope, so that the deletion of it will be not recorded by YAMS
   {
      string opt(option);
      transform(opt.begin(), opt.end(), opt.begin(),
                memstat::ToLower_t());

      useBuiltin = (opt.find("gnubuiltin") != string::npos) ? kTRUE : kFALSE;
   }

   TMemStatMng::GetInstance()->SetUseGNUBuiltinBacktrace(useBuiltin);
   TMemStatMng::GetInstance()->Enable();
   // set this variable only if "NEW" mode is active
   fIsActive = kTRUE;

}

//______________________________________________________________________________
TMemStat::~TMemStat()
{
   if (fIsActive) {
      TMemStatMng::GetInstance()->Disable();
      TMemStatMng::GetInstance()->Close();
   }
}
