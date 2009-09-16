// @(#)root/proof:$Id$
// Author: Gerardo Ganis  March 2008

/*************************************************************************
 * Copyright (C) 1995-2005, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TSlaveLite                                                           //
//                                                                      //
// This is the version of TSlave for local worker servers.              //
// See TSlave for details.                                              //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "RConfigure.h"
#include "TSlaveLite.h"
#include "TProof.h"
#include "TProofServ.h"
#include "TSystem.h"
#include "TEnv.h"
#include "TROOT.h"
#include "TUrl.h"
#include "TMessage.h"
#include "TMonitor.h"
#include "TError.h"
#include "TSocket.h"
#include "TSysEvtHandler.h"
#include "TVirtualMutex.h"
#include "TThread.h"
#ifdef WIN32
#include <process.h>
#endif

ClassImp(TSlaveLite)

//______________________________________________________________________________
//---- error handling ----------------------------------------------------------
//---- Needed to avoid blocking on the CINT mutex in printouts -----------------

//______________________________________________________________________________
void TSlaveLite::DoError(int level, const char *location,
                                    const char *fmt, va_list va) const
{
   // Interface to ErrorHandler (protected).

   ::ErrorHandler(level, Form("TSlaveLite::%s", location), fmt, va);
}

//______________________________________________________________________________
TSlaveLite::TSlaveLite(const char *ord, Int_t perf,
               const char *image, TProof *proof, Int_t stype,
               const char *workdir, const char *msd) : TSlave()
{
   // Create a PROOF slave object. Called via the TProof ctor.
   fName = ord;
   fImage = image;
   fProofWorkDir = workdir;
   fWorkDir = workdir;
   fOrdinal = ord;
   fPerfIdx = perf;
   fProof = proof;
   fSlaveType = (ESlaveType)stype;
   fMsd = msd;
   fIntHandler = 0;
   fValid = kFALSE;
   fProtocol = kPROOF_Protocol;

   if (fPerfIdx > 0) Init();
}

//______________________________________________________________________________
void TSlaveLite::Init()
{
   // Init a PROOF worker object. Called via the TSlaveLite ctor.

   // Command to be executed
#ifdef WIN32
   char *args[7];
   const char *cmd = "proofserv";
   args[0] = StrDup(cmd);
   args[1] = "proofslave";
   args[2] = "lite";
   args[3] = Form("%d", gSystem->GetPid());
   args[4] = Form("%d", gDebug);
   args[5] = Form("%s/worker-%s.env", fWorkDir.Data(), fOrdinal.Data());
   args[6] = NULL;
   _flushall();
   if (_spawnvp(_P_NOWAIT, cmd, args) == -1) {
      Info("Init", "an error occured while executing 'proofserv'");
      SetBit(kInvalidObject);
      return;
   }
   // sleep a bit to let start the process...
   gSystem->Sleep(100);
#else
   TString cmd;
#ifdef R__HAVE_CONFIG
   cmd.Form("export ROOTBINDIR=\"%s\"; %s/proofserv proofslave lite %d %d %s/worker-%s.env &",
            ROOTBINDIR, ROOTBINDIR,
#else
   cmd.Form("export ROOTBINDIR=\"%s/bin\"; %s/bin/proofserv proofslave lite %d %d %s/worker-%s.env &",
            gSystem->Getenv("ROOTSYS"), gSystem->Getenv("ROOTSYS"),
#endif
            gSystem->GetPid(), gDebug, fWorkDir.Data(), fOrdinal.Data());

   // Execute
   if (gSystem->Exec(cmd) != 0) {
      Info("Init", "an error occured while executing 'proofserv'");
      SetBit(kInvalidObject);
      return;
   }
#endif
}

//______________________________________________________________________________
Int_t TSlaveLite::SetupServ(Int_t, const char *)
{
   // Init a PROOF slave object. Called via the TSlaveLite ctor.
   // The Init method is technology specific and is overwritten by derived
   // classes.

   // Get back startup message of proofserv (we are now talking with
   // the real proofserver and not anymore with the proofd front-end)
   Int_t what;
   char buf[512];
   if (fSocket->Recv(buf, sizeof(buf), what) <= 0) {
      Error("SetupServ", "failed to receive slave startup message");
      Close("S");
      SafeDelete(fSocket);
      fValid = kFALSE;
      return -1;
   }

   if (what == kMESS_NOTOK) {
      SafeDelete(fSocket);
      fValid = kFALSE;
      return -1;
   }

   // Receive the unique tag and save it as name of this object
   TMessage *msg = 0;
   if (fSocket->Recv(msg) <= 0 || !msg || msg->What() != kPROOF_SESSIONTAG) {
      Error("SetupServ", "failed to receive unique session tag");
      Close("S");
      SafeDelete(fSocket);
      fValid = kFALSE;
      return -1;
   }
   // Extract the unique tag
   (*msg) >> fName;

   // We are done
   return 0;
}

//______________________________________________________________________________
TSlaveLite::~TSlaveLite()
{
   // Destroy slave.

   Close();
}

//______________________________________________________________________________
void TSlaveLite::Close(Option_t *opt)
{
   // Close slave socket.

   if (fSocket)
      // Closing socket ...
      fSocket->Close(opt);

   SafeDelete(fInput);
   SafeDelete(fSocket);
}

//______________________________________________________________________________
void TSlaveLite::Print(Option_t *) const
{
   // Printf info about slave.

   const char *sst[] = { "invalid" , "valid", "inactive" };
   Int_t st = fSocket ? ((fStatus == kInactive) ? 2 : 1) : 0;

   Printf("*** Worker %s  (%s)", fOrdinal.Data(), sst[st]);
   Printf("    Worker session tag:      %s", GetName());
   Printf("    ROOT version|rev|tag:    %s", GetROOTVersion());
   Printf("    Architecture-Compiler:   %s", GetArchCompiler());
   if (fSocket) {
      Printf("    Working directory:       %s", GetWorkDir());
      Printf("    MB's processed:          %.2f", float(GetBytesRead())/(1024*1024));
      Printf("    MB's sent:               %.2f", float(fSocket->GetBytesRecv())/(1024*1024));
      Printf("    MB's received:           %.2f", float(fSocket->GetBytesSent())/(1024*1024));
      Printf("    Real time used (s):      %.3f", GetRealTime());
      Printf("    CPU time used (s):       %.3f", GetCpuTime());
   }
}
