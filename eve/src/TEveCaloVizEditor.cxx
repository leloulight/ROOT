// @(#)root/eve:$Id$
// Author: Matevz Tadel 2007

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TEveCaloVizEditor.h"
#include "TEveCalo.h"
#include "TEveGValuators.h"
#include "TEveRGBAPaletteEditor.h"
#include "TEveCaloData.h"

#include "TGClient.h"
#include "TGFont.h"
#include "TGedEditor.h"

#include "TGLabel.h"
#include "TGNumberEntry.h"
#include "TGDoubleSlider.h"
#include "TGNumberEntry.h"
#include "TG3DLine.h"
#include "TGButtonGroup.h"
#include "TColor.h"
#include "TGColorSelect.h"

#include "TMathBase.h"
#include "TMath.h"

//______________________________________________________________________________
// GUI editor for TEveCaloEditor.
//

ClassImp(TEveCaloVizEditor);

//______________________________________________________________________________
TEveCaloVizEditor::TEveCaloVizEditor(const TGWindow *p, Int_t width, Int_t height,
                                     UInt_t options, Pixel_t back) :
   TGedFrame(p, width, height, options | kVerticalFrame, back),
   fM(0),

   fPlotE(0),
   fPlotEt(0),

   fScaleAbs(0),
   fMaxValAbs(0),
   fMaxTowerH(0),

   fEtaRng(0),
   fPhi(0),
   fPhiOffset(0),
   fTowerFrame(0),
   fSliceFrame(0)
{
   // Constructor.

   MakeTitle("TEveCaloViz");

   // E/Et Plot
   {
      TGHorizontalFrame* group = new   TGHorizontalFrame(this);

      TGCompositeFrame *labfr = new TGHorizontalFrame(group, 28, 20, kFixedSize);
   
      TGFont *myfont = gClient->GetFont("-adobe-times-bold-r-*-*-12-*-*-*-*-*-iso8859-1");
      TGLabel* label = new TGLabel(labfr, "Plot:");
      label->SetTextFont(myfont);
      labfr->AddFrame(label, new TGLayoutHints(kLHintsLeft  | kLHintsBottom));
      group->AddFrame(labfr, new TGLayoutHints(kLHintsLeft));

      fPlotE = new TGRadioButton(group, new TGHotString("E"), 11);
      fPlotE->Connect("Clicked()", "TEveCaloVizEditor", this, "DoPlot()");
      group->AddFrame(fPlotE, new TGLayoutHints(kLHintsLeft  | kLHintsBottom, 2, 2, 0, 0));

      fPlotEt = new TGRadioButton(group, new TGHotString("Et"), 22);
      fPlotEt->Connect("Clicked()", "TEveCaloVizEditor", this, "DoPlot()");
      group->AddFrame(fPlotEt, new TGLayoutHints(kLHintsLeft  | kLHintsBottom,  2, 2, 0, 0));

      AddFrame(group, new TGLayoutHints(kLHintsTop, 4, 1, 1, 0));
   }
   // scaling
   TGHorizontalFrame* scf = new TGHorizontalFrame(this);

   TGLabel* label = new TGLabel(scf, "ScaleAbsolute:");
   scf->AddFrame(label, new TGLayoutHints(kLHintsLeft  | kLHintsBottom));
 
   fScaleAbs  = new TGCheckButton(scf);
   scf->AddFrame(fScaleAbs, new TGLayoutHints(kLHintsLeft, 3, 5, 3, 0));
   fScaleAbs->Connect("Toggled(Bool_t)", "TEveCaloVizEditor", this, "DoScaleAbs()");


   fMaxValAbs = new TEveGValuator(scf, "MaxEVal:", 70, 0);
   fMaxValAbs->SetLabelWidth(56);
   fMaxValAbs->SetNELength(5);
   fMaxValAbs->SetShowSlider(kFALSE);
   fMaxValAbs->Build();
   fMaxValAbs->SetLimits(0, 1000);
   fMaxValAbs->Connect("ValueSet(Double_t)", "TEveCaloVizEditor", this, "DoMaxValAbs()");
   scf->AddFrame(fMaxValAbs, new TGLayoutHints(kLHintsTop, 1, 1, 1, 1));

   AddFrame(scf, new TGLayoutHints(kLHintsTop, 4, 1, 1, 0));


   // tower height
   fMaxTowerH = new TEveGValuator(this, "MaxTowerH:", 96, 0);
   fMaxTowerH->SetLabelWidth(71);
   fMaxTowerH->SetNELength(5);
   fMaxTowerH->SetShowSlider(kFALSE);
   fMaxTowerH->Build();
   fMaxTowerH->SetLimits(0.1, 500, 501, TGNumberFormat::kNESRealOne);
   fMaxTowerH->Connect("ValueSet(Double_t)", "TEveCaloVizEditor", this, "DoMaxTowerH()");
   AddFrame(fMaxTowerH, new TGLayoutHints(kLHintsTop, 4, 1, 1, 1));

   
   //______________________________________________________________________________

   fTowerFrame = CreateEditorTabSubFrame("Data");
   Int_t  labelW = 45;

   // eta
   fEtaRng = new TEveGDoubleValuator(fTowerFrame,"Eta rng:", 40, 0);
   fEtaRng->SetNELength(6);
   fEtaRng->SetLabelWidth(labelW);
   fEtaRng->Build();
   fEtaRng->GetSlider()->SetWidth(195);
   fEtaRng->SetLimits(-5.5, 5.5, TGNumberFormat::kNESRealTwo);
   fEtaRng->Connect("ValueSet()", "TEveCaloVizEditor", this, "DoEtaRange()");
   fTowerFrame->AddFrame(fEtaRng, new TGLayoutHints(kLHintsTop, 1, 1, 4, 5));

   // phi
   fPhi = new TEveGValuator(fTowerFrame, "Phi:", 90, 0);
   fPhi->SetLabelWidth(labelW);
   fPhi->SetNELength(6);
   fPhi->Build();
   fPhi->SetLimits(-TMath::Pi(), TMath::Pi(), TGNumberFormat::kNESRealTwo);
   fPhi->Connect("ValueSet(Double_t)", "TEveCaloVizEditor", this, "DoPhi()");
   fTowerFrame->AddFrame(fPhi, new TGLayoutHints(kLHintsTop, 1, 1, 1, 1));

   fPhiOffset = new TEveGValuator(fTowerFrame, "PhiOff:", 90, 0);
   fPhiOffset->SetLabelWidth(labelW);
   fPhiOffset->SetNELength(6);
   fPhiOffset->Build();
   fPhiOffset->SetLimits(0, TMath::Pi(), TGNumberFormat::kNESRealTwo);
   fPhiOffset->Connect("ValueSet(Double_t)", "TEveCaloVizEditor", this, "DoPhi()");
   fTowerFrame->AddFrame(fPhiOffset, new TGLayoutHints(kLHintsTop, 1, 1, 1, 1));

   fSliceFrame = new TGVerticalFrame(fTowerFrame);
   fTowerFrame->AddFrame(fSliceFrame);
}

