//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef MEMSCAN
#define MEMSCAN

#include <set>
#include <stdlib.h>
#include "l3path.h"
#include "rmkdir.h"
#include "dv.h"


/* example:

        if (enDV) {
            mscan->load();
            foundhere = mscan->query(im_being_lost);
        }
*/

struct memregion {
    char** _beg;
    char** _end;
    memregion(char** b, char** e) : _beg(b), _end(e) {}
};

bool operator< (const memregion& a, const memregion& b) {
    return a._beg < b._beg;
}

struct memscanner {

    std::set<memregion> _pt;
    bool writable_started;
    bool done;
    bool loaded;
    memregion newreg;
    
    bool newreg_loaded(l3path& line) {
        if (done) return false;
        if (line.len() < 10) return false;

        const char starter[] = "==== Writable regions for process";
        const char stopper[] = "==== Legend";
        if (!writable_started) {
            if (0==strncmp(line(),starter,sizeof(starter)-1)) {
                writable_started = true;
            }
            return false;
        } else {
            // writable_started
            if (0==strncmp(line(),stopper,sizeof(stopper)-1)) {
                done = true;
                return false;
            }
            
            char* p = line();
            assert(p[39]=='-');
            assert(line.len() > 56);
            p[56]='\0';
            p[39]='\0';
            long lbeg = strtol(&p[23],0,16);
            long lend = strtol(&p[40],0,16);
            newreg._beg = *((char***)(&lbeg));
            newreg._end = *((char***)(&lend));
            return true;
        }
        return false;
    }

    memscanner() : newreg(0,0) {
        writable_started = false;
        done = false;
        loaded = false;
    }

    void load() {
        _pt.clear();
        file_ty ty;
        long sz = 0;
        l3path fn("vmmap.terp.out");
        l3path cmd(0,"/usr/bin/vmmap %d > %s",getpid(),fn());
        system(cmd());

        l3path line;
        bool newpt = false;
        if(file_exists(fn(), &ty, &sz)) {
            FILE* f = fopen(fn(),"r");
            char* status = 0;
            while(1) {
                status = fgets(line(),PATH_MAX,f);
                if (0==status) break;
                line.set_p_from_strlen();

                DV(printf("%s\n",line()));

                newpt = newreg_loaded(line);

                if (newpt) {
                    _pt.insert(newreg);
                }
                if (done) break;
            }
            fclose(f);
            loaded=true;
            DV(dump());
        }
    }

    // check if a given pointer is present anywhere in memory, returning where or 0 if not found.
    char** query(char* myquery) {
        if (!loaded) return 0;

        std::set<memregion>::iterator be,en;
        be =_pt.begin();
        en =_pt.end();
        char** lastans = 0;
        long i = 0;
        for(; be != en; ++be, ++i) {
            char** star = (*be)._beg;
            char** stop  = (*be)._end;
            for (char** j = star ; j < stop; ++j) {
                if (*j == myquery) {
                    printf("memscan: found query %p in memory at   %p\n", myquery, j);
                    lastans = j;
                }
            }

        }
        return lastans;
    }

    void dump() {
        std::set<memregion>::iterator be,en;
        be =_pt.begin();
        en =_pt.end();
        long i = 0;
        for(; be != en; ++be, ++i) {
            printf("memregion[%02ld] = [%p - %p]\n",i,(*be)._beg, (*be)._end);
        }
    }

};

#endif /* MEMSCAN */

