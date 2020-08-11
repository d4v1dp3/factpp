/* ======================================================================== *\
!
! *
! * This file is part of MARS, the MAGIC Analysis and Reconstruction
! * Software. It is distributed to you in the hope that it can be a useful
! * and timesaving tool in analysing Data of imaging Cerenkov telescopes.
! * It is distributed WITHOUT ANY WARRANTY.
! *
! * Permission to use, copy, modify and distribute this software and its
! * documentation for any purpose is hereby granted without fee,
! * provided that the above copyright notice appear in all copies and
! * that both that copyright notice and this permission notice appear
! * in supporting documentation. It is provided "as is" without express
! * or implied warranty.
! *
!
!
!   Author(s): Thomas Bretz  1/2008 <mailto:tbretz@astro.uni-wuerzburg.de>
!
!   Copyright: MAGIC Software Development, 2000-2008
!
!
\* ======================================================================== */

//////////////////////////////////////////////////////////////////////////////
//
// MThread
//
// Implementing a slightly simplified interface to multi-threading
// based on TThread
//
//////////////////////////////////////////////////////////////////////////////
#include "MThread.h"

using namespace std;

// --------------------------------------------------------------------------
//
// Return the thread's state as string
//
TString MThread::GetThreadStateStr() const
{
    switch (fThread.GetState())
    {
    case TThread::kInvalidState:
        return "Invalid - thread was not created properly";
    case TThread::kNewState:
        return "New - thread object exists but hasn't started";
    case TThread::kRunningState:
        return "Running - thread is running";
    case TThread::kTerminatedState:
        return "Terminated - thread has terminated but storage has not yet been reclaimed (i.e. waiting to be joined)";
    case TThread::kFinishedState:
        return "Finished - thread has finished";
    case TThread::kCancelingState:
        return "Canceling - thread in process of canceling";
    case TThread::kCanceledState:
        return "Canceled - thread has been canceled";
    case TThread::kDeletingState:
        return "Deleting - thread in process of deleting";
    };
    return "Unknown";
}

/*
{
    TMethodCall call(cl, "Name", 0);

    if (!call.IsValid())
        return 0;

    //const char    *GetParams() const { return fParams.Data(); }
    //const char    *GetProto() const { return fProto.Data(); }

    switch (call.ReturnType())
    {
    case kLong:
        break;
    case kDouble:
        break;
    case kString:
        break;
    case kOther:
        break;
    case kNone:
        break;
    }

    // NOTE execute functions are locked by a global mutex!!!

   void     Execute(void *object);
   void     Execute(void *object, Long_t &retLong);
   void     Execute(void *object, Double_t &retDouble);
   void     Execute(void *object, char **retText);

   void     Execute();
   void     Execute(Long_t &retLong);
   void     Execute(Double_t &retDouble);
}
*/
