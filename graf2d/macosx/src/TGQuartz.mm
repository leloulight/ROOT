// @(#)root/graf2d:$Id$
// Author: Olivier Couet, 23/01/2012

/*************************************************************************
 * Copyright (C) 1995-2011, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/
#include <Cocoa/Cocoa.h>

#include "QuartzFillArea.h"
#include "QuartzMarker.h"
#include "CocoaPrivate.h"
#include "QuartzWindow.h"
#include "X11Drawable.h"
#include "QuartzText.h"
#include "QuartzLine.h"

#include "TGQuartz.h"
#include "TPoint.h"
#include "TColor.h"
#include "TStyle.h"
#include "TROOT.h"

ClassImp(TGQuartz)

using namespace ROOT;

//______________________________________________________________________________
TGQuartz::TGQuartz()
{
   // TGQuartz default constructor
}


//______________________________________________________________________________
TGQuartz::TGQuartz(const char *name, const char *title)
            : TGCocoa(name, title)
{
   // TGQuartz normal constructor
}


//______________________________________________________________________________
void TGQuartz::DrawBox(Int_t x1, Int_t y1, Int_t x2, Int_t y2, EBoxMode mode)
{
   // Draw a box

   CGContextRef ctx = (CGContextRef)GetCurrentContext();
   const Quartz::CGStateGuard ctxGuard(ctx);

   if (!SetContextFillColor(GetFillColor())) {
      Error("DrawBox", "Fill color for index %d not found", GetFillColor());
      return;
   }
   
   if (!SetContextStrokeColor(GetLineColor())) {
      Error("DrawBox", "Line color for index %d not found", GetLineColor());
      return;
   }
   
   const TColor *color = gROOT->GetColor(GetFillColor());
   if (!color) {
      //It can not be null (checked already in SetContextFillColor, but
      //just to avoid warnings from coverity in a future.
      return;
   }

   Float_t r = 0.f;
   Float_t g = 0.f;
   Float_t b = 0.f;
   const Float_t a = GetFillAlpha() / 100.f;   
   color->GetRGB(r, g, b);

   Quartz::SetFillStyle(ctx, GetFillStyle(), r, g, b, a);
   Quartz::DrawBox(ctx, x1, y1, x2, y2, (Int_t)mode);
}


//______________________________________________________________________________
void TGQuartz::DrawFillArea(Int_t n, TPoint * xy)
{
   // Draw a filled area through all points.
   // n         : number of points
   // xy        : list of points

   CGContextRef ctx = (CGContextRef)GetCurrentContext();
   const Quartz::CGStateGuard ctxGuard(ctx);

   TColor *color = gROOT->GetColor(GetFillColor());
   if (!color) {
      Error("DrawFillArea", "Could not find TColor for index %d", GetFillColor());
      return;
   }
      
   Float_t rgb[3] = {};
   color->GetRGB(rgb[0], rgb[1], rgb[2]);
   const Float_t a = GetFillAlpha() / 100.f;

   if (GetFillGradient() == kNoGradientFill) {
      //For coverity: I do not check these two calls,
      //if we are here, TColor exists.
      SetContextStrokeColor(GetFillColor());
      SetContextFillColor(GetFillColor());

      Quartz::SetFillStyle(ctx, GetFillStyle(), rgb[0], rgb[1], rgb[2], a);
      Quartz::DrawFillArea(ctx, n, xy, kFALSE);//The last argument - do not draw shadows.
   } else {
      Quartz::DrawFillAreaGradient(GetFillGradient(), ctx, n, xy, rgb, kTRUE);//kTRUE == draw shadows.
   }
}


//______________________________________________________________________________
void TGQuartz::DrawCellArray(Int_t /*x1*/, Int_t /*y1*/, Int_t /*x2*/, Int_t /*y2*/, 
                             Int_t /*nx*/, Int_t /*ny*/, Int_t */*ic*/)
{
   // Draw CellArray
   
   //CGContextRef ctx = (CGContextRef)GetCurrentContext();
}


//______________________________________________________________________________
void TGQuartz::DrawLine(Int_t x1, Int_t y1, Int_t x2, Int_t y2)
{
   // Draw a line.
   // x1,y1        : begin of line
   // x2,y2        : end of line
      
   CGContextRef ctx = (CGContextRef)GetCurrentContext();   
   const Quartz::CGStateGuard ctxGuard(ctx);

   if (!SetContextStrokeColor(GetLineColor())) {
      Error("DrawLine", "Could not find TColor for index %d", GetLineColor());
      return;
   }

   Quartz::SetLineStyle(ctx, GetLineStyle());
   Quartz::SetLineWidth(ctx, GetLineWidth());
   Quartz::DrawLine(ctx, x1, y1, x2, y2);
}


