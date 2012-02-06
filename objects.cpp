//
// Copyright (C) 2011 Jason E. Aten. All rights reserved.
//
#include "jtdd.h"
#include "l3obj.h"
#include "autotag.h"
#include "objects.h"
#include "terp.h"
#include "judydup.h"
#include "tostring.h"
#include "symvec.h"

#include "dynamicscope.h"


/////////////////////// end of includes

//
// obj : the Merlin generic object system.
//


// for all the ( char* -> llref* ) entries in obj->_judyS, ... call llref_del on the pointed to llref*.
//  Called by obj_dtor(), and possibly other dtors.
  L3METHOD_NOTAGCHECK(obj_llref_cleanup)
    LIVEO(obj);
    assert(obj);
    if (0==obj->_judyS) return 0;

    // ===========================
    // free all llref's for my aliases

    size_t       sz =0;
    PWord_t      PValue = 0;               // Judy array element.
    uint8_t      Index[BUFSIZ];            // string to sort.
    Index[0] = '\0';                       // start with smallest string.

    JSLF(PValue, obj->_judyS->_judyS, Index);        // get first string

    while (PValue != NULL)
    {
      llref* r = (llref*)(*PValue);

      llref_del(r,YES_DO_HASH_DELETE); // the key action of importance.

      JSLN(PValue, obj->_judyS->_judyS, Index);   // get next string
      ++sz;
    }

    // done with llref cleanup
    // ===========================

    // and cleanup the _judyS array.
    hash_dtor(L3STDARGS);

L3END(obj_llref_cleanup)


L3STUB(obj_ctor)

// dtor needs to know 3 things, like any class function call:
//
//   what function to run?
//   which object to apply that function to?
//   what arguments to run with?  (call is null/0 for dtor)
//
// universal_object_dispatcher does obj = fun to call (function defintion)
//                                  env = callobject with arguments to call with, can be 0 if no args.
// 
//  but wouldn't it make more sense for obj to be the current object, and curfo to be the function to call?
//    but since we are bluring the function/object dichotomy anyway, just pass it in there! rename curfo to curfo
//
//
//  let curfo = the calling env, in which the call takes place (and where to return any (return @<) argument values)).
//

L3METHOD(obj_dtor)
  LIVEO(obj);
  objstruct* os = (objstruct*)obj->_pdb;

  // call all dtor
  int ndtor = (long)(os->mem_dtor.size());

  l3obj* nextdtor = 0;
  for (int i = 0; i < ndtor;  ++i) {
    // leave env as 0 to indicate no arguments... how does the dtor know which instance
    //  it is being applied to?  that is passed in the curfo (current function or object) slot.
    nextdtor = os->mem_dtor[i];
    if (nextdtor) {

        // problem with the L3STDARGS_OBJONLY: passing in retval == 0 means eval will stop
        // without doing anything!?!?!!? That is why we give actual arguments here...
        //universal_object_dispatcher(nextdtor,L3STDARGS_OBJONLY);
        
      universal_object_dispatcher(nextdtor, /* this is the dtor function to call */
                                -1,0, 
                                env, /*env not a t_cal => no arguments to call*/  
                                retval,owner,
                                obj, /* curfo - this is the object instance to be deleted in the dtor case; e.g. 'this' in C++ */
                                0,
                                retown,ifp);

    }

    // hmm... what if there are many such objects... do they all get copies of the dtor??? or
    //  do we get to re-use the dtor code objects? for now, cp when created, and delete now.
    //  Delete the dtor function itself... since it will be on a parent or other ancestor tag.
    generic_delete(nextdtor, L3STDARGS_OBJONLY);
  }

  os->~objstruct();

L3END(obj_dtor)


L3METHOD(obj_cpctor)
    LIVEO(obj);
    DV(printf("obj_cpctor firing...\n"));

    l3obj* src  = obj;     // template
    l3obj* nobj = *retval; // write to this object
    assert(src);
    assert(nobj); // already allocated, named, _owner set, etc. See objects.cpp: deep_copy_obj()
    Tag* new_tag = (Tag*)(nobj->_mytag);
    
    assert(src->_type == t_obj);

    // objstruct
    objstruct* sos = (objstruct*)src->_pdb;
    objstruct* os  = (objstruct*)nobj->_pdb;
    os = new(os) objstruct(); // placement new

    l3path classnm(sos->classname);
    l3path objnm(0,"%s_cp",sos->objname);    
    *os = *sos; // use C++ copy constructor
    
    // objname will have gotten overwritten...fix that without inf growth...use ser#.
    objnm.reinit(os->objname);
    objnm.pushf("_%ld",src->_ser);
    objstruct_objname_set(os, objnm());

    // we must make our own copies of the dtors...b/c as of now these are only
    // shallow copies. so make them deep. 

    // And, remember the new rule for dtors (in fact the only way they can possibly do their job):
    //  *dtor must be owned by the parent of the tag that owns the object for which they will do the destructing*.
    long N = sos->mem_dtor.size();
    l3obj* cpme = 0;
    l3obj* newcp = 0;
    Tag* uptree_dtor_owner = new_tag->parent() ? new_tag->parent() : glob;

    for (long i = 0; i < N; ++i ) {
      cpme = sos->mem_dtor[i];

      // *retval should be zero to indicate that copy is not yet allocated.
      newcp = 0;
      deep_copy_obj(cpme,-1,0,env,&newcp,new_tag,0,0,uptree_dtor_owner,ifp);

      os->mem_dtor[i] = newcp;
    }


    // tag and obj already copied, not sure we need any of the rest of this...

#if 0    

    // storage allocation already done...now fill in _judyL and _judyS

    // from dump_owned Tag method
    Word_t * PValue = 0;
    Word_t Index = 0;
    llref* llr = 0;
    l3obj* o = 0;

    long i = 0;

    // can we do this all from nobj->_mytag, without referring to src? maybe.


    JLF(PValue, new_tag->_owned, Index);
    while (PValue != NULL)
        {
            o = (l3obj*)Index;       // key
            llr = *(llref**)PValue;  // value
            
            printf("obj_cpctor development: we have member object = %p ser# %ld '%s' ... and llref:\n",o,o->_ser, o->_varname);
            print_llref(llr);
            
            JLN(PValue, new_tag->_owned, Index);
            ++i;
        }
#endif    




L3END(obj_cpctor)

