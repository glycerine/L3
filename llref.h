//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#ifndef LLREF_H
#define LLREF_H

#include "terp.h"
#include "limits.h" // for PATH_MAX
//#include "addalias.h"
#include "jlmap.h" // for ustaq, for holding properties.

/* 
   llref: reference linking for pub-sub alias semantics; where all the aliases disappear upon object removal.

   See Andrei Alexandrescu's Modern C++ Design, section 7.5.4 (p167) on 
   reference linking smart pointers. On Google Books:

   http://books.google.com/books?id=aJ1av7UFBPwC&pg=PA159&lpg=PA159&dq=andrei+alexandrescu+modern+c%2B%2B+design+smart+pointers&source=bl&ots=YRaG1sVe80&sig=Wt7nGMSs9yyePSVj1GAKGp2CF3E&hl=en&ei=TMK1TcOVK4La0QHsidHSAg&sa=X&oi=book_result&ct=result&resnum=3&ved=0CCkQ6AEwAg#v=onepage&q&f=false

   As per section 7.4 in Modern C++ Design, it is a bad idea
   to use member functions on smart pointers. This lets the
   compiler catch when you don't dereference the smart pointer
   correctly. Not an issue in C instead of C++.

*/

struct _l3obj;
class Tag;

#define LLREF_KEY_LEN PATH_MAX
struct llref {

    MIXIN_TOHEADER() // gives us:
    // t_typ   _type;     // lets us distinguish the type of this object
    // long    _ser;      // a serial number, to recognize replacement. (merge with _llsn at some point).
    // uint    _reserved; // allow refs to be marked as sysbuiltin.
    

  llref*     _next;
  llref*     _prev;
  _l3obj*    _obj;

  _l3obj*    _env; // when _obj dies, remove the reference to _obj from this _env->_judyS array.
  char       _key[LLREF_KEY_LEN]; // name of _obj in _env->_judyS

  lnk*       _lnk; // alternatively the llref can be in the lnk, so we  _lnk->_llr = 0, after de-chaining and deleting it.

  Tag*      _refowner; // who owns this llref*, allocated with llr_malloc.  This owner 
    // is not necessarily the same as _obj->_owner; but can be. And may wish to be by default.
    //   the llref* r is allocated with malloc and has owner->llr_push(r) called.

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
    //
    //extern t_typ   t_iso; // ="t_iso"; // in_server_owns type 
    //extern t_typ   t_ico; // ="t_ico"; // in_client_owns type
    //extern t_typ   t_oso; // ="t_oso"; // out_server_owns type
    //extern t_typ   t_oco; // ="t_oco"; // out_client_owns type

    void add_prop(t_typ prop) {
        // if (!_properties.member(prop)) {
        _properties.push_back((char*)prop);
        // }
    }
    void del_prop(t_typ prop) {
        _properties.del_val((char*)prop);
    }
    bool has_prop(t_typ prop) {
        return (_properties.member((char*)prop) != 0);
    }

    void update_key(char* newkey);

    //#ifdef _JLIVECHECK
    long       _llsn;
    //#endif

    static l3path _singleton_printbuf; // shared by all llref, just so we can print them easily.

    char* prop_string() {
        _singleton_printbuf.clear();
        _properties.dump_to_l3path(&_singleton_printbuf);
        return _singleton_printbuf();
    }

    void cp_prop_from(llref* src) {
        for(src->_properties.it.restart();  !src->_properties.it.at_end();  ++src->_properties.it) {
            add_prop( *(src->_properties.it));
        }
    }
};

// prefer llref_new to llr_malloc/llref_init_private, since llref_new does these for you, both at once.
llref* llref_new(const qqchar& key, _l3obj* obj, _l3obj* env_key_lives_in, llref* pre, Tag* owner, lnk* in_lnk);

void llref_init_private(llref* ref, const qqchar& key, l3obj* obj, l3obj* env_key_livesin, llref* pre, Tag* owner, lnk* in_lnk);

