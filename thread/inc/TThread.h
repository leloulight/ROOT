// @(#)root/thread:$Name:  $:$Id: TThread.h,v 1.7 2004/07/16 00:08:17 rdm Exp $
// Author: Fons Rademakers   02/07/97

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_TThread
#define ROOT_TThread


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TThread                                                              //
//                                                                      //
// This class implements threads. A thread is an execution environment  //
// much lighter than a process. A single process can have multiple      //
// threads. The actual work is done via the TThreadImp class (either    //
// TThreadPosix, TThreadSolaris or TThreadNT).                          //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#ifndef ROOT_TObject
#include "TObject.h"
#endif
#ifndef ROOT_TMutex
#include "TMutex.h"
#endif
#ifndef ROOT_TCondition
#include "TCondition.h"
#endif
#ifndef ROOT_TSystem
#include "TSystem.h"
#endif
#ifndef ROOT_TTimer
#include "TTimer.h"
#endif

class TThreadImp;


class TThread : public TNamed {

friend class TThreadImp;
friend class TPosixThread;
friend class TThreadTimer;
friend class TThreadCleaner;
friend class TWin32Thread;

public:

   typedef void *(*VoidRtnFunc_t)(void *);
   typedef void  (*VoidFunc_t)(void *);

   enum EPriority {
      kLowPriority,
      kNormalPriority,
      kHighPriority
   };

   enum EState {
      kInvalidState,            // thread was not created properly
      kNewState,                // thread object exists but hasn't started
      kRunningState,            // thread is running
      kTerminatedState,         // thread has terminated but storage has not
                                // yet been reclaimed (i.e. waiting to be joined)
      kFinishedState,           // thread has finished
      kCancelingState,          // thread in process of canceling
      kCanceledState,           // thread has been canceled
      kDeletingState            // thread in process of deleting
   };

private:
   TThread       *fNext;                  // pointer to next thread
   TThread       *fPrev;                  // pointer to prev thread
   TThread      **fHolder;                // pointer to holder of this (delete only)
   EPriority      fPriority;              // thread priority
   EState         fState;                 // thread state
   EState         fStateComing;           // coming thread state
   Long_t         fId;                    // thread id
   Long_t         fHandle;                // thread handle (Win32)
   Long_t         fJoinId;                // thread id used for joining
   Bool_t         fDetached;              // kTRUE if thread is Detached
   Bool_t         fNamed;                 // kTRUE if thread is Named
   VoidRtnFunc_t  fFcnRetn;               // void* start function of thread
   VoidFunc_t     fFcnVoid;               // void  start function of thread
   void          *fThreadArg;             // thread start function arguments
   void          *fClean;                 // support of cleanup structure
   void          *fTsd[20];               // thread specific data container
   char           fComm[100];             // ????

   static TThreadImp      *fgThreadImp;   // static pointer to thread implementation
   static char  * volatile fgXAct;        // Action name to do by main thread
   static void ** volatile fgXArr;        // pointer to control array of void pointers for action
   static volatile Int_t   fgXAnb;        // size of array above
   static volatile Int_t   fgXArt;        // return XA flag
   static TThread         *fgMain;        // pointer to chain of TThread's
   static TMutex          *fgMainMutex;   // mutex to protect chain of threads
   static TMutex          *fgXActMutex;   // mutex to protect XAction
   static TCondition      *fgXActCondi;   // Condition for XAction

   // Private Member functions
   void           Constructor();
   void           PutComm(const char *txt = 0) { fComm[0] = 0; if (txt) { strncpy(fComm,txt,99); fComm[99] = 0;} }
   const char    *GetComm() const { return fComm; }
   static void   *Fun(void *ptr);
   static Int_t   XARequest(const char *xact, Int_t nb, void **ar, Int_t *iret);
   static ULong_t Call(void  *p2f, void *arg);
   static void    AfterCancel(TThread *th);

public:
   TThread(VoidRtnFunc_t fn, void *arg = 0, EPriority pri = kNormalPriority);
   TThread(VoidFunc_t fn, void *arg = 0, EPriority pri = kNormalPriority);
   TThread(const char *thname, VoidRtnFunc_t fn, void *arg = 0, EPriority pri = kNormalPriority);
   TThread(const char *thname, VoidFunc_t fn, void *arg = 0, EPriority pri = kNormalPriority);
   TThread(Int_t id = 0);
   virtual ~TThread();

   Int_t            Kill();
   Int_t            Run(void *arg = 0);
   void             SetPriority(EPriority pri);
   void             Delete(Option_t *option="") { TObject::Delete(option); }
   EPriority        GetPriority() const { return fPriority; }
   EState           GetState() const { return fState; }
   Long_t           GetId() const { return fId; }
   void             Ps();
   void             ps() { Ps(); }
   Long_t           GetJoinId() const { return fJoinId; }
   void             SetJoinId(TThread *tj);
   void             SetJoinId(Long_t jid);
   void             SetHandle(Long_t handle) { fHandle = handle; }

   Long_t           Join(Long_t id, void **ret=0);
   Long_t           Join(void **ret=0);

   static Int_t     Exit(void *ret = 0);
   static Int_t     Exists();
   static TThread  *GetThread(Long_t id);
   static TThread  *GetThread(const char *name);

   static Int_t     Lock();                  //User's lock of main mutex
   static Int_t     TryLock();               //User's try lock of main mutex
   static Int_t     UnLock();                //User's unlock of main mutex
   static TThread  *Self();
   static Long_t    SelfId();
   static Int_t     Sleep(ULong_t secs, ULong_t nanos = 0);
   static Int_t     GetTime(ULong_t *absSec, ULong_t *absNanoSec);

   static void      Debug(const char *txt);
   static Int_t     Delete(TThread* &th);
   static void    **Tsd(void *dflt, Int_t k);

   // Cancelation
   // there are two types of TThread cancellation:
   //    DEFERRED     - Cancelation only in user provided cancel-points
   //    ASYNCHRONOUS - In any point
   //    DEFERRED is more safe, it is DEFAULT.
   static Int_t     SetCancelOn();
   static Int_t     SetCancelOff();
   static Int_t     SetCancelAsynchronous();
   static Int_t     SetCancelDeferred();
   static Int_t     CancelPoint();
   static Int_t     Kill(Long_t id);
   static Int_t     Kill(const char *name);
   static Int_t     CleanUpPush(void *free, void *arg=0);
   static Int_t     CleanUpPop(Int_t exe=0);
   static Int_t     CleanUp();

   // XActions
   static void      Printf(const char *txt);
   static void      Printf(const char *txt, Long_t i);
   static void      Printf(const char *txt, void *i) { Printf(txt, (Long_t)i); }
   static void      XAction();

   // Compilation and dynamic link
   static Int_t     MakeFun(char *funname);

   ClassDef(TThread,0)  // Thread class
};


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TThreadCleaner                                                       //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

class TThreadCleaner {
public:
   TThreadCleaner() { }
   ~TThreadCleaner();
};


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TThreadTimer                                                         //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

class TThreadTimer : public TTimer {
public:
   TThreadTimer(Long_t ms = 100);
   Bool_t Notify();
};

#endif
