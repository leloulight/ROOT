// @(#)root/qt:$Id$
// Author: Valeri Fine   21/01/2002

/*************************************************************************
 * Copyright (C) 1995-2004, Rene Brun and Fons Rademakers.               *
 * Copyright (C) 2002 by Valeri Fine.                                    *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TQtMarker
#define ROOT_TQtMarker

#ifndef ROOT_TPoint
#include "TPoint.h"
#endif

#ifndef __CINT__
#  include "qglobal.h"
#  if QT_VERSION < 0x40000
#    include <qpointarray.h>
#  else /* QT_VERSION */
#     include <QPolygon>
#  endif /* QT_VERSION */
#else
   class QPointArray;
   class QPolygon;
#endif

////////////////////////////////////////////////////////////////////////
//
// TQtMarker - class-utility to convert the ROOT TMarker object shape 
//             in to the Qt QPointArray.
//
////////////////////////////////////////////////////////////////////////

class TQtMarker {

private:

   int     fNumNode;       // Number of chain in the marker shape
#ifndef __CINT__
#if (QT_VERSION < 0x40000)
   QPointArray  fChain;    // array of the n chains to build a shaped marker
#else /* QT_VERSION */
   QPolygon  fChain;    // array of the n chains to build a shaped marker
#endif /* QT_VERSION */
#endif
   Color_t fCindex;        // Color index of the marker;
   int     fMarkerType;    // Type of the current marker

public:

   TQtMarker(int n=0, TPoint *xy=0,int type=0);
   void operator=(const TQtMarker&);
   TQtMarker(const TQtMarker&);
   virtual ~TQtMarker();
   int     GetNumber() const;
#ifndef __CINT__
#if (QT_VERSION < 0x40000)
   QPointArray &GetNodes();
#else /* QT_VERSION */
   QPolygon &GetNodes();
#endif /* QT_VERSION */
#endif
   int     GetType() const;
   void    SetMarker(int n, TPoint *xy, int type);
   ClassDef(TQtMarker,0) //  Convert  ROOT TMarker objects on to QPointArray
};

//_________________________________________________________
inline void TQtMarker::operator=(const TQtMarker&m) 
{
   fNumNode = m.fNumNode;
   fChain   = m.fChain; 
   fCindex  = m.fCindex;
   fMarkerType=m.fMarkerType;
}
//_________________________________________________________
inline TQtMarker::TQtMarker(const TQtMarker&m) : fNumNode(m.fNumNode),
fChain(m.fChain), fCindex(m.fCindex),fMarkerType(m.fMarkerType) {}

#endif
