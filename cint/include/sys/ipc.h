/* -*- C++ -*- */
/*************************************************************************
 * Copyright(c) 1995~2005  Masaharu Goto (cint@pcroot.cern.ch)
 *
 * For the licensing terms see the file COPYING
 *
 ************************************************************************/
/****************************************************************
* sys/ipc.h
*****************************************************************/
#ifndef G__IPC_H
#define G__IPC_H

/* NOTE: ipc.dll is not generated by default. 
 * Goto $CINTSYSDIR/lib/ipc directory and do 'sh setup' if you use UNIX. */
#ifndef G__IPCDLL_H
#pragma include_noerr "sys/ipc.dll"
#endif

#ifndef __MAKECINT__
#pragma ifndef G__IPCDLL_H /* G__IPCDLL_H is defined in pthread.dll */
#pragma message Note: ipc.dll is not found. Do 'sh setup' in $CINTSYSDIR/lib/ipc directory if you use UNIX.
#pragma endif
#endif

#endif
