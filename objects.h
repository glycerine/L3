//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef OBJECTS_H
#define OBJECTS_H

#include "autotag.h"
#include "l3obj.h"
#include "terp.h"

typedef char label[48];


 struct name_method_pair {
   char       name[40];
   ptr2method method;
 };

typedef double (*bin_op_double)(double a, double b);

struct objstruct {

    label  classname;
    label  objname;
    
    bin_op_double op;
    
    //  std::vector<name_obj_pair>          mem_data;
    std::vector<name_method_pair>       mem_method;
    std::vector<name_method_pair>       mem_finally;
    std::vector<name_method_pair>       mem_ctor;
    std::vector<name_method_pair>       mem_body;
    std::vector<l3obj*>                 mem_dtor;
    
};

void objstruct_objname_set(objstruct* os, const char* newname);
void objstruct_classname_set(objstruct* os, const char* classname);


// if forward_tag_to_me == 0 then we create and
//  use our own tag.
void make_new_obj( const char* classname, 
           const char* objname, 
           Tag* owner, 
           Tag* forward_tag_to_me, 
           l3obj** retval);

// these are all now L3METHOD, listed in terp.h
void*  obj_ctor(void* thisptr, void* expandptr, l3obj** retval);
//void*  obj_trybody(void* function_obj, void* closure_env, l3obj** retval);
//void*  obj_dtor(void* thisptr, void* expandptr, l3obj** retval);

void  print_obj(l3obj* obj,const char* indent, stopset* stoppers);
void dump_objstruct(objstruct* a, const char* indent, stopset* stoppers);

// obj handles env, str, and dou automatically.
L3FORWDECL(deep_copy_obj)

// add a dtor method (add_dtor dtor obj)
//
// obj = new dtor function to add
// env = object to add to
//
L3FORWDECL(dtor_add)

// hold evaluated values ready for a function call to evaluate them.
L3FORWDECL(make_new_ptrvec)


// ptrvec uses the _judyL to store l3obj*, in contrast to
//  env, which store values in the _judyS.
//
//  the _judyL index is the sequential numerical index 0,1,2,... of
//   the order in which l3obj* were pushback()-ed. Not designed
//   to be good a deletion, for which a dq (dstaq) is better.
//
//  we could merge them, but for now they are different,
//  to indicate where and how to get at the values they store.
//
void ptrvec_pushback(l3obj* ptrvec, l3obj* addme);
long ptrvec_size(l3obj* ptrvec);
void ptrvec_get(l3obj* ptrvec, long i, l3obj** ret);
void ptrvec_set(l3obj* ptrvec, long i, l3obj*  val_to_set);
void ptrvec_print(l3obj* ptrvec, const char* indent, stopset* stoppers);
void ptrvec_clear(l3obj* ptrvec);
bool ptrvec_search(l3obj* ptrvec_haystack, l3obj* needle);
bool ptrvec_search_double(l3obj* ptrvec, l3obj* double_needle);

// operations that set and query _parent, _child, _sib
L3FORWDECL(left)
L3FORWDECL(right)
L3FORWDECL(parent)
L3FORWDECL(sib)

L3FORWDECL(set_obj_left)
L3FORWDECL(set_obj_right)
L3FORWDECL(set_obj_parent)
L3FORWDECL(set_obj_sib)

L3FORWDECL(numchild)
L3FORWDECL(firstchild)
L3FORWDECL(ith_child)

//  run the object's trybody method(s); or execute a lambda or function by name.
L3FORWDECL(run) // run is like apply, but with a better name.

void do_obj_init(l3obj* obj, const char* objname, const char* classname);

L3FORWDECL(obj_llref_cleanup)
L3FORWDECL(owner_is_ancestor_of_retown)

void obj_get_obj_class(l3obj* obj, label** po, label** pc);

char* get_objname(l3obj* obj);
char* get_clsname(l3obj* obj);


#endif // OBJECTS_H
