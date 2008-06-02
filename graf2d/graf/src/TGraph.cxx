// @(#)root/graf:$Id$
// Author: Rene Brun, Olivier Couet   12/12/94

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include <string.h>

#include "Riostream.h"
#include "TROOT.h"
#include "TEnv.h"
#include "TGraph.h"
#include "TGaxis.h"
#include "TH1.h"
#include "TF1.h"
#include "TStyle.h"
#include "TMath.h"
#include "TFrame.h"
#include "TVector.h"
#include "TVectorD.h"
#include "Foption.h"
#include "TRandom.h"
#include "TSpline.h"
#include "TPaveStats.h"
#include "TVirtualFitter.h"
#include "TVirtualPad.h"
#include "TVirtualGraphPainter.h"
#include "TBrowser.h"
#include "TClass.h"
#include "TSystem.h"
#include <stdlib.h>
#include <string>

Double_t *gxwork, *gywork, *gxworkl, *gyworkl;

extern void H1LeastSquareSeqnd(Int_t n, Double_t *a, Int_t idim, Int_t &ifail, Int_t k, Double_t *b);

ClassImp(TGraph)


//______________________________________________________________________________
/* Begin_Html
<center><h2>Graph class</h2></center>
A Graph is a graphics object made of two arrays X and Y with npoints each.
This class supports essentially two graph categories:
<ul>
<li> General case with non equidistant points </li>
<li> Special case with equidistant points </li>
</ul>
The various format options to draw a Graph are explained in
<tt>TGraphPainter::PaintGraph</tt>  and <tt>TGraph::PaintGrapHist</tt>
These two functions are derived from the HIGZ routines IGRAPH and IGHIST
and many modifications.
<p>
The picture below gives an example:
End_Html
Begin_Macro(source)
{
   TCanvas *c1 = new TCanvas("c1","A Simple Graph Example",200,10,700,500);
   Double_t x[100], y[100];
   Int_t n = 20;
   for (Int_t i=0;i<n;i++) {
     x[i] = i*0.1;
     y[i] = 10*sin(x[i]+0.2);
   }
   gr = new TGraph(n,x,y);
   gr->Draw("AC*");
   return c1;
}
End_Macro */


//______________________________________________________________________________
TGraph::TGraph(): TNamed(), TAttLine(), TAttFill(1,1001), TAttMarker()
{
   // Graph default constructor.

   fNpoints = -1;  //will be reset to 0 in CtorAllocate
///   fPainter = 0;
   CtorAllocate();
}


//______________________________________________________________________________
TGraph::TGraph(Int_t n)
       : TNamed("Graph","Graph"), TAttLine(), TAttFill(1,1001), TAttMarker()
{
   // Constructor with only the number of points set
   // the arrsys x and y will be set later

   fNpoints = n;
///   fPainter = 0;
   if (!CtorAllocate()) return;
   FillZero(0, fNpoints);
}


//______________________________________________________________________________
TGraph::TGraph(Int_t n, const Int_t *x, const Int_t *y)
       : TNamed("Graph","Graph"), TAttLine(), TAttFill(1,1001), TAttMarker()
{
   // Graph normal constructor with ints.

   if (!x || !y) {
      fNpoints = 0;
   } else {
      fNpoints = n;
   }
///   fPainter = 0;
   if (!CtorAllocate()) return;
   for (Int_t i=0;i<n;i++) {
      fX[i] = (Double_t)x[i];
      fY[i] = (Double_t)y[i];
   }
}


//______________________________________________________________________________
TGraph::TGraph(Int_t n, const Float_t *x, const Float_t *y)
       : TNamed("Graph","Graph"), TAttLine(), TAttFill(1,1001), TAttMarker()
{
   // Graph normal constructor with floats.

   if (!x || !y) {
      fNpoints = 0;
   } else {
      fNpoints = n;
   }
///   fPainter = 0;
   if (!CtorAllocate()) return;
   for (Int_t i=0;i<n;i++) {
      fX[i] = x[i];
      fY[i] = y[i];
   }
}


//______________________________________________________________________________
TGraph::TGraph(Int_t n, const Double_t *x, const Double_t *y)
       : TNamed("Graph","Graph"), TAttLine(), TAttFill(1,1001), TAttMarker()
{
   // Graph normal constructor with doubles.

   if (!x || !y) {
      fNpoints = 0;
   } else {
      fNpoints = n;
   }
///   fPainter = 0;
   if (!CtorAllocate()) return;
   n = fNpoints*sizeof(Double_t);
   memcpy(fX, x, n);
   memcpy(fY, y, n);
}


//______________________________________________________________________________
TGraph::TGraph(const TGraph &gr)
       : TNamed(gr), TAttLine(gr), TAttFill(gr), TAttMarker(gr)
{
   // Copy constructor for this graph

   fNpoints = gr.fNpoints;
   fMaxSize = gr.fMaxSize;
   if (gr.fFunctions) fFunctions = (TList*)gr.fFunctions->Clone();
   else fFunctions = new TList;
   fHistogram = 0;
   fMinimum = gr.fMinimum;
   fMaximum = gr.fMaximum;
   if (!fMaxSize) {
      fX = fY = 0;
      return;
   } else {
      fX = new Double_t[fMaxSize];
      fY = new Double_t[fMaxSize];
   }

   Int_t n = gr.GetN()*sizeof(Double_t);
   memcpy(fX, gr.fX, n);
   memcpy(fY, gr.fY, n);
}


//______________________________________________________________________________
TGraph& TGraph::operator=(const TGraph &gr)
{
   // Equal operator for this graph

   if(this!=&gr) {
      TNamed::operator=(gr);
      TAttLine::operator=(gr);
      TAttFill::operator=(gr);
      TAttMarker::operator=(gr);

      fNpoints = gr.fNpoints;
      fMaxSize = gr.fMaxSize;
      if (gr.fFunctions) fFunctions = (TList*)gr.fFunctions->Clone();
      else fFunctions = new TList;
      if (gr.fHistogram) fHistogram = new TH1F(*(gr.fHistogram));
      else fHistogram = 0;
      fMinimum = gr.fMinimum;
      fMaximum = gr.fMaximum;
      if (!fMaxSize) {
         fX = fY = 0;
         return *this;
      } else {
         fX = new Double_t[fMaxSize];
         fY = new Double_t[fMaxSize];
      }

      Int_t n = gr.GetN()*sizeof(Double_t);
      if (n>0) {
         memcpy(fX, gr.fX, n);
         memcpy(fY, gr.fY, n);
      }
   }
   return *this;
}


//______________________________________________________________________________
TGraph::TGraph(const TVectorF &vx, const TVectorF &vy)
       : TNamed("Graph","Graph"), TAttLine(), TAttFill(1,1001), TAttMarker()
{
   // Graph constructor with two vectors of floats in input
   // A graph is build with the X coordinates taken from vx and Y coord from vy
   // The number of points in the graph is the minimum of number of points
   // in vx and vy.

   fNpoints = TMath::Min(vx.GetNrows(), vy.GetNrows());
///   fPainter = 0;
   if (!CtorAllocate()) return;
   Int_t ivxlow  = vx.GetLwb();
   Int_t ivylow  = vy.GetLwb();
   for (Int_t i=0;i<fNpoints;i++) {
      fX[i]  = vx(i+ivxlow);
      fY[i]  = vy(i+ivylow);
   }
}


//______________________________________________________________________________
TGraph::TGraph(const TVectorD &vx, const TVectorD &vy)
       : TNamed("Graph","Graph"), TAttLine(), TAttFill(1,1001), TAttMarker()
{
   // Graph constructor with two vectors of doubles in input
   // A graph is build with the X coordinates taken from vx and Y coord from vy
   // The number of points in the graph is the minimum of number of points
   // in vx and vy.

   fNpoints = TMath::Min(vx.GetNrows(), vy.GetNrows());
///   fPainter = 0;
   if (!CtorAllocate()) return;
   Int_t ivxlow  = vx.GetLwb();
   Int_t ivylow  = vy.GetLwb();
   for (Int_t i=0;i<fNpoints;i++) {
      fX[i]  = vx(i+ivxlow);
      fY[i]  = vy(i+ivylow);
   }
}


//______________________________________________________________________________
TGraph::TGraph(const TH1 *h)
       : TNamed("Graph","Graph"), TAttLine(), TAttFill(1,1001), TAttMarker()
{
   // Graph constructor importing its parameters from the TH1 object passed as argument

   if (!h) {
      Error("TGraph", "Pointer to histogram is null");
      fNpoints = 0;
   }
///   fPainter = 0;
   if (h->GetDimension() != 1) {
      Error("TGraph", "Histogram must be 1-D; h %s is %d-D",h->GetName(),h->GetDimension());
      fNpoints = 0;
   } else {
      fNpoints = ((TH1*)h)->GetXaxis()->GetNbins();
   }

   if (!CtorAllocate()) return;

   TAxis *xaxis = ((TH1*)h)->GetXaxis();
   for (Int_t i=0;i<fNpoints;i++) {
      fX[i] = xaxis->GetBinCenter(i+1);
      fY[i] = h->GetBinContent(i+1);
   }
   h->TAttLine::Copy(*this);
   h->TAttFill::Copy(*this);
   h->TAttMarker::Copy(*this);

   SetName(h->GetName());
   SetTitle(h->GetTitle());
}


