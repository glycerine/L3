//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef AUTOTAG_H
#define AUTOTAG_H

// from autotag.dj ...

// compiled with: (the -std=c++0x is important for __func__ )
// cd /home/jaten/dj/strongref; g++ -std=c++0x -g autotag.cpp -o at; ./at

 #include "jtdd.h"
 #include "l3throw.h"

 #define JUDYERROR_SAMPLE 1
 #include <Judy.h>  // use default Judy error handler

#include <valgrind/valgrind.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <stdio.h>
#include <set>
#include <list>
#include <map>
#include <vector>
#include <utility> // for make_pair
#include <stdlib.h>
#include <new>
#include <string>
#include <string.h> // for strncpy
#include <tr1/unordered_map>
//#include <umem.h>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <string>
#include <zmq.hpp>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdio.h>

#include "jlmap.h"
#include "quicktype.h"
//#include "minifs.h"
#include "l3obj.h"
#include "merlin.pb.h"
#include "rmkdir.h"
#include "serialfac.h"
#include "terp.h"
#include "llref.h"
#include "addalias.h"
#include "jtdd.h"
#include "ostate.h"
#include "dstaq.h"

/* note where we were declared, for inspecting allocation/deallocation */
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ ":" TOSTRING(__LINE__)
#define STD_STRING_WHERE (std::string(__func__) + "()  " + std::string(AT))
#define STD_STRING_GLOBAL (std::string("global/file scope") + "()  " + std::string(AT))

// globals

// these in l3obj.cpp
extern char* global_ico; // = "!>";
extern char* global_iso; // = "@>";
extern char* global_oco; // = "@<";
extern char* global_oso; // = "!<";

// after the defn below: extern minifs mini; // little convenient filesystem wrapper, global

 struct Tag;
typedef l3obj Obj;

 struct stack_wrapper;

//#define TAG99(PHRASE) virtual PHRASE = 0
//#define TAG99(PHRASE) PHRASE

//#define L3UNVIRT(funcname)  int funcname(l3obj* obj, long arity, sexp_t* exp, l3obj* env, l3obj** retval,  Tag* owner, l3obj* curfo, t_typ etyp, Tag* retown);


struct Tag {

    // begin member variables

    MIXIN_TOHEADER();

  void* _owned; // JudyLArr;

  long     _destruct_started;
  long     _been_here;

  long _sn_tag; // our serial number

  Tag* _parent;

  // my path (file representation)
  l3path mypath;

    // private:
  l3path      _where_declared;
  l3path      _myname;
  l3obj*      _captain;

    
    // psexp state/cache management
    struct stack_wrapper  *pd_cache;
    struct stack_wrapper  *sexp_t_cache;

    // generic stack, client managed.
    ustaq<void>  genstack;

    // atom stack, for atoms contained in sexp_t
    ustaq<void>  atomstack;

    // to be free-ed (with jfree) stack of l3obj* / uh*
    ustaq<l3obj>   tbf_stack;

    // to be Tag::free or tyse_free()-ed.
    ustaq<tyse>   tyse_stack;

    //    dstaq<tyse>  _parse_free_stack;

    // to be freed lnk
    ustaq<lnk>   lnk_stack;

    // good grief templatest are annoying. force instantiation of the dump() method for ustaq<lnk>:
    ustaq<lnk>* print_lnk_stack(char* indent);

    // to be destroy_sexp()-ed; i.e. created with sexp_copy
    //  typedef  std::list<sexp_t*>  sexp_t_stack;
    //  typedef sexp_t_stack::iterator sxsit;
    //  sexp_t_stack sexpstack;

    ustaq<sexp_t>  sexpstack;


  // subtags: Tag* implementers owned by us,
  //  that will have destruct() called on them
  //  when we do the same.
    //  typedef  std::list<Tag*>  itag_stack;
    //  typedef itag_stack::iterator itsit;
    //  typedef itag_stack::reverse_iterator ritsit;
    //    itag_stack subtag_stack;

    ustaq<Tag>  subtag_stack;

