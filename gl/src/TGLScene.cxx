// @(#)root/gl:$Name:  $:$Id: TGLScene.cxx,v 1.21 2005/10/11 10:25:11 brun Exp $
// Author:  Richard Maunder  25/05/2005
// Parts taken from original TGLRender by Timur Pocheptsov

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

// TODO: Function descriptions
// TODO: Class def - same as header

#include "TGLScene.h"
#include "TGLCamera.h"
#include "TGLLogicalShape.h"
#include "TGLPhysicalShape.h"
#include "TGLStopwatch.h"
#include "TGLDisplayListCache.h"
#include "TGLClip.h"
#include "TGLIncludes.h"
#include "TError.h"
#include "TString.h"
#include "Riostream.h"
#include "TClass.h" // For non-TObject reflection

#include <algorithm>

ClassImp(TGLScene)

//______________________________________________________________________________
TGLScene::TGLScene() :
   fLock(kUnlocked), fDrawList(1000), 
   fDrawListValid(kFALSE), fBoundingBox(), fBoundingBoxValid(kFALSE), 
   fLastDrawLOD(kHigh), fSelectedPhysical(0)
{
}

//______________________________________________________________________________
TGLScene::~TGLScene()
{
   TakeLock(kModifyLock);
   DestroyPhysicals(kTRUE); // including modified
   DestroyLogicals();
   ReleaseLock(kModifyLock);

   // Purge out the DL cache - when per drawable done no longer required
   TGLDisplayListCache::Instance().Purge();
}

//TODO: Inline
//______________________________________________________________________________
void TGLScene::AdoptLogical(TGLLogicalShape & shape)
{
   if (fLock != kModifyLock) {
      Error("TGLScene::AdoptLogical", "expected ModifyLock");
      return;
   }

   // TODO: Very inefficient check - disable
   assert(fLogicalShapes.find(shape.ID()) == fLogicalShapes.end());
   fLogicalShapes.insert(LogicalShapeMapValueType_t(shape.ID(), &shape));
}

//______________________________________________________________________________
Bool_t TGLScene::DestroyLogical(ULong_t ID)
{
   if (fLock != kModifyLock) {
      Error("TGLScene::DestroyLogical", "expected ModifyLock");
      return kFALSE;
   }

   LogicalShapeMapIt_t logicalIt = fLogicalShapes.find(ID);
   if (logicalIt != fLogicalShapes.end()) {
      const TGLLogicalShape * logical = logicalIt->second;
      if (logical->Ref() == 0) {
         fLogicalShapes.erase(logicalIt);
         delete logical;
         return kTRUE;
      } else {
         assert(kFALSE);
      }
   }

   return kFALSE;
}

//______________________________________________________________________________
UInt_t TGLScene::DestroyLogicals()
{
   UInt_t count = 0;
   if (fLock != kModifyLock) {
      Error("TGLScene::DestroyLogicals", "expected ModifyLock");
      return count;
   }

   LogicalShapeMapIt_t logicalShapeIt = fLogicalShapes.begin();
   const TGLLogicalShape * logicalShape;
   while (logicalShapeIt != fLogicalShapes.end()) {
      logicalShape = logicalShapeIt->second;
      if (logicalShape) {
         if (logicalShape->Ref() == 0) {
            fLogicalShapes.erase(logicalShapeIt++);
            delete logicalShape;
            ++count;
            continue;
         } else {
            assert(kFALSE);
         }
      } else {
         assert(kFALSE);
      }
      ++logicalShapeIt;
   }

   return count;
}

//TODO: Inline
//______________________________________________________________________________
TGLLogicalShape * TGLScene::FindLogical(ULong_t ID) const
{
   LogicalShapeMapCIt_t it = fLogicalShapes.find(ID);
   if (it != fLogicalShapes.end()) {
      return it->second;
   } else {
      return 0;
   }
}

//TODO: Inline
//______________________________________________________________________________
void TGLScene::AdoptPhysical(TGLPhysicalShape & shape)
{
   if (fLock != kModifyLock) {
      Error("TGLScene::AdoptPhysical", "expected ModifyLock");
      return;
   }
   // TODO: Very inefficient check - disable
   assert(fPhysicalShapes.find(shape.ID()) == fPhysicalShapes.end());

   fPhysicalShapes.insert(PhysicalShapeMapValueType_t(shape.ID(), &shape));
   fBoundingBoxValid = kFALSE;

   // Add into draw list and mark for sorting
   fDrawList.push_back(&shape);
   fDrawListValid = kFALSE;
}