// owner_is_ancestor_of_retown : used by dtor_add
//
// returns 1 if owner is an ancestor of retown, otherwise 0.
L3METHOD(owner_is_ancestor_of_retown)
{
    assert(owner);
    assert(retown);
    if (owner == retown) return 0;

    Tag* top = glob;
    Tag* cur = retown;
    if (cur == top) return 0;

    while(1) {
        cur = cur->parent();

        if (0==cur) return 0;
        if (cur == owner) return 1;
        if (cur == top) return 0;
    }

}
L3END(owner_is_ancestor_of_retown)

// Problem with this adding to the parent tag: we don't actually get to cleanup from the parent, and we can't
//  modularly move ourselves around. Arg.
//
// dtor_add : we realize that we've got to add dtor's to the *parent* tag of the object for which they are to destruct.
//            this allows us to get the sequencing of dtor execution correct, and let it stay flexible and change if need be.
//
//
//
// (dtor_add dtor add_to_me)
//
//
L3METHOD(dtor_add)
{    
    arity = num_children(exp);
    if (arity < 1 || arity > 2) {
        printf("error: dtor_add needs the name of a function to add to the destructor list (and optional object to add to, defaulting to the cur env).\n");
        l3throw(XABORT_TO_TOPLEVEL);
    }

    qqchar new_dtor_name(ith_child(exp,0)->val());
    llref* innermostref = 0;

    l3obj* src_dtor = RESOLVE_REF(new_dtor_name,env,AUTO_DEREF_SYMBOLS,&innermostref,0,new_dtor_name,UNFOUND_RETURN_ZERO);

    qqchar obj_to_addto_name;
    l3obj* nobj_addtome = 0;

    if (0==src_dtor) {
        std::cout << "error: dtor_add could not locate function '"<< new_dtor_name << "' to add to dtor list.\n";
        l3throw(XABORT_TO_TOPLEVEL); 
    }
    LIVEO(src_dtor);
    LIVEO(env);
    llref* innermostref2 = 0;

    if (arity == 2) {
        obj_to_addto_name = ith_child(exp,1)->val(); 
        nobj_addtome = RESOLVE_REF(obj_to_addto_name,env,AUTO_DEREF_SYMBOLS,&innermostref2,0,obj_to_addto_name,UNFOUND_RETURN_ZERO);

    } else {
        if (env == 0) {
            printf("error: env was 0x0 in call to dtor_add: no object available to add to!\n");
            l3throw(XABORT_TO_TOPLEVEL);     
        }
        nobj_addtome = env;
        obj_to_addto_name = env->_varname;
    }

    if (0==nobj_addtome) {
        std::cout << "error: dtor_add could not locate object '" << obj_to_addto_name << "' to add on a dtor to.\n";
        l3throw(XABORT_TO_TOPLEVEL); 
    }
    LIVEO(nobj_addtome);

    if(src_dtor->_type != t_fun && src_dtor->_type != t_clo && src_dtor->_type != t_lda) {
        std::cout << "error: dtor_add must be given a function, closure, or lambda. '"
                  << new_dtor_name << "' is of type " << src_dtor->_type << ".\n";
        l3throw(XABORT_TO_TOPLEVEL);
    }

    // dtor will get zero arguments, and run in the env of the object they are attached to.
    assert(src_dtor->_type == t_fun); // other types, t_clo and t_lda not implemented yet.

    defun* df = (defun*)src_dtor->_pdb;
    assert(df);
    if (df->nprops != 0) {
        printf("error in dtor_add: defun::nprops !=0 for candidate destructor function '%s'. Destructor functions can take no arguments and return no return values.\n",
               src_dtor->_varname);
        l3throw(XABORT_TO_TOPLEVEL);
    }
    
    l3obj* what_to_actually_add = src_dtor;

    // Later comment: this cannot work, since objects/functions may get moved around.
    //
    // start the copy and assign to parent sequence (dtors must be owned by the parent tag of the tag in which they 
    //  are called, so that they don't get destroyed before the objects that they carry out the destruction for.
    assert(nobj_addtome->_mytag);
    assert(nobj_addtome->_owner);

    // new: try to change the uptree owner back to owner, and see what if anything breaks:
#if 1
    Tag* uptree_dtor_owner = nobj_addtome->_owner;

#else
    Tag* uptree_dtor_owner = nobj_addtome->_owner->parent();
    if (0==uptree_dtor_owner || uptree_dtor_owner == nobj_addtome->_owner) {
        printf("internal error: unsupported at the moment: can't do dtor_add to object "
              "without owner parent or owned by the topmost level. Abort "
              "adding {(%p ser# %ld '%s') or copy thereof} to (%p ser# %ld '%s').\n",

              what_to_actually_add,
              what_to_actually_add->_ser,
              what_to_actually_add->_varname,

              nobj_addtome,
              nobj_addtome->_ser,
              nobj_addtome->_varname
              );
        l3throw(XABORT_TO_TOPLEVEL);
    }
#endif

    assert(uptree_dtor_owner);

    // we share the dtor only if it is marked undeletable.
    if (is_undeletable(src_dtor)) {
        assert(what_to_actually_add == src_dtor); // just leave it the same as above.

    } else {
        // this give a copy to owner's parent is problematic, no? what if we are moved?
        // new analysis: if deletable, then always make a copy, and give ownership to nobj_addtome->_owner->_parent

        what_to_actually_add = 0;

        DV(printf("uptree_dtor_owner is: \n"));
        DV(p(uptree_dtor_owner));

        deep_copy_obj(src_dtor,-1,0,env,&what_to_actually_add,nobj_addtome->_mytag,0,0,uptree_dtor_owner,ifp);
        printf("dtor_add: making a copy (got back [%p ser# %ld]) of the dtor method [%p ser# %ld] that the object [%p ser# %ld] will own.\n",
               what_to_actually_add,
               what_to_actually_add->_ser,
               src_dtor, 
               src_dtor->_ser, 
               nobj_addtome,
               nobj_addtome->_ser
               );
    }


    objstruct* os = (objstruct*)nobj_addtome->_pdb;
    assert(os);
    os->mem_dtor.push_back(what_to_actually_add);
}
L3END(dtor_add)



L3METHOD(obj_trybody)

  assert(obj);
  assert(obj->_type == t_obj);
  objstruct* os = (objstruct*)obj->_pdb;

  // call all body methods
  int nbody = (long)(os->mem_body.size());

  for (int i = 0; i < nbody;  ++i) {
    os->mem_body[i].method(L3STDARGS);
  }

//  return res;

L3END(obj_trybody)


void obj_get_obj_class(l3obj* obj, label** po, label** pc) {

    assert(obj->_type == t_obj);
    objstruct* os = (objstruct*)obj->_pdb;

    *po = &(os->objname);
    *pc = &(os->classname);
}