    //// newdel: to be deleted with c++ delete
    // currently Tags cannot be malloced due to use of C++ objects
    //  that have required constructors that must be run, so
    // we have to new and delete them.

    //  typedef  std::list<Tag*>  newdel_itag_stack;
    //  typedef  newdel_itag_stack::iterator its_del_it;
    //  newdel_itag_stack del_itag_stack;

    ustaq<Tag>  del_itag_stack;

     // llref management - to be free-ed
     ustaq<llref> llrstack;

    // methods

    // ctor constructor
    Tag(const char* where_declared, Tag* parent, const char* myname, l3obj* mycap);

    ~Tag(); // prefer dfs_destruct to "delete" alone.

    void init(const char* where_declared, Tag* parent, const char* myname, l3obj* mycap);

    void   print_owned_objs_with_canpath(Tag* visitme, char* indent);
    

    void   copy_ownlist_to_ptrvec(l3obj** vv);
    
    void   shallow_obj_copy_to(Tag* receiving_tag);


  // because of the acyclic nature of the tag tree,
  //   the stoppers stopset  should not be necessary for tag printing!
  //
  void dump_owned(const char* pre);
  void print_subtags(char* indent, char* indent_more);

   void add(l3obj* o, const char* varname = 0, long sn = 0);


    // add for lnk and others using uh...stick in 
    // lnk to be jfree-ed
    void  lnk_push(lnk* p);
    lnk*  lnk_remove(lnk* p);
    lnk*  lnk_exists(lnk* p);
    void  lnk_jfree(lnk* p);
    L3FORWDECL(lnk_jfree_all)
    

    // generic version of release_to that handles cap-tags as well as lone obj.
    void generic_release_to(l3obj* o, Tag* new_owner);

    void name(l3obj* o, const char* objname);
    void delfree(l3obj* o);

   l3obj* enum_owned_from(l3obj* start, llref** llrback);
    void llr_show_stack();
    void expstack_destroy_all();

  L3FORWDECL(print_owned_objs_with_canpath)
  L3FORWDECL(del_owned)
  L3FORWDECL(del_all)
  L3FORWDECL(tbf_jfree_all)


   bool   owns(l3obj* o);
   llref* find(l3obj* o);
   void   erase(l3obj* o);
   long   owned_size();

    // verify that depth-first-search on tags is working stand-alone, before
    // using dfs with the tag_destruct sequence.
  L3FORWDECL(dfs_print)
  L3FORWDECL(bfs_print_obj)
  L3FORWDECL(dfs_destruct)
  L3FORWDECL(dfs_copy)
  L3FORWDECL(shallow_tag_copy)


   long   dfs_been_here();
   void   dfs_been_here_set(long newval);

   void   recursively_been_here_set(long newval);

   long   destruct_started();
   void   destruct_started_set();

  // there is only one captain of a tag at any
  // given time. Captain is the owner of the tag;
  //  should he wish to reveal himself.
   void   captain_set(l3obj* cap);
   l3obj* captain();


   Tag* parent();

    // tmalloc: does tyse_malloc() and then does tyse_push(the returned ptr) as well.
   tyse*   tmalloc(t_typ ty, size_t size, int zerome);

    // tfree: does tyse_remove(ptr), then does tyse_free(ptr)
    //   void    tfree(tyse*  ptr);
   void    tfree(void*  ptr);


    // reference_deleted()
    //
    // notify tag that a references has been deleted...in case tag wants to 
    // delete the object in obj. 
    //
    // Returns 0 if object was deleted, else 1
    L3FORWDECL(reference_deleted)


  // Require passing parent in upon construction. It should not be
  // settable later; therefore we don't define set_parent();
   void  set_parent(Tag* newparent);

   void dump(const char* pre);
   void dump_tbf(const char* pre);
   void dump_lnk_store(const char* pre);

  // generic stack for keeping additional Tags*, etc.
  // client has to keep track of 
  // anything left at the end will be ::free()-ed.
  // so thus stuff had better be *malloced* if it's left on here for the tag to clean up!!!
   void   gen_push(void* p);
   void*  gen_pop();
   void*  gen_top(); // returns 0 if empty.
   void   gen_free_all();
   void*  gen_remove(void* p);
   void*  gen_exists(void* p);

