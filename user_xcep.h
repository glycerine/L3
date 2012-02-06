#ifndef USER_XCEP_H
#define USER_XCEP_H 1

// definitions of user exceptions go in this file, "user_xcep.h"

// Standard integer exception codes:

// XValue  >  0  : positive means no error, returning some value
// XValue ==  0  : means no error, or doing Code block
// XValue == -1 : means doing finally.
// XValue  < -1 : other negative numbers indicate true XRaise() exceptions.

// User exception codes must be -2, -3, -4, ... or more negative.
// The names of the exception codes should start with X.

// special exception codes

#define XNOEX     0      // no exception
#define XCODE     XNOEX  // value for the code block
#define XFINALLY  -1     // value for the finally block

/* User exceptions go here */ 

#define JERROR  -4   // an error code for J's testing.
#define XABORT_TO_TOPLEVEL -5
#define XQUITTIN_TIME      -6
#define XARRAYDUP_FAILURE  -7
#define XBREAK_NEAREST_LOOP  -8
#define XUSER_EXCEPTION    -9
#define XCD_DOT_DOT    -10
#define XPOPTOP    -11
#define XCONTINUE_LOOP    -12


#endif /* USER_XCEP_H */
