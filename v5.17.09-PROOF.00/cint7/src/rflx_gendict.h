/* -*- C++ -*- */
/*************************************************************************
 * Copyright(c) 1995~2005  Masaharu Goto (cint@pcroot.cern.ch)
 *
 * For the licensing terms see the file COPYING
 *
 ************************************************************************/
//$Id: rflx_gendict.h,v 1.1.4.3 2006/07/25 16:05:35 axel Exp $

#ifndef RFLX_GENDICT_H
#define RFLX_GENDICT_H 1

namespace Cint {
   namespace Internal {

      void rflx_gendict(const char * linkfilename,
		        const char * sourcefile);

   } // namespace Internal
} // namespace Cint

#endif // RFLX_GENDICT_H
