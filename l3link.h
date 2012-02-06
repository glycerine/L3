//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef L3LINK_H
#define L3LINK_H

#include "terp.h"
#include "dstaq.h"

// l3obj wrapper for lnk


struct linkobj {

    lnk* _lnk;


    linkobj(l3obj* tar, Tag* owner, const char* key);
    linkobj(const linkobj& src);
    linkobj& operator=(const linkobj& src);
    ~linkobj();

    void  dump(const char* indent, stopset* stoppers) {
        lnk_print(_lnk, (char*)indent,stoppers);
    }
 private:
    void CopyFrom(const linkobj& src);

};

// api
l3obj* make_new_link(l3obj* tar, const char* key, Tag* retown, l3obj* parent_env);

L3FORWDECL(rename)

#endif /* L3LINK_H */

