#ifndef _L3STRING_H_
#define _L3STRING_H_

#include "jlmap.h"
#include "l3obj.h"
#include "autotag.h"
#include "qexp.h"

L3FORWDECL(string_dtor)

void    string_get(l3obj* obj, long i, l3path* val);
l3obj*  string_set(l3obj* obj, long i, const qqchar& key);

l3obj* make_new_string_obj(const qqchar& s, Tag* owner, const qqchar& varname);

//
// sparse indexing is used for strings... so string_size() returns the
//  actual number of elements stored; *not* the highest index nor the highest index+1.
//
long    string_size(l3obj* obj);


//
// string_first(): returns false if empty vector, else true
//
bool string_first(l3obj* obj, char** val, long* index);


//
// string_next(): return false if no more after the supplied *index value
//
bool string_next(l3obj* obj, char** val, long* index);


bool string_is_sparse(l3obj* obj);

// sparse indexing
void string_set_sparse(l3obj* obj);

// dense indexing
void string_set_dense(l3obj* obj);


void string_print(l3obj* obj, const char* indent);

long string_size(l3obj* obj);


#endif /* _L3STRING_H_ */