void dump_objstruct(objstruct* os, const char* indent, stopset* stoppers) {

  l3path indent_more(indent);
  indent_more.pushf("%s","    ");

  //  printf("%s(objstruct.classname: '%s'  objstruct.objname: '%s' )\n", indent, os->classname, os->objname);
  printf("%s(obj %s::%s)\n", indent, os->classname, os->objname);

  l3path details;
  for (uint i = 0; i < os->mem_dtor.size(); ++i) {

      details.clear();
      if (gUglyDetails > 0) {
          details.pushf("%p (ser# %ld)",os->mem_dtor[i], os->mem_dtor[i]->_ser);
      }
      printf("%smem_dtor[%02d] = %s\n",indent_more(), i, details());
      print(os->mem_dtor[i],indent_more(),0);
  }
}

  /*
  assert(a);

  printf("%snarg: %ld\n",indent, a->narg);
  
  for (uint i = 0; i < a->arg_key.size(); ++i) {
    printf("%sarg_key[%d] = '%s'\n",indent, i, a->arg_key[i]);
  }

  for (uint i = 0; i < a->arg_txt.size(); ++i) {
    printf("%sarg_txt[%d] = '%s'\n",indent, i, a->arg_txt[i]);
  }

  for (uint i = 0; i < a->arg_typ.size(); ++i) {
    printf("%sarg_typ[%d] = '%s'\n",indent, i, a->arg_typ[i]);
  }

  l3path indent_more(indent);
  indent_more.pushf("%s","    ");

  for (uint i = 0; i < a->arg_val.size(); ++i) {
    printf("%sarg_val[%d] = \n",indent, i);
    print(a->arg_val[i],indent_more());
  }

  DV(
  printf("%sorig_call_malloced = '%s'\n",indent, a->orig_call_malloced);
  printf("%sorig_call_sxp = '%s'\n",indent, stringify(a->orig_call_sxp));
  printf("%ssrcfile_malloced = '%s'\n",indent, a->srcfile_malloced);
  printf("%sline = %ld\n",indent, a->line);
     );
  */




void  print_obj(l3obj* obj,const char* indent, stopset* stoppers) {
  if (!obj) return;
  assert(obj->_type == t_obj);

  l3path indent_more(indent);
  indent_more.pushf("%s","    ");

  l3path momo(0,"%s          ",indent_more());

  if (gUglyDetails > 0) {
      printf("%p ser# %ld -> ",obj,obj->_ser);
  }

  DV(
     if (obj && obj->_mytag) {  
         printf(" [_mytag: %p ser# %ld '%s'] ",obj->_mytag, obj->_mytag->sn(), obj->_mytag->myname()); 
     } else {
         printf(" [_mytag: 0x0] ");
     }
     );

  objstruct* os = (objstruct*)obj->_pdb;
  dump_objstruct(os, indent, stoppers);

  dump_hash(obj,indent_more(),stoppers);

  if (obj->_sexp) {
      l3path val;
      if (obj->_sexp->_val.len()) {
          val.reinit(obj->_sexp->_val);
          val.prepushf(" val: '");
          val.pushf("', at root of expression:");
      }

      printf("%s_sexp -> %s\n", indent_more(),val());
      obj->_sexp->printspan(0,momo());
  }
}

void objstruct_objname_set(objstruct* os, const char* objname) {
    if (strlen(objname) >= sizeof(label)) {
        printf("error: objname '%s' exceeds length %ld of available space in objstruct.objname.\n",objname,sizeof(label));
        l3throw(XABORT_TO_TOPLEVEL);
    }
    strncpy(os->objname,   objname, sizeof(label));
}

void objstruct_classname_set(objstruct* os, const char* classname) {

    if (strlen(classname) >= sizeof(label)) {
        printf("error: classname '%s' exceeds length %ld of available space in objstruct.classname.\n",classname,sizeof(label));
        l3throw(XABORT_TO_TOPLEVEL);
    }
    strncpy(os->classname,   classname, sizeof(label));
}


//
// do_obj_init : called by make_new_captag(...etyp=t_obj) and by make_new_obj() below.
//
void do_obj_init(l3obj* obj, const char* objname, const char* classname) {
    LIVEO(obj);

    assert(obj->_malloc_size == sizeof(l3obj) + sizeof(objstruct));

    obj->_type = t_obj;
    objstruct* os = (objstruct*)obj->_pdb;
    os = new(os) objstruct(); 

    objstruct_classname_set(os, classname);
    objstruct_objname_set(os, objname);

    obj->_ctor    = &obj_ctor;
    obj->_trybody = &obj_trybody;
    obj->_dtor    = &obj_dtor;
    obj->_cpctor  = &obj_cpctor;

#if 0 // I think this is old and wrong     
    if (global_env_stack.size()) {
        obj->_parent_env  = global_env_stack.front();
    }
#endif

}


// if forward_tag_to_me == 0 then we create and
//  use our own tag.
void make_new_obj( const char* classname, 
                           const char* objname, 
                           Tag* owner, 
                           Tag* forward_tag_to_me, 
                           l3obj** retval)
{
         l3obj* obj = make_new_class(sizeof(objstruct), owner,classname,objname);

         if (forward_tag_to_me) {

           set_forwarded_tag(obj);
           obj->_mytag = forward_tag_to_me;

         } else {

           l3path obj_tag_name(0,"%s_objTag",objname);


           obj->_mytag = new Tag("make_new_obj___objects.cpp", owner, obj_tag_name(),obj);
         }

         do_obj_init(obj,objname,classname);

         *retval = obj;

}