//______________________________________________________________________________
void TGQuartz::DrawPolyLine(Int_t n, TPoint *xy)
{
   // Draw a line through all points.
   // n         : number of points
   // xy        : list of points   
   
   CGContextRef ctx = (CGContextRef)GetCurrentContext();   
   const Quartz::CGStateGuard ctxGuard(ctx);
   
   if (!SetContextStrokeColor(GetLineColor())) {
      Error("DrawPolyLine", "Could not find TColor for index %d", GetLineColor());
      return;
   }
   
   Quartz::SetLineStyle(ctx, GetLineStyle());
   Quartz::SetLineWidth(ctx, GetLineWidth());
   Quartz::DrawPolyLine(ctx, n, xy);
}


//______________________________________________________________________________
void TGQuartz::DrawPolyMarker(Int_t n, TPoint *xy)
{
   // Draw PolyMarker
   // n         : number of points
   // xy        : list of points   
   CGContextRef ctx = (CGContextRef)GetCurrentContext();
   const Quartz::CGStateGuard ctxGuard(ctx);

   if (!SetContextFillColor(GetMarkerColor())) {
      Error("DrawPolyMarker", "Could not find TColor for index %d", GetMarkerColor());
      return;
   }
   
   SetContextStrokeColor(GetMarkerColor());//Can not fail (for coverity).

   Quartz::SetLineStyle(ctx, 1);
   Quartz::SetLineWidth(ctx, 1);
   Quartz::DrawPolyMarker(ctx, n, xy, GetMarkerSize(), GetMarkerStyle());
}


//______________________________________________________________________________
void TGQuartz::DrawText(Int_t x, Int_t y, Float_t angle, Float_t /*mgn*/, 
                        const char *text, ETextMode /*mode*/)
{
   // Draw text
   if (fSelectedDrawable <= 0) {
      Error("DrawText", "internal error, no pixmap was selected");
      return;
   }
   
   if (!fPimpl.get()) {
      Error("DrawText", "internal error, internal data was not initialized correctly");
      return;
   }
   
   assert(fSelectedDrawable > fPimpl->GetRootWindowID() && "DrawText, no pixmap selected");
   NSObject<X11Drawable> *pixmap = fPimpl->GetDrawable(fSelectedDrawable);
   assert(pixmap.fIsPixmap == YES && "DrawText, selected drawable is not a pixmap");
   
   //
   CGContextRef ctx = (CGContextRef)GetCurrentContext();
   const Quartz::CGStateGuard ctxGuard(ctx);

   if (!SetContextFillColor(GetTextColor())) {
      Error("DrawText", "Could not find TColor for index %d", GetTextColor());
      return;
   }

   //Before any core text drawing operations, reset text matrix.
   CGContextSetTextMatrix(ctx, CGAffineTransformIdentity); 
   
   CGContextTranslateCTM(ctx, 0., pixmap.fHeight);
   CGContextScaleCTM(ctx, 1., -1.);

   Quartz::DrawText(ctx, (Double_t)x, 
                    ROOT::MacOSX::X11::LocalYROOTToCocoa(pixmap, y), 
                    angle, 
                    GetTextAlign(),
                    GetTextFont(),
                    GetTextSize(),
                    text);
}

//______________________________________________________________________________
void TGQuartz::GetTextExtent(UInt_t &w, UInt_t &h, char *text)
{
   // Returns the size of the specified character string "mess".
   //
   // w    - the text width
   // h    - the text height
   // text - the string   
   Quartz::GetTextExtent(w, h, GetTextFont(), GetTextSize(), text);
}


//______________________________________________________________________________
Int_t TGQuartz::GetFontAscent() const
{
   // Returns the ascent of the current font (in pixels).
   // The ascent of a font is the distance from the baseline
   // to the highest position characters extend to
   return 0;
}


//______________________________________________________________________________
Int_t TGQuartz::GetFontDescent() const
{
   // Returns the descent of the current font (in pixels.
   // The descent is the distance from the base line
   // to the lowest point characters extend to.
   return 0;
}


//______________________________________________________________________________
Float_t TGQuartz::GetTextMagnitude()
{
   // Returns the current font magnification factor
   return 0;
}

