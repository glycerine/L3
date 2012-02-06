//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef L3_SEXP_OBJ_H
#define L3_SEXP_OBJ_H

#include "terp.h"


 long sexpobj_num_children(l3obj* obj);
 void sexpobj_text(l3obj* obj, l3path* out);
 void sexpobj_get_sexp(l3obj* obj, qtree** out);
 void sexpobj_set_sexp(l3obj* obj, qtree* in);


// convert from tdop
L3FORWDECL(sexp)

L3FORWDECL(sexpobj_dtor)
L3FORWDECL(sexpobj_cpctor)


#endif /* L3_SEXP_OBJ_H */