//______________________________________________________________________________
TGraph::TGraph(const TF1 *f, Option_t *option)
       : TNamed("Graph","Graph"), TAttLine(), TAttFill(1,1001), TAttMarker()
{
   // Graph constructor importing its parameters from the TF1 object passed as argument
   // if option =="" (default), a TGraph is created with points computed
   //                at the fNpx points of f.
   // if option =="d", a TGraph is created with points computed with the derivatives
   //                at the fNpx points of f.
   // if option =="i", a TGraph is created with points computed with the integral
   //                at the fNpx points of f.
   // if option =="I", a TGraph is created with points computed with the integral
   //                at the fNpx+1 points of f and the integral is normalized to 1.

   char coption = ' ';
   if (!f) {
      Error("TGraph", "Pointer to function is null");
      fNpoints = 0;
   } else {
      fNpoints   = f->GetNpx();
      if (option) coption = *option;
      if (coption == 'i' || coption == 'I') fNpoints++;
   }
///   fPainter = 0;
   if (!CtorAllocate()) return;

   Double_t xmin = f->GetXmin();
   Double_t xmax = f->GetXmax();
   Double_t dx   = (xmax-xmin)/fNpoints;
   Double_t integ = 0;
   Int_t i;
   for (i=0;i<fNpoints;i++) {
      if (coption == 'i' || coption == 'I') {
         fX[i] = xmin +i*dx;
         if (i == 0) fY[i] = 0;
         else        fY[i] = integ + ((TF1*)f)->Integral(fX[i]-dx,fX[i]);
         integ = fY[i];
      } else if (coption == 'd' || coption == 'D') {
         fX[i] = xmin + (i+0.5)*dx;
         fY[i] = ((TF1*)f)->Derivative(fX[i]);
      } else {
         fX[i] = xmin + (i+0.5)*dx;
         fY[i] = ((TF1*)f)->Eval(fX[i]);
      }
   }
   if (integ != 0 && coption == 'I') {
      for (i=1;i<fNpoints;i++) fY[i] /= integ;
   }

   f->TAttLine::Copy(*this);
   f->TAttFill::Copy(*this);
   f->TAttMarker::Copy(*this);

   SetName(f->GetName());
   SetTitle(f->GetTitle());
}


//______________________________________________________________________________
TGraph::TGraph(const char *filename, const char *format, Option_t *)
       : TNamed("Graph",filename), TAttLine(), TAttFill(1,1001), TAttMarker()
{
   // Graph constructor reading input from filename
   // filename is assumed to contain at least two columns of numbers
   // the string format is by default "%lg %lg"

   Double_t x,y;
   ifstream infile(filename);
   if(!infile.good()){
      MakeZombie();
      Error("TGraph", "Cannot open file: %s, TGraph is Zombie",filename);
      fNpoints = 0;
   } else {
      fNpoints = 100;  //initial number of points
   }
///   fPainter = 0;
   if (!CtorAllocate()) return;
   std::string line;
   Int_t np=0;
   while(std::getline(infile,line,'\n')){
      if(2 != sscanf(line.c_str(),format,&x,&y) ) {
         continue; // skip empty and ill-formed lines
      }
      SetPoint(np,x,y);
      np++;
   }
   Set(np);
}


//______________________________________________________________________________
TGraph::~TGraph()
{
   // Graph default destructor.

   delete [] fX;
   delete [] fY;
   if (fFunctions) {
      fFunctions->SetBit(kInvalidObject);
      //special logic to support the case where the same object is
      //added multiple times in fFunctions.
      //This case happens when the same object is added with different
      //drawing modes
      TObject *obj;
      while ((obj  = fFunctions->First())) {
         while(fFunctions->Remove(obj)) { }
         delete obj;
      }
      delete fFunctions;
      fFunctions = 0; //to avoid accessing a deleted object in RecursiveRemove
   }
   delete fHistogram;
}


//______________________________________________________________________________
Double_t** TGraph::AllocateArrays(Int_t Narrays, Int_t arraySize)
{
   // Allocate arrays.

   if (arraySize < 0) { arraySize = 0; }
   Double_t **newarrays = new Double_t*[Narrays];
   if (!arraySize) {
      for (Int_t i = 0; i < Narrays; ++i)
         newarrays[i] = 0;
   } else {
      for (Int_t i = 0; i < Narrays; ++i)
         newarrays[i] = new Double_t[arraySize];
   }
   fMaxSize = arraySize;
   return newarrays;
}


//______________________________________________________________________________
void TGraph::Apply(TF1 *f)
{
   // Apply function f to all the data points
   // f may be a 1-D function TF1 or 2-d function TF2
   // The Y values of the graph are replaced by the new values computed
   // using the function

   for (Int_t i=0;i<fNpoints;i++) {
      fY[i] = f->Eval(fX[i],fY[i]);
   }
}


//______________________________________________________________________________
void TGraph::Browse(TBrowser *b)
{
   // Browse

   TString opt = gEnv->GetValue("TGraph.BrowseOption","");
   if (opt.IsNull()) {
      opt = b ? b->GetDrawOption() : "alp";
   }
   Draw(opt.Data());
   gPad->Update();
}


//______________________________________________________________________________
Double_t TGraph::Chisquare(const TF1 *f1) const
{
   // Return the chisquare of this graph with respect to f1.
   // The chisquare is computed as the sum of the quantity below at each point:
   // Begin_Latex
   // #frac{(y-f1(x))^{2}}{ey^{2}+(#frac{1}{2}(exl+exh)f1'(x))^{2}}
   // End_latex
   // where x and y are the graph point coordinates and f1'(x) is the derivative of function f1(x).
   // This method to approximate the uncertainty in y because of the errors in x, is called
   // "effective variance" method.
   // In case of a pure TGraph, the denominator is 1.
   // In case of a TGraphErrors or TGraphAsymmErrors the errors are taken
   // into account.

   if (!f1) return 0;
   Double_t cu,eu,exh,exl,ey,eux,fu,fsum;
   Double_t x[1];
   Double_t chi2 = 0;
   TF1 *func = (TF1*)f1; //EvalPar is not const !
   for (Int_t i=0;i<fNpoints;i++) {
      func->InitArgs(x,0); //must be inside the loop because of TF1::Derivative calling InitArgs
      x[0] = fX[i];
      if (!func->IsInside(x)) continue;
      cu   = fY[i];
      TF1::RejectPoint(kFALSE);
      fu   = func->EvalPar(x);
      if (TF1::RejectedPoint()) continue;
      fsum = (cu-fu);
      //npfits++;
      exh = GetErrorXhigh(i);
      exl = GetErrorXlow(i);
      if (fsum < 0)
         ey = GetErrorYhigh(i);
      else
         ey = GetErrorYlow(i);
      if (exl < 0) exl = 0;
      if (exh < 0) exh = 0;
      if (ey < 0)  ey  = 0;
      if (exh > 0 || exl > 0) {
         //"Effective Variance" method introduced by Anna Kreshuk
         //a copy of the algorithm in GraphFitChisquare from TFitter
         eux = 0.5*(exl + exh)*func->Derivative(x[0]);
      } else
         eux = 0.;
      eu = ey*ey+eux*eux;
      if (eu <= 0) eu = 1;
      chi2 += fsum*fsum/eu;
   }
   return chi2;
}


//______________________________________________________________________________
Bool_t TGraph::CompareArg(const TGraph* gr, Int_t left, Int_t right)
{
   // Return kTRUE if point number "left"'s argument (angle with respect to positive
   // x-axis) is bigger than that of point number "right". Can be used by Sort.

   Double_t xl,yl,xr,yr;
   gr->GetPoint(left,xl,yl);
   gr->GetPoint(right,xr,yr);
   return (TMath::ATan2(yl, xl) > TMath::ATan2(yr, xr));
}


//______________________________________________________________________________
Bool_t TGraph::CompareX(const TGraph* gr, Int_t left, Int_t right)
{
   // Return kTRUE if fX[left] > fX[right]. Can be used by Sort.

   return gr->fX[left]>gr->fX[right];
}


//______________________________________________________________________________
Bool_t TGraph::CompareY(const TGraph* gr, Int_t left, Int_t right)
{
   // Return kTRUE if fY[left] > fY[right]. Can be used by Sort.

   return gr->fY[left]>gr->fY[right];
}


//______________________________________________________________________________
Bool_t TGraph::CompareRadius(const TGraph* gr, Int_t left, Int_t right)
{
   // Return kTRUE if point number "left"'s distance to origin is bigger than
   // that of point number "right". Can be used by Sort.

   return gr->fX[left]*gr->fX[left]+gr->fY[left]*gr->fY[left]
      >gr->fX[right]*gr->fX[right]+gr->fY[right]*gr->fY[right];
}


//______________________________________________________________________________
void TGraph::ComputeRange(Double_t &xmin, Double_t &ymin, Double_t &xmax, Double_t &ymax) const
{
   // Compute the x/y range of the points in this graph
   if (fNpoints <= 0) {
      xmin=xmax=ymin=ymax = 0;
      return;
   }
   xmin = xmax = fX[0];
   ymin = ymax = fY[0];
   for (Int_t i=1;i<fNpoints;i++) {
      if (fX[i] < xmin) xmin = fX[i];
      if (fX[i] > xmax) xmax = fX[i];
      if (fY[i] < ymin) ymin = fY[i];
      if (fY[i] > ymax) ymax = fY[i];
   }
}


//______________________________________________________________________________
void TGraph::CopyAndRelease(Double_t **newarrays, Int_t ibegin, Int_t iend,
                           Int_t obegin)
{
   // Copy points from fX and fY to arrays[0] and arrays[1]
   // or to fX and fY if arrays == 0 and ibegin != iend.
   // If newarrays is non null, replace fX, fY with pointers from newarrays[0,1].
   // Delete newarrays, old fX and fY

   CopyPoints(newarrays, ibegin, iend, obegin);
   if (newarrays) {
      delete[] fX;
      fX = newarrays[0];
      delete[] fY;
      fY = newarrays[1];
      delete[] newarrays;
   }
}


