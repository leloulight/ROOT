// Implementation file for class
// IntegratorMultiDim
//
#include "Math/IFunction.h"
#include "Math/Integrator.h"
#include "Math/IntegratorMultiDim.h"
#include "Math/WrappedParamFunction.h"

#include <cmath>

namespace ROOT {
namespace Math {



IntegratorMultiDim::IntegratorMultiDim(unsigned int dim, double absTol, double relTol, size_t size):
   fdim(dim), 
   fAbsTol(absTol),
   fRelTol(relTol),
   fSize(size)

{
   // constructor - without passing a function
   fFun = 0; 
}

IntegratorMultiDim::IntegratorMultiDim( const IMultiGenFunction &f, unsigned int dim, double absTol, double relTol, size_t size):
   fdim(dim), 
   fAbsTol(absTol),
   fRelTol(relTol),
   fSize(size),
   fFun(&f)
{
   // constructur passing a multi-dimensional function interface
}



//double IntegratorMultiDim::Result() const { return fIntegrator->Result(); }
//double IntegratorMultiDim::Error() const { return fIntegrator->Error(); }

void IntegratorMultiDim::SetFunction(const IMultiGenFunction &f)
{
   // set the integration function
  fFun = &f;
}

void IntegratorMultiDim::SetRelTolerance(double relTol){ this->fRelTol = relTol; }


void IntegratorMultiDim::SetAbsTolerance(double absTol){ this->fAbsTol = absTol; }


double IntegratorMultiDim::Integral(unsigned int dim, double* xmin, double * xmax)
{
   // References:
   //
   //   1.A.C. Genz and A.A. Malik, Remarks on algorithm 006:
   //     An adaptive algorithm for numerical integration over
   //     an N-dimensional rectangular region, J. Comput. Appl. Math. 6 (1980) 295-302.
   //   2.A. van Doren and L. de Ridder, An adaptive algorithm for numerical
   //     integration over an n-dimensional cube, J.Comput. Appl. Math. 2 (1976) 207-217.
  
   //to be changed later
   unsigned int n=dim;
   bool kFALSE = false;
   bool kTRUE = true;

   unsigned int minpts = 0;
   unsigned int maxpts = fSize;//specified maximal number of function evaluations
   double eps = fRelTol; //specified relative accuracy
   //output parameters
   int ifail; //report status
   unsigned int nfnevl; //nr of function evaluations
   double relerr; //an estimation of the relative accuracy of the result

   bool fgAbsValue=0;//for now; maybe later new class member as in TF1..

   double ctr[15], wth[15], wthl[15], z[15];

   static const double xl2 = 0.358568582800318073;//lambda_2
   static const double xl4 = 0.948683298050513796;//lambda_4
   static const double xl5 = 0.688247201611685289;//lambda_5
   static const double w2  = 980./6561; //weights/2^n
   static const double w4  = 200./19683;
   static const double wp2 = 245./486;//error weights/2^n
   static const double wp4 = 25./729;

   static const double wn1[14] = {     -0.193872885230909911, -0.555606360818980835,
     -0.876695625666819078, -1.15714067977442459,  -1.39694152314179743,
     -1.59609815576893754,  -1.75461057765584494,  -1.87247878880251983,
     -1.94970278920896201,  -1.98628257887517146,  -1.98221815780114818,
     -1.93750952598689219,  -1.85215668343240347,  -1.72615963013768225};

   static const double wn3[14] = {     0.0518213686937966768,  0.0314992633236803330,
     0.0111771579535639891,-0.00914494741655235473,-0.0294670527866686986,
    -0.0497891581567850424,-0.0701112635269013768, -0.0904333688970177241,
    -0.110755474267134071, -0.131077579637250419,  -0.151399685007366752,
    -0.171721790377483099, -0.192043895747599447,  -0.212366001117715794};

   static const double wn5[14] = {         0.871183254585174982e-01,  0.435591627292587508e-01,
     0.217795813646293754e-01,  0.108897906823146873e-01,  0.544489534115734364e-02,
     0.272244767057867193e-02,  0.136122383528933596e-02,  0.680611917644667955e-03,
     0.340305958822333977e-03,  0.170152979411166995e-03,  0.850764897055834977e-04,
     0.425382448527917472e-04,  0.212691224263958736e-04,  0.106345612131979372e-04};

   static const double wpn1[14] = {   -1.33196159122085045, -2.29218106995884763,
     -3.11522633744855959, -3.80109739368998611, -4.34979423868312742,
     -4.76131687242798352, -5.03566529492455417, -5.17283950617283939,
     -5.17283950617283939, -5.03566529492455417, -4.76131687242798352,
     -4.34979423868312742, -3.80109739368998611, -3.11522633744855959};

   static const double wpn3[14] = {     0.0445816186556927292, -0.0240054869684499309,
    -0.0925925925925925875, -0.161179698216735251,  -0.229766803840877915,
    -0.298353909465020564,  -0.366941015089163228,  -0.435528120713305891,
    -0.504115226337448555,  -0.572702331961591218,  -0.641289437585733882,
    -0.709876543209876532,  -0.778463648834019195,  -0.847050754458161859};

   double result = 0;
   double abserr = 0;
   ifail  = 3;
   nfnevl = 0;
   relerr = 0;
   if (dim < 2 || dim > 15) return 0;

   double twondm = std::pow(2.0,static_cast<int>(n));
   //unsigned int minpts = Int_t(twondm)+ 2*n*(n+1)+1;
   unsigned int ifncls = 0;
   bool  ldv   = kFALSE;
   unsigned int irgnst = 2*n+3;
   unsigned int  irlcls = (unsigned int)(twondm) +2*n*(n+1)+1;//minimal number of nodes in n dim
   unsigned int isbrgn = irgnst;
   unsigned int isbrgs = irgnst;

   if (minpts < 1)      minpts = irlcls;
   if (maxpts < minpts) maxpts = 10*minpts;

   // The original agorithm expected a working space array WK of length IWK
   // with IWK Length ( >= (2N + 3) * (1 + MAXPTS/(2**N + 2N(N + 1) + 1))/2).
   // Here, this array is allocated dynamically

   unsigned int iwk = irgnst*(1 +maxpts/irlcls)/2;
   double *wk = new double[iwk+10];

   unsigned int j; 
   for (j=0; j<n; j++) {
     ctr[j] = (xmax[j] + xmin[j])*0.5;//center of a hypercube
     wth[j] = (xmax[j] - xmin[j])*0.5;//its width
   }

   double rgnvol, sum1, sum2, sum3, sum4, sum5, difmax, f2, f3, dif, aresult;
   double rgncmp=0, rgnval, rgnerr;

   unsigned int j1, k, l, m, idvaxn=0, idvax0=0, isbtmp, isbtpp;
 
   //InitArgs(z,fParams);

L20:
   rgnvol = twondm;//=2^n
   for (j=0; j<n; j++) {
     rgnvol *= wth[j]; //region volume
     z[j]    = ctr[j]; //temporary node
   }
   sum1 = (*fFun)((const double*)z);//EvalPar(z,fParams); //evaluate function

   difmax = 0;
   sum2   = 0;
   sum3   = 0;

   //loop over coordinates
   for (j=0; j<n; j++) {
      z[j]    = ctr[j] - xl2*wth[j];
      if (fgAbsValue) f2 = std::abs((*fFun)(z));
      else            f2 = (*fFun)(z);
      z[j]    = ctr[j] + xl2*wth[j];
      if (fgAbsValue) f2 += std::abs((*fFun)(z));
      else            f2 += (*fFun)(z);
      wthl[j] = xl4*wth[j];
      z[j]    = ctr[j] - wthl[j]; 
      if (fgAbsValue) f3 = std::abs((*fFun)(z));
      else            f3 = (*fFun)(z);
      z[j]    = ctr[j] + wthl[j];
      if (fgAbsValue) f3 += std::abs((*fFun)(z));
      else            f3 += (*fFun)(z);
      sum2   += f2;//sum func eval with different weights separately
      sum3   += f3;//for a given region
      dif     = std::abs(7*f2-f3-12*sum1);
      //storing dimension with biggest error/difference (?)
      if (dif >= difmax) {
         difmax=dif;
         idvaxn=j+1;
      }
      z[j]    = ctr[j];
   }

   sum4 = 0;
   for (j=1;j<n;j++) {
      j1 = j-1;
      for (k=j;k<n;k++) {
         for (l=0;l<2;l++) {
            wthl[j1] = -wthl[j1];
            z[j1]    = ctr[j1] + wthl[j1];
            for (m=0;m<2;m++) {
               wthl[k] = -wthl[k];
               z[k]    = ctr[k] + wthl[k];
               if (fgAbsValue) sum4 += std::abs((*fFun)(z));
               else            sum4 += (*fFun)(z);
            }
         }
         z[k] = ctr[k];
      }
      z[j1] = ctr[j1];
   }

   sum5 = 0;

   for (j=0;j<n;j++) {
      wthl[j] = -xl5*wth[j];
      z[j] = ctr[j] + wthl[j];
   }
 L90: //sum over end nodes ~gray codes
   if (fgAbsValue) sum5 += std::abs((*fFun)(z));
   else            sum5 += (*fFun)(z);
   for (j=0;j<n;j++) {
      wthl[j] = -wthl[j];
      z[j] = ctr[j] + wthl[j];
      if (wthl[j] > 0) goto L90;
   }

   rgncmp  = rgnvol*(wpn1[n-2]*sum1+wp2*sum2+wpn3[n-2]*sum3+wp4*sum4);
   rgnval  = wn1[n-2]*sum1+w2*sum2+wn3[n-2]*sum3+w4*sum4+wn5[n-2]*sum5;
   rgnval *= rgnvol;
   // avoid difference of too small numbers
   //rgnval = 1.0E-30;
   //rgnerr  = TMath::Max( std::abs(rgnval-rgncmp), TMath::Max(std::abs(rgncmp), std::abs(rgnval) )*4.0E-16 );
   rgnerr  = std::abs(rgnval-rgncmp);//compares estim error with expected error

   result += rgnval;
   abserr += rgnerr;
   ifncls += irlcls;
   aresult = std::abs(result);
   //if (result > 0 && aresult< 1e-100) {
   //   delete [] wk;
   //   ifail = 0;  //function is probably symmetric ==> integral is null: not an error
   //   return result;
   //}

   //if division
   if (ldv) {
L110:
      isbtmp = 2*isbrgn;
      if (isbtmp > isbrgs) goto L160;
      if (isbtmp < isbrgs) {
         isbtpp = isbtmp + irgnst;
         if (wk[isbtmp-1] < wk[isbtpp-1]) isbtmp = isbtpp;
      }
      if (rgnerr >= wk[isbtmp-1]) goto L160;
      for (k=0;k<irgnst;k++) {
         wk[isbrgn-k-1] = wk[isbtmp-k-1];
      }
      isbrgn = isbtmp;
      goto L110;
   }
L140:
   isbtmp = (isbrgn/(2*irgnst))*irgnst;
   if (isbtmp >= irgnst && rgnerr > wk[isbtmp-1]) {
      for (k=0;k<irgnst;k++) {
         wk[isbrgn-k-1] = wk[isbtmp-k-1];
      }
      isbrgn = isbtmp;
      goto L140;
   }

 L160: //to divide or not
   wk[isbrgn-1] = rgnerr;//storing value & error in last 
   wk[isbrgn-2] = rgnval;//table records
   wk[isbrgn-3] = double(idvaxn);//coordinate with biggest error
   for (j=0;j<n;j++) {
      isbtmp = isbrgn-2*j-4;
      wk[isbtmp]   = ctr[j];
      wk[isbtmp-1] = wth[j];
   }
   if (ldv) {//divison along chosen coordinate
     ldv = kFALSE;
      ctr[idvax0-1] += 2*wth[idvax0-1];
      isbrgs += irgnst;//updating the number of nodes/regions(?)
      isbrgn  = isbrgs;
      goto L20;
   }
   //if no divisions to be made..
   relerr = abserr;
   if (aresult != 0)  relerr = abserr/aresult;


   if (relerr < 1e-1 && aresult < 1e-20) ifail = 0;
   if (relerr < 1e-3 && aresult < 1e-10) ifail = 0;
   if (relerr < 1e-5 && aresult < 1e-5)  ifail = 0;
   if (isbrgs+irgnst > iwk) ifail = 2;
   if (ifncls+2*irlcls > maxpts) {
      if (sum1==0 && sum2==0 && sum3==0 && sum4==0 && sum5==0){
         ifail = 0;
         result = 0;
      }
      else
         ifail = 1;
   }
   //..and accuracy appropriare
   if (relerr < eps && ifncls >= minpts) ifail = 0;
   if (ifail == 3) {
     ldv = kTRUE;
      isbrgn  = irgnst;
      abserr -= wk[isbrgn-1];
      result -= wk[isbrgn-2];
      idvax0  = (unsigned int)(wk[isbrgn-3]);
      for (j=0;j<n;j++) {
         isbtmp = isbrgn-2*j-4;
         ctr[j] = wk[isbtmp];
         wth[j] = wk[isbtmp-1];
      }
      wth[idvax0-1]  = 0.5*wth[idvax0-1];
      ctr[idvax0-1] -= wth[idvax0-1];
      goto L20;
   }
   nfnevl = ifncls;       //number of function evaluations performed.
   fResult = result;
   fError = abserr;//wk[isbrgn-1];
   fNEval = nfnevl;
   delete [] wk;
  
   return result;         //an approximate value of the integral
}


  
double IntegratorMultiDim::Integral(const IMultiGenFunction &f, unsigned int dim, double* xmin, double * xmax)
{
  fFun = &f;
  return Integral(dim, xmin, xmax);

}



} // namespace Math
} // namespace ROOT



