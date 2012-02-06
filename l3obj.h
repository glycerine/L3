//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef L3OBJ_H
#define L3OBJ_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <Judy.h>

#include <string>
#include <list>
#include <vector>
#include <string>
#include <tr1/unordered_map>
#include <map>
#include <set>
#include "l3path.h"

// quick type system
#include "quicktype.h"
#include "tyse.h"
#include "dv.h"
#include "ut.h"
#include "addalias.h"
#include "jlmap.h"
//#include "dstaq.h"
#include <ext/malloc_allocator.h>


//struct elt;
//typedef struct elt sexp_t;

typedef  qtree sexp_t;


#include "bool.h"
#include "l3header.h"


#ifndef _MACOSX

 #define HAVE_DECL_BASENAME 1
 #include <demangle.h>

#else

typedef unsigned long ulong;
#define  isnan(x) __inline_isnand(x)

#define  isinf(x) __inline_isinfd(x)

#define  fflush_unlocked(x)  fflush(x)
#define  fdatasync(x)     fsync(x)

#endif


typedef judySmap<llref*> judys_llref;


//typedef char* t_typ; // refer to one of the above.

struct llref;

// l3obj.cpp, called in terp.cpp during NEW_AUTO_TAG()
void global_function_entry();
void global_function_finally();

// owntags for describing parameter movement

// get/set the reserved bits
 uint get_reserved_bit(uint i, uint v);
 uint set_reserved_bit(uint i, uint v, uint store);
uint get_reserved_bit(uint i, uint startingbitset);
uint set_reserved_bit(uint i, uint startingbitset, uint store);

// replace owntag_en with references to the types t_ico, t_iso, t_oco, t_oso
// but there are still places we need this; in setq in terp.cpp
typedef  enum { bad_owntag=0, in_c_own=1, in_s_own=2, out_c_own=3, out_s_own=4 } owntag_en;

// const char* owntag_en2str(owntag_en ot);

extern char cwd[PATH_MAX+1];
char* pwd();

t_typ get_atom_type(sexp_t* s, double& dval);
bool is_atom(sexp_t* s);
bool is_list(sexp_t* s);

void  dump(sexp_t* s );
char* stringify(sexp_t* s);

void gdump();
void tags();

char* owntagtype2abbrev(t_typ s);

// temp compile with C++.... for the dtor in l3obj
//#ifdef __cplusplus
//extern "C" {
//#endif 


/*
faster than vtable dynamic dispatch in LowLevelLush/Djitia

posted 3 minutes ago by Jason Aten
dynamic dispatch through vtables can be slow.

We can get faster but still runtime dispatch, by doing a small space for time trade-off.  Instead of double indirection from object to vtable, we simple embed the pointers to methods right into the table.

  This has the added benefit that it automatically gives us runtime time information, since the address of the destructor method for a class will always uniquely identify the class.

               -------------------------------------
object* ---->  |  pointer to dtor function         | ------> dtor method
               ------------------------------------|
               |  pointer to trybody func          | ------> try/body method
               ------------------------------------| 
           |  pointer to ctor/reinit function  | ------> ctor/reinit method
               ------------------------------------|  + always have a start and end data pointer, so you can extend an object in place/ or
            /- |  pointer to end+1 of data         |  note: can point to a totally different data area for this objects "Data".   
            |  ------------------------------------|   
            |  |  pointer to start of data         | --\ 
            |  ------------------------------------|   |
            |  |  pointer to end+1 of addl methods |   |
            |  ------------------------------------|   |
            |  |  pointer to start of addl methods | (so if you really wanted speed, you could inline methods into the data area)
            |  -------------------------------------   |
          ...                                          ...
            |  ------------------------------------- (might be contiguous, if known in advance. Or might not--if dynamically grown!)
            |  |        .....                      |   |
            |  -------------------------------------   |
            |  | pointer to last method            |   |
            |  -------------------------------------   |
            |  |    start of data                  | <-/
            |  |                                   |
            |  |         ...                       |
            |  |                                   |
            |  |     end of data                   |
            |  -------------------------------------
            \->


Adv/nice: objects are always, or at least can be, always a fixed size. Seven words only.
          designed for speed of dispatch!
          designed for extensibility.
          designed for finally/dtor methods that happen upon end of stack or upon throw.
          easy to wrap another existing (C++?) object by putting it in the data area (or pointing to it in the data area).
          possible to inline methods for speed into trybody pointer, try body actual body, or in the data area.
 
New design notes:
  With this design, if you allocate on object on the stack, then dispatch to
that objects methods should be as fast as a regular function call. Because
the addresses you want to jump to are *right there* !!! Nice! But still virtual!!! EXCELLENT!

Original notes:

Notice that the we can elide the ctor because it is always known at object creation time, and if really need be, the dtor
can contain a traditional vtable or just a pointer to the ctor. Or the ctor can be listed explicitly as method1. But 
all those are typically unnecessary, so we elide the ctor to save space.

How small can objects be? an empty object would have end of data pointing to itself, and start of data pointing to end of data, and so the minimum object size for a trivial object with no data would be 3 words, or 3*8 = 24 bytes. not bad for a totally dynamically extensible system with faster-than-vtable dispatch!

Actually we can do even better, 16 bytes, which is what the C compiler allocates for us automatically. We just adopt the convention that a start of data pointer that points to itself represents an otherwise empty object. Done!

And by putting the end of data +1 pointer in the 2nd position before the start of data, we can make the convention just be that the end of data points just beyond itself for no other data. This both makes more intuitive sense, and allows automatic chaining of destructors or functions (currying?) or exception calls.

  Thus methods can be shared easily, or modified at runtime. And methods1..N always start at a fixed offset into the object, but can be extended at rutnime. And data can be extended too; the object keeps the start and end of data pointers in itself. And the methods go from object* + 3 pointers to the start of the data. Each method can contain a static pointer to a string identifying itself for runtime introspection.


3 Pictures :

*** OLD DESIGN (see below for latest new design) ***

I. An object with 0 methods and 0 data. Has _pde == &_pde + 1word. 16 bytes.

               -----------------------------
object* ---->  |  pointer to dtor function | ------> dtor method
               -----------------------------
            /- | ptr to end of data _pde |   
            |  -----------------------------   
            \->

we have some data at all iff: _pde > (&_pde) + 1word
in which case we will always have a _pdb (pointer to start of data) too.

call this state: _pdb exists, which happens iff { _pde > (&_pde) + 1word}


I.+ Alternative (non-default) representation of an empty object (0 methods, 0 data) is therefore this 24 byte rep:


Has _pdb exists, *and* _pdb == _pde.
If  _pdb exists, then databytes   = _pde - _pdb; databytes == 0 here.
If  _pdb exists, then methodbytes = ( &_pdb + 1word - _pdb). methodbytes == 0 here.

               -----------------------------
object* ---->  |  pointer to dtor function | ------> dtor method
               -----------------------------
            /- | pointer to end of data    |   
            |  -----------------------------   
            |  | ptr to start data  _pdb | -\
            |  -----------------------------   |
            \->                             <-/

Objects may shrink to this, or pass through this as an intermediate stage in construction.


***NEW DESIGN***

I. An object with 0 methods and 0 data. 7 words, 56 bytes. memory is cheap. speed is not.

               -------------------------------------
object* ---->  |  pointer to dtor function         |
               ------------------------------------|
               |  pointer to trybody func          |
               ------------------------------------|
           |  pointer to ctor/reinit function  |
               ------------------------------------|
               |  pointer to end+1 of data         | or null if no data
               ------------------------------------| 
               |  pointer to start of data         | or null if no data
               ------------------------------------|
               |  pointer to end+1 of addl methods | or null if no addl methods
               ------------------------------------|
               |  pointer to start of addl methods | or null if no addl methods
               -------------------------------------

Notice this is waay simpler.



II. An object with 1 method and 0 data. Has _pdb exists, *and* _pdb == _pde.
    4 words (32 bytes) total.

In general , if databytes + methodbytes > 0, then _pdb will exist too.

If  _pdb exists, then databytes   = _pde - _pdb; databytes == 0 here.
If  _pdb exists, then methodbytes = ( &_pdb + 1word - _pdb). methodbytes == 8 here.
First method is at &_bdbeg + 1word.

               -----------------------------
object* ---->  |  pointer to dtor function | ------> dtor method
               -----------------------------
            /- | pointer to end of data    |   
            |  -----------------------------   
            |  | ptr to start data  _pdb | -\
            |  -----------------------------   |
            |  | pointer to method1 of cls |   |
            |  -----------------------------   |
            \->                             <-/

III. An object with 0 methods and 1 word of data. 4 words (32 bytes) total. Has 

If  _pdb exists, then databytes   = _pde - _pdb; databytes == 8 here.
If  _pdb exists, then methodbytes = ( &_pdb + 1word - _pdb). methodbytes == 0 here.

               -----------------------------
object* ---->  |  pointer to dtor function | ------> dtor method
               -----------------------------
            /- | pointer to end of data    |   
            |  -----------------------------   
            |  |  pointer to start of data | -\
            |  -----------------------------   |
            |  |    one word of data       | <-/
            |  -----------------------------
            \->

IV. An object with 1 method and 1 word of data. 5 words (40 bytes) total.

If  _pdb exists, then databytes   = _pde - _pdb; databytes == 8 here.
If  _pdb exists, then methodbytes = ( &_pdb + 1word - _pdb). methodbytes == 8 here.
First  method is at &_bdbeg + 1word.
Second method is at &_pdb + 2words (but here there is no 2nd method).
...

               -----------------------------
object* ---->  |  pointer to dtor function | ------> dtor method
               -----------------------------
            /- | pointer to end of data+1  |   
            |  -----------------------------   
            |  |  pointer to start of data | -\
            |  -----------------------------   |
            |  | pointer to method1 of cls |   |
            |  -----------------------------   |
            |  |  one word of data         | <-/
            |  -----------------------------
            \->


*/

  // simple 2-bit encoding per owntag parameter. Hence in
  // a 64-bit unsigned long we get tag codes for
  // 32 tag parameters; i.e. one of ot {in_c_own, in_s_own,
  //   out_c_own, out_s_own}. So 32 (numbered 0..31 is the max
  // number of parameters that we support now (use a vector
  // if you need more!)


  //     no_owner_change   trans_tag         trans_tag     no_owner_change
  //                 ']           '>         '<            '[
  //                 in           in         out           out
  //            like const               like ref/pointer
  //  
  //  so ! (not) symbols starts the no-transfer tags
  //  and the @ symbol starts the transfering ownership tags.
  //