// deep_copy_obj ... this is really like a base class copy_ctor, that
//  once it does some universal work, calls the type-specific cpctor.
//
// assume: _mytag is already allocated and available in owner
//  therefore: no need to create any new Tag.
//
// obj = object to copy
// exp  = ignored.
// arity = ignored.
// retown = tag that will own the new copy
// owner  = what to set as _mytag
// *retval = if non-zero, then the obj already allocated for us (as in for a captag pair) : can also already have some members in _judyS
L3METHOD(deep_copy_obj)
          LIVEO(obj);

          l3obj* src = obj;
          l3path classnm;
          l3path objnm(0,"deep_copy_of_%s",src->_varname);
          objstruct* sos = 0;

          if (src->_type==t_obj) {
               sos = (objstruct*)src->_pdb;
               classnm.init(sos->classname);
               objnm.reinit("%s_cp",sos->objname);
          }

          l3obj* nobj = 0;

          // _mytag 
          if (*retval) {
              // captain obj already pre-allocated for us
              nobj = *retval;
              assert(nobj->_mytag == owner);
          } else {

              // we've got to make a new object to be the copy. So--
              // 1) do we make a captag?
              if (pred_is_captag(src->_owner, src)) {

                  make_new_captag((l3obj*)&objnm ,src->_malloc_size - sizeof(l3obj),exp,env, &nobj, retown, (l3obj*)&classnm, src->_type, retown,ifp);

              } else {
                  // 2) otherwise we call make_new_class and forward the mytag, if that's what our template did.
                  // 3) we can also leave the _mytag = 0, if thats what our template has.

                     nobj = make_new_class(src->_malloc_size - sizeof(l3obj), retown, classnm(), objnm());
                     *retval = nobj;
                     nobj->_type = src->_type; // _type
                  
                     if (src->_mytag == 0) {
                         assert(nobj->_mytag == 0);
                     } 
                     /*
                     else {
                         if (is_forwarded(src)) {
                             // okay, if we set_forwarded(nobj), who do we forward _mytag to ?

                         } else {
                             assert(0);
                         }
                     }
                     */
              }

          }


          // _malloc_size already taken care of
          
          // _reserved
          nobj->_reserved = src->_reserved;

          // need to have a judys here, but cannot just dumb copy all the llref.
          // no! already filled in!! don't overwrite here:         nobj->_judyS = new judys_llref;

          // but do some sanity checks that _judyS is good.
          assert(nobj->_judyS);
#ifndef  _DMALLOC_OFF
          long ser_judys = 0;
          BOOL found_newed_up_judys = gsermon->found_sermon_number(nobj->_judyS, &ser_judys);
          assert(found_newed_up_judys);
#endif

#if 0
         // _judyS
         copy_judySL(src->_judyS, &(nobj->_judyS));

         // _judyL
         copy_judyL(src->_judyL, &(nobj->_judyL));
#endif

          // methods... these point to compiled in functions, but in
          // obj_dtor for example will reference stuff from src  objstruct, which must be copied...
          nobj->_dtor    = src->_dtor;
          nobj->_trybody = src->_trybody;
          nobj->_ctor    = src->_ctor;

          nobj->_cpctor    = src->_cpctor;

          nobj->_parent_env = 0; // can't assume that this will still be around, right: src->_parent_env;
          nobj->_myname_livesin_env  = src->_myname_livesin_env;

          nobj->_parent     = src->_parent;
          // or: nobj->_parent  = env; // not sure, do this?

          nobj->_child  = src->_child;
          nobj->_sib    = src->_sib;


          // these arern't handled/used/implemented at the moment
          // _pme and _pmb
          nobj->_pme = 0; assert(0 == src->_pme);
          nobj->_pmb = 0; assert(0 == src->_pmb);

          // call the type specific cpctor
          src->_cpctor(src, arity,exp, env, &nobj, owner,curfo,etyp,retown,ifp);

          *retval = nobj;
L3END(deep_copy_obj)


// hold a bunch of values, as primitive as possible, down to data objects, or closures/objects as data instead of as immediate invocations
//   i.e. eval all the stuff in a combination, and then let our caller decide what to do with it.
//  return an object that owns all of the values in its own tag.
//
L3METHOD(make_new_ptrvec)
{

     l3path clsname("ptrvec");
     l3path objname("ptrvec");
     l3obj* nobj = 0;
     make_new_captag((l3obj*)&objname,0,exp,   env,&nobj,owner,   (l3obj*)&clsname,t_cap,retown,ifp);

     nobj->_type = t_vvc;
     nobj->_parent_env  = env;

     *retval = nobj;

}
L3END(make_new_ptrvec)



void ptrvec_pushback(l3obj* ptrvec, l3obj* addme) {
     assert(ptrvec);

   long N = ptrvec_size(ptrvec);
   ptrvec_set(ptrvec,N,addme);
   assert((N+1)==ptrvec_size(ptrvec));
}


long ptrvec_size(l3obj* ptrvec) {
     assert(ptrvec);

     Word_t    array_size;
     JLC(array_size, ptrvec->_judyL, 0, -1);

     return (long)array_size;
}


void ptrvec_get(l3obj* ptrvec, long i, l3obj** ret) {
     assert(ptrvec);
     assert(i>=0);

  PWord_t   PValue = 0;
  JLG(PValue,ptrvec->_judyL,i);
  if (!PValue) {
      printf("bad index request to vector: index %ld not present.\n",i);
      l3throw(XABORT_TO_TOPLEVEL);
  } else {
      *ret = (l3obj*) (*PValue);
  }
}


L3METHOD(aref)
{
   arity = 2; // number of args, not including the operator pos.
   k_arg_op(L3STDARGS); // *retval has our t_vvc
   l3obj* vv = *retval; // copy this, so we can be sure to delete it when done.
   *retval = 0; // in case we screw up...we don't want to return the array for sure.

   l3obj* arr = 0;
   l3obj* whichindex = 0;

   double  dindex   = -1;
   double  dres     = 0;
   long    lindex   = -1;
   long    lenavail = 0;

   XTRY
       case XCODE:
       ptrvec_get(vv, 0, &arr);

       if (arr->_type == t_syv) {
           symvec_aref(L3STDARGS);
           break; // cleanup vv in finally block.
       }

       ptrvec_get(vv, 1, &whichindex);

       if (whichindex->_type != t_dou) {
           printf("error in aref: index was not numeric.\n");
           l3throw(XABORT_TO_TOPLEVEL);
       }
       dindex = double_get(whichindex,0);
       if (isnan(dindex) || isinf(dindex) || dindex < 0 ) {
           printf("error in aref: bad index value: %f\n",dindex);
           l3throw(XABORT_TO_TOPLEVEL);
       }
       lindex = (long)dindex;
       if (arr->_type == t_vvc) {
           lenavail = ptrvec_size(arr);
           if (lindex > (lenavail-1)) {
               printf("error in aref: index to pos %ld is out of bounds for array of size %ld.\n",lindex, lenavail);
               l3throw(XABORT_TO_TOPLEVEL);
           }
           // replace retval... okay because we saved the stuff to delete in vv.
           ptrvec_get(arr, lindex, retval);

       } else if (arr->_type == t_dou) {
           lenavail = double_size(arr);
           if (lindex > (lenavail-1)) {
               printf("error in aref: index to pos %ld is out of bounds for array of size %ld.\n",lindex, lenavail);
               l3throw(XABORT_TO_TOPLEVEL);
           }
           dres = double_get(arr, lindex);
           *retval = make_new_double_obj(dres,retown,"aref_output");
       }
       
       break;
      case XFINALLY:
          if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); vv = 0; }
       break;
   XENDX

}
L3END(aref)