//______________________________________________________________________________
Bool_t TGLScene::DestroyPhysical(ULong_t ID)
{
   if (fLock != kModifyLock) {
      Error("TGLScene::DestroyPhysical", "expected ModifyLock");
      return kFALSE;
   }
   PhysicalShapeMapIt_t physicalIt = fPhysicalShapes.find(ID);
   if (physicalIt != fPhysicalShapes.end()) {
      TGLPhysicalShape * physical = physicalIt->second;
      if (fSelectedPhysical == physical) {
         fSelectedPhysical = 0;
      }
      fPhysicalShapes.erase(physicalIt);
      fBoundingBoxValid = kFALSE;
  
      // Zero the draw list entry - will be erased as part of sorting
      DrawListIt_t drawIt = find(fDrawList.begin(), fDrawList.end(), physical);
      if (drawIt != fDrawList.end()) {
         *drawIt = 0;
         fDrawListValid = kFALSE;
      } else {
         assert(kFALSE);
      }
      delete physical;
      return kTRUE;
   }

   return kFALSE;
}

//______________________________________________________________________________
UInt_t TGLScene::DestroyPhysicals(Bool_t incModified, const TGLCamera * camera)
{
   if (fLock != kModifyLock) {
      Error("TGLScene::DestroyPhysicals", "expected ModifyLock");
      return kFALSE;
   }
   UInt_t count = 0;
   PhysicalShapeMapIt_t physicalShapeIt = fPhysicalShapes.begin();
   const TGLPhysicalShape * physical;
   while (physicalShapeIt != fPhysicalShapes.end()) {
      physical = physicalShapeIt->second;
      if (physical) {
         // Destroy any physical shape no longer of interest to camera
         // If modified options allow this physical to be destoyed
         if (incModified || (!incModified && !physical->IsModified())) {
            // and no camera is passed, or it is no longer of interest
            // to camera
            if (!camera || (camera && !camera->OfInterest(physical->BoundingBox()))) {

               // Then we can destroy it - remove from map
               fPhysicalShapes.erase(physicalShapeIt++);

               // Zero the draw list entry - will be erased as part of sorting
               DrawListIt_t drawIt = find(fDrawList.begin(), fDrawList.end(), physical);
               if (drawIt != fDrawList.end()) {
                  *drawIt = 0;
               } else {
                  assert(kFALSE);
               }

               // Ensure if selected object this is cleared
               if (fSelectedPhysical == physical) {
                  fSelectedPhysical = 0;
               }
               // Finally destroy actual object
               delete physical;
               ++count;
               continue; // Incremented the iterator during erase()
            }
         }
      } else {
         assert(kFALSE);
      }
      ++physicalShapeIt;
   }

   if (count > 0) {
      fBoundingBoxValid = kFALSE;
      fDrawListValid = kFALSE;
   }

   return count;
}

//TODO: Inline
//______________________________________________________________________________
TGLPhysicalShape * TGLScene::FindPhysical(ULong_t ID) const
{
   PhysicalShapeMapCIt_t it = fPhysicalShapes.find(ID);
   if (it != fPhysicalShapes.end()) {
      return it->second;
   } else {
      return 0;
   }
}

