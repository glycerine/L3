//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//

#ifndef JMEMLOGGER_H
#define JMEMLOGGER_H

#include "l3path.h"

// make all logging turn-offable with -D_NOLOG

#ifdef _NOLOG 
#define MLOG_ADD(x)
#define MLOG_ADD_SYNC(x)
#define MLOG_ONLY(x)
#define MLOG_JMSYNC()     ((void)0)
#define HLOG_ADD_SYNC(x)  ((void)0)
#define HLOG_ADD(x)       ((void)0)


#else
#define MLOG_ADD(x)       mlog->add(x)
#define MLOG_ADD_SYNC(x)  mlog->add_sync(x)
#define MLOG_ONLY(x) x
#define MLOG_JMSYNC()     mlog->jmsync()
#define HLOG_ADD_SYNC(x)  histlog->add_sync(x)
#define HLOG_ADD(x)       histlog->add(x)

#endif


struct jmemlogger {
  l3path mypath;
  FILE* memlog;
  int memlog_fileno;
  int is_initialized;

    jmemlogger() {
        is_initialized = 0;
    }

    jmemlogger(const char* logpath_dir) {
        l3path memlogpath;
        if (logpath_dir && strlen(logpath_dir) > 0) {
            memlogpath.pushf("%s/",logpath_dir);
        }
        memlogpath.pushf("memlog_%d",getpid());
        init(memlogpath); // startup memory logging
    }

  void init(l3path& path) {
    mypath.init(path());
    memlog=fopen(mypath(),"w");
    if (!memlog) {
        l3path msg(0,"error: could not w open logging file '%s':",path());
        perror(msg());
        assert(0);
        exit(1);
    }
    memlog_fileno = fileno(memlog);
    is_initialized = 1;
  }

  void add(const char* msg);
  void add_sync(const char* msg);
  void jmsync();

  char* get_mypath() { return mypath(); }

    void destruct() {
        if (is_initialized) {
            if (memlog) { 
                fclose(memlog); 
                memlog=0;
            }
            is_initialized = 0;
        }
    }

    ~jmemlogger() {
        destruct();
    }
};



#endif /*  JMEMLOGGER_H */