L3METHOD(open_square_bracket)
{
   arity = 1; // number of args, not including the operator pos.
   k_arg_op(L3STDARGS); // *retval has our t_vvc
   l3obj* vv = *retval; // copy this, so we can be sure to delete it when done.
   *retval = 0; // in case we screw up...we don't want to return the array for sure.

   l3obj* arr = 0;
   l3obj* whichindex = 0;

   double  dindex   = -1;
   double  dres     = 0;
   long    lindex   = -1;
   long    lenavail = 0;


   XTRY
       case XCODE:

           if(!(exp->_headnode)) {
               printf("error in open_square_bracket: no array reference specified in headnode field.\n");
               l3throw(XABORT_TO_TOPLEVEL);
           }

       eval(0, -1, exp->_headnode,  env,&arr,owner, 0, etyp, owner,ifp);

       if (!arr) {
           l3path hd(exp->_headnode);
           printf("error in open_square_bracket: array reference '%s' not found.\n",hd());
           l3throw(XABORT_TO_TOPLEVEL);
       }

       if (arr->_type == t_syv) {
           symvec_aref(L3STDARGS);
           break; // cleanup vv in finally block.
       }

       ptrvec_get(vv, 0, &whichindex);

       if (whichindex->_type != t_dou) {
           printf("error in aref: index was not numeric.\n");
           l3throw(XABORT_TO_TOPLEVEL);
       }
       dindex = double_get(whichindex,0);
       if (isnan(dindex) || isinf(dindex) || dindex < 0 ) {
           printf("error in aref: bad index value: %f\n",dindex);
           l3throw(XABORT_TO_TOPLEVEL);
       }
       lindex = (long)dindex;
       if (arr->_type == t_vvc) {
           lenavail = ptrvec_size(arr);
           if (lindex > (lenavail-1)) {
               printf("error in aref: index to pos %ld is out of bounds for array of size %ld.\n",lindex, lenavail);
               l3throw(XABORT_TO_TOPLEVEL);
           }
           // replace retval... okay because we saved the stuff to delete in vv.
           ptrvec_get(arr, lindex, retval);

       } else if (arr->_type == t_dou) {
           lenavail = double_size(arr);
           if (lindex > (lenavail-1)) {
               printf("error in aref: index to pos %ld is out of bounds for array of size %ld.\n",lindex, lenavail);
               l3throw(XABORT_TO_TOPLEVEL);
           }
           dres = double_get(arr, lindex);
           *retval = make_new_double_obj(dres,retown,"aref_output");
       }
       
       break;
      case XFINALLY:
          if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); vv = 0; }
       break;
   XENDX

}
L3END(open_square_bracket)



void ptrvec_set(l3obj* ptrvec, long i, l3obj*  val_to_set) {
     assert(ptrvec);
     assert(i>=0);

    l3obj**   po = 0;
    PWord_t   PValue = 0;
    JLI(PValue, ptrvec->_judyL, i);

    po = (l3obj**)(PValue);
    *po = val_to_set;
}



void ptrvec_print(l3obj* ptrvec, const char* indent, stopset* stoppers) {
     assert(ptrvec);
  
     long N = ptrvec_size(ptrvec);
  
     printf("%s%p : (ser# %ld ptrvec of size %ld): \n", indent, ptrvec, ptrvec->_ser, N);
  
     l3path more_indent(indent);
     more_indent.pushf("%s","     ");


       Word_t * PValue;                    // pointer to array element value
       Word_t Index = 0;
      
       JLF(PValue, ptrvec->_judyL, Index);
       while (PValue != NULL)
       {
           //           printf("%sptrvec content [%02lu] =  '%p' ", indent, Index, (l3obj*)(*PValue));
           printf("%sptrvec[%02lu]=", indent, Index); //, (l3obj*)(*PValue));
           l3obj* ptr = (l3obj*)(*PValue);
           // print(p,more_indent());
           if (obj_in_stopset(stoppers,ptr)) {
             printf("%p : ser# %ld  type %s (stopper)\n",ptr,ptr->_ser, ptr->_type);
           } else {
             print(ptr,more_indent(),stoppers);
           }
           JLN(PValue, ptrvec->_judyL, Index);
       }


}

void ptrvec_clear(l3obj* ptrvec) {
  assert(ptrvec);

   long  Rc = 0;

   if (ptrvec->_judyL) {
       JLFA(Rc,  (ptrvec->_judyL));
   }

   assert(ptrvec_size(ptrvec)==0);
}


// simple linear search
bool ptrvec_search(l3obj* ptrvec, l3obj* needle) {

       Word_t * PValue;                    // pointer to array element value
       Word_t Index = 0;
      
       JLF(PValue, ptrvec->_judyL, Index);
       while (PValue != NULL)
       {
           l3obj* ptr = (l3obj*)(*PValue);
           if (ptr == needle) return true;
           JLN(PValue, ptrvec->_judyL, Index);
       }

  return false;
}


// simple linear search for a double needle in a varying haystack.
//
bool ptrvec_search_double(l3obj* ptrvec, l3obj* double_needle) {
       assert(double_needle->_type == t_dou);

       if (1 != double_size(double_needle)) return false; // one double can never be equal to a vector, whatever the contents.

       double dneedle = double_get(double_needle,0);
       if (isnan(dneedle)) return false; // NAN never matches anything, even NAN.

       // search through the array
       Word_t * PValue;                    // pointer to array element value
       Word_t Index = 0;
       double dhay = 0;

       JLF(PValue, ptrvec->_judyL, Index);
       while (PValue != NULL)
       {
           l3obj* ptr = (l3obj*)(*PValue);
           if (ptr == double_needle) return true;

           if (ptr->_type == t_dou) {
               if (1==double_size(ptr)) {
                   dhay = double_get(ptr,0);
                   if (feqs(dneedle,dhay,1e-6)) return true;                   
               }
           }
           
           JLN(PValue, ptrvec->_judyL, Index);
       }

  return false;
}



// operations that set and query _parent, _child, _sib

L3METHOD(firstchild)
    left(L3STDARGS);
L3END(firstchild)

