// @(#)root/gl:$Id$
// Author:  Olivier Couet  12/04/2007

/*************************************************************************
 * Copyright (C) 1995-2006, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/
#include "Riostream.h"
#include "TROOT.h"
#include "TError.h"

#include "TGLText.h"
#include "TColor.h"
#include "TSystem.h"
#include "TEnv.h"

#include "FTGLExtrdFont.h"
#include "FTGLOutlineFont.h"
#include "FTGLPolygonFont.h"
#include "FTGLTextureFont.h"
#include "FTGLPixmapFont.h"
#include "FTGLBitmapFont.h"

#define FTGL_BITMAP  0
#define FTGL_PIXMAP  1
#define FTGL_OUTLINE 2
#define FTGL_POLYGON 3
#define FTGL_EXTRUDE 4
#define FTGL_TEXTURE 5

ClassImp(TGLText)


//______________________________________________________________________________
TGLText::TGLText()
{
   fX      = 0;
   fY      = 0;
   fZ      = 0;
   fAngle1 = 90;
   fAngle2 = 0;
   fAngle3 = 0;
   fGLTextFont = 0;
   SetGLTextFont(13); // Default font.
}


//______________________________________________________________________________
TGLText::TGLText(Double_t x, Double_t y, Double_t z, const char * /*text*/)
{
   // TGLext normal constructor.

   fX      = x;
   fY      = y;
   fZ      = z;
   fAngle1 = 90;
   fAngle2 = 0;
   fAngle3 = 0;
   fGLTextFont = 0;
   SetGLTextFont(13); // Default font.
}


//______________________________________________________________________________
TGLText::~TGLText()
{
   if (fGLTextFont) delete fGLTextFont;
}


//______________________________________________________________________________
void TGLText::PaintGLText(Double_t x, Double_t y, Double_t z, const char *text)
{
   // Draw text

   if (!fGLTextFont) return;

   glPushMatrix();
   glTranslatef(x, y, z);

   // Set Text color.
   TColor *col;
   Float_t red, green, blue;
   col = gROOT->GetColor(GetTextColor());
   col->GetRGB(red, green, blue);
   glColor3d(red, green, blue);

   // Text size
   Double_t s = GetTextSize();
   glScalef(s,s,s);

   // Text alignment
   Float_t llx, lly, llz, urx, ury, urz;
   fGLTextFont->BBox(text, llx, lly, llz, urx, ury, urz);
   Short_t halign = fTextAlign/10;
   Short_t valign = fTextAlign - 10*halign;
   Float_t dx = 0, dy = 0;
   switch (halign) {
      case 1 : dx =  0    ; break;
      case 2 : dx = -urx/2; break;
      case 3 : dx = -urx  ; break;
   }
   switch (valign) {
      case 1 : dy =  0    ; break;
      case 2 : dy = -ury/2; break;
      case 3 : dy = -ury  ; break;
   }
   glTranslatef(dx, dy, 0);

   //In XY plane
   glRotatef(fAngle1,1.,0.,0.);

   //In XZ plane
   glRotatef(fAngle2,0.,1.,0.);

   //In YZ plane
   glRotatef(fAngle3,0.,0.,1.);

   // Render text
   fGLTextFont->Render(text);

   glPopMatrix();
}


//______________________________________________________________________________
void TGLText::PaintBBox(const char *text)
{
   Float_t llx, lly, llz, urx, ury, urz;
   fGLTextFont->BBox(text, llx, lly, llz, urx, ury, urz);
   glBegin(GL_LINES);
   glVertex3f(   0,   0, 0); glVertex3f( urx,   0, 0);
   glVertex3f(   0,   0, 0); glVertex3f(   0, ury, 0);
   glVertex3f(   0, ury, 0); glVertex3f( urx, ury, 0);
   glVertex3f( urx, ury, 0); glVertex3f( urx,   0, 0);
   glEnd();
}

//______________________________________________________________________________
void TGLText::BBox(const char* string, float& llx, float& lly, float& llz,
                                       float& urx, float& ury, float& urz)
{
   // Calculate bounding-box for given string.

   fGLTextFont->BBox(string, llx, lly, llz, urx, ury, urz);
}

//______________________________________________________________________________
void TGLText::SetGLTextAngles(Double_t a1, Double_t a2, Double_t a3)
{
   // Set the text rotation angles.

   fAngle1 = a1;
   fAngle2 = a2;
   fAngle3 = a3;
}


//______________________________________________________________________________
void TGLText::SetGLTextFont(Font_t fontnumber)
{
   int fontid = fontnumber / 10;

   char *fontname=0;
   if (fontid == 0)  fontname = "arialbd.ttf";
   if (fontid == 1)  fontname = "timesi.ttf";
   if (fontid == 2)  fontname = "timesbd.ttf";
   if (fontid == 3)  fontname = "timesbi.ttf";
   if (fontid == 4)  fontname = "arial.ttf";
   if (fontid == 5)  fontname = "ariali.ttf";
   if (fontid == 6)  fontname = "arialbd.ttf";
   if (fontid == 7)  fontname = "arialbi.ttf";
   if (fontid == 8)  fontname = "cour.ttf";
   if (fontid == 9)  fontname = "couri.ttf";
   if (fontid == 10) fontname = "courbd.ttf";
   if (fontid == 11) fontname = "courbi.ttf";
   if (fontid == 12) fontname = "symbol.ttf";
   if (fontid == 13) fontname = "times.ttf";
   if (fontid == 14) fontname = "wingding.ttf";

   // try to load font (font must be in Root.TTFontPath resource)
   const char *ttpath = gEnv->GetValue("Root.TTFontPath",
# ifdef TTFFONTDIR
                                        TTFFONTDIR
# else
                                        "$(ROOTSYS)/fonts"
# endif
                                        );

   char *ttfont = gSystem->Which(ttpath, fontname, kReadPermission);

   if (fGLTextFont) delete fGLTextFont;

// fGLTextFont = new FTGLOutlineFont(ttfont);

   fGLTextFont = new FTGLPolygonFont(ttfont);

   if (!fGLTextFont->FaceSize(1))
      Error("SetGLTextFont","Cannot set FTGL::FaceSize"),
   delete [] ttfont;
}
