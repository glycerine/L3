//
// Modificdations Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef XCEP_H
#define XCEP_H

//
// 
// 
// Copyright (c) 1998,99 On Time
// http://www.on-time.com
// 
// C Exception Handling Library Header File
// 
// 

#ifdef __cplusplus
extern "C" {
#endif 

// definitions of user exceptions go in #include "user_xcep.h"
// integer exception codes:

// XValue  >  0  : positive means no error, returning some value
// XValue ==  0  : means no error, or doing Code block
// XValue == -1 : means doing finally.
// XValue  < -1 : other negative numbers indicate true XRaise() exceptions.

// User exception codes must be -2, -3, -4, ... or more negative.

// special exception codes

#define XNOEX     0      // no exception
#define XCODE     XNOEX  // value for the code block
#define XFINALLY  -1     // value for the finally block

#include "user_xcep.h"


  // setjmp/longjmp  --- should this go to sigsetjmp/siglongjmp?

   #include <setjmp.h>
   #define XCONTEXT              jmp_buf
    // jea: orig:  #define XSAVECONTEXT(C)       sigsetjmp(*(C),1)
    // jea: new:
   #define XSAVECONTEXT(C)       sigsetjmp(*(C),0)

   #define XRESTORECONTEXT(C, V) siglongjmp(*(C), V)





// data needed for each XTRY block

typedef   enum { XCode, 
		 XHandling, 
		 XFinally } xrec_state;

typedef struct _XRecord {
   struct _XRecord * Next;
   XCONTEXT          Context;
   int               XValue;
   int               IsHandled;
   xrec_state        State;
} XRecord;




// internal functions

void XLinkExceptionRecord(XRecord * XD);
int  XUnLinkExceptionRecordCanReturn(XRecord * XD);
int  XUnLinkExceptionRecordNoReturn(XRecord * XD);


// API macros and functions

#define XTRY    while (1) { XRecord XData; XLinkExceptionRecord(&XData); switch (XSAVECONTEXT(&XData.Context)) {
#define XEND    } { int x = XData.XValue; if (XUnLinkExceptionRecordCanReturn(&XData)) return XData.XValue; else if (x <= 0) break; } }
#define XENDX   } if (XUnLinkExceptionRecordNoReturn(&XData) == 0) break; }
#define XVALUE  XData.XValue

void XRaise(int XValue);
void XReExecute(void);
void XHandled(void);


#ifdef __cplusplus
}
#endif 



#endif /* XCEP_H */

