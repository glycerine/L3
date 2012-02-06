//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#include "l3obj.h"
#include "autotag.h"
#include "objects.h"
#include "terp.h"
#include "judydup.h"
#include "tostring.h"
#include "symvec.h"
#include "llref.h"
#include "addalias.h"
#include "dynamicscope.h"
#include "l3string.h"

/////////////////////// end of includes

//
// Important note:
//    Because struct llref contains ustaq sub-objects that are C++ template objects
//    that require recursive initialation, llref are allocated with new and
//    dealloated with delete. For this reason they do not have normal _ser serial
//    numbers.  
//
//    We could give them dmalloc (in debug mode) serial numbers for debugging purposes,
//    and use negative serial numbers to indicate that this is the case. So
//    a negative _ser implies allocated with new. If we decide to implement.
//
//    Currently these _ser are just 0.

// global for tracking and checking all llref
llref* llr_global_debug_list = 0;

// class statics
l3path llref::_singleton_printbuf; // shared by all llref, just so we can print them easily

void common_llref_del(llref* w, en_do_hash_delete  do_hash_delete);

inline long lmax(long a, long b) {
    if (a >= b) return a;
    return b;
}

// return max priority seen across the ring
long llref_primax(llref* r) {
  assert(r);
  LIVEREF(r);

  long maxpri = r->_priority;
  llref* cur = r->_next;
  while(cur != r) {
    maxpri = lmax(cur->_priority,maxpri);
    cur = cur->_next;
  }
  return maxpri;
}


// new: llref_new allocates and does llref_init() call into one, 
//      like C++ new, and offers better atomicity of initialization

llref* llref_new(const qqchar& key, _l3obj* obj, _l3obj* env_key_lives_in, llref* pre, Tag* owner, lnk* in_lnk) {
    if (env_key_lives_in) { LIVEO(env_key_lives_in); }
    LIVET(owner);
    if (pre) { LIVEREF(pre); }

    // use new now instead of malloc to get the ustaq initialization done
    llref* r = new llref;

    assert(r);
    assert(owner);
    llref_init_private(r, key, obj, env_key_lives_in, pre, owner, in_lnk);
    owner->llr_push(r);
    

    LIVEREF(r);
    return r;
}

//
//
// pre : points to any pre-existing chain. Can be 0 if none.
//
// refs of priority 1 are hidden (b/c they are internal use) when ranking for priority
//
void llref_init_private(llref* ref, const qqchar& key, l3obj* obj, l3obj* env_key_livesin, llref* pre, Tag* owner, lnk* in_lnk) {
    // cannot do this now that we are using new, because _properties are already set:    bzero(ref,sizeof(llref));

    assert(obj);
    assert(obj->_owner == owner);

    ref->_type = t_llr;

    // now that llref are also MIXIN_TOHEADER, set owner and type and ser so things don't look corrupted.

    // _ser will be negative, by convention, to indicate a new/delete sermon serial num.
    long smn = 0;

#ifndef  _DMALLOC_OFF
    BOOL foundit = gsermon->found_sermon_number(ref, &smn);
#else
    BOOL foundit = FALSE;
#endif
    if (foundit) {
        // not necessarily found, on Mac, where not all new's get through to dmalloc.
        ref->_ser = smn;
    } else {
        ref->_ser = -1; // not found sentinel. at least not uninitialized.
    }

    ref->_owner      = obj->_owner;
    ref->_refowner = obj->_owner;
    ref->_obj = obj;

    // lifetime of llref must match lifetime of objects they point to, so make sure this invarient holds:
    assert(ref->_obj->_owner == ref->_refowner);

    ref->_lnk = in_lnk;

    ref->_env = env_key_livesin;
    if (key.len()) {
        long wrote=key.copyto(ref->_key,  LLREF_KEY_LEN-1);
        ref->_key[wrote]='\0';
    } else {
        ref->_key[0]='\0';
    }
    ref->_reserved =0;

    if (pre) {
        DV(print_llref(pre));

        // set our priority to 1 over the biggest yet seen.
        ref->_priority = 1 + llref_primax(pre);

        // insert our ref into the double linked chain
        ref->_next = pre->_next;
        ref->_prev = pre;

        ref->_next->_prev = ref;
        pre->_next = ref;

        DV(print_llref(pre));
    } else {
        // this _priority == 1 reference is what lives in the 
        // the _owned map, and so should not be deleted until
        // the _owned map entry for _obj is simultaneously deleted.
        ref->_priority = 1;
        // we are the first here, just point to ourself.
        ref->_next = ref;
        ref->_prev = ref;

        DV(print_llref(ref));
    }

#ifdef _JLIVECHECK
    ref->_llsn = LLRADD(ref);
    DV(printf("llref_new() debug: allocated _llsn %ld\n", ref->_llsn));
#endif
}


