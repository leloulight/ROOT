// @(#)root/alien:$Name:  $:$Id: TAlienResult.h,v 1.5 2005/08/12 15:46:40 rdm Exp $
// Author: Fons Rademakers   3/1/2002

/*************************************************************************
 * Copyright (C) 1995-2002, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TAlienResult
#define ROOT_TAlienResult

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TAlienResult                                                         //
//                                                                      //
// Class defining interface to a Alien result set.                      //
// Objects of this class are created by TGrid methods.                  //
//                                                                      //
// Related classes are TAlien.                                          //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifndef ROOT_TGridResult
#include "TGridResult.h"
#endif

class TAlienResult : public TGridResult {

private:
   mutable TString fFilePath;  // file path

public:
   virtual void DumpResult();

   virtual const char *GetFileName(UInt_t i) const;             // returns the file name of list item i
   virtual const char *GetFileNamePath(UInt_t i) const;         // returns the full path + file name of list item i
   virtual const char *GetPath(UInt_t i) const;                 // returns the file path of list item i
   virtual const char *GetKey(UInt_t i, const char *key) const; // returns the key value of list item i
   virtual TList      *GetFileInfoList() const;                 // returns a new allocated List of TFileInfo Objects
           void        Print(Option_t *option="") const;
           void        Print(Option_t *wildcard, Option_t *option) const;
   ClassDef(TAlienResult,0)  // Alien query result set
};

#endif
