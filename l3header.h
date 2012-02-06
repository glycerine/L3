//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef L3HEADER_H
#define L3HEADER_H

#include "quicktype.h"

typedef unsigned int uint;
struct Tag;

#define VLEN (200)


#define MIXIN_TOHEADER() \
     MIXIN_MINHEADER()   \
     char       _varname[VLEN]; \


 // note that code in set_sha1 depends on knowing that _varname starts *just after* _sha1 in this layout:
 // see definition of JUST_PAST_SHA1_OFFSET below.
 //

typedef struct uhead {
     MIXIN_MINHEADER()
} uh;


//
// otracer : for serialfactory to store details of allocations out-of-line.
//
class otracer {

#define GETSET(Type,Name) private: Type _##Name;       public: inline void   Name(Type tmp) { _##Name = tmp; }   inline Type  Name() { return _##Name; } private:

     GETSET(t_typ,    type)
     GETSET(long,     ser)

     GETSET(uint,     reserved)
     GETSET(uint,     malloc_size)

     GETSET(Tag*,     owner)
     GETSET(void*,    selfptr)
     GETSET(char*,    strdupped_key)

  public:

    otracer() {
        bzero(this,sizeof(otracer));
    }

    ~otracer() {
        if (_strdupped_key) {
            ::free(_strdupped_key);
        }
    }
 private:
    otracer(const otracer& src);
    otracer&  operator=(const otracer& src);
};



#endif /*  L3HEADER_H */