#if 0
  // actually this is just the *declaration* of the method, the 
  // user doesn't have to think about this at all!
  @dowork( arg1 !>, arg2 @>,  arg3 @<, arg4 !<  );

byvalue...(by copying)...=> no annotation.

need an easy to parse and turn into runtime code representation of function signatures/declarations/definitions.

this is really an R data frame.
dowork
arg1   int
arg2   l3obj* !>
name3  l3obj* @< 

s-expr?

  method declarations look like this:

(@decl dowork
   (@prop
    (return int)  
    (arg arg1 int       )  ; defaults to pass by value. (server gets a copy; C semantics)
    (arg arg2  l3obj*  !>) ; in.c.own
    (arg name3 l3obj*  !<) ; out.s.own
    (arg name5 l3obj*  @>) ; in.s.own
    (arg name6 l3obj*  @<) ; out.c.own
    )

); declaration finishes without (@body ...)x


(@de dowork        ; defun 
  (@prop
   (return int)  
   (arg arg1 int       )  ; defaults to pass by value. (server gets a copy; C semantics)
   (arg arg2  l3obj*  !>) ; in.c.own
   (arg name3 l3obj*  !<) ; out.s.own
   (arg name5 l3obj*  @>) ; in.s.own
   (arg name6 l3obj*  @<) ; out.c.own
   )
  ... body here...
)

  ; and it works horizontally too.
  (@de dowork (@prop (return int) (arg arg1 int ) (arg arg2 l3obj* !>) (arg arg3 l3obj* @>))

   )


should get turned into the declaration:



  // gets replaced by (an added first argument:
  
  // declared somewhere is an association between 'dowork' -> make_owntag(0,1,2,3)

  at_call_setup('dowork', arg1, arg2, arg3, arg4, ...); : set the global tag transfer...
                              // we need to be able to get 
  dowork( arg1, arg2, arg3, arg4);

  // for <@ (out.c.own) parameters, 
  at_call_clean(arg1, arg2, arg3, arg4);

  // WAY SIMPLER: have this transfer be a property of the object.
  //  so that that it is set upon new/ allocation. Does that work?

  // experiment with tag-transfer being a property of the object, so
  // that client calls are not decorated.

  