//______________________________________________________________________________
//TODO: Merge axes flag and LOD into general draw flag
void TGLScene::Draw(const TGLCamera & camera, EDrawStyle style, UInt_t LOD, 
                    Double_t timeout, const TGLClip * clip)
{
   if (fLock != kDrawLock && fLock != kSelectLock) {
      Error("TGLScene::Draw", "expected Draw or Select Lock");
   }

   // Reset debug draw stats
   ResetDrawStats();

   // Sort the draw list if required
   if (!fDrawListValid) {
      SortDrawList();
   }

   // Setup GL for current draw style - fill, wireframe, outline
   // Any GL modifications need to be defered until drawing time - 
   // to ensure we are in correct thread/context under Windows
   // TODO: Could detect change and only mod if changed for speed
   switch (style) {
      case (kFill): {
         glEnable(GL_LIGHTING);
         glEnable(GL_CULL_FACE);
         glPolygonMode(GL_FRONT, GL_FILL);
         glClearColor(0.0, 0.0, 0.0, 1.0); // Black
         break;
      }
      case (kWireFrame): {
         glDisable(GL_CULL_FACE);
         glDisable(GL_LIGHTING);
         glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
         glClearColor(0.0, 0.0, 0.0, 1.0); // Black
         break;
      }
      case (kOutline): {
         glEnable(GL_LIGHTING);
         glEnable(GL_CULL_FACE);
         glPolygonMode(GL_FRONT, GL_FILL);
         glClearColor(1.0, 1.0, 1.0, 1.0); // White
         break;
      }
      default: {
         assert(kFALSE);
      }
   }

   // If no clip object
   if (!clip) {
      DrawPass(camera, style, LOD, timeout);
   } else {
      // Get the clip plane set from the clipping object
      std::vector<TGLPlane> planeSet;
      clip->PlaneSet(planeSet);

      // Strip any planes that outside the scene bounding box - no effect
      for (std::vector<TGLPlane>::iterator it = planeSet.begin();
           it != planeSet.end(); ) {
         if (BoundingBox().Overlap(*it) == kOutside) {
            it = planeSet.erase(it);
         } else {
            ++it;
         }
      }

      if (gDebug>2) {
         Info("TGLScene::Draw()", "%d active clip planes", planeSet.size());
      }
      // Limit to smaller of plane set size or GL implementation plane support
      Int_t maxGLPlanes;
      glGetIntegerv(GL_MAX_CLIP_PLANES, &maxGLPlanes);
      UInt_t maxPlanes = maxGLPlanes;
      UInt_t planeInd;
      if (planeSet.size() < maxPlanes) {
         maxPlanes = planeSet.size();
      }

      // Note : OpenGL Reference (Blue Book) states
      // GL_CLIP_PLANEi = CL_CLIP_PLANE0 + i

      // Clip away scene outside of the clip object
      if (clip->Mode() == TGLClip::kOutside) {
         // Load all negated clip planes (up to max) at once
         for (UInt_t i=0; i<maxPlanes; i++) {
            planeSet[i].Negate();
            glClipPlane(GL_CLIP_PLANE0+i, planeSet[i].CArr());
            glEnable(GL_CLIP_PLANE0+i);
         }

          // Draw scene once with full time slot, passing all the planes
          DrawPass(camera, style, LOD, timeout, &planeSet);
      }
      // Clip away scene inside of the clip object
      else {
         std::vector<TGLPlane> activePlanes;
         for (planeInd=0; planeInd<maxPlanes; planeInd++) {
            if (planeInd > 0) {
               activePlanes[planeInd - 1].Negate();
               glClipPlane(GL_CLIP_PLANE0+planeInd - 1, activePlanes[planeInd - 1].CArr());
            }
            activePlanes.push_back(planeSet[planeInd]);
            glClipPlane(GL_CLIP_PLANE0+planeInd, activePlanes[planeInd].CArr());
            glEnable(GL_CLIP_PLANE0+planeInd);
            DrawPass(camera, style, LOD, timeout/maxPlanes, &activePlanes);
         }
      }
      // Ensure all clip planes turned off again
      for (planeInd=0; planeInd<maxPlanes; planeInd++) {
         glDisable(GL_CLIP_PLANE0+planeInd);
      }
   }

   // Reset style related modes set above to defaults
   glEnable(GL_LIGHTING);
   glEnable(GL_CULL_FACE);
   glPolygonMode(GL_FRONT, GL_FILL);

   // Record this so that any Select() draw can be redone at same quality and ensure
   // accuracy of picking
   fLastDrawLOD = LOD;

   // TODO: Should record if full scene can be drawn at 100% in a target time (set on scene)
   // Then timeout should not be passed - just bool if termination is permitted - which may
   // be ignored if all can be done. Pass back bool to indicate if whole scene could be drawn
   // at desired quality. Need a fixed target time so valid across multiple draws

   // Dump debug draw stats
   DumpDrawStats();

   return;
}

