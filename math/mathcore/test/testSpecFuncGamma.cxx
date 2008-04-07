#include <iostream>
#include <fstream>
#include <vector>

#include <cmath>

#include <TMath.h>
#include <Math/SpecFuncMathCore.h>

#include <TApplication.h>

#include <TCanvas.h>
#include <TH2F.h>
#include <TGraph.h>
#include <TLegend.h>

const double ERRORLIMIT = 1E-8;
const double MIN = -2.5;
const double MAX = +2.5;
const double INCREMENT = 0.01;
const int ARRAYSIZE = (int) (( MAX - MIN ) / INCREMENT);
inline int arrayindex(double i) { return ARRAYSIZE - (int) ( (MAX - i) / INCREMENT ) ; };

bool showGraphics = true;

using namespace std;

TGraph* drawPoints(Double_t x[], Double_t y[], int color, int style = 1)
{
   TGraph* g = new TGraph(ARRAYSIZE, x, y);
   g->SetLineColor(color);
   g->SetLineStyle(style);
   g->SetLineWidth(3);
   g->Draw("SAME");

   return g;
}

int testSpecFuncGamma() 
{
   vector<Double_t> x( ARRAYSIZE );
   vector<Double_t> yg( ARRAYSIZE );
   vector<Double_t> ymtg( ARRAYSIZE );
   vector<Double_t> yga( ARRAYSIZE );
   vector<Double_t> ymga( ARRAYSIZE );
   vector<Double_t> ylng( ARRAYSIZE );
   vector<Double_t> ymlng( ARRAYSIZE );

   Double_t a = 0.56;

   int status = 0;

   //ofstream cout ("values.txt");

   for ( double i = MIN; i < MAX; i += INCREMENT )
   {
//       cout << "i:"; cout.width(5); cout << i 
//            << " index: "; cout.width(5); cout << arrayindex(i) 
//            << " TMath::Gamma(x): "; cout.width(10); cout << TMath::Gamma(i)
//            << " ROOT::Math::tgamma(x): "; cout.width(10); cout << ROOT::Math::tgamma(i)
//            << " TMath::Gamma(a, x): "; cout.width(10); cout << TMath::Gamma(a, i)
//            << " ROOT::Math::Inc_Gamma(a, x): "; cout.width(10); cout << ROOT::Math::inc_gamma(a, i)
//            << " TMath::LnGamma(x): "; cout.width(10); cout << TMath::LnGamma(i)
//            << " ROOT::Math::lgamma(x): "; cout.width(10); cout << ROOT::Math::lgamma(i)
//            << endl;

      x[arrayindex(i)] = i;
      yg[arrayindex(i)] = TMath::Gamma(i);
      ymtg[arrayindex(i)] = ROOT::Math::tgamma(i);
      // take the infinity values out of the error checking!
      if ( std::fabs(yg[arrayindex(i)]) < 1E+12 && std::fabs( yg[arrayindex(i)] - ymtg[arrayindex(i)] ) > ERRORLIMIT )
      {
         cout << "i " << i   
              << " yg[arrayindex(i)] " << yg[arrayindex(i)]
              << " ymtg[arrayindex(i)] " << ymtg[arrayindex(i)]
              << " " << std::fabs( yg[arrayindex(i)] - ymtg[arrayindex(i)] )
              << endl;
         status += 1;
      }

      yga[arrayindex(i)] = TMath::Gamma(a, i);
      ymga[arrayindex(i)] = ROOT::Math::inc_gamma(a, i);
      if ( std::fabs( yga[arrayindex(i)] - ymga[arrayindex(i)] ) > ERRORLIMIT )
      {
         cout << "i " << i   
              << " yga[arrayindex(i)] " << yga[arrayindex(i)]
              << " ymga[arrayindex(i)] " << ymga[arrayindex(i)]
              << " " << std::fabs( yga[arrayindex(i)] - ymga[arrayindex(i)] )
              << endl;
         status += 1;
      }

      ylng[arrayindex(i)] = TMath::LnGamma(i);
      ymlng[arrayindex(i)] = ROOT::Math::lgamma(i);
      if ( std::fabs( ylng[arrayindex(i)] - ymlng[arrayindex(i)] ) > ERRORLIMIT )
      {
         cout << "i " << i   
              << " ylng[arrayindex(i)] " << ylng[arrayindex(i)]
              << " ymlng[arrayindex(i)] " << ymlng[arrayindex(i)]
              << " " << std::fabs( ylng[arrayindex(i)] - ymlng[arrayindex(i)] )
              << endl;
         status += 1;
      }


   }

   if ( showGraphics )
   {
      
      TCanvas* c1 = new TCanvas("c1", "Two Graphs", 600, 400); 
      TH2F* hpx = new TH2F("hpx", "Two Graphs(hpx)", ARRAYSIZE, MIN, MAX, ARRAYSIZE, -1,5);
      hpx->SetStats(kFALSE);
      hpx->Draw();
      
      TGraph* gg    = drawPoints(&x[0], &yg[0], 1);
      TGraph* gmtg  = drawPoints(&x[0], &ymtg[0], 2, 7);
      TGraph* gga   = drawPoints(&x[0], &yga[0], 3);
      TGraph* gmga  = drawPoints(&x[0], &ymga[0], 4, 7);
      TGraph* glng  = drawPoints(&x[0], &ylng[0], 5);
      TGraph* gmlng = drawPoints(&x[0], &ymlng[0], 6, 7);
      
      TLegend* legend = new TLegend(0.61,0.52,0.86,0.86);
      legend->AddEntry(gg,    "TMath::Gamma()");
      legend->AddEntry(gmtg,  "ROOT::Math::tgamma()");
      legend->AddEntry(gga,   "TMath::GammaI()");
      legend->AddEntry(gmga,  "ROOT::Math::inc_gamma()");
      legend->AddEntry(glng,  "TMath::LnGamma()");
      legend->AddEntry(gmlng, "ROOT::Math::lgamma()");
      legend->Draw();
      
      c1->Show();
   }

   cout << "Test Done!" << endl;

   return status;
}


int main(int argc, char **argv) 
{
   if ( argc > 1 && argc != 2 )
   {
      cerr << "Usage: " << argv[0] << " [-ng]\n";
      cerr << "  where:\n";
      cerr << "     -ng : no graphics mode";
      cerr << endl;
      exit(1);
   }

   if ( argc == 2 && strcmp( argv[1], "-ng") == 0 ) 
   {
      showGraphics = false;
   }

   TApplication* theApp = 0;
   if ( showGraphics )
      theApp = new TApplication("App",&argc,argv);

   int status = testSpecFuncGamma();

   if ( showGraphics )
   {
      theApp->Run();
      delete theApp;
      theApp = 0;
   }

   return status;
}