void llref_verify_chain(llref* r, long max_llref_chain_len) {

  assert(r == r->_next->_prev);
  assert(r == r->_prev->_next);

  long nref = 1;
  llref* cur = r->_next;
  long num_priority_one_refs = 0;

  // check that there is just one _priority one reference
  if (r->_priority == 1) {
      ++num_priority_one_refs;
  }
  
  DVV({ printf("\n");
          printf("verifying chain %ld node = %p\n",nref, r); } );
  while(cur != r) {
    ++nref;
    DVV(printf("verifying chain %ld node = %p\n",nref, cur));

    // check that there is just one _priority one reference
    if (cur->_priority == 1) {
        ++num_priority_one_refs;
    }

    assert(cur == cur->_next->_prev);
    assert(cur == cur->_prev->_next);
    cur = cur->_next;

    assert(nref < max_llref_chain_len);
  }

  DVV(printf("llref_verify_chain(): chain looks good. %p is in a chain with %ld references\n",r,nref));

  // check that there is just one _priority one reference. but it can be zero too.
  assert(num_priority_one_refs <= 1);
  
}


void llref_del_priority1_because_we_deleted_from_owned_set(llref* w, en_do_hash_delete  do_hash_delete) {
    assert(w);

    common_llref_del(w,do_hash_delete);
}

void llref_del(llref* w,en_do_hash_delete  do_hash_delete) {
    assert(w);
    LIVEREF(w);

    if (w->_priority == 1) {
        DV(printf("NOTE: skipping deletion of llref::_priority == 1 llref %p, since this needs to live until the _owned map is done with object %p ser# %ld\n",
                  w,w->_obj, w->_obj->_ser););
        return;
    }

    common_llref_del(w,do_hash_delete);
}

// ignore priority 1, just get it off the ring
void llref_del_any_priority(llref* w, en_do_hash_delete  do_hash_delete) {
    assert(w);
    common_llref_del(w,do_hash_delete);
}

// an optimization for entire object deletion...
void llref_del_any_priority_and_no_hash_del(llref* w) {
    assert(w);
    common_llref_del(w,NO_DO_NOT_HASH_DELETE);
}


void llref_hash_delete(llref* w) {

    // priority 1 references aren't stored in env, only in the _owned map
    if (w->_priority == 1) {
      
    } else {

        // don't bother if it's a lnk embedded llref...
        if (w->_env) {
            assert(w->_lnk == 0);

            if (w->_env->_type == t_syv) {
                symvec_del(w->_env,w->_key);
            } else {
                delete_key_from_hashtable(w->_env, w->_key, w);
            }
        }
    }
}


//
// unchain: manual management: remove w from chain. Optionally delete _key from _env if do_hash_delete.
//
void unchain(llref* w) {   // , en_do_hash_delete  do_hash_delete) {

    if (w->_prev == w) {
        // last ref, no chain to modify
        assert(w->_next == w);
    } else {
        // take ourselves out of the loop
        w->_prev->_next = w->_next;
        w->_next->_prev = w->_prev;
        
        // and isolate ourselves, to avoid any inf loops upon debug examining.
        w->_prev = w;
        w->_next = w;
    }

} // end unchain

//
// called by llref_del()  and  llref_del_priority1()
//
void common_llref_del(llref* w, en_do_hash_delete  do_hash_delete) {
    LIVEREF(w);

    unchain(w);
    if (do_hash_delete==YES_DO_HASH_DELETE) { llref_hash_delete(w); }

#if 1
    // zero the lnk, if embedded there.
    if (w->_lnk) {
        assert(w->_env == 0);
        w->_lnk->zeroref();  // tell the lnk that it is dangling.
    }
#endif
    // let the llf_free zero this ref, so it can display first. 
    Tag* owner = w->_refowner;

    // may or may not free immediately.
    if (owner) {
        owner->llr_free(w); 
    } else {
        printf("internal error: there should always be an owner for the llref... not found for %p :\n",w);
        assert(0);
        exit(1);
    }

    // ustaq<char> _properties; // this should be taken care of by the destructor now.

}