//______________________________________________________________________________
void TGLScene::DrawPass(const TGLCamera & camera, EDrawStyle style, UInt_t LOD, 
                        Double_t timeout, const std::vector<TGLPlane> * clipPlanes)
{
   // Set stopwatch running
   TGLStopwatch stopwatch;
   stopwatch.Start();

   // Setup draw style function pointer
   void (TGLPhysicalShape::*drawPtr)(UInt_t)const = &TGLPhysicalShape::Draw;
   if (style == kWireFrame) {
      drawPtr = &TGLPhysicalShape::DrawWireFrame;
   } else if (style == kOutline) {
      drawPtr = &TGLPhysicalShape::DrawOutline;
   }

   // Step 1: Loop through the main sorted draw list 
   Bool_t                   run = kTRUE;
   const TGLPhysicalShape * drawShape;
   Bool_t                   doSelected = (fSelectedPhysical != 0);

   // Transparent list built on fly
   static DrawList_t transDrawList;
   transDrawList.reserve(fDrawList.size() / 10); // assume less 10% of total
   transDrawList.clear();

   // Opaque only objects drawn in first loop
   // TODO: Sort front -> back for better performance
   glDepthMask(GL_TRUE);
   // If the scene bounding box is inside the camera frustum then
   // no need to check individual shapes - everything is visible
   Bool_t useFrustumCheck = (camera.FrustumOverlap(BoundingBox()) != kInside);

   glDisable(GL_BLEND);

   DrawListIt_t drawIt;
   for (drawIt = fDrawList.begin(); drawIt != fDrawList.end() && run;
        drawIt++) {
      drawShape = *drawIt;
      if (!drawShape)
      {
         assert(kFALSE);
         continue;
      }

      // Selected physical should always be drawn once only if visible, 
      // regardless of timeout object drop outs
      if (drawShape == fSelectedPhysical) {
         doSelected = kFALSE;
      }

      // TODO: Do small skipping first? Probably cheaper than frustum check
      // Profile relative costs? The frustum check could be done implictly 
      // from the LOD as we project all 8 verticies of the BB onto viewport

      // Work out if we need to draw this shape - assume we do first
      Bool_t drawNeeded = kTRUE;
      EOverlap overlap;

      // Draw test against passed clipping planes
      // Do before camera clipping on assumption clip planes remove more objects
      if (clipPlanes) {
         for (UInt_t i = 0; i < clipPlanes->size(); i++) {
            overlap = drawShape->BoundingBox().Overlap((*clipPlanes)[i]);
            if (overlap == kOutside) {
               drawNeeded = kFALSE;
               break;
            }
         }
      }

      // Draw test against camera frustum if require
      if (drawNeeded && useFrustumCheck)
      {
         overlap = camera.FrustumOverlap(drawShape->BoundingBox());
         drawNeeded = overlap == kInside || overlap == kPartial;
      }

      // Draw?
      if (drawNeeded)
      {
         // Collect transparent shapes and draw after opaque
         if (drawShape->IsTransparent()) {
            transDrawList.push_back(drawShape);
            continue;
         }

         // Get the shape draw quality
         UInt_t shapeLOD = CalcPhysicalLOD(*drawShape, camera, LOD);

         //Draw, DrawWireFrame, DrawOutline
         (drawShape->*drawPtr)(shapeLOD);
         UpdateDrawStats(*drawShape);
      }

      // Terminate the draw if over opaque fraction timeout
      if (timeout > 0.0) {
         Double_t opaqueTimeFraction = 1.0;
         if (fDrawStats.fOpaque > 0) {
            opaqueTimeFraction = (transDrawList.size() + fDrawStats.fOpaque) / fDrawStats.fOpaque; 
         }
         if (stopwatch.Lap() * opaqueTimeFraction > timeout) {
            run = kFALSE;
         }   
      }
   }

   // Step 2: Deal with selected physical in case skipped by timeout of above loop
   if (doSelected) {
      // Draw now if non-transparent
      if (!fSelectedPhysical->IsTransparent()) {
         UInt_t shapeLOD = CalcPhysicalLOD(*fSelectedPhysical, camera, LOD);
         (fSelectedPhysical->*drawPtr)(shapeLOD);
         UpdateDrawStats(*fSelectedPhysical);
      } else {
         // Add to transparent drawlist
         transDrawList.push_back(fSelectedPhysical);
      }
   }

   // Step 3: Draw the filtered transparent objects with GL depth writing off
   // blending on
   // TODO: Sort to draw back to front with depth test off for better blending
   glDepthMask(GL_FALSE);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   for (drawIt = transDrawList.begin(); drawIt != transDrawList.end(); drawIt++) {
      drawShape = *drawIt;

      UInt_t shapeLOD = CalcPhysicalLOD(*drawShape, camera, LOD);

      //Draw, DrawWireFrame, DrawOutline
      (drawShape->*drawPtr)(shapeLOD);
      UpdateDrawStats(*drawShape);
   }

   // Reset these after transparent done
   glDepthMask(GL_TRUE);
   glDisable(GL_BLEND);

   // Finally: Draw selected object bounding box
   if (fSelectedPhysical) {

      //BBOX is white for wireframe mode and fill,
      //red for outlines
      switch (style) {
      case kFill:
      case kWireFrame :
         glColor3d(1., 1., 1.);

         break;
      case kOutline:
         glColor3d(1., 0., 0.);
         
         break;
      }

      if (style == kFill || style == kOutline) {
         glDisable(GL_LIGHTING);
      }
      glDisable(GL_DEPTH_TEST);

      fSelectedPhysical->BoundingBox().Draw();

      if (style == kFill || style == kOutline) {
         glEnable(GL_LIGHTING);
      }
      glEnable(GL_DEPTH_TEST);
   }

   return;
}