//______________________________________________________________________________
Bool_t TGraph::CopyPoints(Double_t **arrays, Int_t ibegin, Int_t iend,
                        Int_t obegin)
{
   // Copy points from fX and fY to arrays[0] and arrays[1]
   // or to fX and fY if arrays == 0 and ibegin != iend.

   if (ibegin < 0 || iend <= ibegin || obegin < 0) { // Error;
      return kFALSE;
   }
   if (!arrays && ibegin == obegin) { // No copying is needed
      return kFALSE;
   }
   Int_t n = (iend - ibegin)*sizeof(Double_t);
   if (arrays) {
      memmove(&arrays[0][obegin], &fX[ibegin], n);
      memmove(&arrays[1][obegin], &fY[ibegin], n);
   } else {
      memmove(&fX[obegin], &fX[ibegin], n);
      memmove(&fY[obegin], &fY[ibegin], n);
   }
   return kTRUE;
}


//______________________________________________________________________________
Bool_t TGraph::CtorAllocate()
{
   // In constructors set fNpoints than call this method.
   // Return kFALSE if the graph will contain no points.

   fHistogram = 0;
   fMaximum = -1111;
   fMinimum = -1111;
   SetBit(kClipFrame);
   fFunctions = 0;
   if (fNpoints >= 0) fFunctions = new TList;
   if (fNpoints <= 0) {
      fNpoints = 0;
      fMaxSize   = 0;
      fX         = 0;
      fY         = 0;
      return kFALSE;
   } else {
      fMaxSize   = fNpoints;
      fX = new Double_t[fMaxSize];
      fY = new Double_t[fMaxSize];
   }
   return kTRUE;
}


//______________________________________________________________________________
void TGraph::Draw(Option_t *option)
{
   // Draw this graph with its current attributes.
   //
   //   Options to draw a graph are described in TGraphPainter::PaintGraph

   TString opt = option;
   opt.ToLower();

   if (opt.Contains("same")) {
      opt.ReplaceAll("same","");
   }

   // in case of option *, set marker style to 3 (star) and replace
   // * option by option P.
   Ssiz_t pos;
   if ((pos = opt.Index("*")) != kNPOS) {
      SetMarkerStyle(3);
      opt.Replace(pos, 1, "p");
   }
   if (gPad) {
      if (!gPad->IsEditable()) gROOT->MakeDefCanvas();
      if (opt.Contains("a")) gPad->Clear();
   }
   AppendPad(opt);
}


//______________________________________________________________________________
Int_t TGraph::DistancetoPrimitive(Int_t px, Int_t py)
{
   // Compute distance from point px,py to a graph.
   //
   //  Compute the closest distance of approach from point px,py to this line.
   //  The distance is computed in pixels units.

   return TVirtualGraphPainter::GetPainter()->DistancetoPrimitiveHelper(this, px,py);
}


//______________________________________________________________________________
void TGraph::DrawGraph(Int_t n, const Int_t *x, const Int_t *y, Option_t *option)
{
   // Draw this graph with new attributes.

   TGraph *newgraph = new TGraph(n, x, y);
   TAttLine::Copy(*newgraph);
   TAttFill::Copy(*newgraph);
   TAttMarker::Copy(*newgraph);
   newgraph->SetBit(kCanDelete);
   newgraph->AppendPad(option);
}


//______________________________________________________________________________
void TGraph::DrawGraph(Int_t n, const Float_t *x, const Float_t *y, Option_t *option)
{
   // Draw this graph with new attributes.

   TGraph *newgraph = new TGraph(n, x, y);
   TAttLine::Copy(*newgraph);
   TAttFill::Copy(*newgraph);
   TAttMarker::Copy(*newgraph);
   newgraph->SetBit(kCanDelete);
   newgraph->AppendPad(option);
}


//______________________________________________________________________________
void TGraph::DrawGraph(Int_t n, const Double_t *x, const Double_t *y, Option_t *option)
{
   // Draw this graph with new attributes.

   const Double_t *xx = x;
   const Double_t *yy = y;
   if (!xx) xx = fX;
   if (!yy) yy = fY;
   TGraph *newgraph = new TGraph(n, xx, yy);
   TAttLine::Copy(*newgraph);
   TAttFill::Copy(*newgraph);
   TAttMarker::Copy(*newgraph);
   newgraph->SetBit(kCanDelete);
   newgraph->AppendPad(option);
}


//______________________________________________________________________________
void TGraph::DrawPanel()
{
   // Display a panel with all graph drawing options.

   printf("TGraph::DrawPanel: not yet implemented\n");
}


//______________________________________________________________________________
Double_t TGraph::Eval(Double_t x, TSpline *spline, Option_t *option) const
{
   // Interpolate points in this graph at x using a TSpline
   //  -if spline==0 and option="" a linear interpolation between the two points
   //   close to x is computed. If x is outside the graph range, a linear
   //   extrapolation is computed.
   //  -if spline==0 and option="S" a TSpline3 object is created using this graph
   //   and the interpolated value from the spline is returned.
   //   the internally created spline is deleted on return.
   //  -if spline is specified, it is used to return the interpolated value.

   if (!spline) {
      TString opt = option;
      opt.ToLower();
      if (opt.Contains("s")) {
         // spline interpolation creating a new spline
         TSpline3 *s = new TSpline3("",this);
         Double_t result = s->Eval(x);
         delete s;
         return result;
      }
      //linear interpolation
      //find point in graph immediatly below x
      //In case x is < fX[0] or > fX[fNpoints-1] return the extrapolated point
      Int_t low = TMath::BinarySearch(fNpoints,fX,x);
      Int_t up = low+1;
      if (low == fNpoints-1) {up=low; low = up-1;}
      if (low == -1) {low=0; up=1;}
      if (fX[low] == fX[up]) return fY[low];
      Double_t yn = x*(fY[low]-fY[up]) +fX[low]*fY[up] - fX[up]*fY[low];
      return yn/(fX[low]-fX[up]);
   } else {
      //spline interpolation using the input spline
      return spline->Eval(x);
   }
}


//______________________________________________________________________________
void TGraph::ExecuteEvent(Int_t event, Int_t px, Int_t py)
{
   // Execute action corresponding to one event.
   //
   //  This member function is called when a graph is clicked with the locator
   //
   //  If Left button clicked on one of the line end points, this point
   //     follows the cursor until button is released.
   //
   //  if Middle button clicked, the line is moved parallel to itself
   //     until the button is released.

   TVirtualGraphPainter::GetPainter()->ExecuteEventHelper(this, event, px, py);
}


//______________________________________________________________________________
void TGraph::Expand(Int_t newsize)
{
   // If array sizes <= newsize, expand storage to 2*newsize.

   Double_t **ps = ExpandAndCopy(newsize, fNpoints);
   CopyAndRelease(ps, 0, 0, 0);
}


//______________________________________________________________________________
void TGraph::Expand(Int_t newsize, Int_t step)
{
   // If graph capacity is less than newsize points then make array sizes
   // equal to least multiple of step to contain newsize points.
   // Returns kTRUE if size was altered

   if (newsize <= fMaxSize) {
      return;
   }
   Double_t **ps = Allocate(step*(newsize/step + (newsize%step?1:0)));
   CopyAndRelease(ps, 0, fNpoints, 0);
}


//______________________________________________________________________________
Double_t **TGraph::ExpandAndCopy(Int_t size, Int_t iend)
{
   // if size > fMaxSize allocate new arrays of 2*size points
   //  and copy oend first points.
   // Return pointer to new arrays.

   if (size <= fMaxSize) { return 0; }
   Double_t **newarrays = Allocate(2*size);
   CopyPoints(newarrays, 0, iend, 0);
   return newarrays;
}


//______________________________________________________________________________
void TGraph::FillZero(Int_t begin, Int_t end, Bool_t)
{
   // Set zero values for point arrays in the range [begin, end)
   // Should be redefined in descendant classes

   memset(fX + begin, 0, (end - begin)*sizeof(Double_t));
   memset(fY + begin, 0, (end - begin)*sizeof(Double_t));
}


//______________________________________________________________________________
TObject *TGraph::FindObject(const char *name) const
{
   // Search object named name in the list of functions

   if (fFunctions) return fFunctions->FindObject(name);
   return 0;
}


//______________________________________________________________________________
TObject *TGraph::FindObject(const TObject *obj) const
{
   // Search object obj in the list of functions

   if (fFunctions) return fFunctions->FindObject(obj);
   return 0;
}


//______________________________________________________________________________
Int_t TGraph::Fit(const char *fname, Option_t *option, Option_t *, Axis_t xmin, Axis_t xmax)
{
   // Fit this graph with function with name fname.
   //
   //  interface to TGraph::Fit(TF1 *f1...
   //
   //      fname is the name of an already predefined function created by TF1 or TF2
   //      Predefined functions such as gaus, expo and poln are automatically
   //      created by ROOT.
   //      fname can also be a formula, accepted by the linear fitter (linear parts divided
   //      by "++" sign), for example "x++sin(x)" for fitting "[0]*x+[1]*sin(x)"

   char *linear;
   linear= (char*) strstr(fname, "++");
   TF1 *f1=0;
   if (linear)
      f1=new TF1(fname, fname, xmin, xmax);
   else {
      f1 = (TF1*)gROOT->GetFunction(fname);
      if (!f1) { Printf("Unknown function: %s",fname); return -1; }
   }
   return Fit(f1,option,"",xmin,xmax);
}


