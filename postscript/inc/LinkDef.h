/* @(#)root/postscript:$Id: LinkDef.h,v 1.4 2004/02/13 17:04:35 brun Exp $ */

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ class TPostScript+;
#pragma link C++ class TSVG+;
#pragma link C++ class TPDF+;
#pragma link C++ class TImageDump+;

#endif