long llref_size(llref* r) {
  assert(r);

  unsigned long numref = 1;
  llref* cur = r->_next;
  while(cur != r) {
    ++numref;
    cur = cur->_next;

    if (numref>10000) {
        printf("internal error: over 10K llrefs are suspect of inifinite loop. Aborting.\n");
        assert(0);
        exit(1);
    }
  }
  return numref;
}


// priority_ref : in typical use, this will return the _priority == 2 reference, which is the most readable one
//  if not available, it will return the smallest priority that is > 1.
//
//
// specify minpri as -1 to get the lowest priority...still, a zero found always stops the search immediately.
//
// refs of priority 1 are hidden (b/c they are internal use) when ranking for priority
//
llref* priority_ref(llref* r) {
  assert(r);
  LIVEREF(r);

  long   N = llref_size(r);
  //  long   minpri = 1L << 62;
  long   minpri = r->_priority;
  llref* minref = r;

  llref* cur = r;
  for (long i =0 ; i < N; ++i, cur = cur->_next) {
      assert(cur->_priority >= 0); // we only allow priorities of >= 0.

      DV(printf("%02ld   cur %p: priority %ld   key %s\n",i,cur,cur->_priority,cur->_key));

      // 0 gives a sticky ref : we don't look elsewhere after hitting a 0.
      if (cur->_priority == 0) {
          // done with search
          minpri = cur->_priority;
          minref = cur;
          break;
      }

      // prefer the lowest number greater than 1, if available, which is the readable one.
      if (minpri < 2 && cur->_priority >= 2) {
          minpri = cur->_priority;
          minref = cur; 
          continue;
      }

      if (cur->_priority >= minpri) continue;
      if (cur->_priority < minpri && cur->_priority >= 2) {
          minpri = cur->_priority;
          minref = cur;
      }
  }
  LIVEREF(minref);
  return minref;
}


void print_llref(llref* r, char* pre) {
  assert(r);
  LIVEREF(r);
  const char emptystring[1]="";
  if (0==pre) {pre = (char*)emptystring; }

  long N = llref_size(r);
  if (0==N) {
      printf("%sllref %p was empty; size zero!?!?\n",pre,r);
      assert(0);
      exit(1);
  }
  llref* cur = r;

  printf("%sn  llref %p :\n",pre,r);
  for (long i =0 ; i < N; ++i, cur = cur->_next) {
      assert(cur->_priority >= 0); // we only allow priorities of >= 0.
      printf("%s%02ld   (llref*)=%p:  _priority=%ld;  _next=%p  _prev=%p  "
             "_obj=(%p ser# %ld %s)  "
             "_env=(%p ser# %ld %s)  "
             "_lnk=(%p _name:'%s' target()->_ser:%ld)  "
             "_key='%s'  "
             "_refowner=%p  _llsn=%ld  _properties='%s'\n",
             pre, i,           cur,  cur->_priority,  cur->_next, cur->_prev, 
             cur->_obj, cur->_obj->_ser, cur->_obj->_varname,

             cur->_env, cur->_env ? cur->_env->_ser : 0, cur->_env ? cur->_env->_varname : 0,
             cur->_lnk, cur->_lnk ? cur->_lnk->name() : 0, (cur->_lnk && cur->_lnk->target() )? cur->_lnk->target()->_ser : 0,

             cur->_key, cur->_refowner, cur->_llsn, cur->prop_string()
             );
  }
}


// if we shifted to using a ustaq<llref>, we could use the index for (perhaps)
// faster deletes. for later.
 
 // return 0 if no such key in this ring, else return the llref*
 llref* llref_match_key(llref* r, char* key) {
     LIVEREF(r);
     long   N = llref_size(r);
     llref* cur = r;
     for (long i =0 ; i < N; ++i, cur = cur->_next) {
         if (0==strcmp(cur->_key,key)) return cur;
     }
     return 0;
 }

 // remove the link named by key; throws if no such key.
 void llref_rm_key(llref* r, char* key) {
     assert(r);
     llref* match = llref_match_key(r, key);

     if (match) {
         llref_del(match,YES_DO_HASH_DELETE);
         return;
     }

     printf("error in llref_rm_key: could not match requested key: '%s'.\n",key);
     l3throw(XABORT_TO_TOPLEVEL);
 }

//typedef enum {DELETE_ALL_EXCEPT_PRIORITY_ONE, DELETE_PRIORITY_ONE_TOO} en_leave_priority_one;

