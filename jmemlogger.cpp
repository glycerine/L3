//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//

#include "jmemlogger.h"
#include "dv.h"

/////////////////////// end of includes

#ifdef _MACOSX

#define  fflush_unlocked(x)  fflush(x)
#define  fdatasync(x)     fsync(x)

#endif




void jmemlogger::add(const char* msg) {
    assert(is_initialized);
    if (!is_initialized) return;
    fprintf(memlog,"%s\n",msg);

#ifdef _FFLUSH_EACH_MEMLOG
    //  fflush_unlocked(memlog);
    //  fdatasync(memlog_fileno);
    jmsync();
#endif

}

void jmemlogger::add_sync(const char* msg) {
    assert(is_initialized);
    if (!is_initialized) return;
    add(msg);

#ifdef _FFLUSH_EACH_MEMLOG
    //  fsync(memlog_fileno);
    //  ::sync();
    jmsync();
#endif

}

void jmemlogger::jmsync() {
    assert(is_initialized);
    if (!is_initialized) return;

    if (memlog) {
        ::fflush_unlocked(memlog);
        ::fdatasync(memlog_fileno);
        ::sync();
    }
    DV(printf("jmemlogger::jmsync() done.\n"));
}