//______________________________________________________________________________
void TEveCaloVizEditor::MakeSliceInfo()
{

   // Create slice info gui.

   Int_t ns = fM->GetData()->GetNSlices();
   Int_t nf = fSliceFrame->GetList()->GetSize();

   if (ns > nf)
   {
      for (Int_t i=nf; i<ns; ++i)
      {
         TGHorizontalFrame* f = new TGHorizontalFrame(fSliceFrame);
  
         TEveGValuator* threshold = new TEveGValuator(f,"", 90, 0, i);
         threshold->SetLabelWidth(50);
         threshold->SetNELength(6);
         threshold->SetShowSlider(kFALSE);
         threshold->Build();
         threshold->SetLimits(0, 1000, TGNumberFormat::kNESRealTwo);
         threshold->Connect("ValueSet(Double_t)", "TEveCaloVizEditor", this, "DoSliceThreshold()");
         f->AddFrame(threshold, new TGLayoutHints(kLHintsTop, 1, 1, 1, 1));

         TGColorSelect* color = new TGColorSelect(f, 0, i);
         f->AddFrame(color, new TGLayoutHints(kLHintsLeft|kLHintsTop, 3, 1, 0, 1));
         color->Connect("ColorSelected(Pixel_t)", "TEveCaloVizEditor", this, "DoSliceColor(Pixel_t)");

         fSliceFrame->AddFrame(f, new TGLayoutHints(kLHintsTop, 1, 1, 1, 0));
      }
      nf = ns;
   }

   TIter frame_iterator(fSliceFrame->GetList());
   for (Int_t i=0; i<nf; ++i)
   {
      TGFrameElement    *el = (TGFrameElement*)    frame_iterator();
      TGHorizontalFrame *fr = (TGHorizontalFrame*) el->fFrame;
      if (i < ns)
      {
         TEveCaloData::SliceInfo_t &si = fM->GetData()->RefSliceInfo(i);

         TEveGValuator *threshold = (TEveGValuator*) ((TGFrameElement*) fr->GetList()->First())->fFrame;
         TGColorSelect *color     = (TGColorSelect*) ((TGFrameElement*) fr->GetList()->Last() )->fFrame;

         threshold->GetLabel()->SetText(si.fName);
         threshold->SetValue(si.fThreshold);
         color->SetColor(TColor::Number2Pixel(si.fColor), kFALSE);

         if (! fr->IsMapped()) {
            fr->MapSubwindows();
            fr->MapWindow();
         }
      }
      else
      {
         if (fr->IsMapped()) {
            fr->UnmapWindow();
         }
      }
   }
}

