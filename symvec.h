//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef SYMBOLVEC_H
#define SYMBOLVEC_H

#include "autotag.h"
#include "objects.h"
#include "l3obj.h"
#include "terp.h"
#include "l3pratt.h"
#include "qexp.h"

#include <iostream>
using std::ostream;

L3FORWDECL(test_symvec)

L3FORWDECL(make_new_symvec)


typedef struct _symbol {

   char   _name[SYMBOL_NAME_LEN];
   l3obj* _obj;
   long   _judyLIndex;
   llref* _llr;

    _symbol() {
        init();
    }
    void init() {
        bzero(this,sizeof(_symbol));
    }

    _symbol(const char* nm = 0, l3obj* o = 0, long ind = 0) {
        init();
        
        if (nm) { strncpy(_name,nm,SYMBOL_NAME_LEN-1); }
        if (o) { _obj = o; }
        if (ind) { _judyLIndex = ind; }
        //_llr = 0; // bzero does this for us.
    }

    // default shallow copy ctr and operator= are fine, except for _llr.

    _symbol(const _symbol& s) {
        memcpy(this, &s, sizeof(_symbol));
        _llr = 0;
    }
    _symbol& operator=(const _symbol& s) {
        if (this == &s) return *this;
        memcpy(this, &s, sizeof(_symbol));
        _llr = 0;
    }

} symbol;

ostream& operator<<(ostream& os, const symbol& printme);


//void symvec_pushback(l3obj* symvec, l3path* addname, l3obj* addme);
void internal_symvec_pushback(l3obj* symvec, const qqchar& addname, l3obj* addme);
long symvec_size(l3obj* symvec);

// fills in name as a side effect
symbol* symvec_get_by_number(l3obj* symvec, long i, l3path* nameback, l3obj** ret);

symbol* symvec_get_by_name(l3obj* symvec, const qqchar& name, l3obj** ret);

symbol* symvec_set(l3obj* symvec, long i, const qqchar& addname, l3obj*  val_to_set);


void symvec_print(l3obj* symvec, const char* indent, stopset* stoppers);
void symvec_clear(l3obj* symvec);

bool symvec_simple_linear_search(l3obj* symvec, l3obj* needle);

// _judyL[i]  -   >  symbol*  ->  l3obj*  and _name
// _judyS[_name] ->  symbol* (same one as above)
//
// return the address of the newly malloc-ed symbol struct that was set;

// delete a symbol from the symbol table
void symvec_del(l3obj* symvec, const qqchar& delname);

L3FORWDECL(quote)
L3FORWDECL(symvec_aref)
L3FORWDECL(symvec_setf)
L3FORWDECL(symvec_clear)

#endif // SYMBOLVEC_H
