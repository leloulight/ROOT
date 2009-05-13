// @(#)root/proofd:$Id$
// Author: G. Ganis  Jan 2008

/*************************************************************************
 * Copyright (C) 1995-2005, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_XrdProofdNetMgr
#define ROOT_XrdProofdNetMgr

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// XrdProofdNetMgr                                                     //
//                                                                      //
// Authors: G. Ganis, CERN, 2008                                        //
//                                                                      //
// Manages connections between PROOF server daemons                     //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifdef OLDXRDOUC
#  include "XrdSysToOuc.h"
#  include "XrdOuc/XrdOucPthread.hh"
#else
#  include "XrdSys/XrdSysPthread.hh"
#endif

#include "XrdOuc/XrdOucHash.hh"

#include "XrdProofConn.h"
#include "XrdProofdConfig.h"

class XrdProofdDirective;
class XrdProofdManager;
class XrdProofdProtocol;
class XrdProofdResponse;
class XrdProofWorker;

class XrdProofdNetMgr : public XrdProofdConfig {

private:

   XrdSysRecMutex     fMutex;          // Atomize this instance

   XrdProofdManager  *fMgr;
   XrdOucHash<XrdProofConn> fProofConnHash;            // Available connections
   int                fNumLocalWrks;   // Number of workers to be started locally
   int                fResourceType;   // resource type
   XrdProofdFile      fPROOFcfg;       // PROOF static configuration
   bool               fReloadPROOFcfg; // Whether the file should regurarl checked for updates
   bool               fWorkerUsrCfg;   // user cfg files enabled / disabled
   int                fRequestTO;      // Timeout on broadcast request

   std::list<XrdProofWorker *> fWorkers;               // List of possible workers
   std::list<XrdProofWorker *> fNodes;                 // List of worker unique nodes

   void               CreateDefaultPROOFcfg();
   int                ReadPROOFcfg(bool reset = 1);
   int                FindUniqueNodes();

   int                DoDirectiveAdminReqTO(char *, XrdOucStream *, bool);
   int                DoDirectiveResource(char *, XrdOucStream *, bool);
   int                DoDirectiveWorker(char *, XrdOucStream *, bool);

public:
   XrdProofdNetMgr(XrdProofdManager *mgr, XrdProtocol_Config *pi, XrdSysError *e);
   virtual ~XrdProofdNetMgr();

   int                Config(bool rcf = 0);
   int                DoDirective(XrdProofdDirective *d,
                                  char *val, XrdOucStream *cfg, bool rcf);
   void               RegisterDirectives();

   void               Dump();

   const char        *PROOFcfg() const { return fPROOFcfg.fName.c_str(); }
   bool               WorkerUsrCfg() const { return fWorkerUsrCfg; }

   int                Broadcast(int type, const char *msg, const char *usr = 0,
                                XrdProofdResponse *r = 0, bool notify = 0);
   XrdProofConn      *GetProofConn(const char *url);
   XrdClientMessage  *Send(const char *url, int type,
                           const char *msg, int srvtype, XrdProofdResponse *r, bool notify = 0);

   int                ReadBuffer(XrdProofdProtocol *p);
   char              *ReadBufferLocal(const char *file, kXR_int64 ofs, int &len);
   char              *ReadBufferLocal(const char *file, const char *pat, int &len, int opt);
   char              *ReadBufferRemote(const char *url, const char *file,
                                       kXR_int64 ofs, int &len, int grep);
   char              *ReadLogPaths(const char *url, const char *stag, int isess);

   // List of available and unique workers (on master only)
   std::list<XrdProofWorker *> *GetActiveWorkers();
   std::list<XrdProofWorker *> *GetNodes();

};

#endif