// (left obj) get left child of obj
// a.k.a. (firstchild obj)
L3METHOD(left)

   arity = 1; // number of args, not including the operator pos.
   k_arg_op(L3STDARGS);
   l3obj* vv = *retval;
   qqchar opname(first_child(exp)->val());

   l3obj* queryme = 0;

   XTRY
       case XCODE:
   
   ptrvec_get(vv,0,&queryme);
   if (0==queryme) {
      std::cout << "error: could not locate subject of "<< opname << ".\n";
      l3throw(XABORT_TO_TOPLEVEL);
   }

   if (queryme->_child) {
       *retval = queryme->_child->chase();
   } else {
       *retval = gnil;
   }

    break;
       case XFINALLY:
            generic_delete(vv, L3STDARGS_OBJONLY);   
       break;
   XENDX


L3END(left)


L3METHOD(right)

   arity = 1; // number of args, not including the operator pos.
   k_arg_op(L3STDARGS);
   l3obj* vv = *retval;
   qqchar opname(first_child(exp)->val());

   l3obj* queryme = 0;
   XTRY
       case XCODE:

   ptrvec_get(vv,0,&queryme);
   if (0==queryme) {
      std::cout << "error: could not locate subject of "<< opname << ".\n";
      l3throw(XABORT_TO_TOPLEVEL);
   }

   if (0 == queryme->_child) {
       *retval = gnil;
   } else {
       if (0 == queryme->_child->chase()->_sib->chase()) {
       *retval = gnil;
     } else {
           *retval = queryme->_child->chase()->_sib->chase();
     }
   }

    break;
       case XFINALLY:
            generic_delete(vv, L3STDARGS_OBJONLY);   
       break;
   XENDX

L3END(right)


L3METHOD(parent)

   arity = 1; // number of args, not including the operator pos.
   k_arg_op(L3STDARGS);
   l3obj* vv = *retval;
   qqchar opname(first_child(exp)->val());

   l3obj* queryme = 0;
   XTRY
       case XCODE:

   ptrvec_get(vv,0,&queryme);
   if (0==queryme) {
      std::cout << "error: could not locate subject of "<< opname << ".\n";
      l3throw(XABORT_TO_TOPLEVEL);
   }

   if (queryme->_parent) {
       *retval = queryme->_parent->chase();
   } else {
       *retval = gnil;
   }

    break;
       case XFINALLY:
            generic_delete(vv, L3STDARGS_OBJONLY);   
       break;
   XENDX

L3END(parent)


//
// (ischild parent child) -> T
//
L3METHOD(ischild)
{
    l3path sexp(exp);
    arity = 2; // number of args, not including the operator pos.
    k_arg_op(L3STDARGS);
    l3obj* vv = *retval;
    
    l3obj* par = 0;
    l3obj* chld = 0;
    l3obj* firstchild = 0;
    l3obj* nextchild = 0;

    XTRY
 case XCODE:
    
    ptrvec_get(vv,0,&par);
    if (0==par) {
        printf("error: could not locate candidate parent--1st arg--in '%s'.\n", sexp());
        l3throw(XABORT_TO_TOPLEVEL);
    }
    
    ptrvec_get(vv,1,&chld);
    if (0==chld) {
        printf("error: could not locate candidate child--2nd arg--in '%s'.\n", sexp());
        l3throw(XABORT_TO_TOPLEVEL);
    }
    
    firstchild = par->child();
    assert(firstchild);
    if (gnil == firstchild) {
        *retval = gnil;
        break;
    } 
    
    if (firstchild == chld) {
        *retval = gtrue;
        break;
    }

    nextchild = firstchild->sib();
    assert(nextchild);
    while(1) {
        if (nextchild == gnil) { *retval = gnil; break; }
        if (nextchild == chld) { *retval = gtrue; break; }
        nextchild = nextchild->sib();
    }

    break;
       case XFINALLY:
            generic_delete(vv, L3STDARGS_OBJONLY);   
       break;
   XENDX

}
L3END(ischild)


L3METHOD(sib)

   arity = 1; // number of args, not including the operator pos.
   k_arg_op(L3STDARGS);
   l3obj* vv = *retval;
   qqchar opname(first_child(exp)->val());

   l3obj* queryme = 0;
   XTRY
       case XCODE:

   ptrvec_get(vv,0,&queryme);
   if (0==queryme) {
      std::cout << "error: could not locate subject of "<< opname << ".\n";
      l3throw(XABORT_TO_TOPLEVEL);
   }

   if (queryme->_sib) {
       *retval = queryme->_sib->chase();
   } else {
       *retval = gnil;
   }

    break;
       case XFINALLY:
            generic_delete(vv, L3STDARGS_OBJONLY);   
       break;
   XENDX

L3END(sib)


L3METHOD(numchild)
{
    l3path sexp;
    l3obj* vv = 0;
    qqchar opname;
    long count = 0;
    long maxcheck = 300;
    l3obj* par = 0;
    
    if (exp==0) {
        if (0==obj) {
           printf("error: obj not set in numchild() and exp not provided. Bad numchild request.\n");
           l3throw(XABORT_TO_TOPLEVEL);
        }
        par = obj;
    } else {
        sexp.reinit(exp);
        arity = 1; // number of args, not including the operator pos.
        k_arg_op(L3STDARGS);
        vv = *retval;
        opname = first_child(exp)->val();
    }

   XTRY
       case XCODE:

    if (0==par) {
        ptrvec_get(vv,0,&par);
        if (0==par) {
            printf("error in numchild: could not locate object to count children of in '%s'.\n", sexp());
            l3throw(XABORT_TO_TOPLEVEL);
        }
    }
    assert(par);

    if (par->child() != gnil) {
        ++count;

        l3obj* cur = par->child()->sib();
        while(cur != gnil) {
            ++count;
            cur = cur->sib();
            if (count > maxcheck) {
                printf("error: inf loop detected (> %ld iterations) in numchild during eval of '%s'.\n", 
                       maxcheck, sexp() ? sexp() : "");
                l3throw(XABORT_TO_TOPLEVEL);
            }
        }
    }

   *retval = make_new_double_obj((double)count,retown,"numchild");

    break;
       case XFINALLY:
           if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); }
       break;
   XENDX

}
L3END(numchild)


