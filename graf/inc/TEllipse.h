// @(#)root/graf:$Name:  $:$Id: TEllipse.h,v 1.1.1.1 2000/05/16 17:00:50 rdm Exp $
// Author: Rene Brun   16/10/95

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TEllipse
#define ROOT_TEllipse


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TEllipse                                                             //
//                                                                      //
// An ellipse.                                                          //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifndef ROOT_TObject
#include "TObject.h"
#endif
#ifndef ROOT_TAttLine
#include "TAttLine.h"
#endif
#ifndef ROOT_TAttFill
#include "TAttFill.h"
#endif


class TEllipse : public TObject, public TAttLine, public TAttFill {

protected:
        Double_t    fX1;        //X coordinate of centre
        Double_t    fY1;        //Y coordinate of centre
        Double_t    fR1;        //first radius
        Double_t    fR2;        //second radius
        Double_t    fPhimin;    //Minimum angle (degrees)
        Double_t    fPhimax;    //Maximum angle (degrees)
        Double_t    fTheta;     //Rotation angle (degrees)

public:
        TEllipse();
        TEllipse(Double_t x1, Double_t y1,Double_t r1,Double_t r2=0,Double_t phimin=0, Double_t phimax=360,Double_t theta=0);
        TEllipse(const TEllipse &ellipse);
        virtual ~TEllipse();
                void   Copy(TObject &ellipse);
        virtual Int_t  DistancetoPrimitive(Int_t px, Int_t py);
        virtual void   Draw(Option_t *option="");
        virtual void   DrawEllipse(Double_t x1, Double_t y1, Double_t r1,Double_t r2,Double_t phimin, Double_t phimax,Double_t theta);
        virtual void   ExecuteEvent(Int_t event, Int_t px, Int_t py);
        Double_t       GetX1() {return fX1;}
        Double_t       GetY1() {return fY1;}
        Double_t       GetR1() {return fR1;}
        Double_t       GetR2() {return fR2;}
        Double_t       GetPhimin() {return fPhimin;}
        Double_t       GetPhimax() {return fPhimax;}
        Double_t       GetTheta()  {return fTheta;}
        virtual void   ls(Option_t *option="");
        virtual void   Paint(Option_t *option="");
        virtual void   PaintEllipse(Double_t x1, Double_t y1, Double_t r1,Double_t r2,Double_t phimin, Double_t phimax,Double_t theta);
        virtual void   Print(Option_t *option="");
        virtual void   SavePrimitive(ofstream &out, Option_t *option);
        virtual void   SetPhimin(Double_t phi=0)   {fPhimin=phi;} // *MENU*
        virtual void   SetPhimax(Double_t phi=360) {fPhimax=phi;} // *MENU*
        virtual void   SetR1(Double_t r1) {fR1=r1;} // *MENU*
        virtual void   SetR2(Double_t r2) {fR2=r2;} // *MENU*
        virtual void   SetTheta(Double_t theta=0) {fTheta=theta;} // *MENU*
        virtual void   SetX1(Double_t x1) {fX1=x1;} // *MENU*
        virtual void   SetY1(Double_t y1) {fY1=y1;} // *MENU*

        ClassDef(TEllipse,2)  //An ellipse
};

#endif