    // atomstack : from atoms in sexp_t
   void   atom_push(void* p);
   void   atom_free_all();
   void*  atom_remove(void* p);
   void*  atom_exists(void* p);

   char*  strdup_atom(char* src); // strdup and atom_push
   char*  strdup_qq(const qqchar& src);   // strdup and atom_push

    // used by Tag::malloc / Tag::free memory, a proxy for tyse_malloc() and tyse_free()    
   void   tyse_push(tyse* p);
   tyse*  tyse_remove(tyse* p);
   tyse*  tyse_exists(tyse* p);
   void   tyse_free_all();
   L3FORWDECL(tyse_tfree_all)


  // to be jfree-ed
   void    tbf_push(l3obj* p);
   l3obj*  tbf_pop();
   l3obj*  tbf_top();


   void    sexpstack_push(sexp_t* p);
   sexp_t* sexpstack_pop() ;
   sexp_t* sexpstack_top() ;
   void    sexpstack_destroy_all();
   sexp_t* sexpstack_exists(sexp_t* p);
   sexp_t* sexpstack_remove(sexp_t* p);

  // sub tags to call destruct on
   void    subtag_push(Tag* p);
   Tag*   subtag_pop() ;
   Tag*   subtag_top() ;
   Tag*   subtag_remove(Tag* psubtag) ;
  L3FORWDECL(subtag_destroy_all)

  // subtags to call destruct on, using delete
   void  newdel_push(Tag* p);
   Tag* newdel_pop();
   Tag* newdel_top();
   void  newdel_destroy_all();


    // llref* to free
   void    llr_push(llref* p);
   void    llr_remove(llref* p); // without free-ing

   // to verify the invarient that
   // lifetime of llref must match lifetime of objects they point to.
   // returns 0 if not a  member.
   BOOL  llr_is_member(llref* p);
    
   llref*  llr_pop();
   llref*  llr_top();

   void    llr_free(llref* freeme);
    // same as llr_free but also notify owner...so preferred to just plain llr_free
   L3FORWDECL(llr_free_and_notify)

   void    llr_free_all(en_do_hash_delete do_hash_delete);

   void    release_llref_to(llref* transfer_me, Tag* newowner);

   const char* myname();
   long sn();
   void        set_myname(const char* m);
   const char* get_tag_srcwhere();
   const char* path();

 //   void copy_ownlist_to_ptrvec(l3obj** vv);
 //   void shallow_obj_copy_to(Tag* receiving_tag);

   Tag* reg(const char* key, l3obj* val);

  void AllocateMyDir();

  void test_owned();



  // the main point of clean up is destruct(). 
  //
  // protocol: destruct() and its minions are the only ones allowed to
  //  call jfree() directly. Other object deletion must go through the
  //  tag so that cleanup can be managed.
  //
  L3FORWDECL(tag_destruct)



      //};
};

// manual ctor: now called init.
//Tag*  newTag(const char* where_declared, Tag* parent, const char* myname, l3obj* mycap);



// use l3obj for l3obj now

void jfree(l3obj* ptr);

// old way: std::set
typedef std::set<l3obj*> objset;
typedef objset::iterator osit;

// new way: JudyL
typedef  Pvoid_t JudyLArr;

// rm or replace with judySL: typedef std::tr1::unordered_map<std::string, l3obj*>  hashmap;
// rm or replace with judySL: typedef hashmap::iterator hmit;


void* jmalloc(size_t size, 
          Tag* owntag, 
          merlin::jmemrec_TagOrObj_en type, 
          const char* classname,
          const char* where_srcfile_line,
              l3path* varname,
              long notl3obj, // defaults should be 0, for allocating l3obj*; for other structures, set to 1
              t_typ ty
              );
        
// easy printing from gdb                
Tag* p(Tag* t);

// use merlin.proto declared jmemrec class

