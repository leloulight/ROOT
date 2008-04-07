#include <iostream>
#include <fstream>
#include <vector>

#include <cmath>

#include <TMath.h>
#include <Math/SpecFunc.h>

#include <TApplication.h>

#include <TCanvas.h>
#include <TH2F.h>
#include <TGraph.h>
#include <TLegend.h>

const double ERRORLIMIT = 1E-8;
const double MIN = 0;
const double MAX = 1;
const double INCREMENT = 0.01;
const int ARRAYSIZE = (int) (( MAX - MIN ) / INCREMENT);
inline int arrayindex(double i) { return ARRAYSIZE - (int) ( (MAX - i) / INCREMENT ); };

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

int testSpecFuncBetaI() 
{
   vector<Double_t> x( ARRAYSIZE );
   vector<Double_t> yb( ARRAYSIZE );
   vector<Double_t> ymb( ARRAYSIZE );

   int status = 0;

   double b = 0.2, a= 0.9;
   cout << "** b = " << b << " **" << endl;
   for ( double i = MIN; i < MAX; i += INCREMENT )
   {
      cout << "i:"; cout.width(5); cout << i 
           << " index: "; cout.width(5); cout << arrayindex(i) 
           << " TMath::BetaIncomplete(x,a,b): "; cout.width(10); cout << TMath::BetaIncomplete(i,a,b)
           << " ROOT::Math::inc_beta(a,a,b): "; cout.width(10); cout << ROOT::Math::inc_beta(i,a,b)
           << endl;
      
      x[arrayindex(i)] = i;

      yb[arrayindex(i)] = TMath::BetaIncomplete(i,a,b);
      ymb[arrayindex(i)] = ROOT::Math::inc_beta(i,a,b);
      if ( std::fabs( yb[arrayindex(i)] - ymb[arrayindex(i)] ) > ERRORLIMIT )
      {
         cout << "i " << i   
              << " yb[arrayindex(i)] " << yb[arrayindex(i)]
              << " ymb[arrayindex(i)] " << ymb[arrayindex(i)]
              << " " << std::fabs( yb[arrayindex(i)] - ymb[arrayindex(i)] )
              << endl;
         status += 1;
      }
   }

   if ( showGraphics )
   {

      TCanvas* c1 = new TCanvas("c1", "Two Graphs", 600, 400); 
      TH2F* hpx = new TH2F("hpx", "Two Graphs(hpx)", ARRAYSIZE, MIN, MAX, ARRAYSIZE, 0, 5);
      hpx->SetStats(kFALSE);
      hpx->Draw();
      
      int color = 2;
      
      TGraph *gb, *gmb;
      gb = drawPoints(&x[0], &yb[0], color++);
      gmb = drawPoints(&x[0], &ymb[0], color++, 7);
      
      TLegend* legend = new TLegend(0.61,0.72,0.86,0.86);
      legend->AddEntry(gb, "TMath::BetaIncomplete()");
      legend->AddEntry(gmb, "ROOT::Math::inc_beta()");
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

   int status = testSpecFuncBetaI();

   if ( showGraphics )
   {
      theApp->Run();
      delete theApp;
      theApp = 0;
   }

   return status;
}
