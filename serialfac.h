//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef SERIALFAC_H
#define SERIALFAC_H


 #define _ISOC99_SOURCE 1 // for NAN
 #include <math.h>
 #include <stdio.h>
 #include <limits.h>
 #include <climits>
 #include <time.h>
 #include <zmq.hpp>
 #include "l3obj.h"
 #include "autotag.h"
 #include "xcep.h"
 #include "merlin.pb.h"
 #include "qexp.h"
// #include <ffi.h>
 #include <cxxabi.h>
 #include <stdlib.h>
 #include <stdint.h>
 #include <stdio.h>
 #include <unistd.h>
 #include <string.h>
 #include <string>
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <unistd.h>
 #include "rmkdir.h"
 #include <libgen.h>
 #include <assert.h>
 using std::string;

#include <uuid/uuid.h>

#ifndef _MACOSX
 #define HAVE_DECL_BASENAME 1
 #include <demangle.h>
#endif

 #define JUDYERROR_SAMPLE 1
 #include <Judy.h>  // use default Judy error handler
 // quick type system; moved to quicktype.h/cpp
 #include "quicktype.h"

struct uuidstruct {
    uuid_t uuid;
};

// typedefs

typedef std::tr1::unordered_map<std::string, void*>  hashmap_key2ptr;
typedef hashmap_key2ptr::iterator      key2ptr_it;

typedef std::tr1::unordered_map<void*, long>         hashmap_ptr2serial;
typedef hashmap_ptr2serial::iterator   ptr2serial_it;

typedef std::tr1::unordered_map<long, void*>         hashmap_serial2ptr;
typedef hashmap_serial2ptr::iterator   serial2ptr_it;

typedef std::tr1::unordered_map<void*, std::string>  hashmap_ptr2key;
typedef hashmap_ptr2key::iterator                    ptr2key_it;

typedef std::tr1::unordered_map<void*, uuidstruct>       hashmap_ptr2uuid;
typedef hashmap_ptr2uuid::iterator                   ptr2uuid_it;

typedef std::tr1::unordered_map<std::string, void*>      hashmap_uuid2ptr;
typedef hashmap_uuid2ptr::iterator                   uuid2ptr_it;


 // globals
 struct serialnum_f {
   
   static long         last_serialnum;

   static long         count_reg_calls;
   static long         count_serialnum_calls;

   static hashmap_key2ptr      key2ptr;
   static hashmap_ptr2key      ptr2key;

   static hashmap_ptr2serial   ptr2serial;
   static hashmap_serial2ptr   serial2ptr;

   static hashmap_ptr2uuid  ptr2uuid;
   static hashmap_uuid2ptr  uuid2ptr;

   static long _stop_on_allocation_sn;
   static long _stopline;

   // check invariants hold: all maps the same size.
   enum enSERIALKIND { OBJSER=0, TAGSER=1, SXPSER=2 }; 

   void check();
   long serialnum_priv(void* val, Tag* owntag, const char* key);

 public:

   long serialnum_obj(void* obj, Tag* owntag, const char* key /* key == varname */ );
   long serialnum_tag(Tag*  tag, Tag* owntag, const char* key /* key == varname */ );
   long serialnum_sxp(void*  sxp, Tag* owntag);

   long  del(void* val, l3path* name_freed); // return serialnum, and name_freed
   void  show_named_mem();
   uint  size();

   void halt_on_sn(long stopon_sn);
   long get_halt_on_sn();

   long get_last_issued_sn();
   long get_stopline();
   void dump();

   // returns 1 if all objects on heap are marked sysbuiltin, 0 otherwise
   // one exception (for the current command) allowed, in exclude_me.
   int  heap_all_builtin(qtree* exclude_me);

   void check_alive_ptr_or_assert(void* v);
   void check_alive_sn(long sn);
   bool is_alive_sn(long sn);

   void  ptr_to_uuid(void* ptr, uuidstruct* uuid_out);
   void* uuid_to_ptr(const std::string& uuid);
   bool  remove_uuid(const std::string& uuid);

   // already have a uuid from an old save, want to assign it to
   // a newly reinflated object, so that its keys can find it.
   void link_ptr_to_uuid(void* ptr, const std::string& uuid_string);

   void* ser2ptr(long sn);

   // scan all known objects for justdeleted l3obj*, in everyone's _judyS : l3obj* -> llref* -> _env
   // dont call this in production code, since it is O(n^2).
   //
     //     void loose_ends_check(l3obj* justdeleted, long sn_justdeleted);
     // one and only one of justdeleted and deltag must be non-nul.
     void loose_ends_check(l3obj* justdeleted, long sn_justdeleted);

 };
extern serialnum_f* serialfactory;

// do liveness checking, to discover references to dead objects/pointers.
// LIVE asserts that this pointer is registered with the serialfactory as alive and good.


// in terp.h
//L3FORWDECL(allbuiltin)



#endif /* SERIALFAC_H */