//______________________________________________________________________________
Int_t TGraph::Fit(TF1 *f1, Option_t *option, Option_t *, Axis_t rxmin, Axis_t rxmax)
{
   // Fit this graph with function f1.
   //
   //   f1 is an already predefined function created by TF1.
   //   Predefined functions such as gaus, expo and poln are automatically
   //   created by ROOT.
   //
   //   The list of fit options is given in parameter option.
   //      option = "W" Set all weights to 1; ignore error bars
   //             = "U" Use a User specified fitting algorithm (via SetFCN)
   //             = "Q" Quiet mode (minimum printing)
   //             = "V" Verbose mode (default is between Q and V)
   //             = "B" Use this option when you want to fix one or more parameters
   //                   and the fitting function is like "gaus","expo","poln","landau".
   //             = "R" Use the Range specified in the function range
   //             = "N" Do not store the graphics function, do not draw
   //             = "0" Do not plot the result of the fit. By default the fitted function
   //                   is drawn unless the option"N" above is specified.
   //             = "+" Add this new fitted function to the list of fitted functions
   //                   (by default, any previous function is deleted)
   //             = "C" In case of linear fitting, not calculate the chisquare
   //                    (saves time)
   //             = "F" If fitting a polN, switch to minuit fitter
   //             = "ROB" In case of linear fitting, compute the LTS regression
   //                     coefficients (robust(resistant) regression), using
   //                     the default fraction of good points
   //               "ROB=0.x" - compute the LTS regression coefficients, using
   //                           0.x as a fraction of good points
   //
   //   When the fit is drawn (by default), the parameter goption may be used
   //   to specify a list of graphics options. See TGraph::Paint for a complete
   //   list of these options.
   //
   //   In order to use the Range option, one must first create a function
   //   with the expression to be fitted. For example, if your graph
   //   has a defined range between -4 and 4 and you want to fit a gaussian
   //   only in the interval 1 to 3, you can do:
   //        TF1 *f1 = new TF1("f1","gaus",1,3);
   //        graph->Fit("f1","R");
   //
   //
   // Who is calling this function:
   //
   //   Note that this function is called when calling TGraphErrors::Fit
   //   or TGraphAsymmErrors::Fit ot TGraphBentErrors::Fit
   //   see the discussion below on the errors calulation.
   //
   // Linear fitting:
   //
   //   When the fitting function is linear (contains the "++" sign) or the fitting
   //   function is a polynomial, a linear fitter is initialised.
   //   To create a linear function, use the following syntaxis: linear parts
   //   separated by "++" sign.
   //   Example: to fit the parameters of "[0]*x + [1]*sin(x)", create a
   //    TF1 *f1=new TF1("f1", "x++sin(x)", xmin, xmax);
   //   For such a TF1 you don't have to set the initial conditions
   //   Going via the linear fitter for functions, linear in parameters, gives a considerable
   //   advantage in speed.
   //
   // Setting initial conditions:
   //
   //   Parameters must be initialized before invoking the Fit function.
   //   The setting of the parameter initial values is automatic for the
   //   predefined functions : poln, expo, gaus, landau. One can however disable
   //   this automatic computation by specifying the option "B".
   //   You can specify boundary limits for some or all parameters via
   //        f1->SetParLimits(p_number, parmin, parmax);
   //   if parmin>=parmax, the parameter is fixed
   //   Note that you are not forced to fix the limits for all parameters.
   //   For example, if you fit a function with 6 parameters, you can do:
   //     func->SetParameters(0,3.1,1.e-6,0.1,-8,100);
   //     func->SetParLimits(4,-10,-4);
   //     func->SetParLimits(5, 1,1);
   //   With this setup, parameters 0->3 can vary freely
   //   Parameter 4 has boundaries [-10,-4] with initial value -8
   //   Parameter 5 is fixed to 100.
   //
   // Fit range:
   //
   //   The fit range can be specified in two ways:
   //     - specify rxmax > rxmin (default is rxmin=rxmax=0)
   //     - specify the option "R". In this case, the function will be taken
   //       instead of the full graph range.
   //
   // Changing the fitting function:
   //
   //   By default the fitting function GraphFitChisquare is used.
   //   To specify a User defined fitting function, specify option "U" and
   //   call the following functions:
   //     TVirtualFitter::Fitter(mygraph)->SetFCN(MyFittingFunction)
   //   where MyFittingFunction is of type:
   //   extern void MyFittingFunction(Int_t &npar, Double_t *gin, Double_t &f, Double_t *u, Int_t flag);
   //
   // How errors are used in the chisquare function (see TFitter GraphFitChisquare)//   Access to the fit results
   //
   //   In case of a TGraphErrors object, ex, the error along x,  is projected
   //   along the y-direction by calculating the function at the points x-exlow and
   //   x+exhigh.
   //
   //   The chisquare is computed as the sum of the quantity below at each point:
   //
   // Begin_Latex
   // #frac{(y-f(x))^{2}}{ey^{2}+(#frac{1}{2}(exl+exh)f'(x))^{2}}
   // End_Latex
   //
   //   where x and y are the point coordinates, and f'(x) is the derivative of function f(x).
   //
   //   In case the function lies below (above) the data point, ey is ey_low (ey_high).
   //
   //   thanks to Andy Haas (haas@yahoo.com) for adding the case with TGraphasymmerrors
   //             University of Washington
   //
   //   The approach used to approximate the uncertainty in y because of the
   //   errors in x, is to make it equal the error in x times the slope of the line.
   //   The improvement, compared to the first method (f(x+ exhigh) - f(x-exlow))/2
   //   is of (error of x)**2 order. This approach is called "effective variance method".
   //   This improvement has been made in version 4.00/08 by Anna Kreshuk.
   //
   // NOTE:
   //   1) By using the "effective variance" method a simple linear regression
   //      becomes a non-linear case, which takes several iterations
   //      instead of 0 as in the linear case .
   //
   //   2) The effective variance technique assumes that there is no correlation
   //      between the x and y coordinate .
   //
   //   Note, that the linear fitter doesn't take into account the errors in x. If errors
   //   in x are important, go through minuit (use option "F" for polynomial fitting).
   //
   //   3) When fitting a TGraph (ie no errors associated to each point),
   //   a correction is applied to the errors on the parameters with the following
   //   formula:
   //      errorp *= sqrt(chisquare/(ndf-1))
   //
   // Associated functions:
   //
   //   One or more object (typically a TF1*) can be added to the list
   //   of functions (fFunctions) associated to each graph.
   //   When TGraph::Fit is invoked, the fitted function is added to this list.
   //   Given a graph gr, one can retrieve an associated function
   //   with:  TF1 *myfunc = gr->GetFunction("myfunc");
   //
   //   If the graph is made persistent, the list of
   //   associated functions is also persistent. Given a pointer (see above)
   //   to an associated function myfunc, one can retrieve the function/fit
   //   parameters with calls such as:
   //     Double_t chi2 = myfunc->GetChisquare();
   //     Double_t par0 = myfunc->GetParameter(0); //value of 1st parameter
   //     Double_t err0 = myfunc->GetParError(0);  //error on first parameter
   //
   // Fit Statistics
   //
   //   You can change the statistics box to display the fit parameters with
   //   the TStyle::SetOptFit(mode) method. This mode has four digits.
   //   mode = pcev  (default = 0111)
   //     v = 1;  print name/values of parameters
   //     e = 1;  print errors (if e=1, v must be 1)
   //     c = 1;  print Chisquare/Number of degress of freedom
   //     p = 1;  print Probability
   //
   //   For example: gStyle->SetOptFit(1011);
   //   prints the fit probability, parameter names/values, and errors.
   //   You can change the position of the statistics box with these lines
   //   (where g is a pointer to the TGraph):
   //
   //   Root > TPaveStats *st = (TPaveStats*)g->GetListOfFunctions()->FindObject("stats")
   //   Root > st->SetX1NDC(newx1); //new x start position
   //   Root > st->SetX2NDC(newx2); //new x end position
   //
   //      Access to the fit status
   //      ========================
   //   The function return the status of the fit (fitResult) in the following form
   //     fitResult = migradResult + 10*minosResult + 100*hesseResult + 1000*improveResult
   //   The fitResult is 0 is the fit is OK.
   //   The fitResult is negative in case of an error not connected with the fit.

   Int_t fitResult = 0;
   Double_t xmin, xmax, ymin, ymax;
   Int_t i, npar,nvpar,nparx, ndf;
   Double_t par, we, al, bl;
   Double_t eplus,eminus,eparab,globcc,amin,edm,errdef,werr;
   TF1 *fnew1;

   // Check validity of function
   if (!f1) {
      Error("Fit", "function may not be null pointer");
      return -1;
   }
   if (f1->IsZombie()) {
      Error("Fit", "function is zombie");
      return -2;
   }

   npar = f1->GetNpar();
   if (npar <= 0) {
      Error("Fit", "function %s has illegal number of parameters = %d", f1->GetName(), npar);
      return -3;
   }

   // Check that function has same dimension as graph
   if (f1->GetNdim() > 1) {
      Error("Fit", "function %s is not 1-D", f1->GetName());
      return -4;
   }
   //}

   Double_t *arglist = new Double_t[100];

   // Decode string choptin and fill fitOption structure
   Foption_t fitOption;
   Double_t h=0;
   TString opt = option;
   opt.ToUpper();
   opt.ReplaceAll("ROB", "H");
   //for robust fitting, see if # of good points is defined
   if (opt.Contains("H=0.")) {
      int start = opt.Index("H=0.");
      int numpos = start + strlen("H=0.");
      int numlen = 0;
      int len = opt.Length();
      while( (numpos+numlen<len) && isdigit(opt[numpos+numlen]) ) numlen++;
      TString num = opt(numpos,numlen);
      opt.Remove(start+strlen("H"),strlen("=0.")+numlen);
      h = atof(num.Data());
      h*=TMath::Power(10, -numlen);
   }

   if (opt.Contains("U")) fitOption.User    = 1;
   if (opt.Contains("Q")) fitOption.Quiet   = 1;
   if (opt.Contains("V")){fitOption.Verbose = 1; fitOption.Quiet   = 0;}
   if (opt.Contains("W")) fitOption.W1      = 1;
   if (opt.Contains("E")) fitOption.Errors  = 1;
   if (opt.Contains("R")) fitOption.Range   = 1;
   if (opt.Contains("N")) fitOption.Nostore = 1;
   if (opt.Contains("0")) fitOption.Nograph = 1;
   if (opt.Contains("+")) fitOption.Plus    = 1;
   if (opt.Contains("B")) fitOption.Bound   = 1;
   if (opt.Contains("C")) fitOption.Nochisq = 1;
   if (opt.Contains("F")) fitOption.Minuit  = 1;
   if (opt.Contains("H")) fitOption.Robust  = 1;

   xmin    = fX[0];
   xmax    = fX[fNpoints-1];
   ymin    = fY[0];
   ymax    = fY[fNpoints-1];
   Double_t err0 = GetErrorX(0);
   Double_t errn = GetErrorX(fNpoints-1);
   if (err0 > 0) xmin -= 2*err0;
   if (errn > 0) xmax += 2*errn;
   for (i=0;i<fNpoints;i++) {
      if (fX[i] < xmin) xmin = fX[i];
      if (fX[i] > xmax) xmax = fX[i];
      if (fY[i] < ymin) ymin = fY[i];
      if (fY[i] > ymax) ymax = fY[i];
   }
   if (rxmax > rxmin) {
      xmin = rxmin;
      xmax = rxmax;
   }

   // Set the fitter

   Int_t special = f1->GetNumber();
   Bool_t linear = f1->IsLinear();
   if (special == 299 + npar)
      linear = kTRUE;
   if (fitOption.Bound || fitOption.User || fitOption.Errors || fitOption.Minuit)
      linear = kFALSE;

   const TString linearFitterClass = "TLinearFitter";
   Bool_t fitterSet = kFALSE;
   Bool_t linearSet = kFALSE;
   if (TVirtualFitter::GetFitter()) {
      // Is a fitter already set? Is it linear?
      fitterSet = kTRUE;
      linearSet = (TVirtualFitter::GetFitter()->IsA()->GetName() == linearFitterClass);
   }

   TVirtualFitter *origFitter = 0;
   if (linear) {
      //
      TClass *cl = TClass::GetClass(linearFitterClass);
      if (fitterSet && !linearSet) {
         origFitter = TVirtualFitter::GetFitter();
         fitterSet = kFALSE;
      }
      if (!fitterSet && cl) {
         TVirtualFitter::SetFitter((TVirtualFitter *)cl->New());
      }
   } else {
      if (fitterSet && linearSet) {
         origFitter = TVirtualFitter::GetFitter();
         fitterSet = kFALSE;
      }
      if (!fitterSet)
         TVirtualFitter::SetFitter(0);
   }
   TVirtualFitter *grFitter = TVirtualFitter::Fitter(this,f1->GetNpar());
   grFitter->Clear();

   // Get pointer to the function by searching in the list of functions in ROOT
   grFitter->SetUserFunc(f1);
   grFitter->SetFitOption(fitOption);

   // Is a Fit range specified?

   if (fitOption.Range) {
      f1->GetRange(xmin, xmax);
   } else {
      f1->SetRange(xmin, xmax);
   }

   //*-*- Compute sum of squares of errors in the bin range
   Bool_t hasErrors = kFALSE;
   Double_t ex, ey, sumw2=0;
   for (i=0;i<fNpoints;i++) {
      if(fX[i]<xmin || fX[i]>xmax) continue;
      ex = GetErrorX(i);
      ey = GetErrorY(i);
      if (ex > 0 || ey > 0) hasErrors = kTRUE;
      sumw2 += ey*ey;
   }

   // If case of a predefined function, then compute initial values of parameters

   if (linear) {
      if (fitOption.Robust)
         fitResult = grFitter->ExecuteCommand("FitGraph", &h, 0);
      else
         fitResult = grFitter->ExecuteCommand("FitGraph", 0, 0);
      amin = f1->GetChisquare();
      ndf  = f1->GetNDF();
   } else {

      //Int_t special = f1->GetNumber();
      if (fitOption.Bound) special = 0;
      if      (special == 100)      InitGaus(xmin,xmax);
      else if (special == 400)      InitGaus(xmin,xmax);
      else if (special == 200)      InitExpo(xmin,xmax);
      else if (special == 299+npar) InitPolynom(xmin,xmax);

      //*-*- Some initialisations
      if (!fitOption.Verbose) {
         arglist[0] = -1;
         grFitter->ExecuteCommand("SET PRINT", arglist,1);
         arglist[0] = 0;
         grFitter->ExecuteCommand("SET NOW",   arglist,0);
      }
      /////////////////////////////////////////////////////////
      // Set error criterion for chisquare
      arglist[0] = TVirtualFitter::GetErrorDef();
      if (!fitOption.User) grFitter->SetFitMethod("GraphFitChisquare");
      fitResult = grFitter->ExecuteCommand("SET err",arglist,1);
      if (fitResult != 0) {
         //   Abnormal termination, MIGRAD might not have converged on a
         //   minimum.
         if (!fitOption.Quiet) {
            Warning("Fit","Abnormal termination of minimization.");
         }
         delete [] arglist;
         return fitResult;
      }

      // Transfer names and initial values of parameters to Minuit
      Int_t nfixed = 0;
      for (i=0;i<npar;i++) {
         par = f1->GetParameter(i);
         f1->GetParLimits(i,al,bl);
         if (al*bl != 0 && al >= bl) {
            al = bl = 0;
            arglist[nfixed] = i+1;
            nfixed++;
         }
         we  = 0.3*TMath::Abs(par);
         if (we <= TMath::Abs(par)*1e-6) we = 1;
         grFitter->SetParameter(i,f1->GetParName(i),par,we,al,bl);
      }
      if (nfixed > 0)grFitter->ExecuteCommand("FIX",arglist,nfixed); // Otto

      // Reset Print level
      if (!fitOption.Quiet) {
         if (fitOption.Verbose) { arglist[0] = 2; grFitter->ExecuteCommand("SET PRINT", arglist,1); }
         else                   { arglist[0] = 0; grFitter->ExecuteCommand("SET PRINT", arglist,1); }
      }

      // Perform minimization

      arglist[0] = TVirtualFitter::GetMaxIterations();
      arglist[1] = sumw2*TVirtualFitter::GetPrecision();
      fitResult += grFitter->ExecuteCommand("MIGRAD",arglist,2);
      if (fitOption.Errors) {
         fitResult += 100*grFitter->ExecuteCommand("HESSE",arglist,0);
         fitResult +=  10*grFitter->ExecuteCommand("MINOS",arglist,0);

      }

      grFitter->GetStats(amin,edm,errdef,nvpar,nparx);
      f1->SetChisquare(amin);
      ndf = f1->GetNumberFitPoints()-npar+nfixed;
      f1->SetNDF(ndf);

      //////////////////////////////////////

      //*-*- Get return status
      char parName[50];
      for (i=0;i<npar;i++) {
         grFitter->GetParameter(i,parName, par,we,al,bl);
         if (!fitOption.Errors) werr = we;
         else {
            grFitter->GetErrors(i,eplus,eminus,eparab,globcc);
            if (eplus > 0 && eminus < 0) werr = 0.5*(eplus-eminus);
            else                         werr = we;
         }
         f1->SetParameter(i,par);
         f1->SetParError(i,werr);
      }
   }

   //In case the graph has no errors (eg TGraph) we can compute
   //a better parameter error estimation using the chisquare and ndf
   for (i=0;i<npar;i++) {
      if (!hasErrors && ndf > 1) {
         werr  = f1->GetParError(i);
         werr *= TMath::Sqrt(amin/(ndf-1));
         f1->SetParError(i,werr);
      }
   }

   // Print final values of parameters.
   if (!fitOption.Quiet) {
      if (fitOption.Errors) grFitter->PrintResults(4,amin);
      else                  grFitter->PrintResults(3,amin);
   }
   delete [] arglist;


   // Store fitted function in histogram functions list and draw
   if (!fitOption.Nostore) {
      if (!fFunctions) fFunctions = new TList;
      if (!fitOption.Plus) {
         TIter next(fFunctions, kIterBackward);
         TObject *obj;
         while ((obj = next())) {
            if (obj->InheritsFrom(TF1::Class())) delete obj;
         }
      }
      fnew1 = new TF1();
      f1->Copy(*fnew1);
      fFunctions->Add(fnew1);
      fnew1->SetParent(this);
      fnew1->Save(xmin,xmax,0,0,0,0);
      if (fitOption.Nograph) fnew1->SetBit(TF1::kNotDraw);
      fnew1->SetBit(TFormula::kNotGlobal);

      if (TestBit(kCanDelete)) return fitResult;
      if (gPad) gPad->Modified();
   }

   // Restore the original fitter
   if (origFitter) {
      delete TVirtualFitter::GetFitter();
      TVirtualFitter::SetFitter(origFitter);
   }

   return fitResult;
}