OUT: server gives up ownership and client is responsible for the pointer.
4. server_method(@out.c.own SomePtr* ptr_received_from_server)
compiler assigns ownership of someptr to the default tag, which would typically be the current function.
  
    server is going to new up the object in any case, so server knows its giving it
    to client, so server can set a temporary ("transfer me to client" on the object,
    and then upon returning to the client, the compiler can remove that temporary transfer tag.)

    // SIMPLER PROTOCOL:

    p = new_transfer_to_server Obj;
  call_with(p, q);     // server now owns p...for the duration of the call, unless server sets bits different upon return.
  //  auto-inserted checks, after the call
  if (q...or p for that matter)...now has the "client take ownership tag set" {
    clear the temp label                
    then we add ownership of this object to the default tag in force now.
      }

    p = new Obj;
    mark_transfer_to_server(p);
  call_with(p, q);
    // server now owns p
    
  on function entry:
  void call_with(p, q) {
    allocate default autotag for this method; (does it need to be declared volatile?)
    for each formal arg p_i in { p, q, ...} :
    if (has_server_take_ownership_tag(p_i)) {
        clear the temp label.
        add p_i to the default auto tag for this method (top of stack of autotags).
    

    exception handling setup...

      user written code
   trasfer_ownership(from autocontext, to object_more_pernanant tag);

   exception cleanup stuff...
     e.g. delete all temp tag stuff...
    
  }

    in.s.own:

IN: call library and pass object in, where library takes ownership of pointer and will dispose when done.
  action: assign the pointer to the object to the library's object handle, so that when the library finishes, if it hasn't deallocated that memory already, we can do so.
  
1. server_method(@in.s.own SomePtr* ptr_to_pass_to_server); // this is the forward declaration of the method on the server class.

METHOD: upon entry to a method, the compiler can insert scan of all argument objects...if they have a "server take ownership" tag on them, remove that temp label, and add that tag for the object to the temp tag for the call. 

    The code would then manually transfer that tag to more permanant storage (the objects default tag), if it wishes to keep the object around.


#endif /* 0 */



  // end experiment with tag-transfer being a property of the object, so
  // that client calls are not decorated.


                                                        //typedef  enum { in_c_own=0, in_s_own=1, out_c_own=2, out_s_own=3 } owntag_en;

 uint get_owntag(uint i, uint v);
 uint set_owntag(uint i, uint v, uint store);


 /* move these to l3obj.cpp 
uint get_owntag(uint i, uint v) {
  return (uint)(((v & ((uint)3 << (i << 1))) >> (i << 1)));
}

uint set_owntag(uint i, uint v, uint store) {
  return ((uint)(store << (i << 1) | ((~((uint)3 << (i << 1))) & v)));
}
 */    

 struct _l3obj;
 struct Tag;



 // generic L3 function, used to thread all important state into 
 // methods easily.
 //



/* Merlin data layout: dtor is most important part,
   and we want the data beg to be extensible (at the end of the fixed size object,
   so therefore we always do last...first for methods, extra methods, and data. 
*/

/* A merlin object: A C-compatible object layout, that often starts at the
      opposite end of time.

   is a 128 byte struct on 64-bit. This is a great, fast, well-aligned size, keep it there. 
   It already has tons of extensibility built in. e.g. a pointer to a judy hashmap
   for string -> pointer lookup. pointers to form trees or any branching factor; 
   pointers for linked lists for for garbage management. A set of state flags, one
   int reserved for the system, one for users of the merlin object system. 
*/

 class Tag;

 // codes for _reserved:
 const      uint    RESERVED_MYTAG_BIT00 = 0; // 2^(1 + (X from BITX))
 const      uint    RESERVED_MYTAG_BIT01 = 1;
 const      uint    RESERVED_MYTAG_BIT02 = 2;
 const      uint    RESERVED_MYTAG_BIT03 = 3;
 const      uint    RESERVED_MYTAG_BIT04 = 4;
 const      uint    RESERVED_MYTAG_BIT05 = 5;
 const      uint    RESERVED_MYTAG_BIT06 = 6;
 const      uint    RESERVED_MYTAG_BIT07 = 7;
 const      uint    RESERVED_MYTAG_BIT08 = 8;
 const      uint    RESERVED_MYTAG_BIT09 = 9;

 const      uint    RESERVED_MYTAG_BIT10 = 10;
 const      uint    RESERVED_MYTAG_BIT11 = 11;
 const      uint    RESERVED_MYTAG_BIT12 = 12;
 const      uint    RESERVED_MYTAG_BIT13 = 13;
 const      uint    RESERVED_MYTAG_BIT14 = 14;
 const      uint    RESERVED_MYTAG_BIT15 = 15;
 const      uint    RESERVED_MYTAG_BIT16 = 16;
 const      uint    RESERVED_MYTAG_BIT17 = 17;
 const      uint    RESERVED_MYTAG_BIT18 = 18;
 const      uint    RESERVED_MYTAG_BIT19 = 19;

 const      uint    RESERVED_MYTAG_BIT20 = 20;
 const      uint    RESERVED_MYTAG_BIT21 = 21;
 const      uint    RESERVED_MYTAG_BIT22 = 22;
 const      uint    RESERVED_MYTAG_BIT23 = 23;
 const      uint    RESERVED_MYTAG_BIT24 = 24;
 const      uint    RESERVED_MYTAG_BIT25 = 25;
 const      uint    RESERVED_MYTAG_BIT26 = 26;
 const      uint    RESERVED_MYTAG_BIT27 = 27;
 const      uint    RESERVED_MYTAG_BIT28 = 28;
 const      uint    RESERVED_MYTAG_BIT29 = 29;

 const      uint    RESERVED_MYTAG_BIT30 = 30;
 const      uint    RESERVED_MYTAG_BIT31 = 31;

// meaningful names for the above
 const      uint    RESERVED_FORWARDED_MYTAG    = RESERVED_MYTAG_BIT00;
 const      uint    RESERVED_ISCAPTAG           = RESERVED_MYTAG_BIT01;
 const      uint    RESERVED_INVISIBLE          = RESERVED_MYTAG_BIT02;
 const      uint    RESERVED_SEALED_ENV         = RESERVED_MYTAG_BIT03;
 const      uint    RESERVED_DTOR_DONE          = RESERVED_MYTAG_BIT04;
 const      uint    RESERVED_UNDELETABLE        = RESERVED_MYTAG_BIT05;
 const      uint    RESERVED_SINGLETON          = RESERVED_MYTAG_BIT06;
 const      uint    RESERVED_SYSBUILTIN         = RESERVED_MYTAG_BIT07;
 const      uint    RESERVED_CLASSPRIVATE       = RESERVED_MYTAG_BIT08;
 const      uint    RESERVED_DANGLEABLE_LLREF   = RESERVED_MYTAG_BIT09;
 const      uint    RESERVED_SYM_LINK           = RESERVED_MYTAG_BIT10;
 const      uint    RESERVED_SPARSE_ARRAY       = RESERVED_MYTAG_BIT11;


#define RES_TARG(ooo) (ooo)->_reserved

 // query:
#define is_forwarded_tag(obj)     get_reserved_bit(RESERVED_FORWARDED_MYTAG, RES_TARG(obj))

 // the default... so if _mytag is zero then the object has no default tag for new alloations.
 // say that _mytag is actually forwarded, which means don't tell the tag to delete all at when this object dies.
 //  used by smaller objects (doubles)
#define set_forwarded_tag(obj)    { RES_TARG(obj) = set_reserved_bit(RESERVED_FORWARDED_MYTAG, RES_TARG(obj), 1); }

 //
 // set the default: (always set for callobjects, closures, and objects that have their own internal heap allocated variables)
#define set_notforwarded_tag(obj) { RES_TARG(obj) = set_reserved_bit(RESERVED_FORWARDED_MYTAG, RES_TARG(obj), 0); }


 // captags are a pair of circularly owned captain l3obj* and Tag; they need special handling, so the captain objects
 // get a special bit
#define is_captag(obj)     get_reserved_bit(RESERVED_ISCAPTAG, RES_TARG(obj))
#define set_captag(obj)    { RES_TARG(obj) = set_reserved_bit(RESERVED_ISCAPTAG, RES_TARG(obj), 1); }
#define set_notcaptag(obj) { RES_TARG(obj) = set_reserved_bit(RESERVED_ISCAPTAG, RES_TARG(obj), 0); }


// sealed: locked, can't delete or change from these environments
#define is_sealed(obj)     get_reserved_bit(RESERVED_SEALED_ENV, RES_TARG(obj))
#define set_sealed(obj)    { RES_TARG(obj) = set_reserved_bit(RESERVED_SEALED_ENV, RES_TARG(obj), 1); }
#define set_notsealed(obj) { RES_TARG(obj) = set_reserved_bit(RESERVED_SEALED_ENV, RES_TARG(obj), 0); }


// invisible: not printed by default, like R.
#define is_invisible(obj)     get_reserved_bit(RESERVED_INVISIBLE, RES_TARG(obj))
#define set_invisible(obj)    { RES_TARG(obj) = set_reserved_bit(RESERVED_INVISIBLE, RES_TARG(obj), 1); }
#define set_notinvisible(obj) { RES_TARG(obj) = set_reserved_bit(RESERVED_INVISIBLE, RES_TARG(obj), 0); }


// during cleanup, the dtor_done bit tells us that we've already run the dtor for this object.
#define is_dtor_done(obj)     get_reserved_bit(RESERVED_DTOR_DONE, RES_TARG(obj))
#define set_dtor_done(obj)    { RES_TARG(obj) = set_reserved_bit(RESERVED_DTOR_DONE, RES_TARG(obj), 1); }
#define set_notdtor_done(obj) { RES_TARG(obj) = set_reserved_bit(RESERVED_DTOR_DONE, RES_TARG(obj), 0); }

// undeletable flag is set on main_env and others that we dont want to be able to delete
#define is_undeletable(obj)     get_reserved_bit(RESERVED_UNDELETABLE, RES_TARG(obj))
#define set_undeletable(obj)    { RES_TARG(obj) = set_reserved_bit(RESERVED_UNDELETABLE, RES_TARG(obj), 1); }
#define set_notundeletable(obj) { RES_TARG(obj) = set_reserved_bit(RESERVED_UNDELETABLE, RES_TARG(obj), 0); }

// singleton: you cannot copy or duplicate this object directly (like for FILE* proxies) without
//  using a smarter method (aka a factory), but you can delete it (e.g. close the file) at will. Hence
//  singleton is distinct from undeletable and sealed.
#define is_singleton(obj)     get_reserved_bit(RESERVED_SINGLETON, RES_TARG(obj))
#define set_singleton(obj)    { RES_TARG(obj) = set_reserved_bit(RESERVED_SINGLETON, RES_TARG(obj), 1); }
#define set_notsingleton(obj) { RES_TARG(obj) = set_reserved_bit(RESERVED_SINGLETON, RES_TARG(obj), 0); }

// sysbuiltin: distinguish system parts from user defined parts.
#define is_sysbuiltin(obj)     get_reserved_bit(RESERVED_SYSBUILTIN, RES_TARG(obj))
#define set_sysbuiltin(obj)    { RES_TARG(obj) = set_reserved_bit(RESERVED_SYSBUILTIN, RES_TARG(obj), 1); }
#define set_notsysbuiltin(obj) { RES_TARG(obj) = set_reserved_bit(RESERVED_SYSBUILTIN, RES_TARG(obj), 0); }

// classprivate: don't let outsiders change these objects
#define is_classprivate(obj)     get_reserved_bit(RESERVED_CLASSPRIVATE, RES_TARG(obj))
#define set_classprivate(obj)    { RES_TARG(obj) = set_reserved_bit(RESERVED_CLASSPRIVATE, RES_TARG(obj), 1); }
#define set_notclassprivate(obj) { RES_TARG(obj) = set_reserved_bit(RESERVED_CLASSPRIVATE, RES_TARG(obj), 0); }


// reasonable defaults for new objects: not forwarded, not in the middle of deletion.
//define set_newborn_obj_default_flags(obj) { set_notcaptagdelstarted_tag(obj); set_notforwarded_tag(obj); set_notcaptag(obj); set_notsealed(obj); }
//#define set_newborn_obj_default_flags(obj) { set_notforwarded_tag(obj); set_notcaptag(obj); set_notsealed(obj); }
#define set_newborn_obj_default_flags(obj) { RES_TARG(obj) = 0; }


// dangleable_llref: if set, when the llref is "deleted", replace with a llref->_obj = 0, owned by the env->_mytag.
#define is_dangleable_llref(obj)     get_reserved_bit(RESERVED_DANGLEABLE_LLREF, RES_TARG(obj))
#define set_dangleable_llref(obj)    { RES_TARG(obj) = set_reserved_bit(RESERVED_DANGLEABLE_LLREF, RES_TARG(obj), 1); }
#define set_notdangleable_llref(obj) { RES_TARG(obj) = set_reserved_bit(RESERVED_DANGLEABLE_LLREF, RES_TARG(obj), 0); }

// sym_link: this llref is not found in the env, so don't try to delete this key from there, in case there is another key of the same name.
#define is_sym_link(obj)     get_reserved_bit(RESERVED_SYM_LINK, RES_TARG(obj))
#define set_sym_link(obj)    { RES_TARG(obj) = set_reserved_bit(RESERVED_SYM_LINK, RES_TARG(obj), 1); }
#define set_notsym_link(obj) { RES_TARG(obj) = set_reserved_bit(RESERVED_SYM_LINK, RES_TARG(obj), 0); }


// sparse_array: if sparse indexing is in use, rather than dense 0..(size-1) indexing.
#define is_sparse_array(obj)     get_reserved_bit(RESERVED_SPARSE_ARRAY, RES_TARG(obj))
#define set_sparse_array(obj)    { RES_TARG(obj) = set_reserved_bit(RESERVED_SPARSE_ARRAY, RES_TARG(obj), 1); }
#define set_notsparse_array(obj) { RES_TARG(obj) = set_reserved_bit(RESERVED_SPARSE_ARRAY, RES_TARG(obj), 0); }


// forward decl
class lnk;

typedef struct _l3obj {

    MIXIN_TOHEADER()

    //    char          _sha1[48];  // _varname should start right after _sha1, so we know where to hash from.
    
    
            /* the actual Tag instance owned by this object. 
            _mytag is used by callobjects for temporaries.
            _mytag is used by objects to keep local state they own.
            _mytag is used by closures to hold copies of the closed over variables.
            _mytag can be a forwarding Tag, in which case all instances of
              allocation actions are forwarded to another Tag*. In reality this
                  forwarding is implemented by writing the address of the forwarded to
              Itag* here in _mytag and simultaneously setting bit 0 of the reserved
              bits to 1 to indicate that none of the tag cleanup actions should
              be invoked upon object destruction. 
             */
  Tag*     _mytag;

    judys_llref*  _judyS;
    //  void*      _judyS;  /* pointer to hash table - judy map of string -> pointer */

  /* _judyL is also overloaded in make_new_actual_call_param_obj() and make_new_sexp() to hold an sexp. */

  /* In general _judyL is where the actual bulk data of an object may be pointed to; e.g. since it
     holds the vector of data when an actual judyL is in use */

  /* It might be better to name this _data, but _judyL is much more greppable and distinct. */
  void*      _judyL;  /* pointer to sparse numerical or pointer (double, long, void*) vector- judy map of word -> word */


  /* We use Merlin ordering: die, live, born methods. */
  ptr2method _dtor;
  ptr2method _trybody;
  ptr2method _ctor;

  ptr2method _cpctor; // and copy method

  // environments I'm in...
  struct _l3obj*     _parent_env; 
  struct _l3obj*     _myname_livesin_env; // I have been inserted into this env as a named variable, if non 0x0.

    // merge the lnk features, make it lnk and l3obj one; so any object can also be a symbol
    llref*     _link_llr; // owner will zero this if the object is deleted. and _target=0 too, by calling zeroref().

  /* allow common tree construction pattern easily, without need for external objects */ 

   lnk*     _parent; 
   lnk*     _child;
   lnk*     _sib;

    _l3obj* parent();
    _l3obj* child();
    _l3obj* sib();

    void parent(_l3obj* p);
    void child(_l3obj* p);
    void sib(_l3obj* p);

  /* my s-expression */
  qtree*     _sexp;

  /* additional methods: pointer to method end, pointer to method begin. */
  void*      _pme; /* methods end+1 (like iterator end()) */
  void*      _pmb; /* methods begin */

  /* additional data: pointer to data end, pointer to data begin. */
  void*      _pde;     /* data end+1 / begin */
  void*      _pdb;     /* begin is at the end so that if we really need to optimize, we can
              layout the rest of the data right after this object, and 
                          cast/treat this last pointer as an array. 
              We don't declare it as void* _pdb[1]; or void* _pdb[]; 
              for ease of use / consistency of addressing now. But we could/can,
              if we are optimizing storage for speed.
               */
}* l3obj_p, l3obj;


 typedef l3obj      MerlinObj;
 typedef MerlinObj  Mob; /* reserve these */
 typedef MerlinObj  mob; /* reserve these */


 // ===================================
 // ===================================
 //
 // stopperset : track which pointers give us cycles.
 //
 // ===================================
 // ===================================

 typedef std::set<void*> stopset_set;
 typedef stopset_set::iterator stopset_set_it;

 typedef struct _stopset {
   stopset_set           _stoppers;
   stopset_set_it        _fi;
   stopset_set_it        _en;
 } stopset;

 extern stopset* global_stopset;

 bool obj_in_stopset(stopset* st, void* obj);
 void stopset_push(stopset* st, void* o);
 void stopset_clear(stopset* st);
 void p(stopset* st);

 uint l3_get_ownt(l3obj* myl3obj);

// return the old value, set it to a new value.
 uint  l3_set_ownt(l3obj* myl3obj, uint newvalue);


// return the old value of the tag, reading it before the clear.
 uint l3_clear_ownt(l3obj* myl3obj);

 //////////////
 //  lnk
 //////////////

const int MAX_FUNC_NAME_ARG_NAME = 40;

// our lnk/symbols have a name of up to this length
const long  SYMBOL_NAME_LEN =  MAX_FUNC_NAME_ARG_NAME;

 const long LNK_NAME_LEN = SYMBOL_NAME_LEN;

// links that are allowed dangle, and can recognize this (lazily) by checking _target_ser.
class lnk {
 public:
    MIXIN_TOHEADER() // gives us:
    // t_typ   _type;     // lets us distinguish the type of this object
    // long    _ser;      // a serial number, to recognize replacement. (merge with _llsn at some point).
    // uint    _reserved; // allow refs to be marked as sysbuiltin.


        private:

    // add symbol like properties to allow lnk to be used in dstaq
    // change _name to _key:     char   _name[LNK_NAME_LEN];
    llref* _link_llr; // owner will zero this if the object is deleted. and _target=0 too, by calling zeroref().

    // begin new members for mirroring llref
    
  lnk*     _next;
  lnk*     _prev;
  _l3obj*    _obj;

  _l3obj*    _env;
  l3str<LNK_NAME_LEN>  _key;

  Tag*      _refowner; // who owns this lnk*.  This owner 
    // is not necessarily the same as _obj->_owner; but can be. And may wish to be by default.

    // priorities:
    // the priority 1 llref is stored in _owned, and so should only be
    // deleted by actions that also delete the corresponding entry in _owned.
    //
    // _priority is used to pick a canonical name for 
    // display... 0 always wins (and so multiple winners possible), or 
    // else we choose the smallest we find in the linked-list chain.
    long       _priority;

    // _properties : a chain of properties, mainly for the tag (ownership)
    //               management and assignment semantics (pass-by-value, 
    //               pass-by-reference, etc., but mainly t_iso, t_ico, t_oso, t_oco

    ustaq<char>   _properties;
    long       _llsn;

    static l3path _singleton_printbuf; // shared by all, just so we can print them easily.


    // end new members for mirroring llref

 public:
    inline    llref* llr() { return _link_llr; }
    void    zeroref(); // zero out _link_llr and _target. here a function so we can breakpoint it.

    l3obj*  chase(); // chase the link, resolving to target or to gnil (if target is stale/ not set, then gnil returned).
    void    linkto(l3obj* tar);

    l3obj*     target();
    inline    char*      name() { return _key(); }
    void       update_key(char* newkey);

    friend lnk* new_lnk(l3obj* tar, Tag* owner, const char* key);
    friend void lnk_print(lnk* link, const char* indent, stopset* stoppers);
};

 lnk* new_lnk(l3obj* tar, Tag* owner, const char* key);
 void lnk_print(lnk* obj,const char* indent, stopset* stoppers);
 void del_lnk(lnk* link);


// since dtor's represent our types, write that first.

void* class_l3base_create(void* parent, void* databytes, void* methodbytes);


 void* class_l3base_destroy(void* thisptr, void* expandptr, _l3obj** retval);


// l3base: empty object.
//           the allocator/ctor

 void* class_l3base_create(void* parent, void* databytes, void* methodbytes);

  // the RTTI mechanism
 const char* rtti(l3obj* p);


// l3base: empty object.
//           the allocator/ctor

 void* new_l3base_create(void* parent, void* databegin, void* methodbytes, const char* varname);


 l3obj* make_new_class(size_t num_addl_bytes, Tag* owner, const char* classname, const char* varname);

 l3obj* make_new_double_obj(double d, Tag* owner, const char* varname);

 l3obj* make_new_hash_obj(Tag* owner, const char* varname);

 l3obj* make_new_string_obj(const qqchar& s, Tag* owner, const qqchar& varname);

 // hash table functions...

 // a private function, used only by add_alias_eno()

 // void    insert_into_hashtable_private_to_add_alias(l3obj* o, char* key, unsigned long value);


 void    insert_into_hashtable_private_to_add_alias_typed(l3obj* o, char* key, llref* addme);


// insert_into_hashtable_unless_already_present():
// return value:
// true => inserted successfully
// false => not inserted b/c was duplicate; previous value is stored in prev_value.
//bool insert_into_hashtable_unless_already_present(l3obj* o, char* key, unsigned long value, PWord_t& prev_value);

 bool insert_into_hashtable_unless_already_present_typed(l3obj* o, char* key, llref* value, llref** prev_value);

void delete_key_from_hashtable(l3obj* obj, char* key, llref* llr);

// returns num elements found
size_t dump_hash(l3obj* o, char* indent, stopset* stoppers);

void clear_hashtable(l3obj* o);
void* lookup_hashtable(l3obj* o, char* key);
int hashtable_has_pointer_to(l3obj* o, l3obj* target);

// treat as an (possibly sparse) array of double

// return the value of JudyL[i] as a double
double double_get(l3obj* obj, long i);

// set the value of JudyL[i] as a double
l3obj* double_set(l3obj* obj, long i, double d);

// get size of JudyL vector stored 
long double_size(l3obj* obj);

// simple linear search
bool double_search(l3obj* obj, double needle);

// treat as a string

// from l3string.h :
void    string_get(l3obj* obj, long i, l3path* val);
l3obj*  string_set(l3obj* obj, long i, const qqchar& key);


 // generic print
 l3obj* print(l3obj* obj, const char* indent, stopset* stoppers);


//// defun stuff for function def

 typedef l3obj dict_t;

 // const int MAX_FUNC_NAME_ARG_NAME = 40;

struct symbol_name {
  char _nam[MAX_FUNC_NAME_ARG_NAME];
  char* operator()() { return _nam; }
};

//
// use the malloc_allocator to work-around valgrind-STL fights and false-positives.
//

#ifdef _DMALLOC_OFF
 typedef    std::vector< ustaq<char>*  >  vec_ustaq_typ;
 typedef    vec_ustaq_typ::iterator                                                   vec_ustaq_typ_it;

 typedef    std::vector<t_typ >    vec_typ;
 typedef    vec_typ::iterator                                         vec_typ_it;

#else
 typedef    std::vector< ustaq<char>* ,__gnu_cxx::malloc_allocator< ustaq<char>* > >  vec_ustaq_typ;
 typedef    vec_ustaq_typ::iterator                                                   vec_ustaq_typ_it;

 typedef    std::vector<t_typ,__gnu_cxx::malloc_allocator<t_typ> >    vec_typ;
 typedef    vec_typ::iterator                                         vec_typ_it;
#endif

struct defun {
    // now separate/can be anonymous:  symbol_name funcname;
  sexp_t* defn_qtree;
  sexp_t* propset;
  sexp_t* body;
  l3obj*  myob;
  l3obj*  env;

  int nprops; // ret_argype.size() + arg_argtyp.size()
  int narg;
  int nret;
    //  std::vector<std::string>  arg_name;

    ddlist<char>                                                arg_name;

    // std::vector<t_typ, __gnu_cxx::malloc_allocator<t_typ> >            arg_owntag;
    //   std::vector< ustaq<char>*,  __gnu_cxx::malloc_allocator< ustaq<char>* > >   

    vec_typ        arg_owntag;
    vec_ustaq_typ  arg_argtyp;

    //  std::vector<std::string>  ret_name;
    ddlist<char>                  ret_name;

    //std::vector<t_typ,__gnu_cxx::malloc_allocator<t_typ> >            ret_owntag;
    //std::vector< ustaq<char>* ,__gnu_cxx::malloc_allocator< ustaq<char>* > >   ret_argtyp;

    vec_typ        ret_owntag;
    vec_ustaq_typ  ret_argtyp;

};

 void  print_defun_full(l3obj* obj,const char* indent, stopset* stoppers);
 void  print_defun(l3obj* obj, const char* indent);
 void defun_dump(defun& d, const char* indent);


 // there are two dtor for function definition objects
 void* function_decl_per_call_dtor(void* thisptr, void* expandptr, l3obj** retval);
 void* function_decl_deallocate_all_done_with_defn(void* thisptr, void* expandptr, l3obj** retval);


 t_typ str2owntag(char* s);
 bool handle_next_prop(defun& d, sexp_t* prop, l3obj* env, bool& is_ret);

// return true if okay, false if bad declaration
 bool parse_defun_propset(defun& d, l3obj* env);



// if a new allocation, use the same tag as current in use.
//  prep_double_returntype(retval);
 void prep_double_returntype(double dval, l3obj** retval);

#ifdef _DMALLOC_OFF

    typedef    std::vector< char* >    vec_pchar;
    typedef    vec_pchar::iterator vec_pchar_it;

    typedef    std::vector< const char* >    vec_cpchar;
    typedef    vec_pchar::iterator vec_cpchar_it;

    typedef    std::vector< t_typ >    vec_typ;
    typedef    vec_typ::iterator vec_typ_it;

    typedef    std::vector< l3obj* >  vec_obj;
    typedef    vec_obj::iterator vec_obj_it;

#else

    typedef    std::vector< char* ,__gnu_cxx::malloc_allocator< char* > >    vec_pchar;
    typedef    vec_pchar::iterator vec_pchar_it;

    typedef    std::vector< const char* ,__gnu_cxx::malloc_allocator< const char* > >    vec_cpchar;
    typedef    vec_pchar::iterator vec_cpchar_it;

    typedef    std::vector< t_typ ,__gnu_cxx::malloc_allocator< t_typ > >    vec_typ;
    typedef    vec_typ::iterator vec_typ_it;

    typedef    std::vector< l3obj* ,__gnu_cxx::malloc_allocator< l3obj* > >  vec_obj;
    typedef    vec_obj::iterator vec_obj_it;

#endif

struct actualcall {

  long narg;

  // arg_key has the name of the formal args that 
  // are matched, filled in upon invocation, during type 
  // checking in universal_object_dispatcher().


    // arg_key:
    // pointer to strings owned by t_fun; since the t_cal lifetime of a callobject 
    // is a strict subet of t_fun function definition, we don't need to manage ownership 
    // of the char*, just point to it and use it.

    // old:   std::vector<const char*>  arg_key; 
    // new:
    vec_cpchar                          arg_key;


    // old:    std::vector<char*>       arg_txt;
    // new:
    vec_pchar                           arg_txt;

    
    // old: std::vector<t_typ>           arg_typ;
    // new:
    vec_typ                              arg_typ;


    // arg_val:
    //
    // if we change what the l3obj* points to in a pass-by-ref situation, the l3obj*
    // can go stale. i.e. we may in the course of updates, end up deleting l3obj* vals...so then arg_val has bad info it.
    // write an update method to fix this: update_arg_val().
    //
    // old:
    // std::vector<l3obj*>          arg_val;
    // new:
    vec_obj                         arg_val;


    ddlist<llref>                 arg_val_ref;

    l3path                       orig_call;
    sexp_t*                      orig_call_sxp;
    char*                        srcfile_malloced;
    long                         line;
    l3obj*                       ret_val;

    actualcall();
    ~actualcall();
};


 // update arg_val when it changes -- replacement takes its place.
 void update_arg_val(actualcall* pcall, l3obj* delme, l3obj* replacement);


 // if given, scan starting env, then up the global_env_stack,
 // trying to replace delme with replacement in actualcall->arg_val
 void scan_env_stack_for_calls_with_cached_obj(l3obj* startingenv,  l3obj* delme, l3obj* replacement);



 // function call parameters (including keyword labeled args)
 // l3obj* make_new_callobject(sexp_t* sexp, Tag* owner, l3obj* curenv);

 double parse_double(char* d, bool& is_double);

 // in terp.cpp :
 extern char linebuf[BUFSIZ]; // in terp.cpp

 


// utility functions for sexp_t* handling
//
 
bool has_children(sexp_t* s);

long num_children(sexp_t* s);

// returns 0 if no such child
sexp_t* first_child(sexp_t* s);

// returns 0 if no such child
sexp_t* ith_child(sexp_t* s, long i);

// end sexp_t* utilities

 Tag* defptag_get();

 void dump_actualcall(actualcall* a, const char* indent, stopset* stoppers);
 void  print_cal(l3obj* obj,const char* indent, stopset* stoppers);


 
 // in l3obj.cpp


double JasonsTOP();
void call_with(l3obj* p, l3obj* q);

 l3obj* make_new_class(size_t num_addl_bytes, Tag* owner, const char* classname, const char* varname);
 l3obj* make_new_double_obj(double d, Tag* owner, const char* nm);
 l3obj* make_new_hash_obj(Tag* owner, const char* varname);


// otherwise known as apply
// void* universal_object_dispatcher(l3obj* fun, l3obj* cal, l3obj** retval);


// send our owner our name
// void name2owner(l3obj* o, const char* name);


// have to put in autotag, once this header is done:
//  extern minifs mini; // little convenient filesystem wrapper

#include "jmemlogger.h"

extern jmemlogger* mlog;
extern cmdhistory* histlog;

 // trace up the chain of environments looking for an identifier: either variable or function.

 typedef enum { AUTO_DEREF_SYMBOLS=0, DONT_DEREF_SYMBOLS=1 } deref_syms;

// what do to if we can't resolve...
typedef enum { UNFOUND_RETURN_ZERO=0, UNFOUND_THROW_TOP=1 } noresolve_action;

typedef std::list<l3obj*>           objlist_t;
typedef std::list<l3obj*>::iterator objlist_it;

 l3obj* resolve_dotted_id(const qqchar&  id, l3obj* startingenv, deref_syms deref, llref** innermostref, objlist_t* penvstack,
                          const qqchar&  curcmd, noresolve_action noresolve_do);

 l3obj* resolve_dotted_up_the_parent_env_chain(const qqchar&  id, l3obj* startingenv, deref_syms deref, llref** innermostref, objlist_t* penvstack,
                                               const qqchar&  curcmd, noresolve_action noresolve_do);

 l3obj* validate_path_and_prep_insert(const qqchar& dottedname, l3obj* startingenv, deref_syms deref, llref** innermostref, objlist_t* penvstack, 
                                      const qqchar& curcmd_for_diag,  noresolve_action noresolve_do, 
                                      // above conforms to other resolve functions, below is extra beyond them:
                                      l3path* name_to_insert
                                      );


 int staticdynamic_validate_path_and_prep_insert(char* dottedname, char* curcmd_for_diag, l3obj* startingenv, l3obj** env_to_insert_in, l3path* name_to_insert, deref_syms deref, llref** innermostref);

 void gen_dotlist(std::vector<char*>* dotlist, l3path* path);
 void dotlist_dump(std::vector<char*>* dotlist);

 // SHA1 
void    set_sha1(l3obj* o);
void    get_sha1(l3obj* o, l3path& sha);
l3path  shaobj(l3obj* o);
void    print_sha1(l3obj* o);

//
// same as SHA1 but return 48 byte hex ascii hex string encoding of the digest, with the last 8 bytes all '\0'
//
unsigned char* HEX_SHA1(const unsigned char *d, unsigned long n, unsigned char* md);


// tag stack


// global shadow stack for the function we are in; to provide default memtags for allocated stuff.

//typedef  std::list< Tag*> TagStack;
//typedef  TagStack::iterator tsit;
//typedef  std::list<Tag*>::iterator TagStack_it;

typedef  ddlist<Tag> TagStack;
typedef  TagStack::ddlist_it tsit;
typedef  tsit  TagStack_it;

extern TagStack global_tag_stack; // global/default tag-owning space
extern TagStack orphan_tag_stack; // separate stack for orphaned objects.


typedef  std::list< std::string> TagStackLoc;
typedef  TagStackLoc::iterator tslocit;
extern TagStackLoc global_tag_stack_loc;

//typedef  ddlist< std::string> TagStackLoc;
//typedef  TagStackLoc::ddlist_it tslocit;

extern TagStackLoc global_tag_stack_loc;



// env stack


struct EnvStack {

    ddlist<l3obj> _stk;

    void push_back(l3obj* o);
    void push_front(l3obj* o);
    l3obj* front_val() {
        return _stk.front_val();
    }
    void   pop_front() {
        _stk.pop_front();
    }
    void  clear() {
        _stk.clear();
    }

    long size();

    void dump();
     

};

typedef  ddlist<l3obj>::ddlist_it esit;
typedef  esit EnvStack_it;

// extern EnvStack global_env_stack; // global/default env for var lookup / namespace


// does balance tag and env pops...so pop_to_tag and pop_to_env pairs should be replaced by this one call:
//
long bal_pop_to_tag(Tag* expected_front, bool removeit, bool assert_notfound);

// long private_pop_to_env(l3obj* expected_front, bool removeit, bool assert_notfound);

// method stack ; so we can work on or in multiple methods at once. 
//           this is used to represent the call stack too, so that it is visible
//           which method we are in.

typedef  std::list<l3obj*> MethodStack;
typedef  MethodStack::iterator msit;
typedef  std::list<l3obj*>::iterator MethodStack_it;

extern MethodStack global_method_stack; // global/default env for method lookup


// conveninece methods for gdb
 l3obj* p(l3obj* obj);
 objlist_t* p(objlist_t* penvpath);


// * insert a variable into the dictionary
 struct llref;
 // l3obj* insert_private_to_add_alias(l3path *varname, llref* val, l3obj* d);
 l3obj* insert_private_to_add_alias(char* varname, llref* val, l3obj* d);

 // find element after the rightmost dot
char* rightmost_dot_elem(const char* path);
 
//#ifdef __cplusplus
//}
//#endif 


struct condexpr_struct {

  std::vector<l3obj*>   if_expressions; // t_boe boolean expression
  std::vector<l3obj*>   then_closures;  // t_clo

};


typedef bool (*bool_bin_op) (double a, double b);
typedef bool (*bool_una_op) (double a);

 bool bool_bin_op_ne(double a, double b);
 bool bool_bin_op_eq(double a, double b);

 bool bool_bin_op_gt(double a, double b);
 bool bool_bin_op_ge(double a, double b);

 bool bool_bin_op_lt(double a, double b);
 bool bool_bin_op_le(double a, double b);


 bool bool_una_op_isnan (double a);
 bool bool_una_op_iszero(double a);
 bool bool_una_op_notzero(double a);
 bool bool_una_op_not(double a);

 struct boolexpr_struct {
     bool_bin_op binary;
     bool_una_op unary;
 };


 typedef double (*dou_una_op)  (double a);
 // double unary op : negation.
 double  dou_una_op_negativesign(double a);


 // literals represent keywords or identifers for functions that havne't been
 // invoked directly yet.
 l3obj*  make_new_literal(const qqchar&  s, Tag* owner, const qqchar& varname);
 l3obj*  literal_set(l3obj* obj, const qqchar& key);
 void    literal_get(l3obj* obj, l3path* val);
 void    literal_print(l3obj* obj);

 int feqs(double a, double b, double tol);


 // register with new symboltable, and
 //  simultaneously deregester with old symbol tables.
 // 
 // move_myvarname() clients should use add_alias() now: void move_myvarname_to(l3obj* obj, l3obj* newenv, char* newname, char* oldname);
 // storing cur env in _child;

 void  print_defun_body(l3obj* obj,const char* indent, stopset* stoppers);

 void env_pop_iftop(l3obj* expected_front);
 // void defptag_pop_iftop(Tag* expected_front);

 size_t hash_show_keys(l3obj* o, const char* indent);


 // ========== map

 // map with logN member ship testing, based on judyL arrays
 l3obj* make_new_l3map(Tag* owner, const char* varname);

 // returns 1 if in the map, setting *val
 // otherwise returns 0
 int ele_in_l3map(l3obj* map, void* key, void** val);

 void ins_l3map(l3obj* map, void* key, void* val);
 void del_l3map(l3obj* map, void* key);
 long l3map_size(l3obj* obj);
 void l3map_print(l3obj* l3map, const char* indent, stopset* stoppers);
 void l3map_clear(l3obj* l3map);

 // find the nearest common ancestor of a and b
 // (uses map).
 Tag* compute_nca(Tag* a, Tag* b, l3obj* env, Tag* owner);

 void check_tag_and_break_captag_selfcycle(Tag* tag);
 void check_obj_and_break_captag_selfcycle(l3obj* obj);
 void bye(volatile l3obj* delme, Tag* owner);
 int  pred_is_captag(Tag* tag, l3obj* cp);

 // brute force, sweep everything, set it to given value and recurse unless already set to this value.
 void dfs_reset_all_been_here_to_thisval(Tag* visitme, long val);

 void judySLdump(void* judySL, const char* indent);
 long judySL_size(void* judySL);

 void    print_defun_short(l3obj* obj,const char* indent, stopset* stoppers);
 llref* llref_from_path(const char* path, l3obj* startingenv, l3obj** pfound);

 //
 // assign_det
 //
 // assignment detail: everything that comes back from a lhs_setq() call.
 //
 typedef struct _assign_det {

     l3path    name_to_insert;
     l3obj*    env_to_insert_in;

     llref*    lhs_iref;
     l3obj*    lhs_env;
     Tag*      lhs_owner;
     l3obj*    lhs_obj;
     l3obj*    target;

     Tag*      retown_for_rhs_eval;

     _assign_det() { init(); }

     void init() {
         name_to_insert.init(0);
         env_to_insert_in = 0;
         
         lhs_iref = 0;
         lhs_env = 0;
         lhs_owner = 0;
         lhs_obj   = 0;
         target = 0;
         
         retown_for_rhs_eval = 0;
     }

 } assign_det;

 typedef enum { TNONE=0, TFAL=1, TTRU=2, TNIL=3, TNAN=4, TNAV=5 } LOGICAL_TYPE;

 LOGICAL_TYPE get_logical_type(l3obj* obj);


 // do a particular tag and object form a captag pair?
 int  pred_is_captag(Tag* tag, l3obj* cp);

// common shorthand
 BOOL pred_is_captag_obj(l3obj* o);

 struct sexpobj_struct {
     qtree*  _psx;
 };

 // in l3dstaq.cpp
 void dq_print(l3obj* l3dq, const char* indent, stopset* stoppers);

 bool double_first(l3obj* obj, double* val, long* index);
 bool double_next(l3obj* obj, double* val, long* index);
 bool double_is_sparse(l3obj* obj);
 void double_set_sparse(l3obj* obj);
 void double_set_dense(l3obj* obj);

 long first_child_chain_len(sexp_t* s);




#endif /*  L3OBJ_H */
