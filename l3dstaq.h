//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef L3DSTAQ_H
#define L3DSTAQ_H

#include "terp.h"
#include "dstaq.h"

// l3obj wrapper for dstaq


class dstaqobj {

    dstaq<lnk> _symlinks;
 public:

    // ctor must have a tag to save new lnk on.
    dstaqobj(Tag* tag);

    dstaqobj(const dstaqobj& src);
    dstaqobj& operator=(const dstaqobj& src);
    ~dstaqobj();

    void   push_back(l3obj* val, char* key);
    l3obj* pop_back();
    l3obj* back();

    void   push_front(l3obj* val, char* key);
    l3obj* pop_front();
    l3obj* front();

    l3obj* del_ith(long i);

    //    l3obj* ith(long i);
    l3obj* ith(long i, dstaq<lnk>::ll** phit);

    // currently there is no way to delete by key/value
    //  Instead, you should create an lobj with named components, each
    //    of which can be a dstaq. That efficiely makes a multimap (multi-map, multi_map, multi map) out of an l3obj.
    //
    //    l3obj* remove_val(l3obj* val);
    //    l3obj* remove_key(char*  key);

    inline long  len()  const { return _symlinks.size(); }
    inline long  size() const { return _symlinks.size(); }

    void  dump(const char* pre, stopset* stoppers) {
        _symlinks.dump((char*)pre,stoppers,"");
    }
    void  clear();

};

void dq_pushback_api(l3obj* dqobj, l3obj* val, l3obj* optional_name);

long dq_len_api(l3obj* dqobj);

l3obj* dq_ith_api(l3obj* dqobj, long i, l3path* key);

void dq_print(l3obj* l3dq, const char* indent, stopset* stoppers);


// make a new dstaq, in terp.h

// programmatic api here:
L3FORWDECL(make_new_dq)

//L3FORWDECL(dq_clear)

// expression invocation here:
//L3FORWDECL(dq)
                 
//L3FORWDECL(dq_dtor)
//L3FORWDECL(dq_cpctor)
//L3FORWDECL(dq_del_ith)

//L3FORWDECL(dq_ith)

//L3FORWDECL(dq_len)
//L3FORWDECL(dq_size)
//L3FORWDECL(dq_find_name)
//L3FORWDECL(dq_find_val)

//L3FORWDECL(dq_erase_name)
//L3FORWDECL(dq_erase_val)

//L3FORWDECL(dq_front)
//L3FORWDECL(dq_pushfront)
//L3FORWDECL(dq_popfront)

//L3FORWDECL(dq_back)
//L3FORWDECL(dq_pushback)
//L3FORWDECL(dq_popback)




#endif /* L3DSTAQ_H */