//______________________________________________________________________________
void TGLScene::SortDrawList()
{
   assert(!fDrawListValid);

   TGLStopwatch stopwatch;

   if (gDebug>2) {
      stopwatch.Start();
   }

   fDrawList.reserve(fPhysicalShapes.size());

   // Delete all zero (to-be-deleted) objects
   fDrawList.erase(remove(fDrawList.begin(), fDrawList.end(), static_cast<const TGLPhysicalShape *>(0)), fDrawList.end());
   
   assert(fDrawList.size() == fPhysicalShapes.size());

   //TODO: partition the selected to front

   // Sort by volume of shape bounding box
   sort(fDrawList.begin(), fDrawList.end(), TGLScene::ComparePhysicalVolumes);

   if (gDebug>2) {
      Info("TGLScene::SortDrawList", "sorting took %f msec", stopwatch.End());
   }

   fDrawListValid = kTRUE;
}

//______________________________________________________________________________
Bool_t TGLScene::ComparePhysicalVolumes(const TGLPhysicalShape * shape1, const TGLPhysicalShape * shape2)
{
   return (shape1->BoundingBox().Volume() > shape2->BoundingBox().Volume());
}

//______________________________________________________________________________
void TGLScene::DrawAxes() const
{
   if (fLock != kDrawLock && fLock != kSelectLock) {
      Error("TGLScene::DrawAxes", "expected Draw or Select Lock");
   }
   // Draw out the scene axes
   // Taken directly from TGLRender by Timur Pocheptsov
   static const UChar_t xyz[][8] = {{0x44, 0x44, 0x28, 0x10, 0x10, 0x28, 0x44, 0x44},
                                     {0x10, 0x10, 0x10, 0x10, 0x10, 0x28, 0x44, 0x44},
                                     {0x7c, 0x20, 0x10, 0x10, 0x08, 0x08, 0x04, 0x7c}};

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glPushAttrib(GL_DEPTH_BUFFER_BIT);
   glDisable(GL_DEPTH_TEST);
   glDisable(GL_LIGHTING);

   const Double_t axisColors[][3] = {{1., 0., 0.}, // X axis red
                                     {0., 1., 0.}, // Y axis green
                                     {0., 0., 1.}};// Z axis blue
   const TGLBoundingBox & box = BoundingBox();
   Double_t xmin = box.XMin(), xmax = box.XMax();
   Double_t ymin = box.YMin(), ymax = box.YMax();
   Double_t zmin = box.ZMin(), zmax = box.ZMax();

   glBegin(GL_LINES);
   glColor3dv(axisColors[0]);
   glVertex3d(xmin, ymin, zmin);
   glVertex3d(xmax, ymin, zmin);
   glColor3dv(axisColors[1]);
   glVertex3d(xmin, ymin, zmin);
   glVertex3d(xmin, ymax, zmin);
   glColor3dv(axisColors[2]);
   glVertex3d(xmin, ymin, zmin);
   glVertex3d(xmin, ymin, zmax);
   glEnd();

   // X label
   glColor3dv(axisColors[0]);
   glRasterPos3d(xmax, ymin + 12, zmin);
   glBitmap(8, 8, 0., 0., 0., 0., xyz[0]);
   DrawNumber(xmax, xmax, ymin, zmin, 9.);
   DrawNumber(xmin, xmin, ymin, zmin, 0.);

   // Y label
   glColor3dv(axisColors[1]);
   glRasterPos3d(xmin, ymax + 12, zmin);
   glBitmap(8, 8, 0, 0, 12., 0, xyz[1]);
   DrawNumber(ymax, xmin, ymax, zmin, 9.);
   DrawNumber(ymin, xmin, ymin, zmin, 9.);

   // Z label
   glColor3dv(axisColors[2]);
   glRasterPos3d(xmin, ymin, zmax);
   glBitmap(8, 8, 0, 0, 0., 0, xyz[2]);
   DrawNumber(zmax, xmin, ymin, zmax, 9.);
   DrawNumber(zmin, xmin, ymin, zmin, -9.);

   glEnable(GL_LIGHTING);
   glEnable(GL_DEPTH_TEST);
   glPopAttrib();
}