const long MAX_LLREF_CHAIN_LEN = 100;

typedef enum { NO_DO_NOT_HASH_DELETE=0, YES_DO_HASH_DELETE=1 } en_do_hash_delete;

void llref_hash_delete(llref* w);

void llref_verify_chain(llref* r, long max_llref_chain_len);

void llref_del(llref* w, en_do_hash_delete  do_hash_delete);

// deleting a priority1 reference requires calling this separate function, because
// the priority 1 ref lives in the Tag::_owned map, and the deletion from _owned and
// free of the llref* memory has to be coordinated to prevent a dangling reference in _owned.
void llref_del_priority1_because_we_deleted_from_owned_set(llref* w, en_do_hash_delete  do_hash_delete);

// call llref_del on all in the ring.
typedef enum {DELETE_ALL_EXCEPT_PRIORITY_ONE, DELETE_PRIORITY_ONE_TOO} en_leave_priority_one;
void llref_del_ring(llref* r, en_leave_priority_one leave1, en_do_hash_delete  do_hash_delete);

long llref_size(llref* r);

// quick check--is there more than one ref? (this is faster than chasing the whole chain).
long llref_pred_refcount_one(llref* r);

// easy of calling llref_pred_refcount_one on an object:
long refcount_minimal(l3obj* o);


void print_llref(llref* r, char* pre = 0);

// return 0 if no such key in this ring, else return the llref*
llref* llref_match_key(llref* r, char* key);

// remove the link named by key; throws if no such key.
void llref_rm_key(llref* r, char* key);

// priorities are >= 0; and a zero found always stops the search immediately, returning that reference.
// prefer the lowest number greater than 1, if available, which is the readable one.
llref* priority_ref(llref* r);

// return max priority seen across the ring
long llref_primax(llref* r);

// return the reference at a specified priority level, or 0 if none at that level
llref* get_ref_at_fixed_priority(llref* r, long priority_sought);

// check for env in the ring, return the llref if the env is in the ring. (but not a priority 1 ref)
llref* query_for_env(llref* startingref, l3obj* target_env);

//////////////////

void check_dangling(); // sanity check all llrefs. Slow. For debugging only.

//////////////////


// now in addalias.h
//void add_alias(l3path* name_to_insert, l3obj* target, l3obj* env_to_insert_in);
//void rm_alias(l3obj* env_to_rm_from, const qqchar& name_to_rm);

// generate a vector of all aliases for the given object.
L3FORWDECL(aliases)
L3FORWDECL(llref2ptrvec)

// generate a vector of all aliases for each object in a vector
L3FORWDECL(valiases)

// generate a canonical name for an alias, based on the ownership tree.
L3FORWDECL(canon)
L3FORWDECL(canon_env_into_objpath)

// for debug only
long llr_global_debug_list_add(llref* addme);
void llr_global_debug_list_del(llref* delme);
void llr_global_debug_list_check_dangling();

// used in the macro LIVEREF(myllref) in serialfac.h
void check_one_llref(llref* llr);

// an optimization for entire object deletion...
void llref_del_any_priority_and_no_hash_del(llref* w);

void llref_del_any_priority(llref* w, en_do_hash_delete  do_hash_delete);

void move_llref_to_new_ring(llref* moveme, llref* newpeer);

void move_all_visible_llref_to_new_ring(llref* moveme, llref* newpeer);

void conform_refowner_to_obj_owner(llref* adjustme);

//
// unchain: manual management: remove w from chain. Optionally delete _key from _env if do_hash_delete.
//
void unchain(llref* w); // , en_do_hash_delete  do_hash_delete);


// canon now calls this, so can code.
void llref_get_canpath(llref* r, l3path* canpath);

// modifying variables passed by reference uses this...
void llref_update_obj_of_ring_in_place(llref* r, llref* new_priority_one);




#if 0  // ddllref was an experiment, now depricated; lets not use intrinsic pointers, bad for modularity
//     // extrinsic lists instead. i.e. use a ddlist or dstaq directly.