//______________________________________________________________________________
void TGraph::FitPanel()
{
   // Display a GUI panel with all graph fit options.
   //
   //   See class TFitEditor for example

   if (!gPad) {
      Error("FitPanel", "need to draw graph first");
      return;
   }

   if (!TClass::GetClass("TFitEditor")) gSystem->Load("libFitPanel");
   gROOT->ProcessLine(Form("TFitEditor::Open((TVirtualPad*)0x%lx,(TObject*)0x%lx)",gPad,this));
}


//______________________________________________________________________________
Double_t TGraph::GetCorrelationFactor() const
{
   // Return graph correlation factor

   Double_t rms1 = GetRMS(1);
   if (rms1 == 0) return 0;
   Double_t rms2 = GetRMS(2);
   if (rms2 == 0) return 0;
   return GetCovariance()/rms1/rms2;
}


//______________________________________________________________________________
Double_t TGraph::GetCovariance() const
{
   // Return covariance of vectors x,y

   if (fNpoints <= 0) return 0;
   Double_t sum = fNpoints, sumx = 0, sumy = 0, sumxy = 0;

   for (Int_t i=0;i<fNpoints;i++) {
      sumx  += fX[i];
      sumy  += fY[i];
      sumxy += fX[i]*fY[i];
   }
   return sumxy/sum - sumx/sum*sumy/sum;
}