//______________________________________________________________________________
void TGLScene::DrawNumber(Double_t num, Double_t x, Double_t y, Double_t z, Double_t yorig) const
{
   if (fLock != kDrawLock && fLock != kSelectLock) {
      Error("TGLScene::DrawNumber", "expected Draw or Select Lock");
   }
   static const UChar_t
      digits[][8] = {{0x38, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x38},//0
                     {0x10, 0x10, 0x10, 0x10, 0x10, 0x70, 0x10, 0x10},//1
                     {0x7c, 0x44, 0x20, 0x18, 0x04, 0x04, 0x44, 0x38},//2
                     {0x38, 0x44, 0x04, 0x04, 0x18, 0x04, 0x44, 0x38},//3
                     {0x04, 0x04, 0x04, 0x04, 0x7c, 0x44, 0x44, 0x44},//4
                     {0x7c, 0x44, 0x04, 0x04, 0x7c, 0x40, 0x40, 0x7c},//5
                     {0x7c, 0x44, 0x44, 0x44, 0x7c, 0x40, 0x40, 0x7c},//6
                     {0x20, 0x20, 0x20, 0x10, 0x08, 0x04, 0x44, 0x7c},//7
                     {0x38, 0x44, 0x44, 0x44, 0x38, 0x44, 0x44, 0x38},//8
                     {0x7c, 0x44, 0x04, 0x04, 0x7c, 0x44, 0x44, 0x7c},//9
                     {0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},//.
                     {0x00, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00}};//-

   TString str;
   str+=Long_t(num);

   glRasterPos3i(Int_t(x), Int_t(y), Int_t(z));
   for (Ssiz_t i = 0, e = str.Length(); i < e; ++i) {
      if (str[i] == '.') {
         glBitmap(8, 8, 0., yorig, 7., 0., digits[10]);
         if (i + 1 < e)
            glBitmap(8, 8, 0., yorig, 7., 0., digits[str[i + 1] - '0']);
         break;
      } else if (str[i] == '-') {
         glBitmap(8, 8, 0., yorig, 7., 0., digits[11]);
      } else {
         glBitmap(8, 8, 0., yorig, 7., 0., digits[str[i] - '0']);
      }
   }
}

//______________________________________________________________________________
UInt_t TGLScene:: CalcPhysicalLOD(const TGLPhysicalShape & shape, const TGLCamera & camera,
                                 UInt_t sceneLOD) const
{
   // Find diagonal pixel size of projected drawable BB, using camera
   Double_t diagonal = static_cast<Double_t>(camera.ViewportSize(shape.BoundingBox()).Diagonal());

   // TODO: Get real screen size - assuming 2000 pixel screen at present
   // Calculate a non-linear sizing hint for this shape. Needs more experimenting with...
   UInt_t sizeLOD = static_cast<UInt_t>(pow(diagonal,0.4) * 100.0 / pow(2000.0,0.4));

   // Factor in scene quality
   UInt_t shapeLOD = (sceneLOD * sizeLOD) / 100;

   if (shapeLOD > 10) {
      Double_t quant = ((static_cast<Double_t>(shapeLOD)) + 0.3) / 10;
      shapeLOD = static_cast<UInt_t>(quant)*10;
   } else {
      Double_t quant = ((static_cast<Double_t>(shapeLOD)) + 0.3) / 3;
      shapeLOD = static_cast<UInt_t>(quant)*3;
   }

   if (shapeLOD > 100) {
      shapeLOD = 100;
   }

   return shapeLOD;
}

