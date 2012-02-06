//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef L3MQ_H
#define L3MQ_H

#include "terp.h"
#include "mq.h"

// l3obj wrapper for mq


class mqueueobj {

    ddqueue  _symlinks;
 public:

    // ctor must have a tag to save new lnk on.
    mqueueobj(Tag* tag);

    mqueueobj(const mqueueobj& src);
    mqueueobj& operator=(const mqueueobj& src);
    ~mqueueobj();

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

void mq_pushback_api(l3obj* mqobj, l3obj* val, l3obj* optional_name);

long mq_len_api(l3obj* mqobj);

l3obj* mq_ith_api(l3obj* mqobj, long i, l3path* key);

void mq_print(l3obj* l3mq, const char* indent, stopset* stoppers);


// make a new mq, in terp.h

// programmatic api here:
L3FORWDECL(make_new_mq)

// expression invocation here:
//L3FORWDECL(mq)
                 
//L3FORWDECL(mq_dtor)
//L3FORWDECL(mq_cpctor)
//L3FORWDECL(mq_del_ith)

//L3FORWDECL(mq_ith)

//L3FORWDECL(mq_len)
//L3FORWDECL(mq_size)
//L3FORWDECL(mq_find_name)
//L3FORWDECL(mq_find_val)

//L3FORWDECL(mq_erase_name)
//L3FORWDECL(mq_erase_val)

//L3FORWDECL(mq_front)
//L3FORWDECL(mq_pushfront)
//L3FORWDECL(mq_popfront)

//L3FORWDECL(mq_back)
//L3FORWDECL(mq_pushback)
//L3FORWDECL(mq_popback)




#endif /* L3MQ_H */