// if leave_priority_one == 0, then we delete the priority == 1 ref as well, else keep it.
void llref_del_ring(llref* r, en_leave_priority_one leave1, en_do_hash_delete  do_hash_delete) {
    assert(r);
    long   N = llref_size(r);
    assert(N);
    LIVEREF(r);

    int onedeleted = 0;

    llref* cur = r;
    llref* nex = 0;
    for (long i =0 ; i < N; ++i) {
        nex = cur->_next;

        // why? what if we have no priority 1?...we always have a priority 1, that
        // resides in the _owned map from l3obj* -> llref*. we have to remove from
        // the _owned map and delete the free llref* at the same time.

        if (leave1 == DELETE_PRIORITY_ONE_TOO) {
            // always delete
            common_llref_del(cur,do_hash_delete);
            onedeleted =1;
        } else if (cur->_priority != 1) {
            llref_del(cur,do_hash_delete);
            onedeleted =1;
        }
        cur = nex;
    }

}


// old arg order: void add_alias(l3path* name_to_insert, l3obj* target, l3obj* env_to_insert_in) {
// new arg order: match insert_into_hashtable
//void add_alias(l3obj* env_to_insert_in, l3path* name_to_insert, l3obj* target) {
llref*  add_alias_eno(l3obj* env_to_insert_in, const qqchar& name_to_insert, l3obj* target) {

    assert(target);
    LIVEO(target);
    Tag* owner = target->_owner;
    assert(owner);
    LIVET(owner);

    l3path name(name_to_insert);

    llref* pre = owner->find(target);

    ulong sl = name_to_insert.size();
    if (sl > LLREF_KEY_LEN-1) {
        std::cout << "error in add_alias_eno: name_to_insert '"<< name_to_insert << "'";
        printf("exceeds llref capacity %d; lookup will be impossible, aborting.\n",LLREF_KEY_LEN-1);
        l3throw(XABORT_TO_TOPLEVEL);
    }

    // owner for llrefs and owner for object want to be the same for efficient arena-style deallocation,
    //  but if you really need to have an llref owned by someone else, that's alloed, and can be
    // noticed by having the _obj->_owner != _refowner.  In other words, we cannot assume that _obj->_owner == _refowner? We just don't know for sure.
    //
    // Tag*      _refowner; // _refowner is who owns this llref*.   This owner 
    // is not necessarily the same as _obj->_owner; but can be. And may wish to be by default.

    assert(owner);
    llref* r = llref_new(name_to_insert, target, env_to_insert_in, pre, owner,0);

    // symbols (symvec) need special insert/update treatment
    if (env_to_insert_in->_type == t_syv) {
        l3obj* already = 0; 

        symbol* psym = symvec_get_by_name(env_to_insert_in, name_to_insert, &already);
        if (psym) {
            psym->_obj = target;
        } else {
            internal_symvec_pushback(env_to_insert_in, name_to_insert, target);
        }

    } else {
        // we want to add a name; ownership is already done, but add an alias...via adding an llref*

        void* pre_existing_name = lookup_hashtable(env_to_insert_in, name());
        l3obj* preobj = 0;
        llref* prellr = 0;
        //l3obj* fakeret = 0;
        if (pre_existing_name) {

            // do the soft delete...
            prellr = rm_alias(env_to_insert_in, name_to_insert);

            if (prellr) {
                LIVEREF(prellr);
                preobj = prellr->_obj;

#if 0 // I don't think we actually want a hard delete in add_alias_eno()...no. certainly not!
                // do the hard delete...
                hard_delete(preobj,-1,0,
                            0,&fakeret,owner,
                            0,0,owner);
#endif

            }

        }

        // notice the change: we insert the llref* r now, instead of the target directly
        insert_private_to_add_alias(name(), r, env_to_insert_in);
    }

    return r;
 }

// given: env_to_rm_from
//        name_to_rm
//
// returns: 0 if no more aliases, else an llref* to the object that had one alias deleted.
//
llref* rm_alias(l3obj* env_to_rm_from, const qqchar& name_to_rm) {

    l3path rmbase(name_to_rm);
    llref* retval = 0;


    objlist_t* envpath=0;
    l3obj* nobj = 0;
    llref* innermostref = 0;
    nobj = RESOLVE_REF(name_to_rm,env_to_rm_from,AUTO_DEREF_SYMBOLS,&innermostref, envpath, name_to_rm, UNFOUND_RETURN_ZERO);
    
    if (!nobj) {
        std::cout << "error: softrm could not find target '" << name_to_rm << "'.\n";
        l3throw(XABORT_TO_TOPLEVEL);
    }
    
    llref* r = innermostref;

    if (!r) {
        std::cout << "internal error: attempting to rm_alias '" << name_to_rm << "' from";
        printf(" (%p ser# %ld %s) : ",
               env_to_rm_from, 
               env_to_rm_from->_ser, 
               env_to_rm_from->_varname);
        std::cout << "could not find '" << name_to_rm << "'.\n";
        l3throw(XABORT_TO_TOPLEVEL);
    }

    if (r->_next != r) {
        retval = r->_next;
    }

    if (is_sysbuiltin(r)) {
        std::cout << "error: cannot delete builtin reference '"<< name_to_rm << "'.\n";
        l3throw(XABORT_TO_TOPLEVEL);
    }

    LIVEREF(r);
    llref_del(r,YES_DO_HASH_DELETE);

    if (retval) {
        LIVEREF(retval);
    }
    return retval;
 }