//______________________________________________________________________________
Bool_t TGLScene::Select(const TGLCamera & camera, EDrawStyle style, const TGLClip * clip)
{
   Bool_t changed = kFALSE;
   if (fLock != kSelectLock) {
      Error("TGLScene::Select", "expected SelectLock");
   }

   // Create the select buffer. This will work as we have a flat set of physical shapes.
   // We only ever load a single name in TGLPhysicalShape::DirectDraw so any hit record always
   // has same 4 GLuint format
   static std::vector<GLuint> selectBuffer;
   selectBuffer.resize(fPhysicalShapes.size()*4);
   glSelectBuffer(selectBuffer.size(), &selectBuffer[0]);

   // Enter picking mode
   glRenderMode(GL_SELECT);
   glInitNames();
   glPushName(0);

   // Draw out scene at best quality with clipping, no timelimit
   Draw(camera, style, kHigh, 0.0, clip);

   // Retrieve the hit count and return to render
   Int_t hits = glRenderMode(GL_RENDER);

   if (hits < 0) {
      Error("TGLScene::Select", "selection buffer overflow");
      return changed;
   }

   if (hits > 0) {
      // Every hit record has format (GLuint per item) - format is:
      //
      // no of names in name block (1 always)
      // minDepth
      // maxDepth
      // name(s) (1 always)

      // Sort the hits by minimum depth (closest part of object)
      static std::vector<std::pair<UInt_t, Int_t> > sortedHits;
      sortedHits.resize(hits);
      Int_t i;
      for (i = 0; i < hits; i++) {
         assert(selectBuffer[i * 4] == 1); // expect a single name per record
         sortedHits[i].first = selectBuffer[i * 4 + 1]; // hit object minimum depth
         sortedHits[i].second = selectBuffer[i * 4 + 3]; // hit object name
      }
      std::sort(sortedHits.begin(), sortedHits.end());

      // Find first (closest) non-transparent object in the hit stack
      TGLPhysicalShape * selected = 0;
      for (i = 0; i < hits; i++) {
         selected = FindPhysical(sortedHits[i].second);
         if (!selected->IsTransparent()) {
            break;
         }
      }
      // If we failed to find a non-transparent object use the first
      // (closest) transparent one
      if (selected->IsTransparent()) {
         selected = FindPhysical(sortedHits[0].second);
      }
      assert(selected);

      // Swap any selection
      if (selected != fSelectedPhysical) {
         if (fSelectedPhysical) {
            fSelectedPhysical->Select(kFALSE);
         }
         fSelectedPhysical = selected;
         fSelectedPhysical->Select(kTRUE);
         changed = kTRUE;
      }
   } else { // 0 hits
      if (fSelectedPhysical) {
         fSelectedPhysical->Select(kFALSE);
         fSelectedPhysical = 0;
         changed = kTRUE;
      }
   }

   return changed;
}

//______________________________________________________________________________
Bool_t TGLScene::SetSelectedColor(const Float_t color[17])
{
   if (fSelectedPhysical) {
      fSelectedPhysical->SetColor(color);
      return kTRUE;
   } else {
      assert(kFALSE);
      return kFALSE;
   }
}

//______________________________________________________________________________
Bool_t TGLScene::SetColorOnSelectedFamily(const Float_t color[17])
{
   if (fSelectedPhysical) {
      TGLPhysicalShape * physical;
      PhysicalShapeMapIt_t physicalShapeIt = fPhysicalShapes.begin();
      while (physicalShapeIt != fPhysicalShapes.end()) {
         physical = physicalShapeIt->second;
         if (physical) {
            if (physical->GetLogical().ID() == fSelectedPhysical->GetLogical().ID()) {
               physical->SetColor(color);
            }
         } else {
            assert(kFALSE);
         }
         ++physicalShapeIt;
      }
      return kTRUE;
   } else {
      assert(kFALSE);
      return kFALSE;
   }
}

//______________________________________________________________________________
Bool_t TGLScene::ShiftSelected(const TGLVector3 & shift)
{
   if (fSelectedPhysical) {
      fSelectedPhysical->Translate(shift);
      fBoundingBoxValid = kFALSE;
      return kTRUE;
   }
   else {
      assert(kFALSE);
      return kFALSE;
   }
}

//______________________________________________________________________________
Bool_t TGLScene::SetSelectedGeom(const TGLVertex3 & trans, const TGLVector3 & scale)
{
   if (fSelectedPhysical) {
      fSelectedPhysical->SetTranslation(trans);
      fSelectedPhysical->Scale(scale);
      fBoundingBoxValid = kFALSE;
      return kTRUE;
   } else {
      assert(kFALSE);
      return kFALSE;
   }
}

