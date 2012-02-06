#ifndef DEBUGVIEW
#define DEBUGVIEW
//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//

// macros

// DV: debug view macro
//
// to have DV statements executed while debugging
// with gdb, use "set enDV=1" at the (gdb) prompt
// use "set enDV=0" to turn them off again.
// DV statements, like asserts, are not compiled
// into release builds.


#ifndef DV

// may be referred to even if NO_DEBUGVIEW is defined...
extern int enDV; // off by default

#ifdef NO_DEBUGVIEW
#define DV(x) ((void)0);
#define DVV(x) ((void)0);

#else
#define DV(x) if (enDV) { x; }
#define DVV(x) if (enDV >= 2) { x; }

#endif // NO_DEBUGVIEW
#endif // ndef DV



#endif // DEBUGVIEW