//______________________________________________________________________________
void TEveCaloVizEditor::SetModel(TObject* obj)
{
   // Set model object.

   fM = dynamic_cast<TEveCaloViz*>(obj);
   if (fM->GetPlotEt())
   {
      fPlotEt->SetState(kButtonDown, kFALSE);
      fPlotE->SetState(kButtonUp, kFALSE);
   }
   else
   {
      fPlotE->SetState(kButtonDown, kFALSE);
      fPlotEt->SetState(kButtonUp, kFALSE);
   }

   if (fM->fData)
   {
      TGCompositeFrame* p = GetGedEditor()->GetEditorTab("Data");
      if (p->GetList()->IsEmpty())
      {
         p->MapWindow();
         p->MapSubwindows();
      }

      fScaleAbs->SetState(fM->GetScaleAbs() ? kButtonDown : kButtonUp);
      fMaxValAbs->SetValue(fM->GetMaxValAbs());
      fMaxTowerH->SetValue(fM->GetMaxTowerH());

      Double_t min, max;
      fM->GetData()->GetEtaLimits(min, max);
      fEtaRng->SetLimits((Float_t)min, (Float_t)max);
      fEtaRng->SetValues(fM->fEtaMin, fM->fEtaMax);

      fM->GetData()->GetPhiLimits(min, max);
      fPhi->SetLimits(min, max, 101, TGNumberFormat::kNESRealTwo);
      fPhi->SetValue(fM->fPhi);
      fPhi->SetToolTip("Center angle in radians");

      fPhiOffset->SetLimits(0, TMath::Pi(), 101, TGNumberFormat::kNESRealTwo);
      fPhiOffset->SetValue(fM->fPhiOffset);
      fPhiOffset->SetToolTip("Phi range in radians");

      MakeSliceInfo();
   }
   else
   {

      fTowerFrame->UnmapWindow();
   }
}

//______________________________________________________________________________
void TEveCaloVizEditor::DoMaxTowerH()
{
   // Slot for setting max tower height.

   fM->SetMaxTowerH(fMaxTowerH->GetValue());
   Update();
}

//______________________________________________________________________________
void TEveCaloVizEditor::DoScaleAbs()
{
   // Slot for enabling/disabling absolute scale.

   fM->SetScaleAbs(fScaleAbs->IsOn());
   Update();
}

//___________________________________________________________________________
void TEveCaloVizEditor::DoMaxValAbs()
{
   // Slot for setting max E in for absolute scale.

   fM->SetMaxValAbs(fMaxValAbs->GetValue());
   Update();
}

//___________________________________________________________________________
void TEveCaloVizEditor::DoPlot()
{
   // Slot for setting E/Et plot.

   TGButton *btn = (TGButton *) gTQSender;
   Int_t id = btn->WidgetId();

   if (id == fPlotE->WidgetId()) 
      fPlotEt->SetState(kButtonUp);
   else
      fPlotE->SetState(kButtonUp);

   fM->SetPlotEt(fPlotEt->IsDown());
   Update();
}

//______________________________________________________________________________
void TEveCaloVizEditor::DoEtaRange()
{
   // Slot for setting eta range.

   fM->SetEta(fEtaRng->GetMin(), fEtaRng->GetMax());
   Update();
}

//______________________________________________________________________________
void TEveCaloVizEditor::DoPhi()
{
  // Slot for setting phi range.

   fM->SetPhiWithRng(fPhi->GetValue(), fPhiOffset->GetValue());
   Update();
}

//______________________________________________________________________________
void TEveCaloVizEditor::DoSliceThreshold()
{
   // Slot for SliceThreshold.

   TEveGValuator *st = (TEveGValuator *) gTQSender;
   fM->SetDataSliceThreshold(st->WidgetId(), st->GetValue());
   Update();
}

//______________________________________________________________________________
void TEveCaloVizEditor::DoSliceColor(Pixel_t pixel)
{
   // Slot for slice info Color.
   
   TGColorSelect *cs = (TGColorSelect *) gTQSender;
   fM->SetDataSliceColor(cs->WidgetId(), Color_t(TColor::GetColor(pixel)));
   Update();
}