// generate a vector of all aliases for the given object.
L3METHOD(aliases)
{
   arity = 1;
   k_arg_op(L3STDARGS);
   l3obj* vv = *retval;
   *retval = 0;

   long N = 0;
   l3obj* aliasreq = 0;
   l3path saliasreq;   
   long i = 0;
   llref* llr = 0;

   XTRY
       case XCODE:

           N = ptrvec_size(vv);
           assert(N == 1);

           ptrvec_get(vv,i,&aliasreq);
           
           llr = aliasreq->_owner->find(aliasreq);
           llref2ptrvec((l3obj*)llr, -1,0,L3STDARGS_ENV);
      
       break;
       case XFINALLY:
            generic_delete(vv, L3STDARGS_OBJONLY);   
       break;
   XENDX

}
L3END(aliases)


// print the aliases for obj
L3METHOD(llref2ptrvec)

   llref* llr = (llref*) obj;
   long N = llref_size(llr);

   llref* cur = llr;
   make_new_ptrvec(L3STDARGS);
   l3obj* vv = *retval;

   l3path sstr;
   l3path llrefname;
   l3obj* str = 0;

   for(long i = 0 ; i < N; ++i) {
       // make a new string object to hold the reference
       llrefname.init("llref_");
       llrefname.pushf("%ld",i);
       sstr.clear();
       str = make_new_string_obj(sstr(),owner,llrefname());
 
       // get the path to the cur llref into sstr

       // TODO: walk up to the enclosing env and repeat...hmm... who is my parent? 
       // lets trace the ownership tree to provide a canonical dir listing

       //llref* pri = priority_ref(cur);
       llref* pri = cur;

       if (sstr.len()) {
           sstr.prepushf("%s.",pri->_key);
       } else {
           sstr.pushf("%s",pri->_key);
       }

       // store it
       string_set(str,0,sstr());
       ptrvec_set(vv,i,str);
       cur = cur->_next;
   }

   *retval = vv;

L3END(llref2ptrvec)


// generate a vector of all aliases for each object in a vector
L3METHOD(valiases)

   any_k_arg_op(L3STDARGS);
   l3obj* vv = *retval;
   *retval = 0;
   long N = ptrvec_size(vv);

   l3obj* aliasreq = 0;
   l3path saliasreq;
   llref* llr = 0;

   XTRY
       case XCODE:

           for (long i = 0; i < N; ++i ) {
             saliasreq.clear();
             ptrvec_get(vv,i,&aliasreq);
             
             llr = aliasreq->_owner->find(aliasreq);
             llref2ptrvec((l3obj*)llr, -1,0,L3STDARGS_ENV);
             ptrvec_set(vv,i,*retval);
           }
           *retval = vv;

       break;
       case XFINALLY:
            generic_delete(vv, L3STDARGS_OBJONLY);   
       break;
   XENDX


L3END(valiases)


// generate a canonical name for an alias, based on the ownership tree.
// the string is in exp, as the ith_child(exp,1);
L3METHOD(canon)
{
   arity = 1;
   k_arg_op(L3STDARGS);
   l3obj* vv = *retval;
   *retval = 0;

   l3obj* aliasreq = 0;
   l3path canonpath;

   XTRY
       case XCODE:

           ptrvec_get(vv,0,&aliasreq);
           llref_get_canpath(aliasreq->_owner->find(aliasreq), &canonpath);
           *retval = make_new_string_obj(canonpath(),retown,"canon_output");

       break;
       case XFINALLY:
            generic_delete(vv, L3STDARGS_OBJONLY);   
       break;
   XENDX

}
L3END(canon)