//______________________________________________________________________________
void TGQuartz::SetLineColor(Color_t cindex)
{
   // Set color index "cindex" for drawing lines.
   TAttLine::SetLineColor(cindex);
}


//______________________________________________________________________________
void TGQuartz::SetLineStyle(Style_t lstyle)
{
   // Set line style.   
   TAttLine::SetLineStyle(lstyle);
}


//______________________________________________________________________________
void TGQuartz::SetLineWidth(Width_t width)
{
   // Set the line width.
   
   TAttLine::SetLineWidth(width);
}


//______________________________________________________________________________
void TGQuartz::SetFillColor(Color_t cindex)
{
   // Set color index "cindex" for fill areas.

   TAttFill::SetFillColor(cindex);
}


//______________________________________________________________________________
void TGQuartz::SetFillStyle(Style_t style)
{
   // Set fill area style.   
   TAttFill::SetFillStyle(style);
}


//______________________________________________________________________________
void TGQuartz::SetMarkerColor(Color_t cindex)
{
   // Set color index "cindex" for markers.
   TAttMarker::SetMarkerColor(cindex);
}


//______________________________________________________________________________
void TGQuartz::SetMarkerSize(Float_t markersize)
{
   // Set marker size index.
   //
   // markersize - the marker scale factor
   TAttMarker::SetMarkerSize(markersize);
}


//______________________________________________________________________________
void TGQuartz::SetMarkerStyle(Style_t markerstyle)
{
   // Set marker style.

   TAttMarker::SetMarkerStyle(markerstyle);
}


//______________________________________________________________________________
void TGQuartz::SetTextAlign(Short_t talign)
{
   // Set the text alignment.
   //
   // talign = txalh horizontal text alignment
   // talign = txalv vertical text alignment

   TAttText::SetTextAlign(talign);
}


//______________________________________________________________________________
void TGQuartz::SetTextColor(Color_t cindex)
{
   // Set the color index "cindex" for text.

   TAttText::SetTextColor(cindex);
}


//______________________________________________________________________________
void TGQuartz::SetTextFont(Font_t fontnumber)
{
   // Set the current text font number.

   TAttText::SetTextFont(fontnumber);
}


//______________________________________________________________________________
void TGQuartz::SetTextSize(Float_t textsize)
{
   // Set the current text size to "textsize"
   
   TAttText::SetTextSize(textsize);
}


//______________________________________________________________________________
void TGQuartz::SetOpacity(Int_t /*percent*/)
{
   // Set opacity of the current window. This image manipulation routine
   // works by adding to a percent amount of neutral to each pixels RGB.
   // Since it requires quite some additional color map entries is it
   // only supported on displays with more than > 8 color planes (> 256
   // colors).
}


//______________________________________________________________________________
Int_t TGQuartz::SetTextFont(char * /*fontname*/, ETextSetMode /*mode*/)
{
   // Set text font to specified name "fontname".This function returns 0 if
   // the specified font is found, 1 if it is not.
   //
   // mode - loading flag
   //        mode = 0 search if the font exist (kCheck)
   //        mode = 1 search the font and load it if it exists (kLoad)
   
   return 0;
}

//______________________________________________________________________________
Bool_t TGQuartz::SetContextFillColor(Int_t ci)
{
   // Set the current fill color in the current context.

   CGContextRef ctx = (CGContextRef)GetCurrentContext();

   const TColor *color = gROOT->GetColor(ci);
   if (!color)
      return kFALSE;

   const CGFloat a = GetFillAlpha() / 100.;
   Float_t r = 0.f;
   Float_t g = 0.f;
   Float_t b = 0.f;

   color->GetRGB(r, g, b);
   CGContextSetRGBFillColor (ctx, r, g, b, a);
   
   return kTRUE;
}


//______________________________________________________________________________
Bool_t TGQuartz::SetContextStrokeColor(Int_t ci)
{
   // Set the current fill color in the current context.

   CGContextRef ctx = (CGContextRef)GetCurrentContext();

   const TColor *color = gROOT->GetColor(ci);
   if (!color)
      return kFALSE;

   const CGFloat a = 1.f;
   Float_t r = 0.f;
   Float_t g = 0.f;
   Float_t b = 0.f;

   color->GetRGB(r, g, b);
   CGContextSetRGBStrokeColor (ctx, r, g, b, a);
   
   return kTRUE;
}