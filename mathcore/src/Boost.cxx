// @(#)root/mathcore:$Name:  $:$Id: Boost.cxx,v 1.3 2005/12/08 15:52:41 moneta Exp $
// Authors:  M. Fischler  2005  

 /**********************************************************************
  *                                                                    *
  * Copyright (c) 2005 , LCG ROOT FNAL MathLib Team                    *
  *                                                                    *
  *                                                                    *
  **********************************************************************/

// Header file for class Boost, a 4x4 symmetric matrix representation of
// an axial Lorentz transformation
//
// Created by: Mark Fischler Mon Nov 1  2005
//
#include "Math/GenVector/Boost.h"
#include "Math/GenVector/LorentzVector.h"
#include "Math/GenVector/PxPyPzE4D.h"
#include "Math/GenVector/DisplacementVector3D.h"
#include "Math/GenVector/Cartesian3D.h"
#include "Math/GenVector/GenVector_exception.h"

#include <cmath>
#include <algorithm>

//#ifdef TEX
/**   

	A variable names bgamma appears in several places in this file. A few
	words of elaboration are needed to make its meaning clear.  On page 69
	of Misner, Thorne and Wheeler, (Exercise 2.7) the elements of the matrix
	for a general Lorentz boost are given as

	\f[	\Lambda^{j'}_k = \Lambda^{k'}_j
			     = (\gamma - 1) n^j n^k + \delta^{jk}  \f]

	where the n^i are unit vectors in the direction of the three spatial
	axes.  Using the definitions, \f$ n^i = \beta_i/\beta \f$ , then, for example,

	\f[	\Lambda_{xy} = (\gamma - 1) n_x n_y
			     = (\gamma - 1) \beta_x \beta_y/\beta^2  \f]

	By definition, \f[	\gamma^2 = 1/(1 - \beta^2)  \f]

	so that	\f[	\gamma^2 \beta^2 = \gamma^2 - 1  \f]

	or	\f[	\beta^2 = (\gamma^2 - 1)/\gamma^2  \f]

	If we insert this into the expression for \f$ \Lambda_{xy} \f$, we get

	\f[	\Lambda_{xy} = (\gamma - 1) \gamma^2/(\gamma^2 - 1) \beta_x \beta_y \f]

	or, finally

	\f[	\Lambda_{xy} = \gamma^2/(\gamma+1) \beta_x \beta_y  \f]

	The expression \f$ \gamma^2/(\gamma+1) \f$ is what we call <em>bgamma</em> in the code below.

	\class ROOT::Math::Boost
*/
//#endif

namespace ROOT {

