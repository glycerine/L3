#include <iostream>
#include <fstream>
#include <string>
#include <sys/time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>		// for varargs.
#include <string.h>		// for str*().
#include <errno.h>
#include <ctype.h>		// for isspace(), etc.
#include <time.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <uuid/uuid.h>

#include <zmq.h>
#include "l3ts.pb.h"
#include "l3path.h"
#include "l3ts_common.h"
#include "rmkdir.h"
#include "l3obj.h"
#include "autotag.h"
#include "terp.h"
#include "objects.h"
#include <vector>
#include "l3string.h"
#include "jlmap.h"


// storage: use the sparse lookup facilities of the judyL array, indexed by integer.
//  the sparse lookup is worth it!
//


L3METHOD(string_dtor)
{
   int rc = 0;

   if (obj && obj->_judyL) {
     JLFA(rc, obj->_judyL);
   }
}
L3END(string_dtor)


l3obj* make_new_string_obj(const qqchar& s, Tag* owner, const qqchar& varname) {
    FILE* ifp = 0;

    l3obj* obj = 0;
    l3path basenm("make_new_string_obj_");
    basenm.pushq(varname);

    make_new_captag((l3obj*)&basenm ,0,0,  0,&obj,owner,  0,t_str, owner,ifp);
    LIVEO(obj);

    obj->_type = t_str;
    obj->_dtor = &string_dtor;

    // array should start empty.
    assert(obj->_judyL == 0);
    
    if (s.len()) {
        string_set(obj,0,s);
    }
    return obj;
}


l3obj* string_set(l3obj* obj, long i, const qqchar& key) {
        assert(obj->_type == t_str || obj->_type == t_lit);

        assert(obj->_mytag);
        
        char* keydup = obj->_mytag->strdup_qq(key);
        
        LIVEO(obj);
        char**   ps = 0;
        PWord_t   PValue = 0;
        JLI(PValue, obj->_judyL, i);
        
        ps = (char**)(PValue);
        if (*ps != 0) {
            // replace the old first
            void* found = obj->_mytag->atom_remove(*ps);
            assert(found);
            ::free(found);
        }
        
        *ps = keydup;
        return obj;
}



void string_get(l3obj* obj, long i, l3path* val) {
    assert(obj->_type == t_str || obj->_type == t_lit);
    assert(val);

    PWord_t   PValue = 0;
    char** ps = 0;
    JLG(PValue,obj->_judyL,i);
    if (!PValue) {
        val->clear();
        return;
    }
    ps = (char**) (PValue);
    char* s = *ps;
    assert(s);
    val->pushf("%s",s);
    
}



// returns false if empty vector, else true
bool string_first(l3obj* obj, char** val, long* index) {
    assert(obj->_type == t_str);
    LIVEO(obj);
    assert(index);
    assert(val);

    Word_t * PValue;                    // pointer to array element value
    Word_t Index = 0;
    
    JLF(PValue, obj->_judyL, Index);
    if (PValue) {
        *index = Index;
        *val = *(char**)(PValue);
        return true;
    }

    return false;
}

// return false if no more after the supplied *index value
//
bool string_next(l3obj* obj, char** val, long* index) {
    assert(obj->_type == t_str);
    LIVEO(obj);
    
    Word_t * PValue = 0;
    Word_t Index = *index;
    
    JLN(PValue, obj->_judyL, Index);
    if (PValue) {
        *val = *((char**)(PValue));
        *index = Index;
        return true;
    }
    
    return false;
}


bool string_is_sparse(l3obj* obj) {
    assert(obj->_type == t_str || obj->_type == t_lit);
    return is_sparse_array(obj);
}

void string_set_sparse(l3obj* obj) {
    assert(obj->_type == t_str || obj->_type == t_lit);
    set_sparse_array(obj);
}

void string_set_dense(l3obj* obj) {
    assert(obj->_type == t_str || obj->_type == t_lit);
    set_notsparse_array(obj);
}

void string_print(l3obj* obj, const char* indent) {
    LIVEO(obj);
    assert(obj->_type==t_str);

    long str_sz = string_size(obj);
    l3path s;
    for (long i = 0; i < str_sz; ++i) {
        s.clear();
        string_get(obj,i,&s);
        printf("%s[%02ld] \"%s\"\n",indent, i, s());
    }
}


// get the number of strings stored
long string_size(l3obj* obj) {
    //tmpoff    LIVEO(obj);
    Word_t    array_size;
    JLC(array_size, obj->_judyL, 0, -1);

    return (long)array_size;
}