#define NEW_TAG(size,owntag,classname,varname) jmalloc(size, owntag, merlin::jmemrec_TagOrObj_en_JMEM_TAG, classname, 0,varname,1,t_tag)
#define NEW_OBJ(size,owntag,classname,varname) jmalloc(size, owntag, merlin::jmemrec_TagOrObj_en_JMEM_OBJ, classname, 0,varname,0,t_obj)


#define DEL_TAG(ptr,size,owntag) jfree(ptr)
#define DEL_OBJ(ptr,size,owntag) jfree(ptr)
#define DEL_LLR(ptr)             jfree(ptr)


// Nearest common ancestor promotion
void promote_ownership_what_from_to(l3obj* what, Tag* fromtag, Tag* totag);


// reset all the _been_here to a known state
void  top_down_set_been_here(long newval);



////////////////////////////////////////////////////////////
//  stack interface to the global_tag_stack : 
//           the default persistent tag (defptag)
////////////////////////////////////////////////////////////

#if 0
//Tag* defptag_get();

// use the macro push_deftag() below instead of calling this directly
//void indirect_push_defptag_private_to_macro(Tag* newfront);

// use push_deftag() so we get the line number
#define defptag_push(newfronttag, matchingenv)              \
  do { \
      indirect_push_defptag_private_to_macro(newfronttag);              \
      global_tag_stack_loc.push_front(STD_STRING_WHERE);  \
      global_env_stack.push_front(matchingenv); \
  } while(0)

#endif


// C replacement for dtor; called in finally blocks.

void* class_Tag_destroy(void* thisptr, void* expandptr, l3obj** retval);





// put NEW_AUTO_TAG after the formal parameteres of any function you want to have
// managed pointers in.
//
//  e.g.     int main() { NEW_AUTO_TAG; ...main body... }
//


// NEW_AUTO_TAG(tagname_p,defptag_get()); 
#define NEW_AUTO_TAG(tagname,myowner)                    \
  Tag* tagname_p = (Tag*)jmalloc(sizeof(Tag),myowner,           \
               merlin::jmemrec_TagOrObj_en_JMEM_TAG,    \
                                 "NEW_AUTO_TAG",(STD_STRING_WHERE).c_str(),0); \
   tagname->init(STD_STRING_WHERE,myowner,"NEW_AUTO_TAG"); \
   DV(printf("NEW_AUTO_TAG (%p) allocated, with owner %p.\n",tagname_p,myowner)); \
   global_tag_stack.push_front(tagname_p);  \
   global_tag_stack_loc.push_front(STD_STRING_WHERE); \
   global_function_entry()

//   global_function_entry(&stacktag);


// if I am the top of the stack, then pop myself as I destruct. 


#define DONEGC global_function_finally(); \

//  global_function_exit(Tag* pstacktag);
  

// by default, objects will use stack based stuff too. 
//#define OBJGC()     NEW_AUTOTAG(??)

//
//   global_tag_stack.push_front(_default_objtag);          
//   global_tag_stack_loc.push_front(std::string(__func__) + "()  " + std::string(AT));
//

// NO {} braces around GCADD !!
#define GCADD(p)   (defptag_get()->add(p)); printf("GCADD adding %p to defptag_get(%p)\n",p,defptag_get())


#define GCNEW(p) new p; GCADD(p)


#define PARTAG()    (global_tag_stack.size() > 1 ? (*(++(global_tag_stack.begin()))) : defptag_get())

#define GCADDPAR(p) (PARTAG()->add(p))



//#define GCSTACKD 

#define GCSTACKD  stg.global_dump();

// show the auto tag contents
#define ASHOW()  (*(global_tag_stack.begin()))->dump("")

// show the object default tag contents
#define OSHOW()  get_itag()->dump("")


#define GLOBTAG  (*(global_tag_stack.rbegin()))


#if 0


// MemTag 
//


// new autotag.h pointer management.

// There is one central memory manager object.

// Upon entry and exit each function notifies the memory manager, with calls

entering(FunctionTag)
exiting(FunctionTag)


allocate(type); // assigned to the current function's tag. which is the last registered tag (typically)

transfer_tag(pointer, new_tag); // tells the memory manager to transfer the object to the specified tag, instead of
                                // deleting it when the current function goes out of scope.