//______________________________________________________________________________
Double_t TGraph::GetMean(Int_t axis) const
{
   // Return mean value of X (axis=1)  or Y (axis=2)

   if (axis < 1 || axis > 2) return 0;
   if (fNpoints <= 0) return 0;
   Double_t sumx = 0;
   for (Int_t i=0;i<fNpoints;i++) {
      if (axis == 1) sumx += fX[i];
      else           sumx += fY[i];
   }
   return sumx/fNpoints;
}


//______________________________________________________________________________
Double_t TGraph::GetRMS(Int_t axis) const
{
   // Return RMS of X (axis=1)  or Y (axis=2)

   if (axis < 1 || axis > 2) return 0;
   if (fNpoints <= 0) return 0;
   Double_t sumx = 0, sumx2 = 0;
   for (Int_t i=0;i<fNpoints;i++) {
      if (axis == 1) {sumx += fX[i]; sumx2 += fX[i]*fX[i];}
      else           {sumx += fY[i]; sumx2 += fY[i]*fY[i];}
   }
   Double_t x = sumx/fNpoints;
   Double_t rms2 = TMath::Abs(sumx2/fNpoints -x*x);
   return TMath::Sqrt(rms2);
}


//______________________________________________________________________________
Double_t TGraph::GetErrorX(Int_t) const
{
   // This function is called by GraphFitChisquare.
   // It always returns a negative value. Real implementation in TGraphErrors

   return -1;
}


//______________________________________________________________________________
Double_t TGraph::GetErrorY(Int_t) const
{
   // This function is called by GraphFitChisquare.
   // It always returns a negative value. Real implementation in TGraphErrors

   return -1;
}


//______________________________________________________________________________
Double_t TGraph::GetErrorXhigh(Int_t) const
{
   // This function is called by GraphFitChisquare.
   // It always returns a negative value. Real implementation in TGraphErrors
   // and TGraphAsymmErrors

   return -1;
}


//______________________________________________________________________________
Double_t TGraph::GetErrorXlow(Int_t) const
{
   // This function is called by GraphFitChisquare.
   // It always returns a negative value. Real implementation in TGraphErrors
   // and TGraphAsymmErrors

   return -1;
}


//______________________________________________________________________________
Double_t TGraph::GetErrorYhigh(Int_t) const
{
   // This function is called by GraphFitChisquare.
   // It always returns a negative value. Real implementation in TGraphErrors
   // and TGraphAsymmErrors

   return -1;
}


//______________________________________________________________________________
Double_t TGraph::GetErrorYlow(Int_t) const
{
   // This function is called by GraphFitChisquare.
   // It always returns a negative value. Real implementation in TGraphErrors
   // and TGraphAsymmErrors

   return -1;
}


//______________________________________________________________________________
TF1 *TGraph::GetFunction(const char *name) const
{
   // Return pointer to function with name.
   //
   // Functions such as TGraph::Fit store the fitted function in the list of
   // functions of this graph.

   if (!fFunctions) return 0;
   return (TF1*)fFunctions->FindObject(name);
}


//______________________________________________________________________________
TH1F *TGraph::GetHistogram() const
{
   // Returns a pointer to the histogram used to draw the axis
   // Takes into account the two following cases.
   //    1- option 'A' was specified in TGraph::Draw. Return fHistogram
   //    2- user had called TPad::DrawFrame. return pointer to hframe histogram

   if (fHistogram) return fHistogram;
   Double_t rwxmin,rwxmax, rwymin, rwymax, maximum, minimum, dx, dy;
   Double_t uxmin, uxmax;

   ComputeRange(rwxmin, rwymin, rwxmax, rwymax);  //this is redefined in TGraphErrors

   if (rwxmin == rwxmax) rwxmax += 1.;
   if (rwymin == rwymax) rwymax += 1.;
   dx = 0.1*(rwxmax-rwxmin);
   dy = 0.1*(rwymax-rwymin);
   uxmin    = rwxmin - dx;
   uxmax    = rwxmax + dx;
   minimum  = rwymin - dy;
   maximum  = rwymax + dy;
   if (fMinimum != -1111) minimum = fMinimum;
   if (fMaximum != -1111) maximum = fMaximum;

   // the graph is created with at least as many channels as there are points
   // to permit zooming on the full range
   if (uxmin < 0 && rwxmin >= 0) {
      if (gPad && gPad->GetLogx()) uxmin = 0.9*rwxmin;
      else                 uxmin = 0;
   }
   if (uxmax > 0 && rwxmax <= 0) {
      if (gPad && gPad->GetLogx()) uxmax = 1.1*rwxmax;
      else                 uxmax = 0;
   }
   if (minimum < 0 && rwymin >= 0) {
      if(gPad && gPad->GetLogy()) minimum = 0.9*rwymin;
      else                minimum = 0;
   }
   if (minimum <= 0 && gPad && gPad->GetLogy()) minimum = 0.001*maximum;
   if (uxmin <= 0 && gPad && gPad->GetLogx()) {
      if (uxmax > 1000) uxmin = 1;
      else              uxmin = 0.001*uxmax;
   }

   rwxmin = uxmin;
   rwxmax = uxmax;
   Int_t npt = 100;
   if (fNpoints > npt) npt = fNpoints;
   const char *gname = GetName();
   if (strlen(gname) == 0) gname = "Graph";
   ((TGraph*)this)->fHistogram = new TH1F(gname,GetTitle(),npt,rwxmin,rwxmax);
   if (!fHistogram) return 0;
   fHistogram->SetMinimum(minimum);
   fHistogram->SetBit(TH1::kNoStats);
   fHistogram->SetMaximum(maximum);
   fHistogram->GetYaxis()->SetLimits(minimum,maximum);
   fHistogram->SetDirectory(0);
   return fHistogram;
}


//______________________________________________________________________________
void TGraph::GetPoint(Int_t i, Double_t &x, Double_t &y) const
{
   // Get x and y values for point number i.

   if (i < 0 || i >= fNpoints) return;
   if (!fX || !fY) return;
   x = fX[i];
   y = fY[i];
}


//______________________________________________________________________________
TAxis *TGraph::GetXaxis() const
{
   // Get x axis of the graph.

   //if (!gPad) return 0;
   TH1 *h = GetHistogram();
   if (!h) return 0;
   return h->GetXaxis();
}


//______________________________________________________________________________
TAxis *TGraph::GetYaxis() const
{
   // Get y axis of the graph.

   //if (!gPad) return 0;
   TH1 *h = GetHistogram();
   if (!h) return 0;
   return h->GetYaxis();
}


//______________________________________________________________________________
void TGraph::InitGaus(Double_t xmin, Double_t xmax)
{
   // Compute Initial values of parameters for a gaussian.

   Double_t allcha, sumx, sumx2, x, val, rms, mean;
   Int_t bin;
   const Double_t sqrtpi = 2.506628;

   // Compute mean value and RMS of the graph in the given range
   if (xmax <= xmin) {xmin = fX[0]; xmax = fX[fNpoints-1];}
   Int_t np = 0;
   allcha = sumx = sumx2 = 0;
   for (bin=0;bin<fNpoints;bin++) {
      x       = fX[bin];
      if (x < xmin || x > xmax) continue;
      np++;
      val     = fY[bin];
      sumx   += val*x;
      sumx2  += val*x*x;
      allcha += val;
   }
   if (np == 0 || allcha == 0) return;
   mean = sumx/allcha;
   rms  = TMath::Sqrt(sumx2/allcha - mean*mean);
   Double_t binwidx = TMath::Abs((xmax-xmin)/np);
   if (rms == 0) rms = 1;
   TVirtualFitter *grFitter = TVirtualFitter::GetFitter();
   TF1 *f1 = (TF1*)grFitter->GetUserFunc();
   f1->SetParameter(0,binwidx*allcha/(sqrtpi*rms));
   f1->SetParameter(1,mean);
   f1->SetParameter(2,rms);
   f1->SetParLimits(2,0,10*rms);
}


//______________________________________________________________________________
void TGraph::InitExpo(Double_t xmin, Double_t xmax)
{
   // Compute Initial values of parameters for an exponential.

   Double_t constant, slope;
   Int_t ifail;
   if (xmax <= xmin) {xmin = fX[0]; xmax = fX[fNpoints-1];}
   Int_t nchanx = fNpoints;

   LeastSquareLinearFit(-nchanx, constant, slope, ifail, xmin, xmax);

   TVirtualFitter *grFitter = TVirtualFitter::GetFitter();
   TF1 *f1 = (TF1*)grFitter->GetUserFunc();
   f1->SetParameter(0,constant);
   f1->SetParameter(1,slope);
}


//______________________________________________________________________________
void TGraph::InitPolynom(Double_t xmin, Double_t xmax)
{
   // Compute Initial values of parameters for a polynom.

   Double_t fitpar[25];

   TVirtualFitter *grFitter = TVirtualFitter::GetFitter();
   TF1 *f1 = (TF1*)grFitter->GetUserFunc();
   Int_t npar   = f1->GetNpar();
   if (xmax <= xmin) {xmin = fX[0]; xmax = fX[fNpoints-1];}

   LeastSquareFit(npar, fitpar, xmin, xmax);

   for (Int_t i=0;i<npar;i++) f1->SetParameter(i, fitpar[i]);
}


