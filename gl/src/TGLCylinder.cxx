// @(#)root/gl:$Id: TGLCylinder.cxx,v 1.1 2006/02/20 11:10:06 brun Exp $
// Author:  Timur Pocheptsov  03/08/2004
// NOTE: This code moved from obsoleted TGLSceneObject.h / .cxx - see these
// attic files for previous CVS history

/*************************************************************************
 * Copyright (C) 1995-2006, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/
#include "TGLCylinder.h"
#include "TGLDrawFlags.h"
#include "TGLIncludes.h"

#include "TBuffer3D.h"
#include "TBuffer3DTypes.h"

// For debug tracing
#include "TClass.h" 
#include "TError.h"

TGLVector3 gLowNormalDefault(0., 0., -1.);
TGLVector3 gHighNormalDefault(0., 0., 1.);

class TGLMesh {
protected:
   // active LOD (level of detail) - quality
   UInt_t     fLOD;    

   Double_t fRmin1, fRmax1, fRmin2, fRmax2;
   Double_t fDz;

   //normals for top and bottom (for cuts)
   TGLVector3 fNlow;
   TGLVector3 fNhigh;

   void GetNormal(const TGLVertex3 &vertex, TGLVector3 &normal)const;
   Double_t GetZcoord(Double_t x, Double_t y, Double_t z)const;
   const TGLVertex3 &MakeVertex(Double_t x, Double_t y, Double_t z)const;

public:
   TGLMesh(UInt_t LOD, Double_t r1, Double_t r2, Double_t r3, Double_t r4, Double_t dz,
           const TGLVector3 &l = gLowNormalDefault, const TGLVector3 &h = gHighNormalDefault);
   virtual ~TGLMesh() { }
   virtual void Draw() const = 0;
};

//segment contains 3 quad strips:
//one for inner and outer sides, two for top and bottom
class TubeSegMesh : public TGLMesh {
private:
   // Allocate space for highest quality (LOD) meshes
   TGLVertex3 fMesh[(TGLDrawFlags::kLODHigh + 1) * 8 + 8];
   TGLVector3 fNorm[(TGLDrawFlags::kLODHigh + 1) * 8 + 8];

public:
   TubeSegMesh(UInt_t LOD, Double_t r1, Double_t r2, Double_t r3, Double_t r4, Double_t dz,
               Double_t phi1, Double_t phi2, const TGLVector3 &l = gLowNormalDefault, 
               const TGLVector3 &h = gHighNormalDefault);

   void Draw() const;
};

//four quad strips:
//outer, inner, top, bottom
class TubeMesh : public TGLMesh {
private:
   // Allocate space for highest quality (LOD) meshes
   TGLVertex3 fMesh[(TGLDrawFlags::kLODHigh + 1) * 8];
   TGLVector3 fNorm[(TGLDrawFlags::kLODHigh + 1) * 8];

public:
   TubeMesh(UInt_t LOD, Double_t r1, Double_t r2, Double_t r3, Double_t r4, Double_t dz,
            const TGLVector3 &l = gLowNormalDefault, const TGLVector3 &h = gHighNormalDefault);

   void Draw() const;
};

//One quad mesh and 2 triangle funs
class TCylinderMesh : public TGLMesh {
private:
   // Allocate space for highest quality (LOD) meshes
   TGLVertex3 fMesh[(TGLDrawFlags::kLODHigh + 1) * 4 + 2];
   TGLVector3 fNorm[(TGLDrawFlags::kLODHigh + 1) * 4 + 2];

public:
   TCylinderMesh(UInt_t LOD, Double_t r1, Double_t r2, Double_t dz,
                 const TGLVector3 &l = gLowNormalDefault, const TGLVector3 &h = gHighNormalDefault);

   void Draw() const;
};

//One quad mesh and 2 triangle fans
class TCylinderSegMesh : public TGLMesh {
private:
   // Allocate space for highest quality (LOD) meshes
   TGLVertex3 fMesh[(TGLDrawFlags::kLODHigh + 1) * 4 + 10];
   TGLVector3 fNorm[(TGLDrawFlags::kLODHigh + 1) * 4 + 10];

public:
   TCylinderSegMesh(UInt_t LOD, Double_t r1, Double_t r2, Double_t dz, Double_t phi1, Double_t phi2,
                    const TGLVector3 &l = gLowNormalDefault, const TGLVector3 &h = gHighNormalDefault);
   void Draw() const;
};


//______________________________________________________________________________
TGLMesh::TGLMesh(UInt_t LOD, Double_t r1, Double_t r2, Double_t r3, Double_t r4, Double_t dz,
                 const TGLVector3 &l, const TGLVector3 &h) : 
   fLOD(LOD),
   fRmin1(r1), fRmax1(r2), fRmin2(r3), fRmax2(r4),
   fDz(dz), fNlow(l), fNhigh(h)
{
   // constructor
}

//______________________________________________________________________________
void TGLMesh::GetNormal(const TGLVertex3 &v, TGLVector3 &n)const
{
   // get normal
   Double_t z = (fRmax1 - fRmax2) / (2 * fDz);
   Double_t mag = TMath::Sqrt(v[0] * v[0] + v[1] * v[1] + z * z);

   n[0] = v[0] / mag;
   n[1] = v[1] / mag;
   n[2] = z / mag;
}

//______________________________________________________________________________
Double_t TGLMesh::GetZcoord(Double_t x, Double_t y, Double_t z)const
{
   // get Z coordinate
   Double_t newz = 0;
   if (z < 0) newz = -fDz - (x * fNlow[0] + y * fNlow[1]) / fNlow[2];
   else newz = fDz - (x * fNhigh[0] + y * fNhigh[1]) / fNhigh[2];

   return newz;
}

//______________________________________________________________________________
const TGLVertex3 &TGLMesh::MakeVertex(Double_t x, Double_t y, Double_t z)const
{
   // make vertex
   static TGLVertex3 vert(0., 0., 0.);
   vert[0] = x;
   vert[1] = y;
   vert[2] = GetZcoord(x, y, z);

   return vert;
}

//______________________________________________________________________________
TubeSegMesh::TubeSegMesh(UInt_t LOD, Double_t r1, Double_t r2, Double_t r3, Double_t r4, Double_t dz,
                         Double_t phi1, Double_t phi2,
                         const TGLVector3 &l, const TGLVector3 &h)
                 :TGLMesh(LOD, r1, r2, r3, r4, dz, l, h), fMesh(), fNorm()

{
   // constructor
   const Double_t delta = (phi2 - phi1) / LOD;
   Double_t currAngle = phi1;

   Bool_t even = kTRUE;
   Double_t c = TMath::Cos(currAngle);
   Double_t s = TMath::Sin(currAngle);
   const Int_t topShift = (fLOD + 1) * 4 + 8;
   const Int_t botShift = (fLOD + 1) * 6 + 8;
   Int_t j = 4 * (fLOD + 1) + 2;

   //defining all three strips here, first strip is non-closed here
   for (Int_t i = 0, e = (fLOD + 1) * 2; i < e; ++i) {
      if (even) {
         fMesh[i] = MakeVertex(fRmax2 * c, fRmax2 * s, fDz);
         fMesh[j] = MakeVertex(fRmin2 * c, fRmin2 * s, fDz);
         fMesh[i + topShift] = MakeVertex(fRmin2 * c, fRmin2 * s, fDz);
         fMesh[i + botShift] = MakeVertex(fRmax1 * c, fRmax1 * s, - fDz);
         GetNormal(fMesh[j], fNorm[j]);
         fNorm[j].Negate();
         even = kFALSE;
      } else {
         fMesh[i] = MakeVertex(fRmax1 * c, fRmax1 * s, - fDz);
         fMesh[j + 1] = MakeVertex(fRmin1 * c, fRmin1 * s, -fDz);
         fMesh[i + topShift] = MakeVertex(fRmax2 * c, fRmax2 * s, fDz);
         fMesh[i + botShift] = MakeVertex(fRmin1 * c, fRmin1 * s, - fDz);
         GetNormal(fMesh[j + 1], fNorm[j + 1]);
         fNorm[j + 1].Negate();
         even = kTRUE;
         currAngle += delta;
         c = TMath::Cos(currAngle);
         s = TMath::Sin(currAngle);
         j -= 2;
      }

      GetNormal(fMesh[i], fNorm[i]);
      fNorm[i + topShift] = fNhigh;
      fNorm[i + botShift] = fNlow;
   }

   //closing first strip
   Int_t ind = 2 * (fLOD + 1);
   TGLVector3 norm(0., 0., 0.);

   fMesh[ind] = fMesh[ind - 2];
   fMesh[ind + 1] = fMesh[ind - 1];
   fMesh[ind + 2] = fMesh[ind + 4];
   fMesh[ind + 3] = fMesh[ind + 5];
   TMath::Normal2Plane(fMesh[ind].CArr(), fMesh[ind + 1].CArr(), fMesh[ind + 2].CArr(),
                       norm.Arr());
   fNorm[ind] = norm;
   fNorm[ind + 1] = norm;
   fNorm[ind + 2] = norm;
   fNorm[ind + 3] = norm;

   ind = topShift - 4;
   fMesh[ind] = fMesh[ind - 2];
   fMesh[ind + 1] = fMesh[ind - 1];
   fMesh[ind + 2] = fMesh[0];
   fMesh[ind + 3] = fMesh[1];
   TMath::Normal2Plane(fMesh[ind].CArr(), fMesh[ind + 1].CArr(), fMesh[ind + 2].CArr(),
                       norm.Arr());
   fNorm[ind] = norm;
   fNorm[ind + 1] = norm;
   fNorm[ind + 2] = norm;
   fNorm[ind + 3] = norm;
}

//______________________________________________________________________________
void TubeSegMesh::Draw() const
{
   //Tube segment is drawn as three quad strips
   //1. enabling vertex arrays
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_NORMAL_ARRAY);
   //2. setting arrays
   glVertexPointer(3, GL_DOUBLE, sizeof(TGLVertex3), fMesh[0].CArr());
   glNormalPointer(GL_DOUBLE, sizeof(TGLVector3), fNorm[0].CArr());
   //3. draw first strip
   glDrawArrays(GL_QUAD_STRIP, 0, 4 * (fLOD + 1) + 8);
   //4. draw top and bottom strips
   glDrawArrays(GL_QUAD_STRIP, 4 * (fLOD + 1) + 8, 2 * (fLOD + 1));
   glDrawArrays(GL_QUAD_STRIP, 6 * (fLOD + 1) + 8, 2 * (fLOD + 1));

   glDisableClientState(GL_VERTEX_ARRAY);
   glDisableClientState(GL_NORMAL_ARRAY);
}

//______________________________________________________________________________
TubeMesh::TubeMesh(UInt_t LOD, Double_t r1, Double_t r2, Double_t r3, Double_t r4, Double_t z,
                   const TGLVector3 &l, const TGLVector3 &h)
             :TGLMesh(LOD, r1, r2, r3, r4, z, l, h), fMesh(), fNorm()
{
   // constructor
   const Double_t delta = TMath::TwoPi() / fLOD;
   Double_t currAngle = 0.;

   Bool_t even = kTRUE;
   Double_t c = TMath::Cos(currAngle);
   Double_t s = TMath::Sin(currAngle);

   const Int_t topShift = (fLOD + 1) * 4;
   const Int_t botShift = (fLOD + 1) * 6;
   Int_t j = 4 * (fLOD + 1) - 2;

   //defining all four strips here
   for (Int_t i = 0, e = (fLOD + 1) * 2; i < e; ++i) {
      if (even) {
         fMesh[i] = MakeVertex(fRmax2 * c, fRmax2 * s, fDz);
         fMesh[j] = MakeVertex(fRmin2 * c, fRmin2 * s, fDz);
         fMesh[i + topShift] = MakeVertex(fRmin2 * c, fRmin2 * s, fDz);
         fMesh[i + botShift] = MakeVertex(fRmax1 * c, fRmax1 * s, - fDz);
         GetNormal(fMesh[j], fNorm[j]);
         fNorm[j].Negate();
         even = kFALSE;
      } else {
         fMesh[i] = MakeVertex(fRmax1 * c, fRmax1 * s, - fDz);
         fMesh[j + 1] = MakeVertex(fRmin1 * c, fRmin1 * s, -fDz);
         fMesh[i + topShift] = MakeVertex(fRmax2 * c, fRmax2 * s, fDz);
         fMesh[i + botShift] = MakeVertex(fRmin1 * c, fRmin1 * s, - fDz);
         GetNormal(fMesh[j + 1], fNorm[j + 1]);
         fNorm[j + 1].Negate();
         even = kTRUE;
         currAngle += delta;
         c = TMath::Cos(currAngle);
         s = TMath::Sin(currAngle);
         j -= 2;
      }

      GetNormal(fMesh[i], fNorm[i]);
      fNorm[i + topShift] = fNhigh;
      fNorm[i + botShift] = fNlow;
   }
}

//______________________________________________________________________________
void TubeMesh::Draw() const
{
   //Tube is drawn as four quad strips
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_NORMAL_ARRAY);

   glVertexPointer(3, GL_DOUBLE, sizeof(TGLVertex3), fMesh[0].CArr());
   glNormalPointer(GL_DOUBLE, sizeof(TGLVector3), fNorm[0].CArr());
   //draw outer and inner strips
   glDrawArrays(GL_QUAD_STRIP, 0, 2 * (fLOD + 1));
   glDrawArrays(GL_QUAD_STRIP, 2 * (fLOD + 1), 2 * (fLOD + 1));
   //draw top and bottom strips
   glDrawArrays(GL_QUAD_STRIP, 4 * (fLOD + 1), 2 * (fLOD + 1));
   glDrawArrays(GL_QUAD_STRIP, 6 * (fLOD + 1), 2 * (fLOD + 1));
   //5. disabling vertex arrays
   glDisableClientState(GL_VERTEX_ARRAY);
   glDisableClientState(GL_NORMAL_ARRAY);
}

//______________________________________________________________________________
TCylinderMesh::TCylinderMesh(UInt_t LOD, Double_t r1, Double_t r2, Double_t dz,
                             const TGLVector3 &l, const TGLVector3 &h)
                 :TGLMesh(LOD, 0., r1, 0., r2, dz, l, h), fMesh(), fNorm()
{
   // constructor
   const Double_t delta = TMath::TwoPi() / fLOD;
   Double_t currAngle = 0.;

   Bool_t even = kTRUE;
   Double_t c = TMath::Cos(currAngle);
   Double_t s = TMath::Sin(currAngle);

   //central point of top fan
   Int_t topShift = (fLOD + 1) * 2;
   fMesh[topShift][0] = fMesh[topShift][1] = 0., fMesh[topShift][2] = fDz;
   fNorm[topShift] = fNhigh;
   ++topShift;

   //central point of bottom fun
   Int_t botShift = topShift + 2 * (fLOD + 1);
   fMesh[botShift][0] = fMesh[botShift][1] = 0., fMesh[botShift][2] = -fDz;
   fNorm[botShift] = fNlow;
   ++botShift;

   //defining 1 strip and 2 fans
   for (Int_t i = 0, e = (fLOD + 1) * 2, j = 0; i < e; ++i) {
      if (even) {
         fMesh[i] = MakeVertex(fRmax2 * c, fRmax2 * s, fDz);
         fMesh[j + topShift] = MakeVertex(fRmin2 * c, fRmin2 * s, fDz);
         fMesh[j + botShift] = MakeVertex(fRmax1 * c, fRmax1 * s, - fDz);
         even = kFALSE;
      } else {
         fMesh[i] = MakeVertex(fRmax1 * c, fRmax1 * s, - fDz);
         even = kTRUE;
         currAngle += delta;
         c = TMath::Cos(currAngle);
         s = TMath::Sin(currAngle);
         ++j;
      }

      GetNormal(fMesh[i], fNorm[i]);
      fNorm[i + topShift] = fNhigh;
      fNorm[i + botShift] = fNlow;
   }
}

//______________________________________________________________________________
void TCylinderMesh::Draw() const
{
   // draw cylinder mesh
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_NORMAL_ARRAY);

   glVertexPointer(3, GL_DOUBLE, sizeof(TGLVertex3), fMesh[0].CArr());
   glNormalPointer(GL_DOUBLE, sizeof(TGLVector3), fNorm[0].CArr());

   //draw quad strip
   glDrawArrays(GL_QUAD_STRIP, 0, 2 * (fLOD + 1));
   //draw top and bottom funs
   glDrawArrays(GL_TRIANGLE_FAN, 2 * (fLOD + 1), fLOD + 2);
   glDrawArrays(GL_TRIANGLE_FAN, 3 * (fLOD + 1) + 1, fLOD + 2);

   glDisableClientState(GL_VERTEX_ARRAY);
   glDisableClientState(GL_NORMAL_ARRAY);
}

//______________________________________________________________________________
TCylinderSegMesh::TCylinderSegMesh(UInt_t LOD, Double_t r1, Double_t r2, Double_t dz, Double_t phi1,
                                    Double_t phi2, const TGLVector3 &l,
                                    const TGLVector3 &h)
                     :TGLMesh(LOD, 0., r1, 0., r2, dz, l, h), fMesh(), fNorm()
{
   //One quad mesh and two fans
   Double_t delta = (phi2 - phi1) / fLOD;
   Double_t currAngle = phi1;

   Bool_t even = kTRUE;
   Double_t c = TMath::Cos(currAngle);
   Double_t s = TMath::Sin(currAngle);

   const TGLVertex3 vTop(0., 0., fDz);
   const TGLVertex3 vBot(0., 0., - fDz);

   //center of top fan
   Int_t topShift = (fLOD + 1) * 2 + 8;
   fMesh[topShift] = vTop;
   fNorm[topShift] = fNhigh;
   ++topShift;

   //center of bottom fan
   Int_t botShift = topShift + fLOD + 1;
   fMesh[botShift] = vBot;
   fNorm[botShift] = fNlow;
   ++botShift;

   //defining strip and two fans
   //strip is not closed here
   Int_t i = 0;
   for (Int_t e = (fLOD + 1) * 2, j = 0; i < e; ++i) {
      if (even) {
         fMesh[i] = MakeVertex(fRmax2 * c, fRmax2 * s, fDz);
         fMesh[j + topShift] = MakeVertex(fRmax2 * c, fRmax2 * s, fDz);
         fMesh[j + botShift] = MakeVertex(fRmax1 * c, fRmax1 * s, - fDz);
         even = kFALSE;
         fNorm[j + topShift] = fNhigh;
         fNorm[j + botShift] = fNlow;
      } else {
         fMesh[i] = MakeVertex(fRmax1 * c, fRmax1 * s, - fDz);
         even = kTRUE;
         currAngle += delta;
         c = TMath::Cos(currAngle);
         s = TMath::Sin(currAngle);
         ++j;
      }

      GetNormal(fMesh[i], fNorm[i]);
   }

   //closing first strip
   Int_t ind = 2 * (fLOD + 1);
   TGLVector3 norm(0., 0., 0.);

   fMesh[ind] = fMesh[ind - 2];
   fMesh[ind + 1] = fMesh[ind - 1];
   fMesh[ind + 2] = vTop;
   fMesh[ind + 3] = vBot;
   TMath::Normal2Plane(fMesh[ind].CArr(), fMesh[ind + 1].CArr(), fMesh[ind + 2].CArr(),
                          norm.Arr());
   fNorm[ind] = norm;
   fNorm[ind + 1] = norm;
   fNorm[ind + 2] = norm;
   fNorm[ind + 3] = norm;

   ind += 4;
   fMesh[ind] = vTop;
   fMesh[ind + 1] = vBot;
   fMesh[ind + 2] = fMesh[0];
   fMesh[ind + 3] = fMesh[1];
   TMath::Normal2Plane(fMesh[ind].CArr(), fMesh[ind + 1].CArr(), fMesh[ind + 2].CArr(),
                       norm.Arr());
   fNorm[ind] = norm;
   fNorm[ind + 1] = norm;
   fNorm[ind + 2] = norm;
   fNorm[ind + 3] = norm;
}

//______________________________________________________________________________
void TCylinderSegMesh::Draw() const
{
   //Cylinder segment is drawn as one quad strip and
   //two triangle fans
   //1. enabling vertex arrays
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_NORMAL_ARRAY);
   //2. setting arrays
   glVertexPointer(3, GL_DOUBLE, sizeof(TGLVertex3), fMesh[0].CArr());
   glNormalPointer(GL_DOUBLE, sizeof(TGLVector3), fNorm[0].CArr());
   //3. draw quad strip
   glDrawArrays(GL_QUAD_STRIP, 0, 2 * (fLOD + 1) + 8);
   //4. draw top and bottom funs
   glDrawArrays(GL_TRIANGLE_FAN, 2 * (fLOD + 1) + 8, fLOD + 2);
   //      glDrawArrays(GL_TRIANGLE_FAN, 3 * (fLOD + 1) + 9, fLOD + 2);
   //5. disabling vertex arrays
   glDisableClientState(GL_VERTEX_ARRAY);
   glDisableClientState(GL_NORMAL_ARRAY);
}

ClassImp(TGLCylinder)

//______________________________________________________________________________
TGLCylinder::TGLCylinder(const TBuffer3DTube &buffer) :
   TGLLogicalShape(buffer)
{
   // Copy out relevant parts of buffer - we create and delete mesh
   // parts on demand in DirectDraw() and they are DL cached
   fR1 = buffer.fRadiusInner;
   fR2 = buffer.fRadiusOuter;
   fR3 = buffer.fRadiusInner;
   fR4 = buffer.fRadiusOuter;
   fDz = buffer.fHalfLength;

   fLowPlaneNorm = gLowNormalDefault; 
   fHighPlaneNorm = gHighNormalDefault; 

   switch (buffer.Type()) {
      case TBuffer3DTypes::kTube:
         fSegMesh = kFALSE;
         break;
   case TBuffer3DTypes::kTubeSeg:
   case TBuffer3DTypes::kCutTube:
         fSegMesh = kTRUE;

         const TBuffer3DTubeSeg * segBuffer = dynamic_cast<const TBuffer3DTubeSeg *>(&buffer);
         if (!segBuffer) {
            Error("TGLCylinder::TGLCylinder", "cannot cast TBuffer3D");
            return;
         }

         fPhi1 = segBuffer->fPhiMin;
         fPhi2 = segBuffer->fPhiMax;
         if (fPhi2 < fPhi1) fPhi2 += 360.;
         fPhi1 *= TMath::DegToRad();
         fPhi2 *= TMath::DegToRad();

         if (buffer.Type() == TBuffer3DTypes::kCutTube) {
            const TBuffer3DCutTube * cutBuffer = dynamic_cast<const TBuffer3DCutTube *>(&buffer);
            if (!cutBuffer) {
               Error("TGLCylinder::TGLCylinder", "cannot cast TBuffer3D");
               return;
            }

            for (UInt_t i =0; i < 3; i++) {
               fLowPlaneNorm[i] = cutBuffer->fLowPlaneNorm[i];
               fHighPlaneNorm[i] = cutBuffer->fHighPlaneNorm[i];
            }
         }
   }
}

//______________________________________________________________________________
TGLCylinder::~TGLCylinder()
{
   //destructor
}

//______________________________________________________________________________
void TGLCylinder::DirectDraw(const TGLDrawFlags & flags) const
{
   // Debug tracing
   if (gDebug > 4) {
      Info("TGLCylinder::DirectDraw", "this %d (class %s) LOD %d", this, IsA()->GetName(), flags.LOD());
   }

   // As we are now support display list caching we can create, draw and
   // delete mesh parts of suitible LOD (quality) here - it will be cached 
   // into a display list by TGLDisplayListCache/TGLDrawable base,
   // against our id and the LOD value. So this will only occur once
   // for a certain cylinder/LOD combination
   std::vector<TGLMesh *> meshParts;

   // Create mesh parts
   if (!fSegMesh) {
      meshParts.push_back(new TubeMesh(flags.LOD(), fR1, fR2, fR3, fR4, fDz, fLowPlaneNorm, fHighPlaneNorm));
   } else {
      meshParts.push_back(new TubeSegMesh(flags.LOD(), fR1, fR2, fR3, fR4, fDz, fPhi1,
                                          fPhi2, fLowPlaneNorm, fHighPlaneNorm));
   }

   // Draw mesh parts
   for (UInt_t i = 0; i < meshParts.size(); ++i) meshParts[i]->Draw();

   // Delete mesh parts
   for (UInt_t i = 0; i < meshParts.size(); ++i) {
      delete meshParts[i];
      meshParts[i] = 0;//not to have invalid pointer for pseudo-destructor call :)
   }
}
