//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef L3DD_H
#define L3DD_H

#include "terp.h"
#include "mq.h"

// l3obj wrapper for ddlist

class ddqueueobj {

    ddqueue  _symlinks;
 public:

    // ctor must have a tag to save new lnk on.
    ddqueueobj(Tag* tag);

    ddqueueobj(const ddqueueobj& src);
    ddqueueobj& operator=(const ddqueueobj& src);
    ~ddqueueobj();

    void   push_back(t_typ key, l3obj* val);
    l3obj* pop_back();
    l3obj* back();

    void   push_front(t_typ key, l3obj* val);
    l3obj* pop_front();
    l3obj* front();

    l3obj* del_ith(long i);

    l3obj* ith(long i, ddqueue::ll** phit);

    // currently there is no way to delete by key/value
    //  Instead, you should create an lobj with named components, each
    //    of which can be a mqueue. That efficiely makes a multimap (multi-map, multi_map, multi map) out of an l3obj.
    //
    //    l3obj* remove_val(l3obj* val);
    //    l3obj* remove_key(char*  key);

    inline long  len()  const { return _symlinks.size(); }
    inline long  size() const { return _symlinks.size(); }

    void  dump(const char* pre, stopset* stoppers) {
        _symlinks.dump((char*)pre,stoppers,"");
    }
};

void dd_pushback_api(l3obj* mqobj, l3obj* val, l3obj* optional_name);

long dd_len_api(l3obj* mqobj);

l3obj* dd_ith_api(l3obj* mqobj, long i, l3path* key);

void dd_print(l3obj* l3mq, const char* indent, stopset* stoppers);


// make a new mq, in terp.h

// programmatic api here:
L3FORWDECL(make_new_mq)

// expression invocation here:
//L3FORWDECL(dd)
                 
//L3FORWDECL(dd_dtor)
//L3FORWDECL(dd_cpctor)
//L3FORWDECL(dd_del_ith)

//L3FORWDECL(dd_ith)

//L3FORWDECL(dd_len)
//L3FORWDECL(dd_size)

//L3FORWDECL(dd_front)
//L3FORWDECL(dd_pushfront)
//L3FORWDECL(dd_popfront)

//L3FORWDECL(dd_back)
//L3FORWDECL(dd_pushback)
//L3FORWDECL(dd_popback)




#endif /* L3DD_H */

