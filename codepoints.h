//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef CODEPOINTS
#define CODEPOINTS

#include <set>
#include "l3path.h"
#include "rmkdir.h"

struct codepoints {

    std::set<long> _pt;

    long extractpoint(l3path& line) {
        const char phrase[] = "@sermon_num:";
        char* p = line();
        char* num = 0;
        while(*p) {
            if (0==strncmp(p,phrase,sizeof(phrase)-1)) {
                num = p+sizeof(phrase)-1;
                return atol(num);
            }
            ++p;
        }
        return 0;
    }

    codepoints() {

        file_ty ty;
        long sz = 0;
        l3path fn("leakpoints");
        l3path line;
        long newpt = 0;
        if(file_exists(fn(), &ty, &sz)) {
            FILE* f = fopen(fn(),"r");
            char* status = 0;
            while(1) {
                status = fgets(line(),PATH_MAX,f);
                if (0==status) break;
                newpt = extractpoint(line);
                _pt.insert(newpt);
            }
            fclose(f);

        }
    }

    bool is_codepoint(long query) {
        if (0==_pt.size() || _pt.find(query) == _pt.end()) return false;
        return true;
    }

    void dump() {
        std::set<long>::iterator be,en;
        be =_pt.begin();
        en =_pt.end();
        long i = 0;
        for(; be != en; ++be, ++i) {
            printf("leaks[%02ld] = %ld\n",i,*be);
        }
    }

};

#endif /* CODEPOINTS */