L3METHOD(ith_child)
   {
     arity = 2; // number of args, not including the operator pos.
     k_arg_op(L3STDARGS);
     l3obj* vv = *retval;
     qqchar opname(first_child(exp)->val());

     l3obj* queryme = 0;
     long num = 0;
     l3obj* cur = 0;
     double dval = 0;
     long i = 0;
     l3obj* ith = 0;

     XTRY
   case XCODE:

     ptrvec_get(vv,0,&queryme);
     if (0==queryme) {
       std::cout << "error: could not locate subject of "<< opname << ".\n";
       l3throw(XABORT_TO_TOPLEVEL);
     }

     ptrvec_get(vv,1,&ith);
     if (0==ith || ith->_type != t_dou) {
       std::cout << "error: bad ith request in "<< opname << " (unspecified, or not a number).\n";
       l3throw(XABORT_TO_TOPLEVEL);
     }

     dval = double_get(ith,0);
     i = (long)dval;

     if (i==0) {
       if (queryme->_child) {
           *retval = queryme->_child->chase();
       } else {
         *retval = gnil;
       }
       break;
     }

     if (0==queryme->_child) {
       *retval = gnil;
       break;
     }

     // INVAR: we have at least one child, and i > 0, and queryme->_child != 0;

     cur = queryme->_child->chase();

     while(num < i && cur && !(cur==gnil)) {
       ++num;
       cur=cur->_sib->chase();
     }

     if (cur) {
       *retval = cur;
     } else {
       *retval = gnil;
     }

     break;
   case XFINALLY:
     generic_delete(vv, L3STDARGS_OBJONLY);   
     break;
     XENDX
       }
L3END(ith_child)

///////////////// setters:

// (left_is  parent  leftchild)
L3METHOD(set_obj_left)
{
    l3obj* vv = 0;
    qqchar opname;
    l3obj* par = 0;
    l3obj* chld = 0;
    l3path sexp;

    if (exp==0) {
        // programmatically called...
        par = curfo;
        chld = obj;
    } else {
        sexp.reinit(exp);
        arity = 2; // number of args, not including the operator pos.
        k_arg_op(L3STDARGS);
        vv = *retval;
        opname = first_child(exp)->val();
    }

   XTRY
       case XCODE:
        if (!par) {
            ptrvec_get(vv,0,&par);
            if (0==par) {
                printf("error in set_obj_left: in expression '%s', could not locate parent to add to.\n", sexp());
                l3throw(XABORT_TO_TOPLEVEL);
            }

            assert(0==chld);
            ptrvec_get(vv,1,&chld);
            if (0==chld) {
                printf("error in set_obj_left: could not find child to set in '%s'.\n", sexp());
                l3throw(XABORT_TO_TOPLEVEL);
            }
        }

        par->child(chld);
        *retval = par;

    break;
       case XFINALLY:
           if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); }
       break;
   XENDX

}
L3END(set_obj_left)

// (right_is  parent  rightchild)
L3METHOD(set_obj_right)

    l3obj* vv = 0;
    qqchar opname;
    l3obj* par = 0;
    l3obj* chld = 0;
    l3path sexp;

    if (exp==0) {
        // programmatically called...
        par = curfo;
        chld = obj;
    } else {
        sexp.reinit(exp);
        arity = 2; // number of args, not including the operator pos.
        k_arg_op(L3STDARGS);
        vv = *retval;
        opname = first_child(exp)->val();
    }

   XTRY
       case XCODE:
          if (!par) {
              ptrvec_get(vv,0,&par);
              if (0==par) {
                  std::cout << "error: could not locate subject of "<< opname << ".\n";
                  l3throw(XABORT_TO_TOPLEVEL);
              }


              ptrvec_get(vv,1,&chld);
              if (0==chld) {
                  std::cout << "error: could not find reference to point to in "<< opname << ".\n";
                  l3throw(XABORT_TO_TOPLEVEL);
              }
              
              if (gnil == par->child()) {
                  std::cout << "error in "<< opname << ": cannot set right child when there is no left child.\n";
                  l3throw(XABORT_TO_TOPLEVEL);
              }
          }

          par->child()->sib(chld);
          *retval = par;

    break;
       case XFINALLY:
           if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); }
       break;
   XENDX

L3END(set_obj_right)

// (parent_is  onobj  new_ref)
L3METHOD(set_obj_parent)

    l3obj* vv = 0;
    qqchar opname;
    l3obj* par = 0;
    l3obj* chld = 0;
    l3path sexp;

    if (exp==0) {
        // programmatically called...
        par  = obj;
        chld = curfo;
    } else {
        sexp.reinit(exp);
        arity = 2; // number of args, not including the operator pos.
        k_arg_op(L3STDARGS);
        vv = *retval;
        opname = first_child(exp)->val();
    }

   XTRY
       case XCODE:
          if (!par) {
              ptrvec_get(vv,0,&chld);
              if (0==chld) {
                  std::cout << "error: could not locate subject of "<< opname << ".\n";
                  l3throw(XABORT_TO_TOPLEVEL);
              }

              ptrvec_get(vv,1,&par);
              if (0==par) {
                  std::cout << "error: could not new parent in "<< opname << ".\n";
                  l3throw(XABORT_TO_TOPLEVEL);
              }
          }

           //not this: //par->child(chld);
           // but this:
           chld->parent(par);
           *retval = chld;

    break;
       case XFINALLY:
           if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); }
       break;
   XENDX

L3END(set_obj_parent)

L3METHOD(set_obj_sib)

    l3obj* vv = 0;
    qqchar opname;
    l3obj* par = 0;
    l3obj* newsib = 0;
    l3path sexp;

    if (exp==0) {
        // programmatically called...
        par = curfo;
        newsib = obj;
    } else {
        sexp.reinit(exp);
        arity = 2; // number of args, not including the operator pos.
        k_arg_op(L3STDARGS);
        vv = *retval;
        opname = first_child(exp)->val();
    }

   XTRY
       case XCODE:
          if (!par) {
              ptrvec_get(vv,0,&par);
              if (0==par) {
                  std::cout << "error: could not locate subject of "<< opname << ".\n";
                  l3throw(XABORT_TO_TOPLEVEL);
              }

              ptrvec_get(vv,1,&newsib);
              if (0==newsib) {
                  std::cout << "error: could not find reference to point to in "<< opname << ".\n";
                  l3throw(XABORT_TO_TOPLEVEL);
              }
          }

          par->sib(newsib);
          *retval = par;

    break;
       case XFINALLY:
           if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); }
       break;
   XENDX


L3END(set_obj_sib)