allocate(type, tag); // allocate an object and assign ownership to a paritcular tag. Combines alloate and transfer_tag into one call.

allocate_temp(type); // this will be automatically cleaned up  when current function exists, like it was stack allocated, even
                     // if the current default memtag is not the current function.

void* memtag = create_new_memtag(); // get a new tag. Permanent; 

get_memtag(object); // all objects have a memtag associated with them, so that additional objects cleanup can be associated with their destructors.
                        // in this sense, functions are objects too, if only temporarily allocated objects on the stack.


add_memtag(object, addl_tag); // assign ownership of this tag to this object.

clear_memtag(tag); // release all memory associated with a particular tag, the tag can be re-used again though; it doesn't go away.

destruct_memtag(tag); // release all memory and destroy the tag so no further allocations can be made on this tag. used in destructors to destroy all the objects in an objects memtag (each object has one by default).

transfer(from_tag, to_tag); // transfer all the objects on the from_tag to the to_tag
transfer_tags_to_parent();       // these all tags in the current function to my parents tag, or to the global memtag if we are in main().




// when a function exists without deallocating a pointer, all the pointers assigned to it's tag are automatically released (just once).


// test cases

 The pointers need to have a type of one of  
       {@in.c.own, @in.s.own, @out.s.own, @out.c.own}


IN: call library and pass object in, where library takes ownership of pointer and will dispose when done.
// old, reword:  action: assign the pointer to the object to the library's object handle, so that when the library finishes, if it hasn't deallocated that memory already, we can do so.
  
// the library will take ownership of the object. the library can assign the objects tag to other owners if it wishes. for the library/server can put the tag on it's own autotag (stack) to be cleaned up immediately. Or the library/server can put the tag in the persistent tag space (such as a big matrix to be re-used multiple times). Ultimately thought, the server/library code base is now responsible for proper cleanup of the object.

1. server_method(@in.s.own SomePtr* ptr_to_pass_to_server); // this is the forward declaration of the method on the server class.

upon calls like this:
  libobj->call_method(ptr_to_pass_into_lib)

a macro/the compiler can turn them into:
  libobj->own(ptr_to_pass_into_lib)
  libobj->call_method(ptr_to_pass_into_lib)


  try out syntax:
    libobj @> call_method(ptr)

convert to...
libobj->own(ptr);
libobj->call_method(ptr); // how do we get this return value back to the caller? want progn, but this gives prog1

// so the above doesn't work, because the value returned is the result of the first call, and we want it to be the result of the second call, as in

(progn
 (==> libobj own ptr)
 (==> libobj call_method ptr))





//IN: library doesn't take ownership.


2. server_method(@in.c.own SomePtr* ptr_passed_to_server)

//since all pointers already have a default tag assigned to them, this doesn't change anything, and the compiler doesn't need to change the ownership of SomePtr; it just stays with where it's at now.

//OUT: server doesn't give up ownership

3. server_method(@out.s.own SomePtr* ptr_received_from_server)

nothing required of client, since server will still manage this memory.


OUT: server gives up ownership and client is responsible for the pointer.

4. server_method(@out.c.own SomePtr* ptr_received_from_server)

compiler assigns ownership of someptr to the default tag, which would typically be the current function.

  (prog1
   (==> libobj server_method captured-ref)
   (own-assign *current-tag* captured-ref))



-----------------

//programmer doesn't have to do any further memory management.

-----------------

I create a method 

fourway(@in.c.own void* p) {
  
 // what do I do if I duplicate this pointer?
 //   nothing, because I know my caller/client will take care of it.


}

fourway(@in.s.own void* p) {
  
 // what do I do if I duplicate this pointer?
 // well, I own the pointed to object, so as long as it's just a duplication of a pointer, no need to do anything;

 // I haven't done anything with p at this point, but the default safe behavior
 // could be to have the compiler cleanup this pointer, unless it's assigned to a different memtag. Because
 // that is what the semantics of the @ memtag annotation convey--that I'm supposed to cleanup by default, by calling
 // delete on that pointer.
}