//______________________________________________________________________________
Int_t TGraph::InsertPoint()
{
   // Insert a new point at the mouse position

   Int_t px = gPad->GetEventX();
   Int_t py = gPad->GetEventY();

   //localize point where to insert
   Int_t ipoint = -2;
   Int_t i,d=0;
   // start with a small window (in case the mouse is very close to one point)
   for (i=0;i<fNpoints-1;i++) {
      d = DistancetoLine(px, py, gPad->XtoPad(fX[i]), gPad->YtoPad(fY[i]), gPad->XtoPad(fX[i+1]), gPad->YtoPad(fY[i+1]));
      if (d < 5) {ipoint = i+1; break;}
   }
   if (ipoint == -2) {
      //may be we are far from one point, try again with a larger window
      for (i=0;i<fNpoints-1;i++) {
         d = DistancetoLine(px, py, gPad->XtoPad(fX[i]), gPad->YtoPad(fY[i]), gPad->XtoPad(fX[i+1]), gPad->YtoPad(fY[i+1]));
         if (d < 10) {ipoint = i+1; break;}
      }
   }
   if (ipoint == -2) {
      //distinguish between first and last point
      Int_t dpx = px - gPad->XtoAbsPixel(gPad->XtoPad(fX[0]));
      Int_t dpy = py - gPad->YtoAbsPixel(gPad->XtoPad(fY[0]));
      if (dpx*dpx+dpy*dpy < 25) ipoint = 0;
      else                      ipoint = fNpoints;
   }
   Double_t **ps = ExpandAndCopy(fNpoints + 1, ipoint);
   CopyAndRelease(ps, ipoint, fNpoints++, ipoint + 1);

   // To avoid redefenitions in descendant classes
   FillZero(ipoint, ipoint + 1);

   fX[ipoint] = gPad->PadtoX(gPad->AbsPixeltoX(px));
   fY[ipoint] = gPad->PadtoY(gPad->AbsPixeltoY(py));
   gPad->Modified();
   return ipoint;
}


//______________________________________________________________________________
void TGraph::LeastSquareFit(Int_t m, Double_t *a, Double_t xmin, Double_t xmax)
{
   // Least squares lpolynomial fitting without weights.
   //
   //  m     number of parameters
   //  a     array of parameters
   //  first 1st point number to fit (default =0)
   //  last  last point number to fit (default=fNpoints-1)
   //
   //   based on CERNLIB routine LSQ: Translated to C++ by Rene Brun

   const Double_t zero = 0.;
   const Double_t one = 1.;
   const Int_t idim = 20;

   Double_t  b[400]        /* was [20][20] */;
   Int_t i, k, l, ifail;
   Double_t power;
   Double_t da[20], xk, yk;
   Int_t n = fNpoints;
   if (xmax <= xmin) {xmin = fX[0]; xmax = fX[fNpoints-1];}

   if (m <= 2) {
      LeastSquareLinearFit(n, a[0], a[1], ifail, xmin, xmax);
      return;
   }
   if (m > idim || m > n) return;
   da[0] = zero;
   for (l = 2; l <= m; ++l) {
      b[l-1]           = zero;
      b[m + l*20 - 21] = zero;
      da[l-1]          = zero;
   }
   Int_t np = 0;
   for (k = 0; k < fNpoints; ++k) {
      xk     = fX[k];
      if (xk < xmin || xk > xmax) continue;
      np++;
      yk     = fY[k];
      power  = one;
      da[0] += yk;
      for (l = 2; l <= m; ++l) {
         power   *= xk;
         b[l-1]  += power;
         da[l-1] += power*yk;
      }
      for (l = 2; l <= m; ++l) {
         power            *= xk;
         b[m + l*20 - 21] += power;
      }
   }
   b[0]  = Double_t(np);
   for (i = 3; i <= m; ++i) {
      for (k = i; k <= m; ++k) {
         b[k - 1 + (i-1)*20 - 21] = b[k + (i-2)*20 - 21];
      }
   }
   H1LeastSquareSeqnd(m, b, idim, ifail, 1, da);

   if (ifail < 0) {
      a[0] = fY[0];
      for (i=1; i<m; ++i) a[i] = 0;
      return;
   }
   for (i=0; i<m; ++i) a[i] = da[i];
}


//______________________________________________________________________________
void TGraph::LeastSquareLinearFit(Int_t ndata, Double_t &a0, Double_t &a1, Int_t &ifail, Double_t xmin, Double_t xmax)
{
   // Least square linear fit without weights.
   //
   //  Fit a straight line (a0 + a1*x) to the data in this graph.
   //  ndata:  if ndata<0, fits the logarithm of the graph (used in InitExpo() to set
   //          the initial parameter values for a fit with exponential function.
   //  a0:     constant
   //  a1:     slope
   //  ifail:  return parameter indicating the status of the fit (ifail=0, fit is OK)
   //  xmin, xmax: fitting range
   //
   //  extracted from CERNLIB LLSQ: Translated to C++ by Rene Brun

   Double_t xbar, ybar, x2bar;
   Int_t i;
   Double_t xybar;
   Double_t fn, xk, yk;
   Double_t det;
   if (xmax <= xmin) {xmin = fX[0]; xmax = fX[fNpoints-1];}

   ifail = -2;
   xbar  = ybar = x2bar = xybar = 0;
   Int_t np = 0;
   for (i = 0; i < fNpoints; ++i) {
      xk = fX[i];
      if (xk < xmin || xk > xmax) continue;
      np++;
      yk = fY[i];
      if (ndata < 0) {
         if (yk <= 0) yk = 1e-9;
         yk = TMath::Log(yk);
      }
      xbar  += xk;
      ybar  += yk;
      x2bar += xk*xk;
      xybar += xk*yk;
   }
   fn    = Double_t(np);
   det   = fn*x2bar - xbar*xbar;
   ifail = -1;
   if (det <= 0) {
      if (fn > 0) a0 = ybar/fn;
      else        a0 = 0;
      a1 = 0;
      return;
   }
   ifail = 0;
   a0 = (x2bar*ybar - xbar*xybar) / det;
   a1 = (fn*xybar - xbar*ybar) / det;
}


//______________________________________________________________________________
void TGraph::Paint(Option_t *option)
{
   // Draw this graph with its current attributes.

   TVirtualGraphPainter::GetPainter()->PaintHelper(this, option);
}


//______________________________________________________________________________
void TGraph::PaintGraph(Int_t npoints, const Double_t *x, const Double_t *y, Option_t *chopt)
{
   // Draw the (x,y) as a graph.

   TVirtualGraphPainter::GetPainter()->PaintGraph(this, npoints, x, y, chopt);
}


//______________________________________________________________________________
void TGraph::PaintGrapHist(Int_t npoints, const Double_t *x, const Double_t *y, Option_t *chopt)
{
   // Draw the (x,y) as a histogram.

   TVirtualGraphPainter::GetPainter()->PaintGrapHist(this, npoints, x, y, chopt);
}


//______________________________________________________________________________
void TGraph::PaintStats(TF1 *fit)
{
   // Draw the stats

   TVirtualGraphPainter::GetPainter()->PaintStats(this, fit);
}


//______________________________________________________________________________
void TGraph::Print(Option_t *) const
{
   // Print graph values.

   for (Int_t i=0;i<fNpoints;i++) {
      printf("x[%d]=%g, y[%d]=%g\n",i,fX[i],i,fY[i]);
   }
}


//______________________________________________________________________________
void TGraph::RecursiveRemove(TObject *obj)
{
   // Recursively remove object from the list of functions

   if (fFunctions) {
      if (!fFunctions->TestBit(kInvalidObject)) fFunctions->RecursiveRemove(obj);
   }
}


//______________________________________________________________________________
Int_t TGraph::RemovePoint()
{
   // Delete point close to the mouse position

   Int_t px = gPad->GetEventX();
   Int_t py = gPad->GetEventY();

   //localize point to be deleted
   Int_t ipoint = -2;
   Int_t i;
   // start with a small window (in case the mouse is very close to one point)
   for (i=0;i<fNpoints;i++) {
      Int_t dpx = px - gPad->XtoAbsPixel(gPad->XtoPad(fX[i]));
      Int_t dpy = py - gPad->YtoAbsPixel(gPad->YtoPad(fY[i]));
      if (dpx*dpx+dpy*dpy < 25) {ipoint = i; break;}
   }
   return RemovePoint(ipoint);
}


//______________________________________________________________________________
Int_t TGraph::RemovePoint(Int_t ipoint)
{
   // Delete point number ipoint

   if (ipoint < 0) return -1;
   if (ipoint >= fNpoints) return -1;

   Double_t **ps = ShrinkAndCopy(fNpoints - 1, ipoint);
   CopyAndRelease(ps, ipoint+1, fNpoints--, ipoint);
   if (gPad) gPad->Modified();
   return ipoint;
}


//______________________________________________________________________________
void TGraph::SavePrimitive(ostream &out, Option_t *option /*= ""*/)
{
    // Save primitive as a C++ statement(s) on output stream out

   char quote = '"';
   out<<"   "<<endl;
   if (gROOT->ClassSaved(TGraph::Class())) {
      out<<"   ";
   } else {
      out<<"   TGraph *";
   }
   out<<"graph = new TGraph("<<fNpoints<<");"<<endl;
   out<<"   graph->SetName("<<quote<<GetName()<<quote<<");"<<endl;
   out<<"   graph->SetTitle("<<quote<<GetTitle()<<quote<<");"<<endl;

   SaveFillAttributes(out,"graph",0,1001);
   SaveLineAttributes(out,"graph",1,1,1);
   SaveMarkerAttributes(out,"graph",1,1,1);

   for (Int_t i=0;i<fNpoints;i++) {
      out<<"   graph->SetPoint("<<i<<","<<fX[i]<<","<<fY[i]<<");"<<endl;
   }

   static Int_t frameNumber = 0;
   if (fHistogram) {
      frameNumber++;
      TString hname = fHistogram->GetName();
      hname += frameNumber;
      fHistogram->SetName(hname.Data());
      fHistogram->SavePrimitive(out,"nodraw");
      out<<"   graph->SetHistogram("<<fHistogram->GetName()<<");"<<endl;
      out<<"   "<<endl;
   }

   // save list of functions
   TIter next(fFunctions);
   TObject *obj;
   while ((obj=next())) {
      obj->SavePrimitive(out,"nodraw");
      if (obj->InheritsFrom("TPaveStats")) {
         out<<"   graph->GetListOfFunctions()->Add(ptstats);"<<endl;
         out<<"   ptstats->SetParent(graph->GetListOfFunctions());"<<endl;
      } else {
         out<<"   graph->GetListOfFunctions()->Add("<<obj->GetName()<<");"<<endl;
      }
   }

   const char *l = strstr(option,"multigraph");
   if (l) {
      out<<"   multigraph->Add(graph,"<<quote<<l+10<<quote<<");"<<endl;
   } else {
      out<<"   graph->Draw("<<quote<<option<<quote<<");"<<endl;
   }
}