//______________________________________________________________________________
const TGLBoundingBox & TGLScene::BoundingBox() const
{
   if (!fBoundingBoxValid) {
      Double_t xMin, xMax, yMin, yMax, zMin, zMax;
      xMin = xMax = yMin = yMax = zMin = zMax = 0.0;
      PhysicalShapeMapCIt_t physicalShapeIt = fPhysicalShapes.begin();
      const TGLPhysicalShape * physicalShape;
      while (physicalShapeIt != fPhysicalShapes.end())
      {
         physicalShape = physicalShapeIt->second;
         if (!physicalShape)
         {
            assert(kFALSE);
            continue;
         }
         TGLBoundingBox box = physicalShape->BoundingBox();
         if (physicalShapeIt == fPhysicalShapes.begin()) {
            xMin = box.XMin(); xMax = box.XMax();
            yMin = box.YMin(); yMax = box.YMax();
            zMin = box.ZMin(); zMax = box.ZMax();
         } else {
            if (box.XMin() < xMin) { xMin = box.XMin(); }
            if (box.XMax() > xMax) { xMax = box.XMax(); }
            if (box.YMin() < yMin) { yMin = box.YMin(); }
            if (box.YMax() > yMax) { yMax = box.YMax(); }
            if (box.ZMin() < zMin) { zMin = box.ZMin(); }
            if (box.ZMax() > zMax) { zMax = box.ZMax(); }
         }
         ++physicalShapeIt;
      }
      fBoundingBox.SetAligned(TGLVertex3(xMin,yMin,zMin), TGLVertex3(xMax,yMax,zMax));
      fBoundingBoxValid = kTRUE;
   }
   return fBoundingBox;
}

//______________________________________________________________________________
void TGLScene::Dump() const
{
   std::cout << "Scene: " << fLogicalShapes.size() << " Logicals / " << fPhysicalShapes.size() << " Physicals " << std::endl;
}

//______________________________________________________________________________
UInt_t TGLScene::SizeOf() const
{
   UInt_t size = sizeof(this);

   std::cout << "Size: Scene Only " << size << std::endl;

   LogicalShapeMapCIt_t logicalShapeIt = fLogicalShapes.begin();
   const TGLLogicalShape * logicalShape;
   while (logicalShapeIt != fLogicalShapes.end()) {
      logicalShape = logicalShapeIt->second;
      size += sizeof(*logicalShape);
      ++logicalShapeIt;
   }

   std::cout << "Size: Scene + Logical Shapes " << size << std::endl;

   PhysicalShapeMapCIt_t physicalShapeIt = fPhysicalShapes.begin();
   const TGLPhysicalShape * physicalShape;
   while (physicalShapeIt != fPhysicalShapes.end()) {
      physicalShape = physicalShapeIt->second;
      size += sizeof(*physicalShape);
      ++physicalShapeIt;
   }

   std::cout << "Size: Scene + Logical Shapes + Physical Shapes " << size << std::endl;

   return size;
}

//______________________________________________________________________________
void TGLScene::ResetDrawStats()
{
   fDrawStats.fOpaque = 0;
   fDrawStats.fTrans = 0;
   fDrawStats.fByShape.clear();
}

//______________________________________________________________________________
void TGLScene::UpdateDrawStats(const TGLPhysicalShape & shape)
{
   // Update opaque/transparent draw count
   if (shape.IsTransparent()) {
      ++fDrawStats.fTrans;
   } else {
      ++fDrawStats.fOpaque;
   }

   // By type only needed for debug currently
   if (gDebug>3) {
      // Update the stats 
      std::string shapeType = shape.GetLogical().IsA()->GetName();
      typedef std::map<std::string, UInt_t>::iterator MapIt_t;
      MapIt_t statIt = fDrawStats.fByShape.find(shapeType);

      if (statIt == fDrawStats.fByShape.end()) {
         //do not need to check insert(.....).second, because statIt was stats.end() before
         statIt = fDrawStats.fByShape.insert(std::make_pair(shapeType, 0u)).first;
      }

      statIt->second++;   
   }
}
 
//______________________________________________________________________________
void TGLScene::DumpDrawStats()
{
   // Dump some current draw stats for debuggin

   // Draw counts
   if (gDebug>2) {
      Info("TGLScene::DumpDrawStats()", "Drew %i, %i Opaque %i Transparent", fDrawStats.fOpaque + fDrawStats.fTrans,
         fDrawStats.fOpaque, fDrawStats.fTrans);
   }

   // By shape type counts
   if (gDebug>3) {
      std::map<std::string, UInt_t>::const_iterator it = fDrawStats.fByShape.begin();
      while (it != fDrawStats.fByShape.end()) {
         std::cout << it->first << " (" << it->second << ")\t";
         it++;
      }
      std::cout << std::endl;
   }
}
