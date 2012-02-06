
//
// Modifications Copyright (C) 2011 Jason E. Aten. All rights reserved.
//

//
// 
// RTFEX.C -> xcep.c
// 
// Copyright (c) 1998,99 On Time
// http://www.on-time.com
// 
// C Exception Handling Library Main Source File
// 
// 

#include <stdlib.h>
#include <stdio.h>

#include "xcep.h"


/////////////////////// end of includes

// Define DEBUG to enable some extra code to detect internal errors

// #define DEBUG


// Start of OS dependent functions

// The exception handling library needs some operating system services.
// The real implementation for RTFiles uses RTFiles' system driver.
// To be able to link the demo programs, single thread versions
// of these functions are included in this source file:

void RTFSYSErrorExit(const char* Message, int Code)
{
  printf("%s : %d\n",Message,Code);
   exit(Code);
}

static void * SingleTLSValue = NULL;

#define RTFSYSAllocTLS()           0
#define RTFSYSGetTLS(Index)        SingleTLSValue
#define RTFSYSSetTLS(Index, Value) SingleTLSValue = Value

// End of OS dependent functions


static int XTlsIndex = -1;  // assume -1 is an illegal value

///////////////////////////////////////
static void Init(void)
{
   XTlsIndex = RTFSYSAllocTLS();
   if (XTlsIndex == -1)
      RTFSYSErrorExit("Unable to allocate exception TLS", 1);
}

///////////////////////////////////////
void XRaise(int XValue)
{
   XRecord * XD;

   if (XTlsIndex == -1)
      Init();

#ifdef DEBUG
   if (XValue >= XFINALLY)
      RTFSYSErrorExit("Cannot raise positive or reserved error code", XValue);
#endif

   XD = (XRecord*)RTFSYSGetTLS(XTlsIndex);
   while (XD != NULL)
   {
      XD->IsHandled = 0;
      switch (XD->State)
      {
      case XCode:               // execute the handler
            XD->XValue = XValue;
            XD->State  = XHandling;
            XRESTORECONTEXT(&XD->Context, XValue);
      case XHandling:           // reraise, do finally first
            XD->XValue = XValue;
            XD->State  = XFinally;
            XRESTORECONTEXT(&XD->Context, XFINALLY);
      case XFinally:            // raise within finally; propagate
#ifdef DEBUG
            RTFSYSErrorExit("Raise within finally", 1);
#endif
            RTFSYSSetTLS(XTlsIndex, XD = XD->Next);
            break;
         default:
            RTFSYSErrorExit("Corrupted exception handler chain", 1);
      }
   }
   RTFSYSErrorExit("Unhandled xcep.h defined exception", XValue);
}

///////////////////////////////////////
void XReExecute(void)
{
  XRecord * XD = (XRecord*)RTFSYSGetTLS(XTlsIndex);

#ifdef DEBUG
   if (XD == NULL)
      RTFSYSErrorExit("no exception handler frame in XReExecute", 1);
#endif

   XD->XValue    = XNOEX;
   XD->State     = XCode;
   XD->IsHandled = 1;
   XRESTORECONTEXT(&XD->Context, XCODE);
}

///////////////////////////////////////
void XLinkExceptionRecord(XRecord * XD)
{
   if (XTlsIndex == -1)
      Init();
   XD->Next = (XRecord*)RTFSYSGetTLS(XTlsIndex);
   RTFSYSSetTLS(XTlsIndex, XD);
   XD->XValue    = XNOEX;
   XD->State     = XCode;
   XD->IsHandled = 1;
}

///////////////////////////////////////
void  XHandled(void)
{
  XRecord * XD = (XRecord*)RTFSYSGetTLS(XTlsIndex);

#ifdef DEBUG
   if (XD == NULL)
      RTFSYSErrorExit("no exception handler frame in XReExecute", 1);
   switch (XD->State)
   {
      case XHandling:
         break;
   case XCode:
   case XFinally:
         RTFSYSErrorExit("Internal: exception handled outside handler", 1);
      default:
         RTFSYSErrorExit("Corrupted exception handler chain", 1);
   }
#endif
   XD->XValue    = XNOEX;
   XD->State     = XCode;
   XD->IsHandled = 1;
}

///////////////////////////////////////
int  XUnLinkExceptionRecordNoReturn(XRecord * XD)
{
#ifdef DEBUG
   if (RTFSYSGetTLS(XTlsIndex) != XD)
      RTFSYSErrorExit("Corrupted exception handler chain", 1);
#endif

   switch (XD->State)
   {
      case XHandling:              // exception is now handled
      case XCode:                  // no exception
         XD->State = XFinally;
         XRESTORECONTEXT(&XD->Context, XFINALLY);
      case XFinally:               // we are done
         RTFSYSSetTLS(XTlsIndex, XD->Next);
         if (XD->IsHandled)
            return 0;              // continue after XTRY block
         else
            XRaise(XD->XValue);    // propagate exception 
         break;
      default:
         RTFSYSErrorExit("Corrupted exception handler chain", 1);
   }
   return 0;                       // never gets here
}

///////////////////////////////////////
int XUnLinkExceptionRecordCanReturn(XRecord * XD)
{
#ifdef DEBUG
   if (RTFSYSGetTLS(XTlsIndex) != XD)
      RTFSYSErrorExit("Corrupted exception handler chain", 1);
#endif

   switch (XD->State)
   {
      case XHandling:              // exception is now handled
      case XCode:                  // no exception
         XD->State = XFinally;
         XRESTORECONTEXT(&XD->Context, XFINALLY);
      case XFinally:               // we are done
         RTFSYSSetTLS(XTlsIndex, XD->Next);
         if (XD->IsHandled)
            return 0;              // continue after XTRY block
         else
            if (XD->Next != NULL)
               XRaise(XD->XValue); // propagate exception 
            else
               return 1;
         break;
      default:
         RTFSYSErrorExit("Corrupted exception handler chain", 1);
   }
   return 0;
}
