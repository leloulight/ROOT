// @(#)root/proofd:$Id$
// Author: Gerardo Ganis  12/12/2005

/*************************************************************************
 * Copyright (C) 1995-2005, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/
#include <sys/stat.h>

#include "XrdNet/XrdNet.hh"

#include "XrdProofdAux.h"
#include "XrdProofdProofServ.h"
#include "XrdProofWorker.h"
#include "XrdProofSched.h"
#include "XrdProofdManager.h"

// Tracing utils
#include "XrdProofdTrace.h"

#ifndef SafeDelete
#define SafeDelete(x) { if (x) { delete x; x = 0; } }
#endif
#ifndef SafeDelArray
#define SafeDelArray(x) { if (x) { delete[] x; x = 0; } }
#endif

//__________________________________________________________________________
XrdProofdProofServ::XrdProofdProofServ()
{
   // Constructor

   fMutex = new XrdSysRecMutex;
   fResponse = 0;
   fProtocol = 0;
   fParent = 0;
   fPingSem = 0;
   fQueryNum = 0;
   fStartMsg = 0;
   fStatus = kXPD_idle;
   fSrvPID = -1;
   fSrvType = kXPD_AnyServer;
   fID = -1;
   fIsShutdown = false;
   fIsValid = true;  // It is created for a valid server ...
   fSkipCheck = false;
   fProtVer = -1;
   fNClients = 0;
   fClients.reserve(10);
   fDisconnectTime = -1;
   fSetIdleTime = time(0);
   fROOT = 0;
   // Strings
   fAdminPath = "";
   fAlias = "";
   fClient = "";
   fFileout = "";
   fGroup = "";
   fOrdinal = "";
   fTag = "";
   fUserEnvs = "";
   fUNIXSock = 0;
   fUNIXSockPath = "";
}

//__________________________________________________________________________
XrdProofdProofServ::~XrdProofdProofServ()
{
   // Destructor

   SafeDelete(fQueryNum);
   SafeDelete(fStartMsg);
   SafeDelete(fPingSem);

   std::vector<XrdClientID *>::iterator i;
   for (i = fClients.begin(); i != fClients.end(); i++)
       if (*i)
          delete (*i);
   fClients.clear();

   // Cleanup worker info
   ClearWorkers();

   // Cleanup queries info
   fQueries.clear();

   // Remove the associated UNIX socket path
   unlink(fUNIXSockPath.c_str());

   SafeDelete(fMutex);
}

//__________________________________________________________________________
static int DecreaseWorkerCounters(const char *, XrdProofWorker *w, void *x)
{
   // Decrease active session counters on worker w
   XPDLOC(PMGR, "DecreaseWorkerCounters")

   XrdProofdProofServ *xps = (XrdProofdProofServ *)x;

   if (w && xps) {
      w->RemoveProofServ(xps);
      TRACE(REQ, w->fHost.c_str() <<" done");
      // Check next
      return 0;
   }

   // Not enough info: stop
   return 1;
}

//__________________________________________________________________________
static int DumpWorkerCounters(const char *k, XrdProofWorker *w, void *)
{
   // Decrease active session counters on worker w
   XPDLOC(PMGR, "DumpWorkerCounters")

   if (w) {
      TRACE(ALL, k <<" : "<<w->fHost.c_str()<<":"<<w->fPort <<" act: "<<w->Active());
      // Check next
      return 0;
   }

   // Not enough info: stop
   return 1;
}

//__________________________________________________________________________
void XrdProofdProofServ::ClearWorkers()
{
   // Decrease worker counters and clean-up the list

   XrdSysMutexHelper mhp(fMutex);

   // Decrease workers' counters and remove this from workers
   fWorkers.Apply(DecreaseWorkerCounters, this);
   fWorkers.Purge();
   if (fSrvType == kXPD_TopMaster && fStatus == kXPD_running
       && fWrksStr.length())
      fProtocol->Mgr()->ProofSched()->Reschedule();
   fWrksStr = "";
}

//__________________________________________________________________________
void XrdProofdProofServ::AddWorker(const char *o, XrdProofWorker *w)
{
   // Add a worker assigned to this session with label 'o'

   if (!o || !w) return;

   XrdSysMutexHelper mhp(fMutex);

   fWorkers.Add(o, w, 0, Hash_keepdata);
}

//__________________________________________________________________________
void XrdProofdProofServ::RemoveWorker(const char *o)
{
   // Release worker assigned to this session with label 'o'
   XPDLOC(SMGR, "ProofServ::RemoveWorker")

   if (!o) return;

   TRACE(DBG,"removing: "<<o);

   XrdSysMutexHelper mhp(fMutex);

   XrdProofWorker *w = fWorkers.Find(o);
   if (w)
      w->RemoveProofServ(this);
   fWorkers.Del(o);
   if (TRACING(HDBG)) fWorkers.Apply(DumpWorkerCounters, 0);
}

//__________________________________________________________________________
int XrdProofdProofServ::Reset(const char *msg, int type)
{
   // Reset this instance, broadcasting a message to the clients.
   // return 1 if top master, 0 otherwise

   int rc = 0;
   XrdSysMutexHelper mhp(fMutex);
   // Broadcast msg
   Broadcast(msg, type);
   // What kind of server is this?
   if (fSrvType == kXPD_TopMaster) rc = 1;
   // Reset instance
   Reset();
   // Done
   return rc;
}

//__________________________________________________________________________
void XrdProofdProofServ::Reset()
{
   // Reset this instance
   XrdSysMutexHelper mhp(fMutex);

   fResponse = 0;
   fProtocol = 0;
   fParent = 0;
   SafeDelete(fQueryNum);
   SafeDelete(fStartMsg);
   SafeDelete(fPingSem);
   fSrvPID = -1;
   fID = -1;
   fIsShutdown = false;
   fIsValid = false;
   fSkipCheck = false;
   fProtVer = -1;
   fNClients = 0;
   fClients.clear();
   fDisconnectTime = -1;
   fSetIdleTime = -1;
   fROOT = 0;
   // Cleanup worker info
   ClearWorkers();
   // ClearWorkers depends on the fSrvType and fStatus
   fSrvType = kXPD_AnyServer;
   fStatus = kXPD_idle;
   // Cleanup queries info
   fQueries.clear();
   // Strings
   fAdminPath = "";
   fAlias = "";
   fClient = "";
   fFileout = "";
   fGroup = "";
   fOrdinal = "";
   fTag = "";
   fUserEnvs = "";
   DeleteUNIXSock();
   fWrksStr = "";
}

//__________________________________________________________________________
void XrdProofdProofServ::DeleteUNIXSock()
{
   // Delete the current UNIX socket

   SafeDelete(fUNIXSock);
   unlink(fUNIXSockPath.c_str());
   fUNIXSockPath = "";
}

//__________________________________________________________________________
bool XrdProofdProofServ::SkipCheck()
{
   // Return the value of fSkipCheck and reset it to false

   XrdSysMutexHelper mhp(fMutex);

   bool rc = fSkipCheck;
   fSkipCheck = false;
   return rc;
}

//__________________________________________________________________________
XrdClientID *XrdProofdProofServ::GetClientID(int cid)
{
   // Get instance corresponding to cid
   XPDLOC(SMGR, "ProofServ::GetClientID")

   XrdClientID *csid = 0;

   if (cid < 0) {
      TRACE(XERR, "negative ID: protocol error!");
      return csid;
   }

   XrdOucString msg;
   {  XrdSysMutexHelper mhp(fMutex);

      // Count new attached client
      fNClients++;

      // If in the allocate range reset the corresponding instance and
      // return it
      if (cid < (int)fClients.size()) {
         csid = fClients.at(cid);
         csid->Reset();

         // Notification message
         if (TRACING(DBG)) {
            msg.form("cid: %d, size: %d", cid, fClients.size());
         }
      }

      if (!csid) {
         // If not, allocate a new one; we need to resize (double it)
         if (cid >= (int)fClients.capacity())
            fClients.reserve(2*fClients.capacity());

         // Allocate new elements (for fast access we need all of them)
         int ic = (int)fClients.size();
         for (; ic <= cid; ic++)
            fClients.push_back((csid = new XrdClientID()));

         // Notification message
         if (TRACING(DBG)) {
            msg.form("cid: %d, new size: %d", cid, fClients.size());
         }
      }
   }
   TRACE(DBG, msg);

   // We are done
   return csid;
}

//__________________________________________________________________________
int XrdProofdProofServ::FreeClientID(int pid)
{
   // Free instance corresponding to protocol connecting process 'pid'
   XPDLOC(SMGR, "ProofServ::FreeClientID")

   TRACE(DBG, "svrPID: "<<fSrvPID<< ", pid: "<<pid<<", session status: "<<
              fStatus<<", # clients: "<< fClients.size());
   int rc = -1;
   if (pid <= 0) {
      TRACE(XERR, "undefined pid!");
      return rc;
   }
   if (!IsValid()) return rc;

   {  XrdSysMutexHelper mhp(fMutex);

      // Remove this from the list of clients
      std::vector<XrdClientID *>::iterator i;
      for (i = fClients.begin(); i != fClients.end(); ++i) {
         if ((*i) && (*i)->P() && (*i)->P()->Pid() == pid) {
            (*i)->Reset();
            fNClients--;
            // Record time of last disconnection
            if (fNClients <= 0)
               fDisconnectTime = time(0);
            rc = 0;
            break;
         }
      }
   }
   if (TRACING(REQ)) {
      int spid = SrvPID();
      TRACE(REQ, spid<<": slot for client pid: "<<pid<<" has been reset");
   }

   // Out of range
   return -1;
}

//__________________________________________________________________________
int XrdProofdProofServ::GetNClients(bool check)
{
   // Get the number of connected clients. If check is true check that
   // they are still valid ones and free the slots for the invalid ones

   XrdSysMutexHelper mhp(fMutex);

   if (check) {
      fNClients = 0;
      // Remove this from the list of clients
      std::vector<XrdClientID *>::iterator i;
      for (i = fClients.begin(); i != fClients.end(); ++i) {
         if ((*i) && (*i)->P() && (*i)->P()->Link()) fNClients++;
      }
   }

   // Done
   return fNClients;
}

//__________________________________________________________________________
int XrdProofdProofServ::DisconnectTime()
{
   // Return the time (in secs) all clients have been disconnected.
   // Return -1 if the session is running

   XrdSysMutexHelper mhp(fMutex);

   int disct = -1;
   if (fDisconnectTime > 0)
      disct = time(0) - fDisconnectTime;
   return ((disct > 0) ? disct : -1);
}

//__________________________________________________________________________
int XrdProofdProofServ::IdleTime()
{
   // Return the time (in secs) the session has been idle.
   // Return -1 if the session is running

   XrdSysMutexHelper mhp(fMutex);

   int idlet = -1;
   if (fStatus == kXPD_idle)
      idlet = time(0) - fSetIdleTime;
   return ((idlet > 0) ? idlet : -1);
}

//__________________________________________________________________________
void XrdProofdProofServ::SetIdle()
{
   // Set status to idle and update the related time stamp
   //

   XrdSysMutexHelper mhp(fMutex);

   fStatus = kXPD_idle;
   fSetIdleTime = time(0);
}

//__________________________________________________________________________
void XrdProofdProofServ::SetRunning()
{
   // Set status to running and reset the related time stamp
   //

   XrdSysMutexHelper mhp(fMutex);

   fStatus = kXPD_running;
   fSetIdleTime = -1;
}

//______________________________________________________________________________
void XrdProofdProofServ::Broadcast(const char *msg, int type)
{
   // Broadcast message 'msg' at 'type' to the attached clients
   XPDLOC(SMGR, "ProofServ::Broadcast")

   XrdOucString m;
   int len = 0, nc = 0;
   if (msg && (len = strlen(msg)) > 0) {
      XrdProofdProtocol *p = 0;
      int ic = 0, ncz = 0, sid = -1;
      { XrdSysMutexHelper mhp(fMutex); ncz = (int) fClients.size(); }
      for (ic = 0; ic < ncz; ic++) {
         {  XrdSysMutexHelper mhp(fMutex);
            p = fClients.at(ic)->P();
            sid = fClients.at(ic)->Sid(); }
         // Send message
         if (p) {
            XrdProofdResponse *response = p->Response(sid);
            if (response) {
               response->Send(kXR_attn, (XProofActionCode)type, (void *)msg, len);
               nc++;
            } else {
               m.form("response instance for sid: %d not found", sid);
            }
         }
         if (m.length() > 0)
            TRACE(XERR, m);
         m = "";
      }
   }
   if (TRACING(DBG)) {
      m.form("type: %d, message: '%s' notified to %d clients", type, msg, nc);
      XPDPRT(m);
   }
}

//______________________________________________________________________________
int XrdProofdProofServ::TerminateProofServ(bool changeown)
{
   // Terminate the associated process.
   // A shutdown interrupt message is forwarded.
   // If add is TRUE (default) the pid is added to the list of processes
   // requested to terminate.
   // Return the pid of tyhe terminated process on success, -1 if not allowed
   // or other errors occured.
   XPDLOC(SMGR, "ProofServ::TerminateProofServ")

   int pid = fSrvPID;
   TRACE(DBG, "ord: " << fOrdinal << ", pid: " << pid);

   // Send a terminate signal to the proofserv
   if (pid > -1) {
      XrdProofUI ui;
      XrdProofdAux::GetUserInfo(fClient.c_str(), ui);
      if (XrdProofdAux::KillProcess(pid, 0, ui, changeown) != 0) {
         TRACE(XERR, "ord: problems signalling process: "<<fSrvPID);
      }
      XrdSysMutexHelper mhp(fMutex);
      fIsShutdown = true;
   }

   // Failed
   return -1;
}

//______________________________________________________________________________
int XrdProofdProofServ::VerifyProofServ(bool forward)
{
   // Check if the associated proofserv process is alive. This is done
   // asynchronously by asking the process to callback and proof its vitality.
   // We do not block here: the caller may setup a waiting structure if
   // required.
   // If forward is true, the process will forward the request to the following
   // tiers.
   // Return 0 if the request was send successfully, -1 in case of error.
   XPDLOC(SMGR, "ProofServ::VerifyProofServ")

   TRACE(DBG, "ord: " << fOrdinal<< ", pid: " << fSrvPID);

   int rc = 0;
   XrdOucString msg;

   // Prepare buffer
   int len = sizeof(kXR_int32);
   char *buf = new char[len];
   // Option
   kXR_int32 ifw = (forward) ? (kXR_int32)1 : (kXR_int32)0;
   ifw = static_cast<kXR_int32>(htonl(ifw));
   memcpy(buf, &ifw, sizeof(kXR_int32));

   {  XrdSysMutexHelper mhp(fMutex);
      // Propagate the ping request
      if (!fResponse || fResponse->Send(kXR_attn, kXPD_ping, buf, len) != 0) {
         msg = "could not propagate ping to proofsrv";
         rc = -1;
      }
   }
   // Cleanup
   delete[] buf;

   // Notify errors, if any
   if (rc != 0)
      TRACE(XERR, msg);

   // Done
   return rc;
}

//__________________________________________________________________________
int XrdProofdProofServ::BroadcastPriority(int priority)
{
   // Broadcast a new group priority value to the worker servers.
   // Called by masters.
   XPDLOC(SMGR, "ProofServ::BroadcastPriority")

   XrdSysMutexHelper mhp(fMutex);

   // Prepare buffer
   int len = sizeof(kXR_int32);
   char *buf = new char[len];
   kXR_int32 itmp = priority;
   itmp = static_cast<kXR_int32>(htonl(itmp));
   memcpy(buf, &itmp, sizeof(kXR_int32));
   // Send over
   if (!fResponse || fResponse->Send(kXR_attn, kXPD_priority, buf, len) != 0) {
      // Failure
      TRACE(XERR,"problems telling proofserv");
      return -1;
   }
   TRACE(DBG, "priority "<<priority<<" sent over");
   // Done
   return 0;
}

//______________________________________________________________________________
int XrdProofdProofServ::SendData(int cid, void *buff, int len)
{
   // Send data to client cid.
   XPDLOC(SMGR, "ProofServ::SendData")

   TRACE(HDBG, "length: "<<len<<" bytes (cid: "<<cid<<")");

   int rs = 0;
   XrdOucString msg;

   // Get corresponding instance
   XrdClientID *csid = 0;
   {  XrdSysMutexHelper mhp(fMutex);
      if (cid < 0 || cid > (int)(fClients.size() - 1) || !(csid = fClients.at(cid))) {
         msg.form("client ID not found (cid: %d, size: %d)", cid, fClients.size());
         rs = -1;
      }
      if (!rs && !(csid->R())) {
         msg.form("client not connected: csid: %p, cid: %d, fSid: %d",
                  csid, cid, csid->Sid());
         rs = -1;
      }
   }

   //
   // The message is strictly for the client requiring it
   if (!rs) {
      rs = -1;
      XrdProofdResponse *response = csid->R() ? csid->R() : 0;
      if (response)
         if (!response->Send(kXR_attn, kXPD_msg, buff, len))
            rs = 0;
   } else {
      // Notify error
      TRACE(XERR, msg);
   }

   // Done
   return rs;
}

//______________________________________________________________________________
int XrdProofdProofServ::SendDataN(void *buff, int len)
{
   // Send data over the open client links of this session.
   // Used when all the connected clients are eligible to receive the message.
   XPDLOC(SMGR, "ProofServ::SendDataN")

   TRACE(HDBG, "length: "<<len<<" bytes");

   XrdOucString msg;

   XrdSysMutexHelper mhp(fMutex);

   // Send to connected clients
   XrdClientID *csid = 0;
   int ic = 0;
   for (ic = 0; ic < (int) fClients.size(); ic++) {
      if ((csid = fClients.at(ic)) && csid->P()) {
         XrdProofdResponse *resp = csid->R();
         if (!resp || resp->Send(kXR_attn, kXPD_msg, buff, len) != 0)
            return -1;
      }
   }

   // Done
   return 0;
}

//______________________________________________________________________________
void XrdProofdProofServ::ExportBuf(XrdOucString &buf)
{
   // Fill buf with relevant info about this session
   XPDLOC(SMGR, "ProofServ::ExportBuf")

   buf = "";
   int id, status, nc;
   XrdOucString tag, alias;
   {  XrdSysMutexHelper mhp(fMutex);
      id = fID;
      status = fStatus;
      nc = fNClients;
      tag = fTag;
      alias = fAlias; }
   buf.form(" | %d %s %s %d %d", id, tag.c_str(), alias.c_str(), status, nc);
   TRACE(HDBG, "buf: "<< buf);

   // Done
   return;
}

//______________________________________________________________________________
int XrdProofdProofServ::CreateUNIXSock(XrdSysError *edest)
{
   // Create UNIX socket for internal connections
   XPDLOC(SMGR, "ProofServ::CreateUNIXSock")

   TRACE(DBG, "enter");

   // Make sure we do not have already a socket
   if (fUNIXSock) {
       TRACE(DBG,"UNIX socket exists already! ("<<fUNIXSockPath<<")");
       return 0;
   }

   // Create socket
   fUNIXSock = new XrdNet(edest);

   // Make sure the admin path exists
   struct stat st;
   if (fAdminPath.length() > 0 &&
       stat(fAdminPath.c_str(), &st) != 0 && (errno == ENOENT)) {;
      FILE *fadm = fopen(fAdminPath.c_str(), "w");
      fclose(fadm);
   }

   // Check the path
   bool rm = 0, ok = 0;
   if (stat(fUNIXSockPath.c_str(), &st) == 0 || (errno != ENOENT)) rm = 1;
   if (rm  && unlink(fUNIXSockPath.c_str()) != 0) {
      if (!S_ISSOCK(st.st_mode)) {
         TRACE(XERR, "non-socket path exists: unable to delete it: " <<fUNIXSockPath);
         return -1;
      } else {
         XPDPRT("WARNING: socket path exists: unable to delete it:"
                " try to use it anyway " <<fUNIXSockPath);
         ok = 1;
      }
   }

   // Create the path
   int fd = 0;
   if (!ok) {
      if ((fd = open(fUNIXSockPath.c_str(), O_EXCL | O_RDWR | O_CREAT, 0700)) < 0) {
         TRACE(XERR, "unable to create path: " <<fUNIXSockPath);
         return -1;
      }
      close(fd);
   }
   if (fd > -1) {
      // Change ownership
      if (fUNIXSock->Bind((char *)fUNIXSockPath.c_str())) {
         TRACE(XERR, " problems binding to UNIX socket; path: " <<fUNIXSockPath);
         return -1;
      } else
         TRACE(DBG, "path for UNIX for socket is " <<fUNIXSockPath);
   } else {
      TRACE(XERR, "unable to open / create path for UNIX socket; tried path "<< fUNIXSockPath);
      return -1;
   }

   // Change ownership if running as super-user
   if (!geteuid()) {
      XrdProofUI ui;
      XrdProofdAux::GetUserInfo(fClient.c_str(), ui);
      if (chown(fUNIXSockPath.c_str(), ui.fUid, ui.fGid) != 0) {
         TRACE(XERR, "unable to change ownership of the UNIX socket"<<fUNIXSockPath);
         return -1;
      }
   }

   // We are done
   return 0;
}

//__________________________________________________________________________
int XrdProofdProofServ::SetAdminPath(const char *a)
{
   // Set the admin path and make sure the file exists

   XPDLOC(SMGR, "ProofServ::SetAdminPath")

   XrdSysMutexHelper mhp(fMutex);

   fAdminPath = a;

   struct stat st;
   if (stat(a, &st) != 0 && errno == ENOENT) {
      // Create the file
      FILE *fpid = fopen(a, "w");
      if (fpid) {
         fclose(fpid);
         return 0;
      }
      TRACE(XERR, "unable to open / create admin path "<< fAdminPath << "; errno = "<<errno);
      return -1;
   }
   // Done
   return 0;
}

//______________________________________________________________________________
int XrdProofdProofServ::Resume()
{
   // Send a resume message to the this session. It is assumed that the session
   // has at least one async query to process and will immediately send
   // a getworkers request (the workers are already assigned).
   XPDLOC(SMGR, "ProofServ::Resume")

   TRACE(DBG, "ord: " << fOrdinal<< ", pid: " << fSrvPID);

   int rc = 0;
   XrdOucString msg;

   {  XrdSysMutexHelper mhp(fMutex);
      // 
      if (!fResponse || fResponse->Send(kXR_attn, kXPD_resume, 0, 0) != 0) {
         msg = "could not propagate resume to proofsrv";
         rc = -1;
      }
   }

   // Notify errors, if any
   if (rc != 0)
      TRACE(XERR, msg);

   // Done
   return rc;
}

//______________________________________________________________________________
const char *XrdProofdProofServ::FirstQueryTag()
{

   if (!fQueries.empty())
      return fQueries.front()->GetTag();
   else
      return 0;
}