// in:  l3obj* to find canonical for, in env
// in/out: *l3path in obj
// in:  arity indicates depth to go to, with -1 being "all the way", and 1 being: stop after the first.
L3METHOD(canon_env_into_objpath)

   assert(obj);
   assert(env);

   l3path* mypathout = (l3path*)obj;
   l3path& canonpath = *mypathout;

   l3obj* capt = env;
   l3obj* new_capt = 0;
   Tag*  mytag_parent = 0;

   llref* llr = 0;
   llref* can = 0;

   long depth_stop = arity;

   while(1) {
     if (!capt) break;
     if (!(capt->_owner)) break;

      llr = capt->_owner->find(capt);
      assert(llr);

      can = priority_ref(llr);
      canonpath.prepushf("%s.",can->_key);

      --depth_stop;
      if (0==depth_stop) break;

      new_capt = can->_obj->_owner->captain();
      if (new_capt == capt) {
        // we have a captag pair... try bumping up to tag's parent
        mytag_parent = can->_obj->_owner->parent();

        if (!mytag_parent) {
          // at global top. Stop.
          break;
        }
        new_capt = mytag_parent->captain();

        if (new_capt == capt) {
          // couldn't get out of the cycle...hmmm
          assert(0);
          break;
        }
      }

      if (new_capt == main_env) break;

      capt = new_capt;
   }
   canonpath.rdel(1); // delete last .

L3END(canon_env_into_objpath)


// return the reference at a specified priority level, or 0 if none at that level
llref* get_ref_at_fixed_priority(llref* r, long priority_sought) {
  assert(r);
  assert(priority_sought >=0);
  LIVEREF(r);

  long   N = llref_size(r);
  llref* cur = r;

  for (long i =0 ; i < N; ++i, cur = cur->_next) {

      assert(cur->_priority >= 0); // we only allow priorities of >= 0.
      DV(printf("%02ld   cur %p: priority %ld   key %s\n",i,cur,cur->_priority,cur->_key));

      if (cur->_priority == priority_sought) {
          return cur;
      }
  }
  return 0;
}

// sanity check all llrefs. Slow. For debugging only.
void check_dangling() {


}

void check_one_llref(llref* llr) {

   llref_verify_chain(llr, MAX_LLREF_CHAIN_LEN);

   long N = llref_size(llr);
   llref* cur = llr;

   for(long i = 0 ; i < N; ++i) {

       llref* pri = cur;

       // assert that this is a sane llref with live everything and no dangling pointers.
       assert(pri);
       assert(pri->_next);
       assert(pri->_prev);
       assert(pri->_obj);
       assert(pri->_env || pri->_lnk);
       assert(pri->_refowner);
       LIVEO(pri->_obj);
       LIVET(pri->_obj->_owner);
       //       LIVEO(pri->_env);
       LIVET(pri->_refowner);

       // actually we cannot even do this, since we could be erasing a reference for transfer, and already made the change.
       //assert(pri->_obj->_owner == pri->_refowner); // the proclaimed owner of the object and the proclaimed owner of the reference must match.

       // actually we cannot assert actual ownership yet, because of chicken-and-egg problem that
       // we have to allocate the ref before it can be added to the owning Tag.
       //assert(pri->_refowner->find(pri->_obj)); // the owner must actually own the obj too.
       //assert(pri->_refowner->llr_is_member(pri)); // and the proclaimed _refowner must actually own the reference.

       cur = cur->_next;
   }

}

typedef  std::map<llref*,long>  gllset;
typedef  gllset::iterator   gllsetit;

gllset global_llref_set;
long   global_last_llref_sn = 0;


long llr_global_debug_list_add(llref* addme) {

    long lastllsn = 0;    
    //printf("wierdness in llref.cpp:767 llr_global_debug_list_add(): pre-incr : global_last_llref_sn=%ld  \n", global_last_llref_sn);

    ++global_last_llref_sn;
    lastllsn = global_last_llref_sn;

    //    printf("wierdness in llref.cpp:767 llr_global_debug_list_add(): post-incr: global_last_llref_sn=%ld   lastllsn=%ld\n", global_last_llref_sn, lastllsn);

    global_llref_set.insert(gllset::value_type(addme,lastllsn));

    l3path msg(0,"333333 %p llref added: llref to obj (%p ser# %ld). llref had @llsn:%ld\n",
               addme, addme->_obj, addme->_obj->_ser,
               //               addme->_env, addme->_env->_ser, // "in env (%p ser# %ld)"
               lastllsn);
    MLOG_ADD(msg());

    return lastllsn;
}