fourway(@out.c.own void*& p) {
  
   // I'm allocating an object and passing it back to the client who will take care of 
   // cleaning it up.

   // so when I do the allocation, I need to know my clients tag, so I can give it to them.
   // so the @out.c.own needs to implicitly pass the client's tag too (if we are going remote). 
   // Or  we can just call parent_memtag() to get it, if we are local.

   // or we could have the assignment of the tag happen on the caller side, which might make it
   // easier to interface with existing C libraries.
}

fourway(@out.s.own void* p) {
  
  // I've got to allocat this object (that p will point to) on my (or some other objects) default tag. 
  //  or I've got to put it on the global memtag if I'm a global level function without an object.

  // in anycase, since the only way to allocate an object is to have it be owned by some tag,
  // there has to be a tag.
}

additionally: we can:
Easy to wrap existing C/C++ objects with this MemTagProxy objects...this gives us a universal base
class for all pointers.


// to use the memtag proxy system, all objects need to inherit from Obj (add as a base class). This
// is necessary so that we can deallocate the object correctly when it is time.


// theorem: this system admits no cycles.

// base case:

// 1) Tags are created when an object is created; when an object is created, it cannot point
//    to any other new-ed up objects created at non-ctor time at first; until later. As long
//    as the object takes care of direct news done in the ctor once it reaches the dtor, then
//    there can't be any pointers (after the ctor is done) that would create cycles immediatedly.
//    what about a parent pointer passed in as a parameter, that is stored. It doesn't get
//    assigned to the empty tag in the ctor, so there is no problem. Known objects at ctor
//    time that are newed, do have to be deleted in the dtor. But INVAR: the tag of an
//    object is empty after the ctor of the superclass Obj completes: we know this
//    by inspection of the Obj class: it adds no pointers to the tag when the Obj class
//    (base class) is constructed.
//

What if: the ctor makes a @< call that transfers in ownership of a pre-existing tag?


  could this create a cycle?
  if so, do we need to prohibit @< calls in ctors?
  if we do, does that solve the issue?

  so: if we insist that ctors have no parameters... then the above analysis certainly applies.

** The "Copy-your-Elders Rule", point to those younger. **
Even easier rule: you cant take ownership of an older (smaller serial number) object.
 If you want to own such information, you must make your own copy of it, so that copy
 will have a larger-than-you serial number.

Guarantees no cycles.

If you want to share state, do it within an object, so the lifetime of the state within
 the object is well defined.



//   Tag: has lifetime of it's containing object, unless it is the root (global) tag.
//
// 2) induction
// 
//    RULE: when an object is newed, the creating function must pass in a pre-existing tag.
//
//    Given this rule, we know that assignment of the ownership of the new object is given to
//      a pre-existing and therefore older tag. Since the tag was older, it cannot be 
//      owned by the new object. i.e.
//      ownership of the tag was already pre-existing, versus this is a brand new object
//      at object creation time. 
//      
//      Therefore, object newing cannot create any cycles in ownership.
//       and ownership cycles are the only thing we are concerned with.
//      
//    Object: is owned by a pre-existing tag. Tag is owned by a pre-existing object, and tag is empty when created. Therefore with these rules there can be no ownership cycles.
// 
//
//  What happens on in.s.own   and   out.c.own when there is a transfer of ownership?
//
//
//  note: that the object was newed by the owing object's tag, prior to any transfer of ownership. Therefore the object could have just as readily been newed by the receiving server library as far as ownership is concerened. Therefore there is no problem giving away ownership to another pre-existing tag.

//
//(C) server_method(@in.s.own SomePtr* ptr_to_pass_to_server); // this is the forward declaration of the method on the server class.

//(progn
// (assert my-tag-owns ptr) ; I must own it so I can give it away. runtime check.
// (==> libobj own ptr)
// (==> libobj call_method ptr))



//OUT: server gives up ownership and client is responsible for the pointer.

//(D) server_method(@out.c.own SomePtr* ptr_received_from_server)


// again ownership was assigned at creation, which means that there could have been no ownership cycles at creation time, and now at transfer time, since both tags are already pre-exising, the tag receiving ownership of the object could have just as well as newed up the object itself. but now the object contains additional tags, because the object is not new. Could some of those tags point back to ourself? 

