//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef SERMON_H
#define SERMON_H

//
// SERMON: SERial-number based memory MONitoring.
//

//
// use: declare these three globals once somewhere in your code
// sermon needs these guys in a client:
//

#if 0
/*
ser_mem_mon*    gsermon = 0;
quicktype_sys*  qtypesys = 0;
jmemlogger*     mlog = 0;

// optional, known leaks/points sermon # to stop at on malloc
codepoint* leaks = 0;

*/
#endif


#include <stdlib.h>
#include <map>
#include <ext/malloc_allocator.h>

#define DMALLOC
#include "dmalloc.h"


#include "bool.h"
#include "l3header.h"

//#include <stdlib.h>
//#include <map>
//#include <ext/malloc_allocator.h>


#ifdef __cplusplus
extern "C" {
#endif
void tracker_sermon(const char *file, const unsigned int line,
                       const int func_id,
                       const DMALLOC_SIZE byte_size,
                       const DMALLOC_SIZE alignment,
                       const DMALLOC_PNT old_addr,
                       const DMALLOC_PNT new_addr);
#ifdef __cplusplus
}
#endif


struct ser_mem_mon {

    typedef  std::map<void*,long>       sermon_map;
    typedef  sermon_map::iterator       sermon_map_it;

    typedef  std::map<long,void*>       ser2ptr;
    typedef  ser2ptr::iterator          ser2ptr_it;

    sermon_map*   _gmonitor;
    ser2ptr*      _s2p;

    long          _sermon_last_sn;

    long          _stop_requested_malloc;
    long          _stop_requested_free;

    BOOL          _on;

    //
    // found_sermon_number : provided so that new-ed memory can be
    //  assigned serial numbers too (negative by convention).
    //
    // returns FALSE if ptr not registered. else sets *smn_out.
    BOOL found_sermon_number(void* ptr, long* smn_out);

    void dump();

    void stop_set_malloc(long s) { _stop_requested_malloc = s; }
    long stop_get_malloc()       { return _stop_requested_malloc; }

    void stop_set_free(long s) { _stop_requested_free = s; }
    long stop_get_free()       { return _stop_requested_free; }

    // allow on/off - in case we want to disable.
    //  return prev value so we can restore to prior state.
    //
    BOOL off() {  BOOL prev = _on; _on=FALSE; dmalloc_track(NULL); return prev; }
    BOOL on()  {  BOOL prev = _on; _on=TRUE;  dmalloc_track(tracker_sermon); return prev; }
    BOOL switch_to(BOOL newval) {
        BOOL prev = _on;
        _on = newval;
        if (newval) {
            dmalloc_track(tracker_sermon);
        } else {
            dmalloc_track(NULL);
        }
        return prev;
    }

    ser_mem_mon() {
        _on = TRUE;
        _stop_requested_malloc = 0;
        _stop_requested_free = 0;

        _gmonitor = new sermon_map;
        _s2p      = new ser2ptr;

        _sermon_last_sn = 0;
        dmalloc_track(tracker_sermon);
    }
    ~ser_mem_mon() {
        dmalloc_track(NULL);
        delete _gmonitor;
        delete _s2p;
        _gmonitor = 0;
    }
};

void p(ser_mem_mon* p);

#endif /*  SERMON_H */