void llr_global_debug_list_del(llref* delme) {
    LIVEREF(delme);

    // may be deleting references to objects already gone. So we cannot 
    // dereference delme->_obj or anything that assumes _obj is still good.
    // e.g.
    //not nec true:   LIVEO(delme->_obj);
    //not nec true:   LIVEO(delme->_env);

    l3path msg(0,"444444 %p llref delete: llref had @llsn:%ld\n",
               delme, 
               delme->_llsn);
    MLOG_ADD(msg());

    global_llref_set.erase(delme);
}


void llr_global_debug_list_check_dangling() {

    gllsetit be = global_llref_set.begin();
    gllsetit en = global_llref_set.end();

    long N = global_llref_set.size();
    if (!N) return;
    long i  = 0;
    llref* cur = 0;
    long cursn = 0;
    for (; be != en; ++be, ++i) {
        cur   = be->first;
        cursn = be->second;
        LIVEREF(cur);
    }
}



// llref_update_obj
//
// do this very carefully... in general this may be dangerous because
//  the owner stores a map between l3obj* -> llref*, which
// means you would also need to update the owner when updating the obj.
//  here we use it to bring the ring into consistency of which object
// Used by llref_update_ring_in_place(), after calling move_all_visible_llref_to_new_ring(r, new_peer).


// llref_update_obj
//
// change the _obj on this ring to newobj (and do some sanity verification asserts too).
//
void llref_update_obj(llref* r, l3obj* newobj) {
    assert(r);

    assert(r == r->_next->_prev);
    assert(r == r->_prev->_next);
    
    long nref = 1;
    llref* cur = r->_next;
    
    r->_obj = newobj; // update _obj
    
    while(cur != r) {
        ++nref;
        
        assert(cur == cur->_next->_prev);
        assert(cur == cur->_prev->_next);
        
        cur->_obj = newobj; // update _obj
        
        cur = cur->_next;
        
        assert(nref < MAX_LLREF_CHAIN_LEN);
    }
    
    conform_refowner_to_obj_owner(r);
}


//
// will make the moveme->_obj = newpeer->_obj, as well as having it join the new ring
//  and leave its old ring.
//
// if _refowner is different from newpeer's, then it is notified, 
//   and moveme->_refowner is changed to match newpeer->_refowner
//
// Later realization: llref* should always live in the same owner as the object that
//  they point to, should they not?
//
//
void move_llref_to_new_ring(llref* moveme, llref* newpeer) {
    assert(moveme->_priority != 1); // can't move out from under the judyS maps.
    LIVEREF(moveme);
    moveme->_obj = newpeer->_obj;

    if (moveme->_prev == moveme) {
        // last ref, no chain to modify
        assert(moveme->_next == moveme);
    } else {
        // take ourselves out of the old peer loop
        moveme->_prev->_next = moveme->_next;
        moveme->_next->_prev = moveme->_prev;
    }

    // insert into the newpeer ring
    moveme->_next = newpeer->_next;
    moveme->_prev = newpeer;
    
    moveme->_next->_prev = moveme;
    newpeer->_next = moveme;        

    
    assert(newpeer->_obj->_owner == newpeer->_refowner);

    // and update llrefowner if need be
    if (moveme->_refowner != newpeer->_refowner) {
        conform_refowner_to_obj_owner(moveme);
    }
    assert(moveme->_refowner == newpeer->_refowner);
    assert(moveme->_refowner == moveme->_obj->_owner);
}

// 
void move_all_visible_llref_to_new_ring(llref* moveme, llref* newpeer) {

    llref* peermv = moveme->_next;
    llref* nex    = 0;
    
    while(1) {
        DV(printf("move_all_visible_llref_to_new_ring(): evaluating whether to move (peermv=%p llsn %ld).\n",
                  peermv, peermv->_llsn));

        nex = peermv->_next;

        if (peermv == moveme) break; // completed the cycle.
        if (peermv->_priority == 1) {
            // skip priority 1

        } else {
            // move it
            move_llref_to_new_ring(peermv, newpeer);
        }        
        peermv = nex;
    }
    move_llref_to_new_ring(moveme, newpeer);
}



//
// allow diagnostics/troubleshooting by reporting all names attached to a reference
//
void llref_get_all_names(llref* r, l3path* names) {
  assert(r);
  LIVEREF(r);
  assert(names);

  l3path& n = *names;
  n.pushf("(");

  long N = llref_size(r);
  if (0==N) {
      n.pushf(")");
      return;
  }

  llref* cur = r;
  for (long i =0 ; i < N; ++i, cur = cur->_next) {
      assert(cur->_priority >= 0); // we only allow priorities of >= 0.
      if (i > 0) {
          n.pushf(" ");
      }
      n.pushf("\"%s\"",cur->_key);
  }
  n.pushf(")");

}