  namespace Math {

void Boost::SetIdentity() {
  fM[XX] = 1.0;  fM[XY] = 0.0; fM[XZ] = 0.0; fM[XT] = 0.0;
                 fM[YY] = 1.0; fM[YZ] = 0.0; fM[YT] = 0.0;
                               fM[ZZ] = 1.0; fM[ZT] = 0.0;
                                             fM[TT] = 1.0;
}


void
Boost::SetComponents (Scalar bx, Scalar by, Scalar bz) {
  Scalar bp2 = bx*bx + by*by + bz*bz;
  if (bp2 >= 1) {
    GenVector_exception e ( 
      "Beta Vector supplied to set Boost represents speed >= c");
    Throw(e);
    // SetIdentity(); 
    return;
  }    
  Scalar gamma = 1.0 / std::sqrt(1.0 - bp2);
  Scalar bgamma = gamma * gamma / (1.0 + gamma);
  fM[XX] = 1.0 + bgamma * bx * bx;
  fM[YY] = 1.0 + bgamma * by * by;
  fM[ZZ] = 1.0 + bgamma * bz * bz;
  fM[XY] = bgamma * bx * by;
  fM[XZ] = bgamma * bx * bz;
  fM[YZ] = bgamma * by * bz;
  fM[XT] = gamma * bx;
  fM[YT] = gamma * by;
  fM[ZT] = gamma * bz;
  fM[TT] = gamma;
}

void
Boost::GetComponents (Scalar& bx, Scalar& by, Scalar& bz) const {
  Scalar gaminv = 1.0/fM[TT];
  bx = fM[XT]*gaminv;
  by = fM[YT]*gaminv;
  bz = fM[ZT]*gaminv;
}

DisplacementVector3D< Cartesian3D<Boost::Scalar> >
Boost::BetaVector() const {
  Scalar gaminv = 1.0/fM[TT];
  return DisplacementVector3D< Cartesian3D<Scalar> >
  			( fM[XT]*gaminv, fM[YT]*gaminv, fM[ZT]*gaminv );
}

void 
Boost::GetLorentzRotation (Scalar r[]) const {
  r[LXX] = fM[XX];  r[LXY] = fM[XY];  r[LXZ] = fM[XZ];  r[LXT] = fM[XT];  
  r[LYX] = fM[XY];  r[LYY] = fM[YY];  r[LYZ] = fM[YZ];  r[LYT] = fM[YT];  
  r[LZX] = fM[XZ];  r[LZY] = fM[YZ];  r[LZZ] = fM[ZZ];  r[LZT] = fM[ZT];  
  r[LTX] = fM[XT];  r[LTY] = fM[YT];  r[LTZ] = fM[ZT];  r[LTT] = fM[TT];  
}

void 
Boost::
Rectify() {
  // Assuming the representation of this is close to a true Lorentz Rotation,
  // but may have drifted due to round-off error from many operations,
  // this forms an "exact" orthosymplectic matrix for the Lorentz Rotation
  // again.

  if (fM[TT] <= 0) {	
    GenVector_exception e ( 
      "Attempt to rectify a boost with non-positive gamma");
    Throw(e);
    return;
  }    
  DisplacementVector3D< Cartesian3D<Scalar> > beta ( fM[XT], fM[YT], fM[ZT] );
  beta /= fM[TT];
  if ( beta.mag2() >= 1 ) {			    
    beta /= ( beta.R() * ( 1.0 + 1.0e-16 ) );  
  }
  SetComponents ( beta );
}

LorentzVector< PxPyPzE4D<double> >
Boost::
operator() (const LorentzVector< PxPyPzE4D<double> > & v) const {
  Scalar x = v.Px();
  Scalar y = v.Py();
  Scalar z = v.Pz();
  Scalar t = v.E();
  return LorentzVector< PxPyPzE4D<double> > 
    ( fM[XX]*x + fM[XY]*y + fM[XZ]*z + fM[XT]*t 
    , fM[XY]*x + fM[YY]*y + fM[YZ]*z + fM[YT]*t
    , fM[XZ]*x + fM[YZ]*y + fM[ZZ]*z + fM[ZT]*t
    , fM[XT]*x + fM[YT]*y + fM[ZT]*z + fM[TT]*t );
}

void 
Boost::
Invert() {
  fM[XT] = -fM[XT];
  fM[YT] = -fM[YT];
  fM[ZT] = -fM[ZT];
}

Boost
Boost::
Inverse() const {
  Boost I(*this);
  I.Invert();
  return I; 
}


// ========== I/O =====================

std::ostream & operator<< (std::ostream & os, const Boost & b) {
  // TODO - this will need changing for machine-readable issues
  //        and even the human readable form needs formatiing improvements
  double m[16];
  b.GetLorentzRotation(m);
  os << "\n" << m[0]  << "  " << m[1]  << "  " << m[2]  << "  " << m[3]; 
  os << "\n" << "\t"  << "  " << m[5]  << "  " << m[6]  << "  " << m[7]; 
  os << "\n" << "\t"  << "  " << "\t"  << "  " << m[10] << "  " << m[11]; 
  os << "\n" << "\t"  << "  " << "\t"  << "  " << "\t"  << "  " << m[15] << "\n";
  return os;
}

} //namespace Math
} //namespace ROOT