L3METHOD(lastchild) 

   l3obj* vv  = 0;
   l3path sexp(exp);
   l3obj* par = 0;
   long num_args_required = 1;

   if (exp == 0) {
       // return lastchild of obj
       if (!obj) {
           printf("error: obj not set in lastchild() and exp not provided. Bad lastchild request.\n");
           l3throw(XABORT_TO_TOPLEVEL);
       } else {
           par = obj;
       }
   } else {
       k_arg_op(0,num_args_required,exp,env,&vv,owner,curfo,etyp,retown,ifp);
       // do the rest inside try block, so we can release vv no matter what.
       par = 0;
   }

    l3obj* firstchild = 0;
    l3obj* nextchild = 0;
    l3obj* prevchild = 0;
    int   i = 0;
    int   maxcheck = 300; //detect inf loops/self-cycles.
    *retval = gnil; // default

    XTRY
 case XCODE:

    if (!par) {
       ptrvec_get(vv,0,&par);
       if (0==par) {
           printf("error: could not locate parent to check lastchild of in '%s'.\n", sexp());
           l3throw(XABORT_TO_TOPLEVEL);
       }
    }
        
    firstchild = par->child();
    assert(firstchild);
    if (gnil == firstchild) {
        *retval = gnil;
        break;
    } 
    
    if (firstchild->sib() == gnil) {
        *retval = firstchild;
        break;
    }

    nextchild = firstchild->sib();
    if (nextchild == gnil) {
        *retval = firstchild;
        break;
    }
    
    prevchild = nextchild;
    while(1) {
        if (nextchild == gnil) { *retval = prevchild; break; }
        prevchild = nextchild;
        nextchild = nextchild->sib();
        ++i;
        if (i > maxcheck) {
            printf("error: inf loop detected (> %d iterations) in lastchild during eval of '%s'.\n", 
                   maxcheck, sexp());
            l3throw(XABORT_TO_TOPLEVEL);
        }
    }

    break;
       case XFINALLY:
           if (vv) { generic_delete(vv, L3STDARGS_OBJONLY); }
       break;
   XENDX

L3END(lastchild) 

//
// want to find the last sib of the obj's first child and add on to it
//
L3KARG(addchild,2)
{

    l3obj* last = 0;
    l3obj* par = 0;
    l3obj* addme = 0;

   XTRY
       case XCODE:

       ptrvec_get(vv,0,&par);
       if (0==par) {
           printf("error: in addchild, could not locate parent to add to in expression '%s'.\n", sexps());
           l3throw(XABORT_TO_TOPLEVEL);
       }
       LIVEO(par);

       lastchild(par,-1,0,L3STDARGS_ENV);
       last = *retval;

       ptrvec_get(vv,1,&addme);
       assert(addme);
       
       if (last == gnil) {
           set_obj_left(addme,-1,0,  env,retval,owner,    par,etyp,retown,ifp);
       } else {
           set_obj_sib(addme,-1,0,  env,retval,owner,  last,etyp,retown,ifp);
       }

       // and make par the parent of addme 
       // seems to be causing some corruption?
       LIVEO(par);
       LIVEO(addme);
       set_obj_parent(par,-1,0,  env,retval,owner,  addme,etyp,retown,ifp);

       *retval = addme;

    break;
       case XFINALLY:
            generic_delete(vv, L3STDARGS_OBJONLY);   
       break;
   XENDX

}
L3END(addchild)


// run is like apply. but with a better name. 
// run the object's trybody method(s); or execute a lambda or function by name.
L3METHOD(run)

#if 0
// not done yet!
   arity = num_children(exp);
   l3path sexps(exp);

   if (arity < 2) {
     printf("error in arity: run requires at least one argument; the object, function, or closure to run.\n");
     l3throw(XABORT_TO_TOPLEVEL);
   }

    sexp_t  rest;
    rest.ty = SEXP_LIST;
    rest.list = exp->list->next; // skip operator position

    l3obj* vv = 0;
    eval_to_ptrvec(obj,arity-1,&rest,env,&vv,owner,curfo,0,retown);

    long N = ptrvec_size(vv);
    assert(N == k);

    // screen for unresolved references
    l3obj* ele = 0;

    for (long i = 0; i < N; ++i) {

        ptrvec_get(vv,i,&ele);
        if (ele->_type == t_lit) {
            l3path litstring;
            literal_get(ele,&litstring);
            printf("error: could not resolve literal '%s' in expression '%s'.\n",litstring(),sexps());
            generic_delete(vv, L3STDARGS_OBJONLY);
            l3throw(XABORT_TO_TOPLEVEL);
        }
    }

    *retval = vv;
#endif


L3END(run)




// boolean 
L3M(and_) {
   arity = num_children(exp);
   as_bool(L3STDARGS);
   l3obj* boolvec = *retval;
   l3obj* ptr = 0;
   for (long i = 0; i <  arity -1; ++i) {
       ptrvec_get(boolvec,i,&ptr);
       if (ptr == gnil) { *retval = gnil; return 0; }
       assert(ptr == gtrue);
   }
   *retval = gtrue;
   return 0;
}

L3M(as_bool) {
   arity = num_children(exp);
   k_arg_op(L3STDARGS);
   l3obj* vv = *retval;

   l3obj* ptr = 0;
   l3obj* boolvec = 0; 
   XTRY
       case XCODE:

   make_new_ptrvec(0, -1, 0, env, &boolvec, owner, curfo, t_vvc, retown,ifp);

   for (long i = 0; i < arity; ++i) {
       ptrvec_get(vv,i,&ptr);
       if (is_true(ptr,0)) {
           ptrvec_set(boolvec,i,gtrue);
       } else {
           ptrvec_set(boolvec,i,gnil);
       }
   }

   *retval = boolvec;

    break;
       case XFINALLY:
            generic_delete(vv, L3STDARGS_OBJONLY);   
       break;
   XENDX

   return 0;
}

L3M(or_) {

return 0; }

L3M(not_) {

return 0; }

L3M(xor_) {

return 0; }

L3M(setdiff) {

return 0; }

L3M(intersect) {

return 0; }

L3M(all) {

return 0; }

L3M(any) {

return 0; }

L3M(union_) {

return 0; }



// lists
L3M(first) {

return 0; }

L3M(rest) {

return 0; }

L3M(cons) {

return 0; }


L3M(to_string) {

    arity = num_children(exp);
    k_arg_op(L3STDARGS);
    l3obj* vv = *retval;
    l3path s;

   XTRY
       case XCODE:

    ptrvec_to_string(vv,&s," ",0);

    *retval = make_new_string_obj(s(),retown,"to_string_output");

    break;
       case XFINALLY:
            generic_delete(vv, L3STDARGS_OBJONLY);   
       break;
   XENDX

    return 0; 
}


char* get_objname(l3obj* obj) { 
    assert(obj);
    assert(obj->_type == t_obj);
    objstruct* os = (objstruct*)obj->_pdb;
    
    return os->objname; 
}

char* get_clsname(l3obj* obj) {
    assert(obj);
    assert(obj->_type == t_obj);
    objstruct* os = (objstruct*)obj->_pdb;
    
    return os->classname;
}