////////////////////////////////////////////////
//
// ddllref : a double linked list of llref*, to create a named list, named queue, or symvec.
//            basically, this is a version of ddlist from jlmap.h that is adjusted to
//            handle llref* directly rather than having a distinct dd list node type.
//
// Advantages: (1) this let's us handle the _judyS index pointing from char* -> llref* uniformly.
//         and (2) symbols were just like llref
//         and (3) avoids having an extra doubly-linked list that mirrors the llref anyway.
//

struct ddllref {

    typedef llref dd;

    // members
    dd*               _head;  // aka front. back is _head->_prevsym;
    long              _size;

    // methods
    ddllref() {
        init();
    }

    // copy constructor
    ddllref(const ddllref& src) {
        init();
        if(src.size()) {
            push_back_all_from(src);
        }
    }

    ddllref& operator=(const ddllref& src) {
        if (&src == this) return *this; // no self-assignment

        clear();
        push_back_all_from(src);

        return *this;
    }


    void init() {
        _head = 0;
        _size = 0;
        it.set_staq(this);
    }

    void destruct_ddllref() {
        clear();
    }

    void    make_head(dd* ref) {
              assert(ref);
              // we are the first here, just point to ourself.
              ref->_nextsym = ref;
              ref->_prevsym = ref;
              _head = ref;
    }

#if 1 // now implemented *without judyLmap*, via linear search.

    // return non-zero if ptr is a member of the set/queue... else 0
    dd*    member(llref* ptr) {
        if (ptr ==0) return 0;

        ddllref_it it(this);
        for (; !it.at_end(); ++it) {
            if (*it == ptr) return it();
        }
        return 0;
    }

    dd*    member(const llref* ptr) {
        if (ptr == 0) return 0;

        ddllref_it it(this);
        for (; !it.at_end(); ++it) {
            if (*it == ptr) return it();
        }
        return 0;
    }


#endif

    dd*  push_back(llref* ref) {        

        if (_head) {
            // insert our ref into the double linked chain, last (== prior to head)
            ref->_prevsym = _head->_prevsym;
            ref->_nextsym = _head;

            _head->_prevsym->_nextsym = ref;
            _head->_prevsym = ref;
        } else make_head(ref);

        ++_size;
        return ref;
    }

    // const: the biggest waste of time, ever, when after-the-fact bolted on.
    //dd*  push_back(const llref* ptr) { return push_back((llref*)ptr); }
    //dd*  push_front(const llref* ptr) { return push_front((llref*)ptr); }


    // ptr becomes the node at the head of the list
    dd*  push_front(llref* ref) {

        if (_head) {
            ref->_prevsym = _head->_prevsym;
            ref->_nextsym = _head;

            _head->_prevsym->_nextsym = ref;
            _head->_prevsym = ref;            

            _head = ref;
        } else make_head(ref);

        ++_size;
        return ref;
    }

    long size() const { return _size; }

    dd* front() const {
        return _head;
    }
    dd* head() const {
        return _head;
    }

    dd* back() const {
        if (0==_head) return 0;
        return _head->_prevsym;
    }
    dd* tail() const {
        if (0==_head) return 0;
        return _head->_prevsym;
    }


    dd* next(dd* ref) {
        return ref->_nextsym;
    }

    dd* prev(dd* ref) {
        return ref->_prevsym;
    }


    // 
    // del: delete from _nextsym list, and (for now don't: call llref_del )
    //        because it isn't clear how to do pop? makes no sense to delete the llref.
    //
    void del(dd* w) {
        assert(w);
        _size--;

        if (w->_prevsym == w) {
            // last ref, no chain to modify
            assert(w->_nextsym == w);
            _head = 0;
            assert(_size == 0);
        } else {
            // take ourselves out of the loop

            if (_head == w) {
                _head = _head->_nextsym;
            }

            w->_prevsym->_nextsym = w->_nextsym;
            w->_nextsym->_prevsym = w->_prevsym;
            
            // and isolate ourselves, to avoid any inf loops upon debug examining.
            w->_prevsym = w;
            w->_nextsym = w;
        }

        // if it is pointing at us, move 

        // probably not...! 
        //llref_del(w, NO_DO_NOT_HASH_DELETE);
    }


