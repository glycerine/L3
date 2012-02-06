//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//

// qqchar : replacement for char*


#include "l3path.h"
#include "rmkdir.h"
#include "unistd.h"
#include "dv.h"
#include "quicktype.h"
#include "lex_twopointer.h"

#include "l3ts.pb.h"
#include "l3path.h"
#include "l3ts_common.h"
#include "rmkdir.h"
#include "l3obj.h"
#include "autotag.h"
#include "terp.h"
#include "objects.h"
#include "l3string.h"
#include <list>
#include "l3pratt.h"
#include "qexp.h"

// copies are always references unless we also call ownit()
qqchar::qqchar(const qqchar& src) 
:
    b(src.b)
    ,e(src.e)
    ,_owner(0)
{ 
    bzero(_internal, sizeof(_internal)); 
}


qqchar::qqchar(const char* src) :
    b((char*)src)
    ,e(src ? b + strlen(src) : 0)
    ,_owner(0)
{
    bzero(_internal, sizeof(_internal));
}


qqchar::~qqchar() {
    if (_owner) {
        assert(b !=  (&_internal[0]));
        assert(_owner->gen_exists(b));
        _owner->gen_remove(b);
        ::free(b);
    }
}


qqchar& qqchar::operator=(const char* src) {
    b = ((char*)src);
    e = b + strlen(src);
    _owner = 0;
    return *this;
}

qqchar&  qqchar::operator=(const qqchar& src) {
    if (this != &src) {
        clear(); // release any previous malloced memory.
        b = src.b;
        e = src.e;
    }
    return *this;
}

std::ostream& operator<<(std::ostream& os, const qqchar& qq) {
    os.write(qq.b, qq.e - qq.b);
    return os;
}


long tokenspan::copyto(char* s, ulong cap) {
        assert(s);
        if (!_tdo) return 0;
        if (!cap) return 0;
        assert(_e >= _b);
        
        return _tdo->copyto(s,cap,*this);
    }


// ownit: "make your own copy of b"-- on heap, or in _intern
// problematic: in the face of a qqchar allocated on the stack and then throwing an exception, the malloced memory will leak.
// so: pass in a owner Tag* on which to store it if need be, and to clean up in case of exception.

void qqchar::ownit(Tag* owner) {

    if (b == &_internal[0]) return; // already own it, in _internal.

    if (_owner) { 
        assert(_owner->gen_exists(b));
        assert(b != &_internal[0]);

        if (_owner == owner) {
            return; // already own it.

        } else {

            // owned, but want a change of ownership
            _owner->gen_remove(b); // tell old owner: forget about it.
            owner->gen_push(b);    // tell new owner: remember this.
            
            _owner = owner;        // note the new owner
            return;
        }

        return; // redundant, but clear.
    }

    assert(0==_owner);

    // note what we have to start with, 
    char* src_b = b;
    
    // we cannot assume we have a null-terminated string.
    long z = sz();
    
    if(z < (long)sizeof(_internal)) {
        
        // small enough string, use _internal
        b = &_internal[0];
        // already true: _owner = 0;

    } else {
        
        // too big for _internal, malloc on the heap.
        b = (char*)malloc(z+1);
        _owner = owner;
        _owner->gen_push(b);
    }
    
    memcpy(b, src_b, z);
    e = b + z;
    b[z]=0; // null terminate, so we can refer to them as strings.

}



void qqchar::transfer_to(Tag* new_owner) {

    if (b == &_internal[0]) return; // already own it, in _internal, and no transfer needed.

    if (_owner) { 
        assert(_owner->gen_exists(b));
        assert(b != &_internal[0]);

        if (_owner == new_owner) {
            return; // no transfer needed.

        } else {

            // do the transfer
            _owner->gen_remove(b); // tell old owner: forget about it.
            new_owner->gen_push(b);    // tell new owner: remember this.
            
            _owner = new_owner;        // note the new owner
            return;
        }

        return; // redundant, but clear.
    }

    ownit(new_owner);
}


void qqchar::clear() {
    if (_owner) {
        assert(b !=  (&_internal[0]));
        assert(_owner->gen_exists(b));
        _owner->gen_remove(b);
        ::free(b);
        _owner = 0;
    }

    b = 0;
    e = 0;
    bzero(_internal, sizeof(_internal));
}
