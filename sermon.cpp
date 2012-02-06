//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//

#include "dv.h"
#include "l3path.h"
#include "jmemlogger.h"
#include "sermon.h"
#include "bool.h"
#include "codepoints.h"

// client must provide these 2 things:
extern jmemlogger* mlog;
extern ser_mem_mon* gsermon;

// the set of known leaks, if any.
// easy to query
extern codepoints* leaks; // loads and makes easy to query file "./leakpoints"
void p(ser_mem_mon* p) { p->dump(); }

void ser_mem_mon::dump() {
    ser_mem_mon::sermon_map_it be = _gmonitor->begin();
    ser_mem_mon::sermon_map_it en = _gmonitor->end();
    long i = 0;
    for ( ; be != en; ++be, ++i) {
        printf("contents of gsermon->_gmonitor [%ld] :  (ptr %p) ->  (ser# %ld)\n", i, be->first, be->second);

        // sanity check that our maps aggree
        assert(_s2p->find(be->second)->second == be->first);
    }

    printf("\n");
    ser_mem_mon::ser2ptr_it be2 = _s2p->begin();
    ser_mem_mon::ser2ptr_it en2 = _s2p->end();
    i = 0;
    for ( ; be2 != en2; ++be2, ++i) {
        printf("contents of gsermon->_s2p [%ld] : (ser# %ld) ->  (ptr %p)\n", i, be2->first, be2->second);

        // sanity check that our maps aggree
        assert(_gmonitor->find(be2->second)->second == be2->first);
    }
}


// returns FALSE if ptr not registered. else sets *smn_out.
BOOL  ser_mem_mon::found_sermon_number(void* ptr, long* smn_out) {

    sermon_map_it it = _gmonitor->find(ptr);
    
    if (it == _gmonitor->end()) {
        // allow this, since mac doesnt' seem to support GLIBCXX_FORCE_NEW and we don't catch all new calls.
        //
        //        printf("error in ser_mem_mon: memory corruption likely: ptr query %p not found!\n",ptr);
        //        dump();
        //        assert(0);
        return FALSE;
    }
    
    long found_ser = it->second;

    // confirm... that _s2p is consistent.
    ser_mem_mon::ser2ptr_it  jt = _s2p->find(found_ser);
    if (jt == _s2p->end()) {
        printf("found_sermon_number() on ptr %p query: sermon serial # %ld  not found in _s2p map! Memory corruption likely.\n",
               ptr,
               found_ser);
        dump();
        assert(0);
        exit(1);
    }

    // consistency check... that our 2 maps aggree
    void* stored_ptr = jt->second;
    assert(stored_ptr == ptr);
    
    // negative by convention, to indicate they are new/delete sermon sn.
    *smn_out = - found_ser;

    return TRUE;
}