    llref* pop_back() {
        llref* r = back();
        del(r);
        return r;
    }

    llref* pop_front() {
        llref* r = front();
        del(r);
        return r;
    }

    void dump(const char* indent = "") {
        printf("\n >>>>>>>>>> begin ddllref::dump()\n");
        if(0==_head) {
            printf("empty\n");
        } else {

            printf("\n");
            dd* cur  = head();
            dd* last = tail();
            
            while(1) {
                printf("(llref: %p  _llsn=%ld)\n",cur,cur->_llsn);
                if (cur == last) break;
                cur = cur->_nextsym;
            }
            
        }
        printf(" >>>>>>>>>> end of ddllref::dump()\n");

    } // end dump()


    void dump_to_l3path(l3path* out) {
        assert(out);
        if(0==_head) return;

        dd* cur  = head();
        dd* last = tail();
        
        while(1) {
            out->pushf("(llref %p  _llsn=%ld) ",cur,cur->_llsn); // allow multiple with space separation
            if (cur == last) break;
            cur = cur->_nextsym;
        }
        out->chomp(); // remove the last space.

    } // end dump_to_l3path()


    void clear() {
        if (!_size) return;
        DV(printf("in ddllref::clear() with size : %ld\n",size()));

        assert(_head);
        dd* cur  = head();
        dd* nex  = 0;
        dd* last = tail();

        while(1) {
            // retreive next from cur before deleting cur
            nex = cur->_nextsym;

            DV(printf("in ddllref::clear(), about to delete : (llref %p _llsn=%ld)\n",cur, cur->_llsn));

            // do the deletion
            del(cur);

            // check if we are done
            if (cur == last) break;

            // not done, move to next
            cur = nex;
        }

    }

    // iterator like-iterface

    //
    // new iterator protocol: to be resilient against deletes we
    //  note the head when we start, and store it in _first. 
    //  But what if that head is subsequently deleted?
    //
    //
    struct ddllref_it {

        dd* _cur;
        dd* _nex;
        dd* _last;
        ddllref* _staq;
        long    _size_at_start;
        long    _numincr;
        
        ddllref_it(ddllref* staq = 0) {
            _staq=staq;
            // cannot restart right away, if we don't have staq yet
            if (_staq) {
                restart();
            }
        }
        void set_staq(ddllref* staq) {
            assert(staq);
            _staq=staq;
            restart();
        }

        inline void restart()
        {
            assert(_staq);
            _cur  = _staq->head();
            if (_cur) { 
                _nex  = _cur->_nextsym; 
            } else {
                _nex = 0;
            }
            _last = _staq->tail();
            
            _size_at_start = _staq->size();
            _numincr = 0;
        }
        
        inline dd* operator()() { 
            assert(_staq);
            return _cur; 
        }
        inline llref* operator*()  { 
            assert(_staq);
            return _cur; 
        }
        
        inline bool       at_end() { 
            assert(_staq);
            return (_numincr == _size_at_start); 
        }
        
        inline void operator++() {
            assert(_staq);
            if (at_end()) {
                assert(_cur == _last);
                return; // we are done
            }
            _numincr++;
            
            _cur = _nex;
            if (!at_end()) { _nex = _cur->_nextsym; }
        }
    };
    
    // a default iterator already instantiated, for ease of use.
    ddllref_it  it;

    void push_back_all_from(const ddllref& src) {
        ddllref& nsrc = (ddllref&)src;
        for (nsrc.it.restart(); !nsrc.it.at_end(); ++nsrc.it ) {
            push_back(*nsrc.it);
        }
    }

};

#endif




#ifdef _JLIVECHECK


#else


#endif


#endif /* LLREF_H */

