//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef ADDALIAS_H
#define ADDALIAS_H

// definitions in llref.cpp

struct _l3obj;
struct llref;

 // arg order: env-name-obj (E.N.O.)

// try going to char* instead of l3path* for name_to_insert
// void add_alias_eno(_l3obj* env_to_insert_in, l3path* name_to_insert, _l3obj* target); 

llref*   add_alias_eno(_l3obj* env_to_insert_in, const qqchar& name_to_insert, _l3obj* target);
llref*   rm_alias(_l3obj* env_to_rm_from, const qqchar& name_to_rm);

#endif /*  ADDALIAS_H */