// this is resolved by insisting that any time ownership is given away, the giver must actually *have* ownership, before it can be given. So the runtime needs to assert that the server, for a out.c.own parameter actually does own the object that is being transfered to the client.  And the runtime needs to assert, at the point of in.s.own parameters, that he client does indeed already, prior to the call, own the object.

// compiler assigns ownership of someptr to the default tag, which would typically be the current function.

//  (prog1
//   (==> libobj server_method captured-ref)
//   (assert     obj-owns-pointer libobj captured-ref)
//   (own-assign *current-tag* captured-ref))


//////////// compile time enforcement : yes we can.

// (1) for in.s.own parameters, the pointer passed must be involved in a new or a out.c.own call in the current body.
          /*
out.c.own calls in the current body, like a call to a factory object, are like deferred news that the factory is doing for the actual client. So perhaps when an out.c.own call is made, the client in the call should also be passing in the storage context-tag to assign to the new-ed up object. Yes. That makes alot of sense. Just like new itself wants to be passed in a storage tag so it can assign ownership of the new object to the tag.

So out.c.own calls are actually charaterized by having a tag passed as an additional parameter to the call, so that any of the out.c.own calls can utilize the given tag for storage.

So how does the client get the server's tag for an in.s.own call?

Every function needs to be associated with a default auto tag (stack based) and an default persistent tag (object/global)

So it boils down to simply needing to be able to assign to a tag for a function that doesn't yet exist at the point at which new is made.

 lazy tags...that grab ownership of the first method they encounter?

 when you are in the interpreter you don't know in advance which method you might call next.... or you're in
a function which branches, so you can't actually tell for sure....

yes, this really is a property of the functions themselves.


function(tag_for(a,b,c), tag_for(d,e), a,b,c, d,e )

it's part of the type signature of the function at hand... that this function may suck the life right out of the 
inflatable dingy that it is passed.

even if that dingy was in the harbor of the interpreter. It's like we called (rm obj) on the obj. Now it's gone, just like would happen in the executing code.



// (2) for out.c.own parameters, the body of the server function that has an out.c.own declared in its formals--this body must either new and assign to the parameter that is out.c.own, or it must receive that pointer from an in.c.own prototyped call (no action required, client already owns the object).


////////////////////////////////// done with analysis of cycles.

// what about interpreter time enforcement in the interpreter? actually we don't really
// care what method is used here; any kind of garbage collection will be fine.

// But. we do what the generated C code to use take care of memory collection correctly.

how do 



//If I'm wrapping an existing object, and passing that object via proxy pointer, then what should the compiler do as it unwraps the proxy and calls the legacy c++ library.



// we overload new for debugging purposes, and add a class/object label.

// have to put the oid at the end, because global delete can't be overloaded,
//  so we have to return the same pointer we asked for.

const size_t size_of_oid = 48;
typedef char oid_t[size_of_oid];

const size_t size_of_sizet = sizeof(size_t);

const size_t sub_to_oid = size_of_sizet + size_of_oid;

void * operator new (size_t size, const char* oidname) {

   size_t real_size = size + sub_to_oid;

   void* ret = umem_zalloc(real_size,UMEM_DEFAULT);  
   if (!ret) throw std::bad_alloc();

   // header, 1st elem: real_size
   *((size_t*)ret) = real_size;

   // header, 2nd elem: oid
   strncpy(((char*)ret)+size_of_sizet, oidname, size_of_oid-1);
   printf("new object of size %uld (pointer = name) : ('%p' = '%s')\n",real_size, ret+sub_to_oid, ((char*)ret)+size_of_sizet);
   return ret;
}

void operator delete (void* p) {
  printf("deleting object pointer: '%p'\n",p );
// how to call...  umem_free(p);
}



struct myclass : public l3obj
{

//  virtual ~myclass() {
  ~myclass() {
    printf("in ~myclass dtor\n");
  }
};



struct myclass2 : public myclass
{   
  void mymethod2()  @{

    myclass* m = new myclass;

    // usually we would have to transfer m from auto to instance default tag.
    get_itag()->add(m)->reg("m",m);

    OSHOW();

    ASHOW();

    //    GCADD(m);
  }


  //  virtual ~myclass2() {
  ~myclass2() {
    printf("in ~myclass2 dtor\n");
  }
};

void myfunc() @{


  l3obj* m = new myclass;
  myclass2* n = new myclass2;

  GCADD(m);
  GCADDPAR(n);

  printf("myfunc: before doing anything\n");

  printf("mymethod2: starting\n");
  n->mymethod2();
  printf("mymethod2: done.\n");

  GCSTACKD;
  printf("the global tag is %p\n",GLOBTAG);

  GCDEL;
  printf("myfunc after doing manual del_all\n");

  GCSTACKD;
}

int autotag_unittest_main() @{

  l3obj* m = new myclass;
  //  l3obj* n = new myclass2;

  GCADD(m);

  ASHOW();

  printf("before doing anything\n");
  myfunc();
  printf("back from myfunc\n");


  
  GCSTACKD;

  (defptag_get()->del_all());
  //  GCDEL;
  
  printf("after doing manual del_all\n");

  printf("sizeof(myclass2) is %ld\n",sizeof(myclass2));
     
  GCSTACKD;
   return 0;  
}



// workspace

// cp t.h temp.h; perl -pi -e 's/[@][{]/\{ GCAUTO\(\);/g' temp.h; cat temp.h

turns

  @{
     something in here
  }

into

  { NEW_AUTO_TAG();
     something in here
  }

// how do we give methods their own default scope/tag ?

... it would seem that every method would need to push their objects tag onto the default allocation stack. yep.

object method definition:
void SomeObj::omethod() &{
   code here
}

expand into

void SomeObj::omethod() { 
  

   code here
}









////////////////////////////////// sim_autotag : do a simulation and see if any leaks are detected.

//std::vector<int> allocation;


// actions: an object can:

//  1. create a new object (which creates a fresh context): (ownership given to creator's tag: the object if in an object; the current function on the stack if not in an object).
//  2. call a function (in.s.own)
//       (a) if I own several pointers already: pick one; or allocate a new object, and transfer ownership to 
//             the called function in the in.s.own parameter.
//       (b) take a pointer I don't own, and copy it, producing an object I do own. Then pass the ownership of this new object to the server via the in.s.own parameter.

//  3. receive a function call (out.c.own): allocate a new object and trasfer ownership to caller.
//       (a) if caller is a function, transfer ownership to callers stack-based tag.
//       (b) if caller is an object method, transfer ownership to caller's stacl based object tag.
//            If object method wants to preserve the object, they can transfer ownership to the object's default tag.
//       So really (a) and (b) are the same: all callers have a stack-based tag, and out.c.own parameters will add the out object to that stack tag.

//  4. return from the current call (if in an object do nothing; if in a stack, delete the stacks tag)
//  5. invoke a method on the new object; (subsumed by 2 and 6)
//  6. call a function (in.c.own) (no transfer of ownership) : just passing a pointer to an object that the server cannot own.

//      Suppose the server stores that pointer. Then the pointer will probably go bad at the end of the call. So the server probably should never store a pointer they don't own; i.e. because it will probably go bad immediately after the call is finished.

//   Conjecture: the Rule is, you can only store pointers you own, but you can receive temporary pointers from others, and if need be copy the object so you do own it. Is this sufficiently expressive?

//  7. receive a fucntion call (out.s.own) (no transfer of ownership) : 

so we conclude:
an object has a default fall back tag for each object, and a stack based method context.
Just like main has a default fall back global tag

What can we say about the validity of pointers 


function(in.c.own, out.c.own, in.s.own, out.s.own)

  method(@in.c.own (anytype) formalname, 
         @out.s.own (anytype) formalname,
         @out.c.own (obj** or obj*&) formalname, 
         @in.s.own  (obj*  or obj&) formalname)




tags hold: anonymous pointers... that should also be tagged with a name, and look-upable by that name.

*/
#endif /* 0 */




#endif /* AUTOTAG_H */