//
// programatically easier to use version of canon above, abstracted out.
//
void llref_get_canpath(llref* r, l3path* canpath) {
    assert(r);
    assert(canpath);
    LIVEREF(r);
    l3path& canonpath = *canpath;
    llref* llr = 0;
    llref* can = 0;
    l3obj* capt = r->_obj;
    LIVEO(capt);
    l3obj* prev_capt = 0;
    
    while(1) {
        assert(canonpath.len() < PATH_MAX-1);// sanity check catcher...

        llr = capt->_owner->find(capt);
        LIVEREF(llr);
        
        can = priority_ref(llr); // returns min priority above 1
        LIVEREF(can);
        canonpath.prepushf("%s.",can->_key);
        
        prev_capt = capt;
        capt = can->_obj->_owner->captain();
        LIVEO(capt);
        
        // don't get stuck in captags...definition: obj->_mytag->captain() == obj; see pred_is_captag(tag,cap) in l3obj.cpp
        // insufficient:        if (capt == can->_obj) {
        if (pred_is_captag(capt->_mytag, capt)
            || is_captag(capt)                  // check the bit
            || prev_capt == capt                // have we actually gone into a tight cycle?
            ) {
            // jump up to the captain of our parent tag

            // first make sure everything is kosher with going up the tag tree.
            LIVET(can->_obj->_owner);
            LIVET(can->_obj->_owner->parent());

            capt = can->_obj->_owner->parent()->captain();
            LIVEO(capt);
        }
        
        if (capt == main_env) break;
    }
    canonpath.rdel(1); // delete last .  (trailing dot)
}



//
// when re-writing an object according to it's references...
//   this moves all priority > 1 references to the ring of new_priority_one
//   and updates _obj to point to new_priority_one->_obj.
//
void llref_update_obj_of_ring_in_place(llref* r, llref* new_priority_one) {

  assert(r);
  LIVEREF(r);

#ifdef _JLIVECHECK
  // verify sanity before...
  llref_verify_chain(r, 100);
  llref_verify_chain(new_priority_one, 100);
#endif

  move_all_visible_llref_to_new_ring(r, new_priority_one);
  llref_update_obj(new_priority_one, new_priority_one->_obj);

#ifdef _JLIVECHECK
  // verify sanity after...
  llref_verify_chain(r, 100);
  llref_verify_chain(new_priority_one, 100);
#endif

}


long llref_pred_refcount_one(llref* r) {
    assert(r);
    assert(r->_next);
    if (r->_next != r) {
        return 0;
    }
    return 1;
}


// check for env in the ring, return the llref if the env is in the ring. (but not a priority 1 ref)
llref* query_for_env(llref* startingref, l3obj* target_env) {

    assert(startingref);
    assert(target_env);

    long N = llref_size(startingref);
    if (0==N) {
        printf("internal errror in query_for_env(): llref %p was empty; size zero!?!?\n",startingref);
        assert(0);
        exit(1);
    }
    llref* cur = startingref;
    
    // simple linear search
    for (long i =0 ; i < N; ++i, cur = cur->_next) {
        if (cur->_env == target_env && cur->_priority != 1) return cur;
    }
    
    return 0;
}

void conform_refowner_to_obj_owner(llref* adjustme) {
    assert(adjustme);
    long N = llref_size(adjustme);
    if (0==N) {
        printf("internal errror in conform_refowner_to_obj_owner(): llref %p was empty; size zero!?!?\n",adjustme);
        assert(0);
        exit(1);
    }
    llref* cur = adjustme;
    l3obj* obj = cur->_obj;
    Tag*   newown = obj->_owner;
    Tag*   oldown =  0;

    for (long i =0 ; i < N; ++i, cur = cur->_next) {
        oldown =  cur->_refowner;
        
        if (oldown != newown) {
            oldown->llr_remove(cur);
            cur->_refowner = newown; // this order is important, to avoid firing asserts in llr_remove and llr_push.
            newown->llr_push(cur);
        }

    }

}



long refcount_minimal(l3obj* o)
{
    LIVEO(o);
    return llref_pred_refcount_one(o->_owner->find(o));
}


void llref::update_key(char* newkey) {
    strncpy(_key, newkey,LLREF_KEY_LEN);
    _key[LLREF_KEY_LEN]='\0';
}