//______________________________________________________________________________
void TGraph::Set(Int_t n)
{
   // Set number of points in the graph
   // Existing coordinates are preserved
   // New coordinates above fNpoints are preset to 0.

   if (n < 0) n = 0;
   if (n == fNpoints) return;
   Double_t **ps = Allocate(n);
   CopyAndRelease(ps, 0, TMath::Min(fNpoints,n), 0);
   if (n > fNpoints) {
      FillZero(fNpoints, n, kFALSE);
   }
   fNpoints = n;
}


//______________________________________________________________________________
Bool_t TGraph::GetEditable() const
{
   // Return kTRUE if kNotEditable bit is not set, kFALSE otherwise.

   return TestBit(kNotEditable) ? kFALSE : kTRUE;
}


//______________________________________________________________________________
void TGraph::SetEditable(Bool_t editable)
{
   // if editable=kFALSE, the graph cannot be modified with the mouse
   //  by default a TGraph is editable

   if (editable) ResetBit(kNotEditable);
   else          SetBit(kNotEditable);
}


//______________________________________________________________________________
void TGraph::SetMaximum(Double_t maximum)
{
   // Set the maximum of the graph.

   fMaximum = maximum;
   GetHistogram()->SetMaximum(maximum);
}


//______________________________________________________________________________
void TGraph::SetMinimum(Double_t minimum)
{
   // Set the minimum of the graph.

   fMinimum = minimum;
   GetHistogram()->SetMinimum(minimum);
}


//______________________________________________________________________________
void TGraph::SetPoint(Int_t i, Double_t x, Double_t y)
{
   // Set x and y values for point number i.

   if (i < 0) return;
   if (fHistogram) {
      delete fHistogram;
      fHistogram = 0;
   }
   if (i >= fMaxSize) {
      Double_t **ps = ExpandAndCopy(i+1, fNpoints);
      CopyAndRelease(ps, 0,0,0);
   }
   if (i >= fNpoints) {
      // points above i can be not initialized
      // set zero up to i-th point to avoid redefenition
      // of this method in descendant classes
      FillZero(fNpoints, i + 1);
      fNpoints = i+1;
   }
   fX[i] = x;
   fY[i] = y;
   if (gPad) gPad->Modified();
}


//______________________________________________________________________________
void TGraph::SetTitle(const char* title)
{
   // Set graph title.

   fTitle = title;
   if (fHistogram) fHistogram->SetTitle(title);
}


//______________________________________________________________________________
Double_t **TGraph::ShrinkAndCopy(Int_t size, Int_t oend)
{
   // if size*2 <= fMaxSize allocate new arrays of size points,
   // copy points [0,oend).
   // Return newarray (passed or new instance if it was zero
   // and allocations are needed)
   if (size*2 > fMaxSize || !fMaxSize) {
      return 0;
   }
   Double_t **newarrays = Allocate(size);
   CopyPoints(newarrays, 0, oend, 0);
   return newarrays;
}


//______________________________________________________________________________
void TGraph::Sort(Bool_t (*greaterfunc)(const TGraph*, Int_t, Int_t) /*=TGraph::CompareX()*/,
                  Bool_t ascending /*=kTRUE*/, Int_t low /* =0 */, Int_t high /* =-1111 */)
{
   // Sorts the points of this TGraph using in-place quicksort (see e.g. older glibc).
   // To compare two points the function parameter greaterfunc is used (see TGraph::CompareX for an
   // example of such a method, which is also the default comparison function for Sort). After
   // the sort, greaterfunc(this, i, j) will return kTRUE for all i>j if ascending == kTRUE, and
   // kFALSE otherwise.
   //
   // The last two parameters are used for the recursive quick sort, stating the range to be sorted
   //
   // Examples:
   //   // sort points along x axis
   //   graph->Sort();
   //   // sort points along their distance to origin
   //   graph->Sort(&TGraph::CompareRadius);
   //
   //   Bool_t CompareErrors(const TGraph* gr, Int_t i, Int_t j) {
   //     const TGraphErrors* ge=(const TGraphErrors*)gr;
   //     return (ge->GetEY()[i]>ge->GetEY()[j]); }
   //   // sort using the above comparison function, largest errors first
   //   graph->Sort(&CompareErrors, kFALSE);

   if (high == -1111) high = GetN()-1;
   //  Termination condition
   if (high <= low) return;

   int left, right;
   left = low; // low is the pivot element
   right = high;
   while (left < right) {
      // move left while item < pivot
      while(left <= high && greaterfunc(this, left, low) != ascending)
         left++;
      // move right while item > pivot
      while(right > low && greaterfunc(this, right, low) == ascending)
         right--;
      if (left < right && left < high && right > low)
         SwapPoints(left, right);
   }
   // right is final position for the pivot
   if (right > low)
      SwapPoints(low, right);
   Sort( greaterfunc, ascending, low, right-1 );
   Sort( greaterfunc, ascending, right+1, high );
}


//______________________________________________________________________________
void TGraph::Streamer(TBuffer &b)
{
   // Stream an object of class TGraph.

   if (b.IsReading()) {
      UInt_t R__s, R__c;
      Version_t R__v = b.ReadVersion(&R__s, &R__c);
      if (R__v > 2) {
         b.ReadClassBuffer(TGraph::Class(), this, R__v, R__s, R__c);
         if (fHistogram) fHistogram->SetDirectory(0);
         TIter next(fFunctions);
         TObject *obj;
         while ((obj = next())) {
            if (obj->InheritsFrom(TF1::Class())) {
               TF1 *f1 = (TF1*)obj;
               f1->SetParent(this);
            }
         }
         fMaxSize = fNpoints;
         return;
      }
      //====process old versions before automatic schema evolution
      TNamed::Streamer(b);
      TAttLine::Streamer(b);
      TAttFill::Streamer(b);
      TAttMarker::Streamer(b);
      b >> fNpoints;
      fMaxSize = fNpoints;
      fX = new Double_t[fNpoints];
      fY = new Double_t[fNpoints];
      if (R__v < 2) {
         Float_t *x = new Float_t[fNpoints];
         Float_t *y = new Float_t[fNpoints];
         b.ReadFastArray(x,fNpoints);
         b.ReadFastArray(y,fNpoints);
         for (Int_t i=0;i<fNpoints;i++) {
            fX[i] = x[i];
            fY[i] = y[i];
         }
         delete [] y;
         delete [] x;
      } else {
         b.ReadFastArray(fX,fNpoints);
         b.ReadFastArray(fY,fNpoints);
      }
      b >> fFunctions;
      b >> fHistogram;
      if (fHistogram) fHistogram->SetDirectory(0);
      if (R__v < 2) {
         Float_t mi,ma;
         b >> mi;
         b >> ma;
         fMinimum = mi;
         fMaximum = ma;
      } else {
         b >> fMinimum;
         b >> fMaximum;
      }
      b.CheckByteCount(R__s, R__c, TGraph::IsA());
      //====end of old versions

   } else {
      b.WriteClassBuffer(TGraph::Class(),this);
   }
}


//______________________________________________________________________________
void TGraph::SwapPoints(Int_t pos1, Int_t pos2)
{
   // Swap points.

   SwapValues(fX, pos1, pos2);
   SwapValues(fY, pos1, pos2);
}


//______________________________________________________________________________
void TGraph::SwapValues(Double_t* arr, Int_t pos1, Int_t pos2)
{
   // Swap values.

   Double_t tmp=arr[pos1];
   arr[pos1]=arr[pos2];
   arr[pos2]=tmp;
}


//______________________________________________________________________________
void TGraph::UseCurrentStyle()
{
   // Set current style settings in this graph
   // This function is called when either TCanvas::UseCurrentStyle
   // or TROOT::ForceStyle have been invoked.

   if (gStyle->IsReading()) {
      SetFillColor(gStyle->GetHistFillColor());
      SetFillStyle(gStyle->GetHistFillStyle());
      SetLineColor(gStyle->GetHistLineColor());
      SetLineStyle(gStyle->GetHistLineStyle());
      SetLineWidth(gStyle->GetHistLineWidth());
      SetMarkerColor(gStyle->GetMarkerColor());
      SetMarkerStyle(gStyle->GetMarkerStyle());
      SetMarkerSize(gStyle->GetMarkerSize());
   } else {
      gStyle->SetHistFillColor(GetFillColor());
      gStyle->SetHistFillStyle(GetFillStyle());
      gStyle->SetHistLineColor(GetLineColor());
      gStyle->SetHistLineStyle(GetLineStyle());
      gStyle->SetHistLineWidth(GetLineWidth());
      gStyle->SetMarkerColor(GetMarkerColor());
      gStyle->SetMarkerStyle(GetMarkerStyle());
      gStyle->SetMarkerSize(GetMarkerSize());
   }
   if (fHistogram) fHistogram->UseCurrentStyle();

   TIter next(GetListOfFunctions());
   TObject *obj;

   while ((obj = next())) {
      obj->UseCurrentStyle();
   }
}


//______________________________________________________________________________
Int_t TGraph::Merge(TCollection* li)
{
   // Adds all graphs from the collection to this graph.
   // Returns the total number of poins in the result or -1 in case of an error.

   TIter next(li);
   while (TObject* o = next()) {
      TGraph *g = dynamic_cast<TGraph*> (o);
      if (!g) {
         Error("Merge",
             "Cannot merge - an object which doesn't inherit from TGraph found in the list");
         return -1;
      }
      Double_t x, y;
      for (Int_t i = 0 ; i < g->GetN(); i++) {
         g->GetPoint(i, x, y);
         SetPoint(GetN(), x, y);
      }
   }
   return GetN();
}