#ifdef __cplusplus
extern "C" {
#endif

// dmalloc_track_t  from dmalloc.h

// from dmalloc.h

//#define DMALLOC_FUNC_MALLOC	10	/* malloc function called */
//#define DMALLOC_FUNC_CALLOC	11	/* calloc function called */
//#define DMALLOC_FUNC_REALLOC	12	/* realloc function called */
//#define DMALLOC_FUNC_RECALLOC	13	/* recalloc called */
//#define DMALLOC_FUNC_MEMALIGN	14	/* memalign function called */
//#define DMALLOC_FUNC_VALLOC	15	/* valloc function called */
//#define DMALLOC_FUNC_STRDUP	16	/* strdup function called */
//#define DMALLOC_FUNC_FREE	17	/* free function called */
//#define DMALLOC_FUNC_CFREE	18	/* cfree function called */
//
//#define DMALLOC_FUNC_NEW	20	/* new function called */
//#define DMALLOC_FUNC_NEW_ARRAY	21	/* new[] function called */
//#define DMALLOC_FUNC_DELETE	22	/* delete function called */
//#define DMALLOC_FUNC_DELETE_ARRAY 23	/* delete[] function called */

// avoid additional alloations inside
static l3path track_msg;

static BOOL static_tracking_suspended = FALSE;

    // move switch embeded code into their own process_malloc/process_free
    // functions so that realloc can call them both.

void tracker_sermon_process_malloc(const char *file, const unsigned int line,
                               const int func_id,
                               const DMALLOC_SIZE byte_size,
                               const DMALLOC_SIZE alignment,
                               const DMALLOC_PNT old_addr,
                               const DMALLOC_PNT new_addr) {


            void* ptr = (void*)new_addr;

            long new_ser = 0;    
            new_ser = ++gsermon->_sermon_last_sn;
            
            if (leaks && leaks->is_codepoint(new_ser)) {
                printf("at codepoint  for known leak %ld  at %p ... ought to be a breakpoint here, sermon:130 or so, to inspect the stack.\n", new_ser,ptr);
                printf("\n"); // a line to put a breakpoint on.
            }

            if (gsermon->_stop_requested_malloc && gsermon->_stop_requested_malloc == new_ser) {
                printf("tracker_sermon(): MALLOC MALLOC stopping at %ld _stop_requested_malloc allocation.\n",gsermon->_stop_requested_malloc);
                MLOG_JMSYNC();
                assert(0);
            }

            // check for already existing that should have been freed first...
            ser_mem_mon::sermon_map_it it = gsermon->_gmonitor->find(ptr);
            if (it != gsermon->_gmonitor->end()) {
                long  leaking_ser = it->second;
                void* leaking    = it->first;
                assert(leaking == ptr);
                printf("tracker_sermon(): MEMORY LEAK DETECTED :  %p ser# %ld is being malloc/overwritten "
                       "before being free by same pointer at ser# %ld.\n", 
                       leaking, leaking_ser, new_ser);
                MLOG_JMSYNC();
                assert(0); // fires once under valgrind, but not without valgrind???
            }

            // have to turn off allocation tracking for our insert, or else we infinite loop (on linux
            // but not on Mac, which raises an interesting issue of why we don't inf loop on OSX ???)
            //
            static_tracking_suspended = TRUE;
            gsermon->_gmonitor->insert(ser_mem_mon::sermon_map::value_type(ptr,new_ser));
            gsermon->_s2p->insert(ser_mem_mon::ser2ptr::value_type(new_ser,ptr));
            static_tracking_suspended = FALSE;
            
            track_msg.reinit("MMMMMM %p generic ser mon pointer added: @sermon_num:%ld\n", ptr, new_ser);
            MLOG_ADD(track_msg());

}

void tracker_sermon_process_free(const char *file, const unsigned int line,
                               const int func_id,
                               const DMALLOC_SIZE byte_size,
                               const DMALLOC_SIZE alignment,
                               const DMALLOC_PNT old_addr,
                               const DMALLOC_PNT new_addr) {

            void* ptr = (void*)old_addr;

            assert(gsermon->_gmonitor->size());
            
            ser_mem_mon::sermon_map_it it = gsermon->_gmonitor->find(ptr);

            if (it == gsermon->_gmonitor->end()) {

                // too many false positives from allocations before we started monitoring...let them go for now
                // so we can focus on allocations we are monitoring.
                return;
                #if 0
                    printf("tracker_sermon() on free/delete: ptr (%p) not found in gmonitor map! Memory corruption likely.\n",
                           ptr);

                    gsermon->dump();
                    assert(0);
                    exit(1);
                #endif
            }

            long delme_ser = it->second;

            if (leaks && leaks->is_codepoint(delme_ser)) {
                printf("at DEL DEL DEL codepoint [DELETION SIDE] for known codepoint %ld  at %p ... ought "
                       "to be a breakpoint here to inspect the stack... sermon.cpp:159 or so.\n", delme_ser,ptr);
                printf("\n"); // a line to put a breakpoint on.
            }
            
            // confirm...
            ser_mem_mon::ser2ptr_it  jt = gsermon->_s2p->find(delme_ser);
            if (jt == gsermon->_s2p->end()) {
                printf("tracker_sermon() on free/delete: sermon serial # %ld  not found in _s2p map! Memory corruption likely.\n",
                       delme_ser);
                gsermon->dump();
                assert(0);
                exit(1);
            }
            // consistency check... that our 2 maps aggree
            void* stored_ptr = jt->second;
            assert(stored_ptr == ptr);


            if (gsermon->_stop_requested_free && gsermon->_stop_requested_free == delme_ser) {
                printf("tracker_sermon(): FREE FREE FREE stopping at %ld _stop_requested_free on free.\n",gsermon->_stop_requested_free);
                MLOG_JMSYNC();
                assert(0);
            }

            track_msg.reinit("FFFFFF %p generic ser mon pointer deleted: @sermon_num:%ld\n", ptr, delme_ser);
            MLOG_ADD(track_msg());

            static_tracking_suspended = TRUE;
            gsermon->_gmonitor->erase(it);
            gsermon->_s2p->erase(jt);
            static_tracking_suspended = FALSE;

}

void tracker_sermon(const char *file, const unsigned int line,
                               const int func_id,
                               const DMALLOC_SIZE byte_size,
                               const DMALLOC_SIZE alignment,
                               const DMALLOC_PNT old_addr,
                               const DMALLOC_PNT new_addr) {


    if (static_tracking_suspended) return;

    //    void* ptr = 0;

    switch(func_id) {

    case DMALLOC_FUNC_MALLOC: 
    case DMALLOC_FUNC_CALLOC: 
    case DMALLOC_FUNC_STRDUP: 
    case DMALLOC_FUNC_NEW:
        {
            tracker_sermon_process_malloc(file, line, func_id, byte_size, alignment, old_addr, new_addr);
        }
        break;
        
    case DMALLOC_FUNC_FREE: 
    case DMALLOC_FUNC_DELETE:
        {
            tracker_sermon_process_free(file, line, func_id, byte_size, alignment, old_addr, new_addr);
        }
        break;

    case DMALLOC_FUNC_REALLOC:

            tracker_sermon_process_free(file, line, func_id, byte_size, alignment, old_addr, new_addr);
            tracker_sermon_process_malloc(file, line, func_id, byte_size, alignment, old_addr, new_addr);

        break;
    default:
        // somebody is using a function we don't handle...better implement support for it above!
        assert(0);
    }

}
    
    /* register like this:    
       dmalloc_track(SERIALNUM_DMALLOC_TRACKER);
    */

#ifdef __cplusplus
}
#endif
